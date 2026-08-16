#include "rodo_internal.h"
#include <stdlib.h>
#include <string.h>

Rodo* rodo_link(const char* path) {
    Rodo* r = calloc(1, sizeof(Rodo));
    if (!r) return NULL;
    r->path = strdup(path);
    if (!r->path) {
        free(r);
        return NULL;
    }
    r->file = fopen(path, "rb+");
    if (r->file) {
        // existing file
        if (rodo_load_file(r) != 0) {
            fclose(r->file);
            free(r->path);
            free(r);
            return NULL;
        }
    } else {
        // new file
        r->file = fopen(path, "wb+");
        if (!r->file) {
            free(r->path);
            free(r);
            return NULL;
        }
        r->header.magic = RODO_MAGIC;
        r->header.version_major = RODO_VERSION_MAJOR;
        r->header.version_minor = RODO_VERSION_MINOR;
        r->header.flags = 0;
        r->header.timestamp = 0;
        r->header.header_crc32 = 0;
    }
    return r;
}

void rodo_close(Rodo* r) {
    if (!r) return;
    if (r->dirty) {
        rodo_save_file(r);
    }
    if (r->file) fclose(r->file);
    for (size_t i = 0; i < r->symbol_count; i++) {
        free(r->symbols[i].name);
    }
    free(r->symbols);
    for (size_t i = 0; i < r->dict_count; i++) {
        free(r->dict[i].str);
    }
    free(r->dict);
    for (size_t i = 0; i < r->block_count; i++) {
        for (size_t j = 0; j < r->blocks[i].pair_count; j++) {
            rodo_value_t* v = &r->blocks[i].pairs[j].value;
            if (v->type == RODO_STRING) free(v->value.string_val);
            else if (v->type == RODO_BYTES) free(v->value.bytes_val.data);
            else if (v->type == RODO_ARRAY) {
                for (size_t k = 0; k < v->value.array_val.count; k++) {
                    // TODO: recursive free
                }
                free(v->value.array_val.items);
            } else if (v->type == RODO_MAP) {
                for (size_t k = 0; k < v->value.map_val.count; k++) {
                    free(v->value.map_val.pairs[k].key);
                    // TODO: recursive free value
                }
                free(v->value.map_val.pairs);
            }
        }
        free(r->blocks[i].pairs);
    }
    free(r->blocks);
    free(r->path);
    free(r);
}

static rodo_block_t* find_block(Rodo* r, uint32_t symbol_id) {
    // For simplicity, we use a single block (block_id 1) as default.
    // In a real implementation, blocks could be used for multiple records.
    if (r->block_count == 0) {
        if (r->block_capacity < 1) {
            r->block_capacity = 1;
            r->blocks = calloc(1, sizeof(rodo_block_t));
            if (!r->blocks) return NULL;
        }
        r->blocks[0].block_id = 1;
        r->blocks[0].pair_count = 0;
        r->blocks[0].pairs = NULL;
        r->block_count = 1;
    }
    return &r->blocks[0];
}

int rodo_set(Rodo* r, const char* key, const rodo_value_t* value) {
    if (!r || !key || !value) return -1;
    uint8_t type_hint = value->type;
    uint32_t symbol_id = rodo_symbol_add(r, key, type_hint);
    if (symbol_id == 0) return -1;
    rodo_block_t* block = find_block(r, symbol_id);
    if (!block) return -1;
    // Check if key already exists
    for (size_t i = 0; i < block->pair_count; i++) {
        if (block->pairs[i].symbol_id == symbol_id) {
            // replace existing value (free old value first)
            rodo_value_t* old = &block->pairs[i].value;
            if (old->type == RODO_STRING) free(old->value.string_val);
            else if (old->type == RODO_BYTES) free(old->value.bytes_val.data);
            // shallow free others for simplicity
            block->pairs[i].value = *value; // shallow copy, caller must not free
            r->dirty = 1;
            return 0;
        }
    }
    // Add new pair
    rodo_kv_t* new_pairs = realloc(block->pairs, (block->pair_count + 1) * sizeof(rodo_kv_t));
    if (!new_pairs) return -1;
    block->pairs = new_pairs;
    block->pairs[block->pair_count].symbol_id = symbol_id;
    block->pairs[block->pair_count].value = *value; // shallow copy
    block->pair_count++;
    r->dirty = 1;
    return 0;
}

rodo_value_t* rodo_get(const Rodo* r, const char* key) {
    if (!r || !key) return NULL;
    int32_t symbol_id = rodo_symbol_find(r, key);
    if (symbol_id < 0) return NULL;
    for (size_t i = 0; i < r->block_count; i++) {
        for (size_t j = 0; j < r->blocks[i].pair_count; j++) {
            if (r->blocks[i].pairs[j].symbol_id == (uint32_t)symbol_id) {
                return &r->blocks[i].pairs[j].value;
            }
        }
    }
    return NULL;
}

int rodo_has(const Rodo* r, const char* key) {
    return rodo_get(r, key) != NULL;
}

int rodo_delete(Rodo* r, const char* key) {
    if (!r || !key) return -1;
    int32_t symbol_id = rodo_symbol_find(r, key);
    if (symbol_id < 0) return 0; // not found, nothing to delete
    for (size_t i = 0; i < r->block_count; i++) {
        for (size_t j = 0; j < r->blocks[i].pair_count; j++) {
            if (r->blocks[i].pairs[j].symbol_id == (uint32_t)symbol_id) {
                // free value
                rodo_value_t* v = &r->blocks[i].pairs[j].value;
                if (v->type == RODO_STRING) free(v->value.string_val);
                else if (v->type == RODO_BYTES) free(v->value.bytes_val.data);
                // shift remaining pairs
                for (size_t k = j; k < r->blocks[i].pair_count - 1; k++) {
                    r->blocks[i].pairs[k] = r->blocks[i].pairs[k + 1];
                }
                r->blocks[i].pair_count--;
                r->dirty = 1;
                return 0;
            }
        }
    }
    return 0;
}

size_t rodo_keys(const Rodo* r, char*** keys_out) {
    if (!r || !keys_out) return 0;
    size_t count = 0;
    for (size_t i = 0; i < r->block_count; i++) {
        count += r->blocks[i].pair_count;
    }
    char** keys = calloc(count ? count : 1, sizeof(char*));
    if (!keys) return 0;
    size_t idx = 0;
    for (size_t i = 0; i < r->block_count; i++) {
        for (size_t j = 0; j < r->blocks[i].pair_count; j++) {
            const char* name = rodo_symbol_get_name(r, r->blocks[i].pairs[j].symbol_id);
            if (name) {
                keys[idx++] = strdup(name);
            }
        }
    }
    *keys_out = keys;
    return idx;
}

int rodo_all(const Rodo* r, rodo_value_t* out_map) {
    // Simplified: not implemented properly
    return -1;
}