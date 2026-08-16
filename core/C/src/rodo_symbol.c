#include "rodo_internal.h"
#include <stdlib.h>
#include <string.h>

static int symbol_resize(Rodo* r, size_t new_capacity) {
    rodo_symbol_t* new = realloc(r->symbols, new_capacity * sizeof(rodo_symbol_t));
    if (!new) return -1;
    r->symbols = new;
    r->symbol_capacity = new_capacity;
    return 0;
}

uint32_t rodo_symbol_add(Rodo* r, const char* name, uint8_t type_hint) {
    int32_t existing = rodo_symbol_find(r, name);
    if (existing >= 0) return (uint32_t)existing;
    if (r->symbol_count >= r->symbol_capacity) {
        size_t new_cap = r->symbol_capacity ? r->symbol_capacity * 2 : 8;
        if (symbol_resize(r, new_cap) != 0) return 0;
    }
    uint32_t id = (uint32_t)(r->symbol_count + 1);
    r->symbols[r->symbol_count].id = id;
    r->symbols[r->symbol_count].name = strdup(name);
    r->symbols[r->symbol_count].type_hint = type_hint;
    r->symbols[r->symbol_count].flags = 0;
    if (!r->symbols[r->symbol_count].name) return 0;
    r->symbol_count++;
    r->dirty = 1;
    return id;
}

const char* rodo_symbol_get_name(const Rodo* r, uint32_t id) {
    for (size_t i = 0; i < r->symbol_count; i++) {
        if (r->symbols[i].id == id) return r->symbols[i].name;
    }
    return NULL;
}

int32_t rodo_symbol_find(const Rodo* r, const char* name) {
    for (size_t i = 0; i < r->symbol_count; i++) {
        if (strcmp(r->symbols[i].name, name) == 0) return (int32_t)r->symbols[i].id;
    }
    return -1;
}