use std::collections::HashMap;
use std::fs::{File, OpenOptions};
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::Path;

pub const MAGIC: u32 = 0x52443031;
const VERSION_MAJOR: u8 = 1;
const VERSION_MINOR: u8 = 0;

#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    Int(i64),
    Float(f64),
    String(String),
    Bytes(Vec<u8>),
    Array(Vec<Value>),
    Map(HashMap<String, Value>),
    Date(i64),
    Uuid([u8; 16]),
}

impl Value {
    fn type_code(&self) -> u8 {
        match self {
            Value::Null => 0x00,
            Value::Bool(_) => 0x01,
            Value::Int(_) => 0x02,
            Value::Float(_) => 0x03,
            Value::String(_) => 0x04,
            Value::Bytes(_) => 0x05,
            Value::Array(_) => 0x06,
            Value::Map(_) => 0x07,
            Value::Date(_) => 0x08,
            Value::Uuid(_) => 0x09,
        }
    }
}

pub struct Rodo {
    file: File,
    symbols: HashMap<String, u32>,
    symbol_id_counter: u32,
    dict: HashMap<u32, String>,
    dict_id_counter: u32,
    // Single block for simplicity (block_id = 1)
    pairs: HashMap<u32, Value>,
    dirty: bool,
}

impl Rodo {
    pub fn link<P: AsRef<Path>>(path: P) -> std::io::Result<Self> {
        let path = path.as_ref();
        let file = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .open(path)?;

        let mut rodo = Rodo {
            file,
            symbols: HashMap::new(),
            symbol_id_counter: 0,
            dict: HashMap::new(),
            dict_id_counter: 0,
            pairs: HashMap::new(),
            dirty: false,
        };

        // Check if file is non-empty and load existing data
        let metadata = rodo.file.metadata()?;
        if metadata.len() > 0 {
            rodo.load()?;
        }

        Ok(rodo)
    }

    fn load(&mut self) -> std::io::Result<()> {
        // Simplified: read all data into memory and parse
        let mut data = Vec::new();
        self.file.seek(SeekFrom::Start(0))?;
        self.file.read_to_end(&mut data)?;

        // Parse header
        if data.len() < 18 {
            return Err(std::io::Error::new(std::io::ErrorKind::InvalidData, "Arquivo .rd inválido"));
        }
        let magic = u32::from_be_bytes(data[0..4].try_into().unwrap());
        if magic != MAGIC {
            return Err(std::io::Error::new(std::io::ErrorKind::InvalidData, "Magic number inválido"));
        }
        // Skip header details for now
        let mut offset = 18;

        // Read symbol table
        let count = read_varint(&data, &mut offset)? as usize;
        for _ in 0..count {
            let _id = read_varint(&data, &mut offset)? as u32;
            let name_len = read_varint(&data, &mut offset)? as usize;
            let name = String::from_utf8(data[offset..offset+name_len].to_vec())
                .map_err(|_| std::io::Error::new(std::io::ErrorKind::InvalidData, "Nome de símbolo inválido"))?;
            offset += name_len;
            let type_hint = data[offset]; offset += 1;
            let _flags = data[offset]; offset += 1;

            self.symbols.insert(name.clone(), self.symbol_id_counter + 1);
            self.symbol_id_counter += 1;
        }

        // Read dictionary
        let count = read_varint(&data, &mut offset)? as usize;
        for _ in 0..count {
            let _id = read_varint(&data, &mut offset)? as u32;
            let str_len = read_varint(&data, &mut offset)? as usize;
            let s = String::from_utf8(data[offset..offset+str_len].to_vec())
                .map_err(|_| std::io::Error::new(std::io::ErrorKind::InvalidData, "String do dicionário inválida"))?;
            offset += str_len;
            self.dict.insert(self.dict_id_counter + 1, s);
            self.dict_id_counter += 1;
        }

        // Read blocks (we assume single block)
        let block_count = read_varint(&data, &mut offset)? as usize;
        if block_count > 0 {
            let _block_id = read_varint(&data, &mut offset)? as u32;
            let pair_count = read_varint(&data, &mut offset)? as usize;
            for _ in 0..pair_count {
                let symbol_id = read_varint(&data, &mut offset)? as u32;
                let value = read_value(&data, &mut offset, &self.dict)?;
                self.pairs.insert(symbol_id, value);
            }
        }

        Ok(())
    }

    pub fn set(&mut self, key: &str, value: Value) -> std::io::Result<()> {
        let symbol_id = match self.symbols.get(key) {
            Some(&id) => id,
            None => {
                self.symbol_id_counter += 1;
                self.symbols.insert(key.to_string(), self.symbol_id_counter);
                self.symbol_id_counter
            }
        };
        self.pairs.insert(symbol_id, value);
        self.dirty = true;
        Ok(())
    }

    pub fn get(&self, key: &str) -> Option<&Value> {
        self.symbols.get(key).and_then(|&id| self.pairs.get(&id))
    }

    pub fn has(&self, key: &str) -> bool {
        self.symbols.contains_key(key) && self.pairs.contains_key(&self.symbols[key])
    }

    pub fn delete(&mut self, key: &str) -> std::io::Result<bool> {
        if let Some(&id) = self.symbols.get(key) {
            if self.pairs.remove(&id).is_some() {
                self.dirty = true;
                return Ok(true);
            }
        }
        Ok(false)
    }

    pub fn keys(&self) -> Vec<String> {
        self.pairs.keys()
            .filter_map(|&id| self.symbols.iter().find(|(_, &sid)| sid == id).map(|(name, _)| name.clone()))
            .collect()
    }

    pub fn all(&self) -> HashMap<String, &Value> {
        let mut result = HashMap::new();
        for (name, &id) in &self.symbols {
            if let Some(val) = self.pairs.get(&id) {
                result.insert(name.clone(), val);
            }
        }
        result
    }

    pub fn close(mut self) -> std::io::Result<()> {
        if self.dirty {
            self.save()?;
        }
        // File is closed on drop
        Ok(())
    }

    fn save(&mut self) -> std::io::Result<()> {
        let mut data = Vec::new();

        // Header
        data.extend_from_slice(&MAGIC.to_be_bytes());
        data.push(VERSION_MAJOR);
        data.push(VERSION_MINOR);
        data.push(0); // flags
        // Timestamp (8 bytes) - use current time as simple value
        let timestamp = 0u64;
        data.extend_from_slice(&timestamp.to_be_bytes());
        // Header CRC32 (simplified: 0)
        data.extend_from_slice(&[0u8; 4]);

        // Symbol table
        write_varint(&mut data, self.symbols.len() as u64)?;
        for (name, &id) in &self.symbols {
            write_varint(&mut data, id as u64)?;
            let name_bytes = name.as_bytes();
            write_varint(&mut data, name_bytes.len() as u64)?;
            data.extend_from_slice(name_bytes);
            // type_hint: we don't store type hints, use 0
            data.push(0);
            data.push(0); // flags
        }

        // Dictionary: collect strings from values that are strings or map keys
        let mut dict_map: HashMap<String, u32> = HashMap::new();
        for value in self.pairs.values() {
            collect_strings(value, &mut dict_map, &mut self.dict_id_counter)?;
        }
        // Override dict with collected
        self.dict.clear();
        for (s, id) in &dict_map {
            self.dict.insert(*id, s.clone());
        }
        write_varint(&mut data, self.dict.len() as u64)?;
        let mut dict_entries: Vec<_> = self.dict.iter().collect();
        dict_entries.sort_by_key(|(id, _)| **id);
        for (id, s) in dict_entries {
            write_varint(&mut data, *id as u64)?;
            let bytes = s.as_bytes();
            write_varint(&mut data, bytes.len() as u64)?;
            data.extend_from_slice(bytes);
        }

        // Block (single block)
        write_varint(&mut data, 1)?; // block_count
        write_varint(&mut data, 1)?; // block_id
        write_varint(&mut data, self.pairs.len() as u64)?;
        for (symbol_id, value) in &self.pairs {
            write_varint(&mut data, *symbol_id as u64)?;
            write_value(&mut data, value, &dict_map)?;
        }

        // Metadata (simplified)
        write_varint(&mut data, self.pairs.len() as u64)?;
        write_varint(&mut data, self.symbols.len() as u64)?;
        write_varint(&mut data, self.dict.len() as u64)?;
        write_varint(&mut data, data.len() as u64)?;
        // CRC32 (0)
        data.extend_from_slice(&[0u8; 4]);

        self.file.seek(SeekFrom::Start(0))?;
        self.file.write_all(&data)?;
        self.file.set_len(data.len() as u64)?;
        self.file.flush()?;
        self.dirty = false;
        Ok(())
    }
}

fn collect_strings(value: &Value, dict: &mut HashMap<String, u32>, counter: &mut u32) -> std::io::Result<()> {
    match value {
        Value::String(s) => {
            if !dict.contains_key(s) {
                *counter += 1;
                dict.insert(s.clone(), *counter);
            }
        }
        Value::Map(map) => {
            for (k, v) in map {
                if !dict.contains_key(k) {
                    *counter += 1;
                    dict.insert(k.clone(), *counter);
                }
                collect_strings(v, dict, counter)?;
            }
        }
        Value::Array(arr) => {
            for v in arr {
                collect_strings(v, dict, counter)?;
            }
        }
        _ => {}
    }
    Ok(())
}

fn read_varint(data: &[u8], offset: &mut usize) -> std::io::Result<u64> {
    let mut result = 0u64;
    let mut shift = 0;
    loop {
        if *offset >= data.len() {
            return Err(std::io::Error::new(std::io::ErrorKind::UnexpectedEof, "Varint incompleto"));
        }
        let byte = data[*offset];
        *offset += 1;
        result |= ((byte & 0x7F) as u64) << shift;
        shift += 7;
        if byte & 0x80 == 0 {
            break;
        }
        if shift > 70 {
            return Err(std::io::Error::new(std::io::ErrorKind::InvalidData, "Varint muito longo"));
        }
    }
    Ok(result)
}

fn write_varint(out: &mut Vec<u8>, value: u64) -> std::io::Result<()> {
    let mut v = value;
    while v >= 0x80 {
        out.push(((v & 0x7F) as u8) | 0x80);
        v >>= 7;
    }
    out.push(v as u8);
    Ok(())
}

fn zigzag_encode(v: i64) -> u64 {
    ((v << 1) ^ (v >> 63)) as u64
}

fn zigzag_decode(v: u64) -> i64 {
    ((v >> 1) as i64) ^ -((v & 1) as i64)
}

fn read_value(data: &[u8], offset: &mut usize, dict: &HashMap<u32, String>) -> std::io::Result<Value> {
    let type_code = data[*offset];
    *offset += 1;
    match type_code {
        0x00 => Ok(Value::Null),
        0x01 => {
            let b = data[*offset] != 0;
            *offset += 1;
            Ok(Value::Bool(b))
        }
        0x02 => {
            let encoded = read_varint(data, offset)?;
            Ok(Value::Int(zigzag_decode(encoded)))
        }
        0x03 => {
            let bytes: [u8; 8] = data[*offset..*offset+8].try_into().unwrap();
            *offset += 8;
            Ok(Value::Float(f64::from_be_bytes(bytes)))
        }
        0x04 => {
            let id = read_varint(data, offset)? as u32;
            let s = dict.get(&id).cloned().unwrap_or_default();
            Ok(Value::String(s))
        }
        0x05 => {
            let len = read_varint(data, offset)? as usize;
            let bytes = data[*offset..*offset+len].to_vec();
            *offset += len;
            Ok(Value::Bytes(bytes))
        }
        0x06 => {
            let count = read_varint(data, offset)? as usize;
            let mut arr = Vec::with_capacity(count);
            for _ in 0..count {
                arr.push(read_value(data, offset, dict)?);
            }
            Ok(Value::Array(arr))
        }
        0x07 => {
            let count = read_varint(data, offset)? as usize;
            let mut map = HashMap::new();
            for _ in 0..count {
                let key_id = read_varint(data, offset)? as u32;
                let key = dict.get(&key_id).cloned().unwrap_or_default();
                let val = read_value(data, offset, dict)?;
                map.insert(key, val);
            }
            Ok(Value::Map(map))
        }
        0x08 => {
            let encoded = read_varint(data, offset)?;
            Ok(Value::Date(zigzag_decode(encoded)))
        }
        0x09 => {
            let mut uuid = [0u8; 16];
            uuid.copy_from_slice(&data[*offset..*offset+16]);
            *offset += 16;
            Ok(Value::Uuid(uuid))
        }
        _ => Err(std::io::Error::new(std::io::ErrorKind::InvalidData, "Tipo desconhecido")),
    }
}

fn write_value(out: &mut Vec<u8>, value: &Value, dict: &HashMap<String, u32>) -> std::io::Result<()> {
    out.push(value.type_code());
    match value {
        Value::Null => {}
        Value::Bool(b) => out.push(if *b { 1 } else { 0 }),
        Value::Int(i) => write_varint(out, zigzag_encode(*i))?,
        Value::Float(f) => out.extend_from_slice(&f.to_be_bytes()),
        Value::String(s) => {
            let id = dict.get(s).copied().unwrap_or(0);
            write_varint(out, id as u64)?;
        }
        Value::Bytes(b) => {
            write_varint(out, b.len() as u64)?;
            out.extend_from_slice(b);
        }
        Value::Array(arr) => {
            write_varint(out, arr.len() as u64)?;
            for v in arr {
                write_value(out, v, dict)?;
            }
        }
        Value::Map(map) => {
            write_varint(out, map.len() as u64)?;
            for (k, v) in map {
                let id = dict.get(k).copied().unwrap_or(0);
                write_varint(out, id as u64)?;
                write_value(out, v, dict)?;
            }
        }
        Value::Date(d) => write_varint(out, zigzag_encode(*d))?,
        Value::Uuid(u) => out.extend_from_slice(u),
    }
    Ok(())
}

pub fn link<P: AsRef<Path>>(path: P) -> std::io::Result<Rodo> {
    Rodo::link(path)
}