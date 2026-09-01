# Challenge 1: Student ID Registry (Hash Set)

**Language:** C · **Difficulty:** easy · **Topic:** Hash maps / sets

## The problem
The `StudentRegistry` tracks student IDs. It works, but `registry_contains`
does a **linear scan** of the array — **O(n) per lookup**. With many students,
that's **O(n²)** total and gets slow.

## What to do
Implement a **hash set** (open addressing with linear probing, or chaining) so
that `registry_contains` is **O(1) average** instead of O(n).

- Replace the array-backed storage in `src/registry.c` with a hash table.
- Keep the same `registry.h` interface (`init` / `add` / `contains` / `free`).
- `registry_add` should insert into the hash table; `registry_contains` should
  hash the ID and look it up in O(1) average.

## Files
- `src/registry.h` — the interface (do not change).
- `src/registry.c` — the naive implementation (fix this).
- `test/test_registry.c` — correctness test (passes on small, slow on large).
- `benchmark/benchmark.c` — measures the naive cost.

## Build & run
Run from the **challenge directory** (`hash-maps-sets/1/`):

```bash
cd hash-maps-sets/1
# test
gcc -o test_registry test/test_registry.c src/registry.c && ./test_registry
# benchmark (output named benchmark_run to avoid clashing with the benchmark/ dir)
gcc -o benchmark_run benchmark/benchmark.c src/registry.c && ./benchmark_run
```

## Expected
- Correctness test passes.
- Benchmark shows the naive version is slow at N=100k; after the fix it should
  be dramatically faster (O(1) lookups).
