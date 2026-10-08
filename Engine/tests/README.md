# Engine tests

Gate G1: the refactored engine must behave exactly like the original OpenKey engine.

| Piece | What it is |
|---|---|
| `original/` | Frozen, unmodified copy of OpenKey's engine (see its README). Never linked into the app. |
| `ref/` | Driver + generator for the original engine. Every case runs in a forked process because the original keeps state in globals. |
| `common/cases.cpp` | The case list (key DSL, configs) and the renderer. Shared by both drivers. |
| `vk/driver_vk.cpp` | Same driver interface on top of the public C API (`viskey_engine.h`). |
| `snapshots/engine.snap` | Output of the original engine: per key `code / backspace / chars / ext` as raw engine words, plus the simulated screen text. |
| `snapshot_test.cpp` | Catch2: C API output must equal the snapshot byte for byte. |
| `api_test.cpp` | Catch2: C API contract (independent engines, long macros, blobs, truncation, null handles). |

## Commands

```sh
cmake -S Engine -B build/engine && cmake --build build/engine -j
ctest --test-dir build/engine --output-on-failure
```

`ctest` also runs `snapshot_matches_original_engine`, which regenerates the snapshot from `original/`
and checks it equals the committed file, so the snapshot cannot drift from the original engine.

## Changing a snapshot

`snapshots/engine.snap` is pinned to the original engine and must never be edited by hand
(`snapshot_matches_original_engine` enforces it). It records original behaviour, bugs included; a case that
shows a known engine bug is tagged `[known-bug]` in `cases.cpp`.

Fixing such a bug (or any intentional behaviour change) is a separate commit that adds the affected blocks
to `snapshots/overrides.snap` (same block format; a block there replaces the one in `engine.snap`) and
states the reason in the commit message and in `docs/decisions.md`.

Adding cases: edit `common/cases.cpp`, then regenerate from the original engine:

```sh
./build/engine/tests/gen_snapshot_original Engine/tests/snapshots/engine.snap
```
