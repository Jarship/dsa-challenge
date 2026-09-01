#ifndef REGISTRY_H
#define REGISTRY_H

typedef struct {
	int value;
	int occupied;
} Slot;

typedef struct {
    Slot *slots;
    int count;
    int capacity;
} Registry;

void registry_init(Registry *r);
void registry_add(Registry *r, int id);
int registry_contains(Registry *r, int id);
void registry_free(Registry *r);

#endif
