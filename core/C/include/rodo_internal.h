#ifndef RODO_INTERNAL_H
#define RODO_INTERNAL_H

#include "rodo.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define RODO_MAGIC      0x52443031
#define RODO_VERSION_MAJOR 1
#define RODO_VERSION_MINOR 0

typedef struct {
    uint32_t magic;
    uint8_t  version_major;
    uint8_t  version_minor;
    uint8_t  flags;
    uint64_t timestamp;
    uint32_t header_crc32;
} rodo_header_t;

typedef struct {
    uint32_t id;
    char*    name;
    uint8_t  type_hint;
    uint8_t  flags;
} rodo_symbol_t;

typedef struct {
    uint32_t id;
    char*    str;
} rodo_dict_entry_t;

typedef struct {
    uint32_t symbol_id;
    rodo_value_t value;
} rodo_kv_t;

typedef struct {
    uint32_t block_id;
    size_t   pair_count;
    rodo_kv_t* pairs;
} rodo_block_t;

struct rodo {
    FILE* file;
    char* path;
    rodo_header_t header;
    rodo_symbol_t* symbols;
    size_t symbol_count;
    size_t symbol_capacity;
    rodo_dict_entry_t* dict;
    size_t dict_count;
    size_t dict_capacity;
    rodo_block_t* blocks;
    size_t block_count;
    size_t block_capacity;
    int dirty;
};

uint32_t rodo_crc32(const uint8_t* data, size_t len);

int rodo_write_varint(FILE* f, uint64_t value);
int rodo_read_varint(FILE* f, uint64_t* value);
int rodo_write_zigzag(FILE* f, int64_t value);
int rodo_read_zigzag(FILE* f, int64_t* value);

int rodo_write_value(FILE* f, Rodo* r, const rodo_value_t* value);
int rodo_read_value(FILE* f, Rodo* r, rodo_value_t* value);

uint32_t rodo_symbol_add(Rodo* r, const char* name, uint8_t type_hint);
const char* rodo_symbol_get_name(const Rodo* r, uint32_t id);
int32_t rodo_symbol_find(const Rodo* r, const char* name);

uint32_t rodo_dict_add(Rodo* r, const char* str);
const char* rodo_dict_get(const Rodo* r, uint32_t id);
int32_t rodo_dict_find(const Rodo* r, const char* str);

int rodo_load_file(Rodo* r);
int rodo_save_file(Rodo* r);

#endif