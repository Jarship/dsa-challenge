#include "../src/registry.h"
#include <stdio.h>
#include <time.h>

int main(void) {
    int N = 100000;
    Registry r;
    registry_init(&r);

    clock_t start = clock();
    for (int i = 0; i < N; i++) registry_add(&r, i);
    for (int i = 0; i < N; i++) registry_contains(&r, i);
    clock_t end = clock();

    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    printf("N=%d: %.3f seconds (naive: O(n) per lookup, O(n^2) total)\n", N, seconds);
    registry_free(&r);
    return 0;
}
