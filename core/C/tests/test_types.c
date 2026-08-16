#include "rodo.h"
#include <stdio.h>
#include <stdint.h>

int main() {
    Rodo* r = rodo_link("test_types.rd");
    if (!r) return 1;
    // Test int, float, bool
    rodo_value_t v;
    v.type = RODO_INT;
    v.value.int_val = -12345;
    rodo_set(r, "neg", &v);
    v.type = RODO_FLOAT;
    v.value.float_val = 3.14159;
    rodo_set(r, "pi", &v);
    v.type = RODO_BOOL;
    v.value.bool_val = 1;
    rodo_set(r, "flag", &v);
    rodo_close(r);
    // Reopen
    r = rodo_link("test_types.rd");
    rodo_value_t* got = rodo_get(r, "pi");
    if (got && got->type == RODO_FLOAT && got->value.float_val > 3.14 && got->value.float_val < 3.15) {
        printf("Float OK\n");
    } else {
        printf("Float failed\n");
    }
    got = rodo_get(r, "neg");
    if (got && got->type == RODO_INT && got->value.int_val == -12345) {
        printf("Int OK\n");
    } else {
        printf("Int failed\n");
    }
    rodo_close(r);
    remove("test_types.rd");
    return 0;
}