#include "rodo_internal.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int write_header(FILE* f, Rodo* r) {
    r->header.magic = RODO_MAGIC;
    r->header.version_major = RODO_VERSION_MAJOR;
    r->header.version_minor = RODO_VERSION_MINOR;
    r->header.flags = 0;
    r->header.timestamp = (uint64_t)time(NULL) * 1000;
    uint8_t hdr[14];
    memcpy(hdr, &r->header.magic, 4);
    hdr[4] = r->header.version_major;
    hdr[5] = r->header.version_minor;
    hdr[6] = r->header.flags;
    memcpy(hdr + 7, &r->header.timestamp, 8);
    r->header.header_crc32 = rodo_crc32(hdr, 14);
    if (fwrite(&r->header, sizeof(rodo_header_t), 1, f) != 1) return -1;
    return 0;
}

static int write_symbols(FILE* f, Rodo* r) {
    if (rodo_write_varint(f, r->symbol_count) != 0) return -1;
    for (size_t i = 0; i < r->symbol_count; i++) {
        if (rodo_write_varint(f, r->symbols[i].id) != 0) return -1;
        size_t len = strlen(r->symbols[i].name);
        if (rodo_write_varint(f, len) != 0) return -1;
        if (fwrite(r->symbols[i].name, 1, len, f) != len) return -1;
        if (fwrite(&r->symbols[i].type_hint, 1, 1, f) != 1) return -1;
        if (fwrite(&r->symbols[i].flags, 1, 1, f) != 1) return -1;
    }
    return 0;
}

static int write_dict(FILE* f, Rodo* r) {
    if (rodo_write_varint(f, r->dict_count) != 0) return -1;
    for (size_t i = 0; i < r->dict_count; i++) {
        if (rodo_write_varint(f, r->dict[i].id) != 0) return -1;
        size_t len = strlen(r->dict[i].str);
        if (rodo_write_varint(f, len) != 0) return -1;
        if (fwrite(r->dict[i].str, 1, len, f) != len) return -1;
    }
    return 0;
}

int rodo_write_value(FILE* f, Rodo* r, const rodo_value_t* value) {
    if (fwrite(&value->type, 1, 1, f) != 1) return -1;
    switch (value->type) {
        case RODO_NULL:
            break;
        case RODO_BOOL:
            if (fwrite(&value->value.bool_val, 1, 1, f) != 1) return -1;
            break;
        case RODO_INT:
            if (rodo_write_zigzag(f, value->value.int_val) != 0) return -1;
            break;
        case RODO_FLOAT:
            if (fwrite(&value->value.float_val, 8, 1, f) != 1) return -1;
            break;
        case RODO_STRING: {
            uint32_t id = rodo_dict_add(r, value->value.string_val);
            if (id == 0) return -1;
            if (rodo_write_varint(f, id) != 0) return -1;
            break;
        }
        case RODO_BYTES:
            if (rodo_write_varint(f, value->value.bytes_val.len) != 0) return -1;
            if (fwrite(value->value.bytes_val.data, 1, value->value.bytes_val.len, f) != value->value.bytes_val.len) return -1;
            break;
        case RODO_ARRAY:
            if (rodo_write_varint(f, value->value.array_val.count) != 0) return -1;
            for (size_t i = 0; i < value->value.array_val.count; i++) {
                if (rodo_write_value(f, r, &value->value.array_val.items[i]) != 0) return -1;
            }
            break;
        case RODO_MAP:
            if (rodo_write_varint(f, value->value.map_val.count) != 0) return -1;
            for (size_t i = 0; i < value->value.map_val.count; i++) {
                uint32_t key_id = rodo_dict_add(r, value->value.map_val.pairs[i].key);
                if (key_id == 0) return -1;
                if (rodo_write_varint(f, key_id) != 0) return -1;
                if (rodo_write_value(f, r, &value->value.map_val.pairs[i].value) != 0) return -1;
            }
            break;
        case RODO_DATE:
            if (rodo_write_zigzag(f, value->value.date_val) != 0) return -1;
            break;
        case RODO_UUID:
            if (fwrite(value->value.uuid_val, 1, 16, f) != 16) return -1;
            break;
        default:
            return -1;
    }
    return 0;
}

static int write_blocks(FILE* f, Rodo* r) {
    if (rodo_write_varint(f, r->block_count) != 0) return -1;
    for (size_t i = 0; i < r->block_count; i++) {
        if (rodo_write_varint(f, r->blocks[i].block_id) != 0) return -1;
        if (rodo_write_varint(f, r->blocks[i].pair_count) != 0) return -1;
        for (size_t j = 0; j < r->blocks[i].pair_count; j++) {
            if (rodo_write_varint(f, r->blocks[i].pairs[j].symbol_id) != 0) return -1;
            if (rodo_write_value(f, r, &r->blocks[i].pairs[j].value) != 0) return -1;
        }
    }
    return 0;
}

static int write_metadata(FILE* f, Rodo* r) {
    if (rodo_write_varint(f, r->block_count) != 0) return -1;
    if (rodo_write_varint(f, r->symbol_count) != 0) return -1;
    if (rodo_write_varint(f, r->dict_count) != 0) return -1;
    long pos = ftell(f);
    if (rodo_write_varint(f, (uint64_t)pos) != 0) return -1;
    fflush(f);
    // CRC32 will be computed later and patched? Simplify: skip CRC for now
    uint32_t crc = 0;
    if (fwrite(&crc, 4, 1, f) != 1) return -1;
    return 0;
}

int rodo_save_file(Rodo* r) {
    if (!r->file) return -1;
    rewind(r->file);
    if (write_header(r->file, r) != 0) return -1;
    if (write_symbols(r->file, r) != 0) return -1;
    if (write_dict(r->file, r) != 0) return -1;
    if (write_blocks(r->file, r) != 0) return -1;
    if (write_metadata(r->file, r) != 0) return -1;
    fflush(r->file);
    r->dirty = 0;
    return 0;
}

static int read_header(FILE* f, Rodo* r) {
    if (fread(&r->header, sizeof(rodo_header_t), 1, f) != 1) return -1;
    if (r->header.magic != RODO_MAGIC) return -1;
    return 0;
}

static int read_symbols(FILE* f, Rodo* r) {
    uint64_t count;
    if (rodo_read_varint(f, &count) != 0) return -1;
    r->symbol_count = (size_t)count;
    r->symbol_capacity = count ? count : 1;
    r->symbols = calloc(r->symbol_capacity, sizeof(rodo_symbol_t));
    if (!r->symbols) return -1;
    for (size_t i = 0; i < r->symbol_count; i++) {
        uint64_t id;
        if (rodo_read_varint(f, &id) != 0) return -1;
        r->symbols[i].id = (uint32_t)id;
        uint64_t len;
        if (rodo_read_varint(f, &len) != 0) return -1;
        r->symbols[i].name = malloc(len + 1);
        if (!r->symbols[i].name) return -1;
        if (fread(r->symbols[i].name, 1, len, f) != len) return -1;
        r->symbols[i].name[len] = '\0';
        if (fread(&r->symbols[i].type_hint, 1, 1, f) != 1) return -1;
        if (fread(&r->symbols[i].flags, 1, 1, f) != 1) return -1;
    }
    return 0;
}

static int read_dict(FILE* f, Rodo* r) {
    uint64_t count;
    if (rodo_read_varint(f, &count) != 0) return -1;
    r->dict_count = (size_t)count;
    r->dict_capacity = count ? count : 1;
    r->dict = calloc(r->dict_capacity, sizeof(rodo_dict_entry_t));
    if (!r->dict) return -1;
    for (size_t i = 0; i < r->dict_count; i++) {
        uint64_t id;
        if (rodo_read_varint(f, &id) != 0) return -1;
        r->dict[i].id = (uint32_t)id;
        uint64_t len;
        if (rodo_read_varint(f, &len) != 0) return -1;
        r->dict[i].str = malloc(len + 1);
        if (!r->dict[i].str) return -1;
        if (fread(r->dict[i].str, 1, len, f) != len) return -1;
        r->dict[i].str[len] = '\0';
    }
    return 0;
}

int rodo_read_value(FILE* f, Rodo* r, rodo_value_t* value) {
    if (fread(&value->type, 1, 1, f) != 1) return -1;
    switch (value->type) {
        case RODO_NULL:
            break;
        case RODO_BOOL:
            if (fread(&value->value.bool_val, 1, 1, f) != 1) return -1;
            break;
        case RODO_INT:
            if (rodo_read_zigzag(f, &value->value.int_val) != 0) return -1;
            break;
        case RODO_FLOAT:
            if (fread(&value->value.float_val, 8, 1, f) != 1) return -1;
            break;
        case RODO_STRING: {
            uint64_t id;
            if (rodo_read_varint(f, &id) != 0) return -1;
            const char* s = rodo_dict_get(r, (uint32_t)id);
            if (!s) return -1;
            value->value.string_val = strdup(s);
            if (!value->value.string_val) return -1;
            break;
        }
        case RODO_BYTES: {
            uint64_t len;
            if (rodo_read_varint(f, &len) != 0) return -1;
            value->value.bytes_val.len = (size_t)len;
            value->value.bytes_val.data = malloc(len ? len : 1);
            if (!value->value.bytes_val.data) return -1;
            if (fread(value->value.bytes_val.data, 1, len, f) != len) return -1;
            break;
        }
        case RODO_ARRAY: {
            uint64_t count;
            if (rodo_read_varint(f, &count) != 0) return -1;
            value->value.array_val.count = (size_t)count;
            value->value.array_val.items = calloc(count ? count : 1, sizeof(rodo_value_t));
            if (!value->value.array_val.items) return -1;
            for (size_t i = 0; i < count; i++) {
                if (rodo_read_value(f, r, &value->value.array_val.items[i]) != 0) return -1;
            }
            break;
        }
        case RODO_MAP: {
            uint64_t count;
            if (rodo_read_varint(f, &count) != 0) return -1;
            value->value.map_val.count = (size_t)count;
            value->value.map_val.pairs = calloc(count ? count : 1, sizeof(rodo_pair_t));
            if (!value->value.map_val.pairs) return -1;
            for (size_t i = 0; i < count; i++) {
                uint64_t key_id;
                if (rodo_read_varint(f, &key_id) != 0) return -1;
                const char* key = rodo_dict_get(r, (uint32_t)key_id);
                if (!key) return -1;
                value->value.map_val.pairs[i].key = strdup(key);
                if (!value->value.map_val.pairs[i].key) return -1;
                if (rodo_read_value(f, r, &value->value.map_val.pairs[i].value) != 0) return -1;
            }
            break;
        }
        case RODO_DATE:
            if (rodo_read_zigzag(f, &value->value.date_val) != 0) return -1;
            break;
        case RODO_UUID:
            if (fread(value->value.uuid_val, 1, 16, f) != 16) return -1;
            break;
        default:
            return -1;
    }
    return 0;
}

static int read_blocks(FILE* f, Rodo* r) {
    uint64_t count;
    if (rodo_read_varint(f, &count) != 0) return -1;
    r->block_count = (size_t)count;
    r->block_capacity = count ? count : 1;
    r->blocks = calloc(r->block_capacity, sizeof(rodo_block_t));
    if (!r->blocks) return -1;
    for (size_t i = 0; i < r->block_count; i++) {
        uint64_t bid;
        if (rodo_read_varint(f, &bid) != 0) return -1;
        r->blocks[i].block_id = (uint32_t)bid;
        uint64_t pcount;
        if (rodo_read_varint(f, &pcount) != 0) return -1;
        r->blocks[i].pair_count = (size_t)pcount;
        r->blocks[i].pairs = calloc(pcount ? pcount : 1, sizeof(rodo_kv_t));
        if (!r->blocks[i].pairs) return -1;
        for (size_t j = 0; j < pcount; j++) {
            uint64_t sym;
            if (rodo_read_varint(f, &sym) != 0) return -1;
            r->blocks[i].pairs[j].symbol_id = (uint32_t)sym;
            if (rodo_read_value(f, r, &r->blocks[i].pairs[j].value) != 0) return -1;
        }
    }
    return 0;
}

int rodo_load_file(Rodo* r) {
    if (!r->file) return -1;
    rewind(r->file);
    if (read_header(r->file, r) != 0) return -1;
    if (read_symbols(r->file, r) != 0) return -1;
    if (read_dict(r->file, r) != 0) return -1;
    if (read_blocks(r->file, r) != 0) return -1;
    // ignore metadata for now
    return 0;
}