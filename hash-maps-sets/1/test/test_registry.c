#include "../src/registry.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    /* small correctness test */
    Registry r;
    registry_init(&r);
    registry_add(&r, 10);
    registry_add(&r, 20);
    registry_add(&r, 30);
    assert(registry_contains(&r, 10));
    assert(registry_contains(&r, 20));
    assert(registry_contains(&r, 30));
    assert(!registry_contains(&r, 99));
    registry_free(&r);

    /* large test: N adds + N lookups (naive is O(n^2) -> slow on large N) */
    int N = 100000;
    registry_init(&r);
    for (int i = 0; i < N; i++) registry_add(&r, i);
    for (int i = 0; i < N; i++) assert(registry_contains(&r, i));
    /* IDs that were never added must NOT be reported as present */
    for (int i = 0; i < N; i++) assert(!registry_contains(&r, N + i));
    registry_free(&r);

    printf("All tests passed.\n");
    return 0;
}
