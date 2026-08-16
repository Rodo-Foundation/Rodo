#include "rodo_internal.h"
#include <stdlib.h>
#include <string.h>

static int dict_resize(Rodo* r, size_t new_capacity) {
    rodo_dict_entry_t* new = realloc(r->dict, new_capacity * sizeof(rodo_dict_entry_t));
    if (!new) return -1;
    r->dict = new;
    r->dict_capacity = new_capacity;
    return 0;
}

uint32_t rodo_dict_add(Rodo* r, const char* str) {
    int32_t existing = rodo_dict_find(r, str);
    if (existing >= 0) return (uint32_t)existing;
    if (r->dict_count >= r->dict_capacity) {
        size_t new_cap = r->dict_capacity ? r->dict_capacity * 2 : 8;
        if (dict_resize(r, new_cap) != 0) return 0;
    }
    uint32_t id = (uint32_t)(r->dict_count + 1);
    r->dict[r->dict_count].id = id;
    r->dict[r->dict_count].str = strdup(str);
    if (!r->dict[r->dict_count].str) return 0;
    r->dict_count++;
    r->dirty = 1;
    return id;
}

const char* rodo_dict_get(const Rodo* r, uint32_t id) {
    for (size_t i = 0; i < r->dict_count; i++) {
        if (r->dict[i].id == id) return r->dict[i].str;
    }
    return NULL;
}

int32_t rodo_dict_find(const Rodo* r, const char* str) {
    for (size_t i = 0; i < r->dict_count; i++) {
        if (strcmp(r->dict[i].str, str) == 0) return (int32_t)r->dict[i].id;
    }
    return -1;
}