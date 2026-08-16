#include "rodo.h"
#include <stdio.h>
#include <string.h>

int main() {
    const char* path = "test_persistence.rd";
    Rodo* r = rodo_link(path);
    rodo_value_t v;
    v.type = RODO_STRING;
    v.value.string_val = "persist";
    rodo_set(r, "key", &v);
    rodo_close(r);
    // Reopen and check
    r = rodo_link(path);
    rodo_value_t* got = rodo_get(r, "key");
    if (got && got->type == RODO_STRING && strcmp(got->value.string_val, "persist") == 0) {
        printf("Persistence OK\n");
    } else {
        printf("Persistence failed\n");
    }
    rodo_close(r);
    remove(path);
    return 0;
}