#ifndef RODO_H
#define RODO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rodo Rodo;

typedef enum {
    RODO_NULL   = 0x00,
    RODO_BOOL   = 0x01,
    RODO_INT    = 0x02,
    RODO_FLOAT  = 0x03,
    RODO_STRING = 0x04,
    RODO_BYTES  = 0x05,
    RODO_ARRAY  = 0x06,
    RODO_MAP    = 0x07,
    RODO_DATE   = 0x08,
    RODO_UUID   = 0x09
} rodo_type_t;

typedef struct rodo_value rodo_value_t;
typedef struct rodo_pair  rodo_pair_t;

struct rodo_value {
    rodo_type_t type;
    union {
        int bool_val;
        int64_t int_val;
        double  float_val;
        char* string_val;
        struct { size_t len; uint8_t* data; } bytes_val;
        struct { size_t count; rodo_value_t* items; } array_val;
        struct { size_t count; rodo_pair_t* pairs; } map_val;
        int64_t date_val;
        uint8_t uuid_val[16];
    } value;
};

struct rodo_pair {
    char* key;
    rodo_value_t value;
};

Rodo* rodo_link(const char* path);
void  rodo_close(Rodo* r);
int   rodo_set(Rodo* r, const char* key, const rodo_value_t* value);
rodo_value_t* rodo_get(const Rodo* r, const char* key);
int   rodo_has(const Rodo* r, const char* key);
int   rodo_delete(Rodo* r, const char* key);
size_t rodo_keys(const Rodo* r, char*** keys_out);
int   rodo_all(const Rodo* r, rodo_value_t* out_map);

#ifdef __cplusplus
}
#endif

#endif