package rodo

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"os"
	"time"
)

const (
	magic       uint32 = 0x52443031
	versionMajor byte = 1
	versionMinor byte = 0
)

// Value types
const (
	TypeNull   byte = 0x00
	TypeBool   byte = 0x01
	TypeInt    byte = 0x02
	TypeFloat  byte = 0x03
	TypeString byte = 0x04
	TypeBytes  byte = 0x05
	TypeArray  byte = 0x06
	TypeMap    byte = 0x07
	TypeDate   byte = 0x08
	TypeUUID   byte = 0x09
)

// Value represents a Rodo value.
type Value struct {
	Type    byte
	BoolVal bool
	IntVal  int64
	FloatVal float64
	StringVal string
	BytesVal []byte
	ArrayVal []Value
	MapVal   map[string]Value
	DateVal  int64
	UUIDVal  [16]byte
}

// Rodo represents a connection to a .rd file.
type Rodo struct {
	file       *os.File
	symbols    map[string]uint32
	symbolNext uint32
	dict       map[uint32]string
	dictNext   uint32
	pairs      map[uint32]Value // single block, block ID = 1
	dirty      bool
}

// Link opens or creates a .rd file at path.
func Link(path string) (*Rodo, error) {
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE, 0644)
	if err != nil {
		return nil, err
	}
	r := &Rodo{
		file:       f,
		symbols:    make(map[string]uint32),
		symbolNext: 0,
		dict:       make(map[uint32]string),
		dictNext:   0,
		pairs:      make(map[uint32]Value),
		dirty:      false,
	}
	// Check if file is non-empty
	stat, err := f.Stat()
	if err != nil {
		return nil, err
	}
	if stat.Size() > 0 {
		if err := r.load(); err != nil {
			f.Close()
			return nil, err
		}
	}
	return r, nil
}

// Close writes changes and closes the file.
func (r *Rodo) Close() error {
	if r == nil {
		return nil
	}
	if r.dirty {
		if err := r.save(); err != nil {
			r.file.Close()
			return err
		}
	}
	return r.file.Close()
}

// Set stores a value for a key.
func (r *Rodo) Set(key string, val Value) error {
	symID, ok := r.symbols[key]
	if !ok {
		r.symbolNext++
		symID = r.symbolNext
		r.symbols[key] = symID
	}
	// If value is string, add to dictionary
	if val.Type == TypeString {
		if _, exists := r.dictFind(val.StringVal); !exists {
			r.dictNext++
			r.dict[r.dictNext] = val.StringVal
		}
	}
	// If map, need to add keys to dict (simplified: ignore)
	r.pairs[symID] = val
	r.dirty = true
	return nil
}

// Get returns the value for a key, or nil if not found.
func (r *Rodo) Get(key string) (Value, bool) {
	symID, ok := r.symbols[key]
	if !ok {
		return Value{}, false
	}
	v, exists := r.pairs[symID]
	return v, exists
}

// Has returns true if the key exists.
func (r *Rodo) Has(key string) bool {
	symID, ok := r.symbols[key]
	if !ok {
		return false
	}
	_, exists := r.pairs[symID]
	return exists
}

// Delete removes a key.
func (r *Rodo) Delete(key string) error {
	symID, ok := r.symbols[key]
	if !ok {
		return nil
	}
	if _, exists := r.pairs[symID]; exists {
		delete(r.pairs, symID)
		r.dirty = true
	}
	return nil
}

// Keys returns all keys.
func (r *Rodo) Keys() []string {
	var keys []string
	for name, id := range r.symbols {
		if _, exists := r.pairs[id]; exists {
			keys = append(keys, name)
		}
	}
	return keys
}

// All returns a map of all keys to values.
func (r *Rodo) All() map[string]Value {
	result := make(map[string]Value)
	for name, id := range r.symbols {
		if v, exists := r.pairs[id]; exists {
			result[name] = v
		}
	}
	return result
}

// ---------------- Internal functions ----------------

func (r *Rodo) dictFind(s string) (uint32, bool) {
	for id, str := range r.dict {
		if str == s {
			return id, true
		}
	}
	return 0, false
}

func (r *Rodo) load() error {
	// Seek to beginning
	if _, err := r.file.Seek(0, io.SeekStart); err != nil {
		return err
	}
	data, err := io.ReadAll(r.file)
	if err != nil {
		return err
	}
	if len(data) < 18 {
		return errors.New("arquivo .rd inválido")
	}
	if binary.BigEndian.Uint32(data[0:4]) != magic {
		return errors.New("magic number inválido")
	}
	offset := 18 // skip header

	// Read symbols
	symbolCount, err := readVarint(data, &offset)
	if err != nil {
		return err
	}
	for i := uint64(0); i < symbolCount; i++ {
		id, err := readVarint(data, &offset)
		if err != nil {
			return err
		}
		nameLen, err := readVarint(data, &offset)
		if err != nil {
			return err
		}
		name := string(data[offset : offset+int(nameLen)])
		offset += int(nameLen)
		// skip typeHint and flags
		offset += 2
		r.symbols[name] = uint32(id)
		if uint32(id) > r.symbolNext {
			r.symbolNext = uint32(id)
		}
	}

	// Read dictionary
	dictCount, err := readVarint(data, &offset)
	if err != nil {
		return err
	}
	for i := uint64(0); i < dictCount; i++ {
		id, err := readVarint(data, &offset)
		if err != nil {
			return err
		}
		strLen, err := readVarint(data, &offset)
		if err != nil {
			return err
		}
		str := string(data[offset : offset+int(strLen)])
		offset += int(strLen)
		r.dict[uint32(id)] = str
		if uint32(id) > r.dictNext {
			r.dictNext = uint32(id)
		}
	}

	// Read blocks (only one)
	blockCount, err := readVarint(data, &offset)
	if err != nil {
		return err
	}
	if blockCount > 0 {
		_, err := readVarint(data, &offset) // block ID
		if err != nil {
			return err
		}
		pairCount, err := readVarint(data, &offset)
		if err != nil {
			return err
		}
		for i := uint64(0); i < pairCount; i++ {
			symID, err := readVarint(data, &offset)
			if err != nil {
				return err
			}
			val, err := readValue(data, &offset, r.dict)
			if err != nil {
				return err
			}
			r.pairs[uint32(symID)] = val
		}
	}
	return nil
}

func (r *Rodo) save() error {
	var buf bytes.Buffer

	// Header
	buf.Write([]byte{byte(magic >> 24), byte(magic >> 16), byte(magic >> 8), byte(magic)})
	buf.WriteByte(versionMajor)
	buf.WriteByte(versionMinor)
	buf.WriteByte(0) // flags
	// Timestamp (8 bytes) - use current Unix ms
	var tsBytes [8]byte
	binary.BigEndian.PutUint64(tsBytes[:], uint64(time.Now().UnixNano()/1e6))
	buf.Write(tsBytes[:])
	// CRC32 placeholder
	buf.Write([]byte{0, 0, 0, 0})

	// Symbols
	writeVarint(&buf, uint64(len(r.symbols)))
	for name, id := range r.symbols {
		writeVarint(&buf, uint64(id))
		writeVarint(&buf, uint64(len(name)))
		buf.WriteString(name)
		buf.WriteByte(0) // typeHint
		buf.WriteByte(0) // flags
	}

	// Dictionary: collect strings from values
	dictMap := make(map[string]uint32)
	for _, val := range r.pairs {
		collectStrings(val, &dictMap, &r.dictNext)
	}
	// Override r.dict with collected
	r.dict = make(map[uint32]string)
	for s, id := range dictMap {
		r.dict[id] = s
	}
	writeVarint(&buf, uint64(len(r.dict)))
	// Write dict sorted by ID
	ids := make([]uint32, 0, len(r.dict))
	for id := range r.dict {
		ids = append(ids, id)
	}
	sort.Slice(ids, func(i, j int) bool { return ids[i] < ids[j] })
	for _, id := range ids {
		writeVarint(&buf, uint64(id))
		s := r.dict[id]
		writeVarint(&buf, uint64(len(s)))
		buf.WriteString(s)
	}

	// Blocks
	writeVarint(&buf, 1) // block count
	writeVarint(&buf, 1) // block ID
	writeVarint(&buf, uint64(len(r.pairs)))
	for symID, val := range r.pairs {
		writeVarint(&buf, uint64(symID))
		if err := writeValue(&buf, val, r.dict); err != nil {
			return err
		}
	}

	// Metadata
	writeVarint(&buf, uint64(len(r.pairs)))
	writeVarint(&buf, uint64(len(r.symbols)))
	writeVarint(&buf, uint64(len(r.dict)))
	writeVarint(&buf, uint64(buf.Len()))
	buf.Write([]byte{0, 0, 0, 0}) // CRC32 placeholder

	// Write to file
	if _, err := r.file.Seek(0, io.SeekStart); err != nil {
		return err
	}
	if err := r.file.Truncate(0); err != nil {
		return err
	}
	if _, err := r.file.Write(buf.Bytes()); err != nil {
		return err
	}
	r.dirty = false
	return nil
}

func collectStrings(val Value, dict *map[string]uint32, nextID *uint32) {
	if val.Type == TypeString {
		if _, exists := (*dict)[val.StringVal]; !exists {
			*nextID++
			(*dict)[val.StringVal] = *nextID
		}
	} else if val.Type == TypeMap {
		for k, v := range val.MapVal {
			if _, exists := (*dict)[k]; !exists {
				*nextID++
				(*dict)[k] = *nextID
			}
			collectStrings(v, dict, nextID)
		}
	} else if val.Type == TypeArray {
		for _, item := range val.ArrayVal {
			collectStrings(item, dict, nextID)
		}
	}
}

func writeVarint(buf *bytes.Buffer, value uint64) {
	for value >= 0x80 {
		buf.WriteByte(byte(value) | 0x80)
		value >>= 7
	}
	buf.WriteByte(byte(value))
}

func readVarint(data []byte, offset *int) (uint64, error) {
	var result uint64
	var shift uint
	for {
		if *offset >= len(data) {
			return 0, io.ErrUnexpectedEOF
		}
		b := data[*offset]
		*offset++
		result |= uint64(b&0x7F) << shift
		shift += 7
		if b&0x80 == 0 {
			break
		}
		if shift > 70 {
			return 0, errors.New("varint muito longo")
		}
	}
	return result, nil
}

func zigzagEncode(v int64) uint64 {
	return uint64(v<<1) ^ uint64(v>>63)
}

func zigzagDecode(v uint64) int64 {
	return int64(v>>1) ^ -int64(v&1)
}

func writeValue(buf *bytes.Buffer, val Value, dict map[uint32]string) error {
	buf.WriteByte(val.Type)
	switch val.Type {
	case TypeNull:
		// nothing
	case TypeBool:
		if val.BoolVal {
			buf.WriteByte(1)
		} else {
			buf.WriteByte(0)
		}
	case TypeInt:
		writeVarint(buf, zigzagEncode(val.IntVal))
	case TypeFloat:
		var fBytes [8]byte
		binary.BigEndian.PutUint64(fBytes[:], math.Float64bits(val.FloatVal))
		buf.Write(fBytes[:])
	case TypeString:
		// Find ID in dict
		id, ok := findKeyByValue(dict, val.StringVal)
		if !ok {
			return errors.New("string não encontrada no dicionário")
		}
		writeVarint(buf, uint64(id))
	case TypeBytes:
		writeVarint(buf, uint64(len(val.BytesVal)))
		buf.Write(val.BytesVal)
	case TypeArray:
		writeVarint(buf, uint64(len(val.ArrayVal)))
		for _, item := range val.ArrayVal {
			if err := writeValue(buf, item, dict); err != nil {
				return err
			}
		}
	case TypeMap:
		writeVarint(buf, uint64(len(val.MapVal)))
		for k, v := range val.MapVal {
			id, ok := findKeyByValue(dict, k)
			if !ok {
				return errors.New("chave de map não encontrada no dicionário")
			}
			writeVarint(buf, uint64(id))
			if err := writeValue(buf, v, dict); err != nil {
				return err
			}
		}
	case TypeDate:
		writeVarint(buf, zigzagEncode(val.DateVal))
	case TypeUUID:
		buf.Write(val.UUIDVal[:])
	default:
		return errors.New("tipo desconhecido")
	}
	return nil
}

func readValue(data []byte, offset *int, dict map[uint32]string) (Value, error) {
	var val Value
	if *offset >= len(data) {
		return val, io.ErrUnexpectedEOF
	}
	val.Type = data[*offset]
	*offset++
	switch val.Type {
	case TypeNull:
	case TypeBool:
		val.BoolVal = data[*offset] != 0
		*offset++
	case TypeInt:
		encoded, err := readVarint(data, offset)
		if err != nil {
			return val, err
		}
		val.IntVal = zigzagDecode(encoded)
	case TypeFloat:
		if *offset+8 > len(data) {
			return val, io.ErrUnexpectedEOF
		}
		bits := binary.BigEndian.Uint64(data[*offset : *offset+8])
		val.FloatVal = math.Float64frombits(bits)
		*offset += 8
	case TypeString:
		id, err := readVarint(data, offset)
		if err != nil {
			return val, err
		}
		s, ok := dict[uint32(id)]
		if !ok {
			return val, errors.New("string não encontrada no dicionário")
		}
		val.StringVal = s
	case TypeBytes:
		length, err := readVarint(data, offset)
		if err != nil {
			return val, err
		}
		if *offset+int(length) > len(data) {
			return val, io.ErrUnexpectedEOF
		}
		val.BytesVal = data[*offset : *offset+int(length)]
		*offset += int(length)
	case TypeArray:
		count, err := readVarint(data, offset)
		if err != nil {
			return val, err
		}
		val.ArrayVal = make([]Value, count)
		for i := uint64(0); i < count; i++ {
			item, err := readValue(data, offset, dict)
			if err != nil {
				return val, err
			}
			val.ArrayVal[i] = item
		}
	case TypeMap:
		count, err := readVarint(data, offset)
		if err != nil {
			return val, err
		}
		val.MapVal = make(map[string]Value)
		for i := uint64(0); i < count; i++ {
			keyID, err := readVarint(data, offset)
			if err != nil {
				return val, err
			}
			key, ok := dict[uint32(keyID)]
			if !ok {
				return val, errors.New("chave de map não encontrada")
			}
			value, err := readValue(data, offset, dict)
			if err != nil {
				return val, err
			}
			val.MapVal[key] = value
		}
	case TypeDate:
		encoded, err := readVarint(data, offset)
		if err != nil {
			return val, err
		}
		val.DateVal = zigzagDecode(encoded)
	case TypeUUID:
		if *offset+16 > len(data) {
			return val, io.ErrUnexpectedEOF
		}
		copy(val.UUIDVal[:], data[*offset:*offset+16])
		*offset += 16
	default:
		return val, errors.New("tipo desconhecido")
	}
	return val, nil
}

func findKeyByValue(m map[uint32]string, target string) (uint32, bool) {
	for id, s := range m {
		if s == target {
			return id, true
		}
	}
	return 0, false
}