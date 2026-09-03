# Common Bugs — Challenge 1 Post-Mortem

Where these come from: commit-by-commit archaeology of `hash-maps-sets/1`
(first solution commit `d147eda`, final `ed0ca4a`) plus review evidence from
2026-09-03. The headline fact: **the official test suite passed while most of
these bugs were live.** It only caught the O(n²) slowness — every correctness
bug below was found by external review, not by the challenge's own test.

Status markers: **[proven]** = reproduced/verified by running code; **[diff]** =
visible in the commit diff but not separately reproduced.

---

## 1. Bug timeline, commit by commit

### `d147eda` — fix: update to hashset (first attempt)
- **Silent drop on collision [proven-by-read]** — `if (slots[index].occupied == 1) return;` — any second ID hashing to an occupied slot is dropped, no error. Data loss.
- **Off-by-one guard, wrong direction [diff]** — `if (index > r->capacity) return 0;` lets `index == capacity` through to `slots[index]` (OOB read); and because the index came from `% capacity`, the guard could never fire for non-negative IDs anyway. Negative IDs (see below) produce *negative* indices, which this guard also misses.
- **No collision probing, no rehash on resize** — resizing moves the array but not the elements; post-resize lookups probe from new hash positions and can hit empty slots before reaching stale-position elements → false negatives.

### `aaf4a34` — fix: probe forward to add and contain
- **Linear probing added, but insert writes to the wrong address [proven-by-read]** — probe loop finds empty slot `i`, then writes `slots[index]` (the original hash slot). On collision this **overwrites a different, live element**. Direct cause of "contains returns false for a value I added."
- **Probe loop with no termination guard [proven-by-read]** — `while (occupied) i = (i+1) % cap;` spins forever if the table is full.

### `bb233b2` — fix: registry does not contain a value at capacity
- **Bounds-guard adjustment that didn't address the cause** — changed `index > r->capacity` to `>=`. Both forms are dead for `%`-produced indices (a `% cap` result is strictly `< cap` for non-negative operands). The "value not found" symptom actually came from the overwrite bug above (fixed later at `e41d844`). Lesson: when a guard fix doesn't make the symptom go away, the bug is elsewhere — keep digging instead of shipping the guard.

### `fb602df` — fix: handle negative ids
- **`abs(id) % capacity`** — works for ordinary negatives, but: `abs(INT_MIN)` is undefined behavior; and `abs()` folds sign, so `-7` and `7` share a probe start (harmless for correctness, worse collisions). Superseded by the unsigned-cast pipeline at `378683e`.

### `e41d844` — fix: set add values at the correct address; fix: guard against infinite loops
- Insert now lands at the probed empty slot `i` (fixed the overwrite corruption).
- Wraparound guard added: `if (i == index) return;` — but note it **silently drops** the insert when the table is full rather than failing loudly (see meta-pattern 6).

### `540534b` — refactor: improve hash function
- **Knuth multiplicative hash** (`2654435761u`) — good choice, odd multiplier scatters sequential IDs well.
- **Residual type bug [diff]** — `(int)((unsigned)id * HASH_MULT) % r->capacity`: the cast back to `int` makes the product signed again → can be negative → signed `%` → negative index → OOB. The unsigned discipline only completed at `378683e` (`% (unsigned)capacity`).

### `0071488` — fix: guard add
- **Guard that cannot fire** — added `if (index >= r->capacity) return;`. For a `%`-derived index this is unreachable; meanwhile the actual danger at that moment was a *negative* index from signed arithmetic (unfixed until `378683e`). Lesson: write down the mathematical range of the value before writing bounds checks.

### `846fd34` — chore: try re-hashing (experimental, badly broken)
- **NULL dereference [diff]** — `new_slots` left `NULL` in the rehash branch, then written through: `new_slots[new_index] = ...` → segfault on first resize with data.
- **Off-by-one loop bound [diff]** — `while (index <= r->capacity)` reads `slots[capacity]`.
- **Loop variable set to a hash, not an increment [diff]** — `index = registry_calculate_index(r, (index + 1) % capacity)` — the loop counter becomes a hash value → uncontrolled iteration / non-termination.

### `172e8f1` — refactor: clean up
- Replaced the broken experimental rehash with calloc/realloc split — but introduced `calloc(new_cap, new_cap * sizeof(Slot))` (swapped `calloc` args; over-allocates, wrong semantics) and **kept `realloc` for the first allocation** → the uninitialized-memory bug survives (fixed arg order later at `76c594a`; the realloc itself survives until `3dae2ef`).

### `378683e` — fix: calculate_new_index as a general purpose util
- Proper unsigned pipeline: `(unsigned)id * HASH_MULT % (unsigned)capacity` — result guaranteed in `[0, capacity)`. Kills the negative-index class.

### `76c594a` — fix: re-hash uses i; fix: calloc; fix: handle collisions when re-hashing
- Fixed the calloc argument order (`calloc(new_cap, sizeof(Slot))`) — but only in the rehash branch. **The initial-allocation `realloc` survived; the commit message claims "fix: calloc"** → uninit-memory bug still shipped.
- "Re-hash uses i" — fixed the **copy** (`new_slots[new_index] = r->slots[i]`) but the **hash** still used `slots[index]` (separate counter). Half the fix.
- Collision probe added inside rehash (`while (new_slots[new_index].occupied) ...`) — correct.

### `3dae2ef` — fix: initialization; fix: reference i not index  ← *the real fixes*
- **Uninitialized memory [proven — the killer]** — first resize used `realloc(r->slots, ...)` = `malloc(NULL, ...)` → `occupied` fields are garbage. Fresh processes get zeroed pages, so it *looks* fine. The challenge test creates two registries; the second one's first allocation reuses the first one's freed chunk → stale `occupied=1` bytes → phantom entries → wrong lookups + silent drops + `count` desync (observed: `capacity=32, count=1` after 69 adds, 38 IDs missing). Fix: `calloc` everywhere (nothing to preserve when `slots == NULL`). After the fix: all tests pass, 131k-element sweep clean.
- **Rehash wrong-slot bug [proven]** — hashing `r->slots[index].value` while copying `r->slots[i]`. Worked only because resize fired at exactly 100% load (dense table ⇒ `index == i`). Proven latent: with the same code + resize at 70% load, 16/37 elements vanish immediately.

### `5a78fc8` — capacity guard (half-applied) + one calloc + 70% load
- Resize at 70% load — correct, verified amortized O(1) (1M ops in 0.09–0.12s).
- Capacity guard added — **to `add` only**; `contains` still crashed on an empty registry.

### `ed0ca4a` — fix: avoid 0 modulo division (final)
- `if (r->capacity <= 0) return 0;` added to `contains` **before** the modulo — empty-registry call returns 0 instead of SIGFPE [proven: exit 136 → 0].
- Dead `index >= r->capacity` guards removed from both functions.
- Challenge complete: correctness, crash-safety, O(1) amortized, ~46ns/op at 1M scale.

---

## 2. Recurring meta-patterns (the lessons that generalize)

**M1 — Half-applied fixes (occurred 3×: `76c594a` calloc + rehash-i, `5a78fc8` capacity guard).**
A commit claims a fix; only part of the pattern gets patched. Remedy: after any fix, grep for *every* occurrence of the pattern, re-run the previously failing test in a fresh process, and only then believe the message.

**M2 — Guard placed after the dangerous operation.**
The empty-capacity check existed since the first hash-set commit — *after* the modulo. A check that runs after the crash it's meant to prevent is decoration. Remedy: guard before the operation; test every API on the empty state.

**M3 — Dead defensive guards while the real danger ships unguarded.**
`index >= capacity` (impossible for `%`-derived indices) guarded nothing, while negative indices from signed arithmetic — the real OOB vector — went unguarded for four commits. Remedy: establish the value range on paper first; delete guards that can't fire.

**M4 — Wrong-variable bugs in rehash loops (`index` vs `i`).**
A second counter drifts from the loop variable; works only under special conditions. Remedy: one loop variable; hash the same slot you copy.

**M5 — Masked bugs: one design choice hiding another bug.**
The rehash bug was invisible because resize-at-100% guaranteed a dense table. Changing the policy exposed it [proven]. Remedy: when two mechanisms interact, test the interaction, not just each in its current configuration.

**M6 — Silent data-loss paths.**
Early `return` on occupied slot (first version) and on full-table probe (`if (i == index) return;`) drop elements with no error. Remedy: every operation either succeeds observably or reports failure — never vanishes.

**M7 — Uninitialized memory.**
`malloc`/`realloc` for flag-carrying structs; behavior depends on allocator history [proven with a two-registry repro]. Remedy: zeroing allocator for flag-carrying structs; test ≥2 create→free→recreate cycles per process.

**M8 — Probe loops without termination guarantees.**
`while occupied: advance` spins forever on a full table. Remedy: termination check *inside* the loop, before assuming an empty slot exists.

**M9 — Type discipline in hashing.**
`abs(INT_MIN)` is UB; `(int)(unsigned)x * M` re-signs the product → negative `%` → OOB. Remedy: all wrap/index arithmetic in unsigned; produce the final index once, at the end.

**M10 — Growth-policy performance cliffs.**
Resize at 100% load ⇒ linear probing costs ~1/(1-α)² near-full; resize without rehash ⇒ false negatives. Remedy: grow at ~0.7 load, rehash correctly on every growth.

---

## 3. Edge cases the challenge test suite missed

The official suite used sequential IDs, a single registry, and only post-add
lookups. Each row = a bug that shipped invisibly because of it:

| # | Edge case | Bug it would have caught |
|---|---|---|
| 1 | `contains` on an empty structure | `% 0` → SIGFPE (survived 3 review rounds) |
| 2 | Negative keys (+ INT_MIN) | negative index OOB; `abs(INT_MIN)` UB |
| 3 | Duplicate inserts | silent drops; count semantics |
| 4 | ≥2 create→free→recreate cycles, one process | uninitialized `occupied` from chunk reuse |
| 5 | Adversarial keys (`i << 20`, arithmetic progressions) | hash clustering / low-bit collisions |
| 6 | Sizes straddling every resize boundary | rehash wrong-slot bug, off-by-ones |
| 7 | Absent keys interleaved (not just a trailing block) | overwrite corruption, stale-position false negatives |
| 8 | `count == distinct inserted` invariant after each phase | silent drops, double-counting |
| 9 | Probe-termination stress (table at capacity) | infinite loops |

---

## 4. Discipline going forward

1. **Every fix ends with**: grep all occurrences of the pattern → re-run full suite → re-run the previously failing test in a fresh process.
2. **Test every API in every state**: empty, partial, at-capacity, after-resize.
3. **Zeroing allocators** for anything carrying flags/state.
4. **Unsigned for wrap math**; signed only at the final index.
5. **Guards go before** the operation they protect; delete guards that can't fire.
6. **Change one policy at a time**, then re-verify the mechanisms it interacts with (resize ↔ rehash ↔ probe).