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
        	Slot *new_slots = realloc(r->slots, new_cap * sizeof(Slot));
        	if (!new_slots) return;
        	r->slots = new_slots;
        	r->capacity = new_cap;
	}
	int index = abs(id) % r->capacity;
	int i = index;
	while (r->slots[i].occupied) {
		if (r-> slots[i].value == id) return;
		i = (i + 1) % r->capacity;
		if (i == index) return;
	}   
    	r->slots[i].value = id;
    	r->slots[i].occupied = 1;
    	r->count++;
}

int registry_contains(Registry *r, int id) {
	int index = abs(id) % r->capacity;
	if (index >= r->capacity)
		return 0;
	int i = index;
	while (r->slots[i].occupied) {
		if (r->slots[i].value == id) return 1;
		i = (i + 1) % r->capacity;
		if (i == index)
			return 0;
	}
	return 0;
}

void registry_free(Registry *r) {
    free(r->slots);
    r->slots = NULL;
    r->count = 0;
    r->capacity = 0;
}
