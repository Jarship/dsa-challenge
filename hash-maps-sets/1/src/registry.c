#include "registry.h"
#include <stdlib.h>

static const unsigned int HASH_MULT = 2654435761u;

void registry_init(Registry *r) {
    r->slots = NULL;
    r->count = 0;
    r->capacity = 0;
}

int calculate_new_index(int id, int capacity) {
	return (int)((unsigned)id * HASH_MULT % (unsigned)capacity);
}

int registry_calculate_index(Registry *r, int id) {
	return calculate_new_index(id, r->capacity);
}

void registry_add(Registry *r, int id) {
	int index = 0;
	if (r->count == r->capacity) {
        	int new_cap = r->capacity == 0 ? 32 : r->capacity * 2;
		Slot *new_slots;
		if (r-> count <= 0)
			new_slots = realloc(r->slots, sizeof(Slot) * new_cap);
		else {
        		new_slots = calloc(new_cap, new_cap * sizeof(Slot));
			for (int i = 0; i < r-> capacity; i++) {
				if (r->slots[index].occupied) {
					int new_index = calculate_new_index(r->slots[index].value, new_cap);
					new_slots[new_index] = r->slots[index];
					index = registry_calculate_index(r, (index + 1) % r->capacity);
				}
			}
		}
		Slot *old_address = r->slots;
        	r->slots = new_slots;
		free(old_address);
        	r->capacity = new_cap;
	}
	index = registry_calculate_index(r, id);
	if (index >= r->capacity) return;
	int i = index;
	while (r->slots[i].occupied) {
		if (r->slots[i].value == id) return;
		i = (i + 1) % r->capacity;
		if (i == index) return;
	}
	Slot s = { .value = id, .occupied = 1 };
    	r->slots[i] = s;
    	r->count++;
}

int registry_contains(Registry *r, int id) {
	int index = registry_calculate_index(r, id);
	if (index >= r->capacity)
		return 0;
	int i = index;
	while (r->slots[i].occupied) {
		if (r->slots[i].value == id) return 1;
		i = (i + 1) % r->capacity;
		if (i == index) return 0;
	}
	return 0;
}

void registry_free(Registry *r) {
    free(r->slots);
    r->slots = NULL;
    r->count = 0;
    r->capacity = 0;
}
