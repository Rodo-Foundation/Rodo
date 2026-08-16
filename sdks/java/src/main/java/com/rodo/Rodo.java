package com.rodo;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class Rodo implements Closeable {
    private static final int MAGIC = 0x52443031;
    private static final byte VERSION_MAJOR = 1;
    private static final byte VERSION_MINOR = 0;

    // Types
    public static final byte TYPE_NULL = 0x00;
    public static final byte TYPE_BOOL = 0x01;
    public static final byte TYPE_INT = 0x02;
    public static final byte TYPE_FLOAT = 0x03;
    public static final byte TYPE_STRING = 0x04;
    public static final byte TYPE_BYTES = 0x05;
    public static final byte TYPE_ARRAY = 0x06;
    public static final byte TYPE_MAP = 0x07;
    public static final byte TYPE_DATE = 0x08;
    public static final byte TYPE_UUID = 0x09;

    public static class Value {
        public byte type;
        public boolean boolVal;
        public long intVal;
        public double floatVal;
        public String stringVal;
        public byte[] bytesVal;
        public List<Value> arrayVal;
        public Map<String, Value> mapVal;
        public long dateVal;
        public byte[] uuidVal = new byte[16];
    }

    private RandomAccessFile file;
    private Map<String, Integer> symbols = new HashMap<>();
    private int symbolNext = 0;
    private Map<Integer, String> dict = new HashMap<>();
    private int dictNext = 0;
    private Map<Integer, Value> pairs = new HashMap<>(); // block 1
    private boolean dirty = false;

    public static Rodo link(String path) throws IOException {
        return new Rodo(path);
    }

    private Rodo(String path) throws IOException {
        file = new RandomAccessFile(path, "rw");
        if (file.length() > 0) {
            load();
        }
    }

    public void set(String key, Value value) throws IOException {
        int symId;
        if (symbols.containsKey(key)) {
            symId = symbols.get(key);
        } else {
            symId = ++symbolNext;
            symbols.put(key, symId);
        }
        if (value.type == TYPE_STRING) {
            addStringToDict(value.stringVal);
        } else if (value.type == TYPE_MAP && value.mapVal != null) {
            for (String k : value.mapVal.keySet()) {
                addStringToDict(k);
            }
        } else if (value.type == TYPE_ARRAY && value.arrayVal != null) {
            collectStrings(value, new HashMap<>());
        }
        pairs.put(symId, value);
        dirty = true;
    }

    public Value get(String key) {
        Integer symId = symbols.get(key);
        if (symId == null) return null;
        return pairs.get(symId);
    }

    public boolean has(String key) {
        Integer symId = symbols.get(key);
        return symId != null && pairs.containsKey(symId);
    }

    public void delete(String key) {
        Integer symId = symbols.get(key);
        if (symId != null && pairs.remove(symId) != null) {
            dirty = true;
        }
    }

    public List<String> keys() {
        List<String> keys = new ArrayList<>();
        for (Map.Entry<String, Integer> entry : symbols.entrySet()) {
            if (pairs.containsKey(entry.getValue())) {
                keys.add(entry.getKey());
            }
        }
        return keys;
    }

    public Map<String, Value> all() {
        Map<String, Value> result = new HashMap<>();
        for (Map.Entry<String, Integer> entry : symbols.entrySet()) {
            Value v = pairs.get(entry.getValue());
            if (v != null) {
                result.put(entry.getKey(), v);
            }
        }
        return result;
    }

    @Override
    public void close() throws IOException {
        if (dirty) {
            save();
        }
        file.close();
    }

    private void addStringToDict(String s) {
        if (!dict.values().contains(s)) {
            dict.put(++dictNext, s);
        }
    }

    private void collectStrings(Value val, Map<String, Integer> dictMap) {
        if (val.type == TYPE_STRING) {
            addStringToDict(val.stringVal);
        } else if (val.type == TYPE_MAP) {
            for (Map.Entry<String, Value> e : val.mapVal.entrySet()) {
                addStringToDict(e.getKey());
                collectStrings(e.getValue(), dictMap);
            }
        } else if (val.type == TYPE_ARRAY) {
            for (Value item : val.arrayVal) {
                collectStrings(item, dictMap);
            }
        }
    }

    private void load() throws IOException {
        file.seek(0);
        byte[] header = new byte[18];
        file.readFully(header);
        ByteBuffer bb = ByteBuffer.wrap(header).order(ByteOrder.BIG_ENDIAN);
        int magic = bb.getInt();
        if (magic != MAGIC) {
            throw new IOException("Magic number inválido");
        }
        DataInputStream dis = new DataInputStream(new BufferedInputStream(new FileInputStream(file.getFD())));
        // Read symbols
        long symbolCount = readVarint(dis);
        for (long i = 0; i < symbolCount; i++) {
            int id = (int) readVarint(dis);
            int nameLen = (int) readVarint(dis);
            byte[] nameBytes = new byte[nameLen];
            dis.readFully(nameBytes);
            String name = new String(nameBytes, StandardCharsets.UTF_8);
            dis.readByte(); // typeHint
            dis.readByte(); // flags
            symbols.put(name, id);
            if (id > symbolNext) symbolNext = id;
        }
        // Read dictionary
        long dictCount = readVarint(dis);
        for (long i = 0; i < dictCount; i++) {
            int id = (int) readVarint(dis);
            int strLen = (int) readVarint(dis);
            byte[] strBytes = new byte[strLen];
            dis.readFully(strBytes);
            dict.put(id, new String(strBytes, StandardCharsets.UTF_8));
            if (id > dictNext) dictNext = id;
        }
        // Read blocks
        long blockCount = readVarint(dis);
        if (blockCount > 0) {
            readVarint(dis); // block ID
            long pairCount = readVarint(dis);
            for (long i = 0; i < pairCount; i++) {
                int symId = (int) readVarint(dis);
                Value val = readValue(dis);
                pairs.put(symId, val);
            }
        }
        dis.close();
    }

    private void save() throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(baos);
        // Header
        dos.writeInt(MAGIC);
        dos.writeByte(VERSION_MAJOR);
        dos.writeByte(VERSION_MINOR);
        dos.writeByte(0); // flags
        dos.writeLong(System.currentTimeMillis());
        dos.writeInt(0); // CRC placeholder
        // Symbols
        writeVarint(dos, symbols.size());
        for (Map.Entry<String, Integer> entry : symbols.entrySet()) {
            writeVarint(dos, entry.getValue());
            byte[] nameBytes = entry.getKey().getBytes(StandardCharsets.UTF_8);
            writeVarint(dos, nameBytes.length);
            dos.write(nameBytes);
            dos.writeByte(0); // typeHint
            dos.writeByte(0); // flags
        }
        // Dictionary: collect from values
        Map<String, Integer> dictMap = new HashMap<>();
        for (Value val : pairs.values()) {
            collectStrings(val, dictMap);
        }
        dict.clear();
        for (Map.Entry<String, Integer> e : dictMap.entrySet()) {
            dict.put(e.getValue(), e.getKey());
        }
        writeVarint(dos, dict.size());
        List<Integer> ids = new ArrayList<>(dict.keySet());
        Collections.sort(ids);
        for (int id : ids) {
            writeVarint(dos, id);
            String s = dict.get(id);
            byte[] sBytes = s.getBytes(StandardCharsets.UTF_8);
            writeVarint(dos, sBytes.length);
            dos.write(sBytes);
        }
        // Blocks
        writeVarint(dos, 1); // block count
        writeVarint(dos, 1); // block ID
        writeVarint(dos, pairs.size());
        for (Map.Entry<Integer, Value> entry : pairs.entrySet()) {
            writeVarint(dos, entry.getKey());
            writeValue(dos, entry.getValue(), dict);
        }
        // Metadata
        writeVarint(dos, pairs.size());
        writeVarint(dos, symbols.size());
        writeVarint(dos, dict.size());
        writeVarint(dos, baos.size());
        dos.writeInt(0); // CRC placeholder
        dos.flush();
        byte[] data = baos.toByteArray();
        file.setLength(0);
        file.seek(0);
        file.write(data);
        dirty = false;
    }

    private void writeValue(DataOutputStream dos, Value val, Map<Integer, String> dict) throws IOException {
        dos.writeByte(val.type);
        switch (val.type) {
            case TYPE_NULL: break;
            case TYPE_BOOL: dos.writeByte(val.boolVal ? 1 : 0); break;
            case TYPE_INT: writeVarint(dos, zigzagEncode(val.intVal)); break;
            case TYPE_FLOAT: dos.writeLong(Double.doubleToLongBits(val.floatVal)); break;
            case TYPE_STRING: {
                int id = findKeyByValue(dict, val.stringVal);
                writeVarint(dos, id);
                break;
            }
            case TYPE_BYTES: writeVarint(dos, val.bytesVal.length); dos.write(val.bytesVal); break;
            case TYPE_ARRAY: {
                writeVarint(dos, val.arrayVal.size());
                for (Value item : val.arrayVal) {
                    writeValue(dos, item, dict);
                }
                break;
            }
            case TYPE_MAP: {
                writeVarint(dos, val.mapVal.size());
                for (Map.Entry<String, Value> e : val.mapVal.entrySet()) {
                    int id = findKeyByValue(dict, e.getKey());
                    writeVarint(dos, id);
                    writeValue(dos, e.getValue(), dict);
                }
                break;
            }
            case TYPE_DATE: writeVarint(dos, zigzagEncode(val.dateVal)); break;
            case TYPE_UUID: dos.write(val.uuidVal); break;
            default: throw new IOException("Tipo desconhecido");
        }
    }

    private Value readValue(DataInputStream dis) throws IOException {
        Value val = new Value();
        val.type = dis.readByte();
        switch (val.type) {
            case TYPE_NULL: break;
            case TYPE_BOOL: val.boolVal = dis.readByte() != 0; break;
            case TYPE_INT: val.intVal = zigzagDecode(readVarint(dis)); break;
            case TYPE_FLOAT: val.floatVal = Double.longBitsToDouble(dis.readLong()); break;
            case TYPE_STRING: {
                int id = (int) readVarint(dis);
                val.stringVal = dict.get(id);
                break;
            }
            case TYPE_BYTES: {
                int len = (int) readVarint(dis);
                val.bytesVal = new byte[len];
                dis.readFully(val.bytesVal);
                break;
            }
            case TYPE_ARRAY: {
                int count = (int) readVarint(dis);
                val.arrayVal = new ArrayList<>();
                for (int i = 0; i < count; i++) {
                    val.arrayVal.add(readValue(dis));
                }
                break;
            }
            case TYPE_MAP: {
                int count = (int) readVarint(dis);
                val.mapVal = new HashMap<>();
                for (int i = 0; i < count; i++) {
                    int keyId = (int) readVarint(dis);
                    String key = dict.get(keyId);
                    Value v = readValue(dis);
                    val.mapVal.put(key, v);
                }
                break;
            }
            case TYPE_DATE: val.dateVal = zigzagDecode(readVarint(dis)); break;
            case TYPE_UUID: dis.readFully(val.uuidVal); break;
            default: throw new IOException("Tipo desconhecido");
        }
        return val;
    }

    private static void writeVarint(DataOutputStream dos, long value) throws IOException {
        while ((value & ~0x7F) != 0) {
            dos.writeByte(((int) value & 0x7F) | 0x80);
            value >>>= 7;
        }
        dos.writeByte((int) value);
    }

    private static long readVarint(DataInputStream dis) throws IOException {
        long result = 0;
        int shift = 0;
        int b;
        do {
            b = dis.readUnsignedByte();
            result |= (long) (b & 0x7F) << shift;
            shift += 7;
            if (shift > 70) throw new IOException("Varint muito longo");
        } while ((b & 0x80) != 0);
        return result;
    }

    private static long zigzagEncode(long v) {
        return (v << 1) ^ (v >> 63);
    }

    private static long zigzagDecode(long v) {
        return (v >>> 1) ^ -(v & 1);
    }

    private static int findKeyByValue(Map<Integer, String> map, String target) {
        for (Map.Entry<Integer, String> e : map.entrySet()) {
            if (e.getValue().equals(target)) return e.getKey();
        }
        return 0;
    }
}