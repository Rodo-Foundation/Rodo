#include "rodo.h"
#include <stdio.h>
#include <string.h>

int main() {
    Rodo* r = rodo_link("test_basic.rd");
    if (!r) {
        printf("Failed to link\n");
        return 1;
    }
    rodo_value_t val;
    val.type = RODO_STRING;
    val.value.string_val = "Davi";
    if (rodo_set(r, "nome", &val) != 0) {
        printf("Set failed\n");
        return 1;
    }
    val.type = RODO_INT;
    val.value.int_val = 87;
    if (rodo_set(r, "level", &val) != 0) {
        printf("Set int failed\n");
        return 1;
    }
    rodo_value_t* v = rodo_get(r, "nome");
    if (v && v->type == RODO_STRING && strcmp(v->value.string_val, "Davi") == 0) {
        printf("Test passed\n");
    } else {
        printf("Test failed\n");
        return 1;
    }
    rodo_close(r);
    remove("test_basic.rd");
    return 0;
}