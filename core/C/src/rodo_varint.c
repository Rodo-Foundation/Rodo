#include "rodo_internal.h"
#include <stdio.h>
#include <stdint.h>

int rodo_write_varint(FILE* f, uint64_t value) {
    uint8_t buf[10];
    size_t i = 0;
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value) byte |= 0x80;
        buf[i++] = byte;
    } while (value);
    return fwrite(buf, 1, i, f) == i ? 0 : -1;
}

int rodo_read_varint(FILE* f, uint64_t* value) {
    uint64_t result = 0;
    int shift = 0;
    uint8_t byte;
    do {
        if (fread(&byte, 1, 1, f) != 1) return -1;
        result |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;
        if (shift > 70) return -1;
    } while (byte & 0x80);
    *value = result;
    return 0;
}

int rodo_write_zigzag(FILE* f, int64_t value) {
    uint64_t encoded = ((uint64_t)value << 1) ^ (value >> 63);
    return rodo_write_varint(f, encoded);
}

int rodo_read_zigzag(FILE* f, int64_t* value) {
    uint64_t encoded;
    if (rodo_read_varint(f, &encoded) != 0) return -1;
    *value = (int64_t)(encoded >> 1) ^ -(int64_t)(encoded & 1);
    return 0;
}