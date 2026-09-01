#include "registry.h"
#include <stdlib.h>

void registry_init(Registry *r) {
    r->ids = NULL;
    r->count = 0;
    r->capacity = 0;
}

void registry_add(Registry *r, int id) {
    if (r->count == r->capacity) {
        int new_cap = r->capacity == 0 ? 16 : r->capacity * 2;
        int *new_ids = realloc(r->ids, new_cap * sizeof(int));
        if (!new_ids) return;
        r->ids = new_ids;
        r->capacity = new_cap;
    }
    r->ids[r->count++] = id;
}

int registry_contains(Registry *r, int id) {
    for (int i = 0; i < r->count; i++) {
        if (r->ids[i] == id) return 1;
    }
    return 0;
}

void registry_free(Registry *r) {
    free(r->ids);
    r->ids = NULL;
    r->count = 0;
    r->capacity = 0;
}
