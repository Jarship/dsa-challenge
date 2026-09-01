#include "registry.h"
#include <stdlib.h>

void registry_init(Registry *r) {
    r->slots = NULL;
    r->count = 0;
    r->capacity = 0;
}

void registry_add(Registry *r, int id) {
	if (r->count == r->capacity) {
        int new_cap = r->capacity == 0 ? 16 : r->capacity * 2;
        int *new_slots = realloc(r->slots, new_cap * sizeof(Slot));
        if (!new_slots) return;
        r->slots = (Slot *)new_slots;
        r->capacity = new_cap;
    }
    int index = id % r->capacity;
    if (r->slots[index].occupied == 1)
	    return;
    r->slots[index].value = id;
    r->slots[index].occupied = 1;
}

int registry_contains(Registry *r, int id) {
	int index = id % r->capacity;
	if (index > r->capacity)
		return 0;
	return r->slots[index].occupied == 1 ? 1 : 0;
}

void registry_free(Registry *r) {
    free(r->slots);
    r->slots = NULL;
    r->count = 0;
    r->capacity = 0;
}
