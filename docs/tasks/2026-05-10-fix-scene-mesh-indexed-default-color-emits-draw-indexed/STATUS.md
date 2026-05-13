# Status

- Task: fix `test_scene_mesh_indexed_default_color_emits_draw_indexed` in `src/scene/tests/test_scene.c`.
- Started: 2026-05-10
- State: completed

## Plan

1. Reproduce the failing assertion and inspect the emitted DRP2 command stream.
2. Patch the test or scene emission so indexed mesh draws are asserted correctly.
3. Re-run the focused scene test, then record validation and remaining risks.

## Update

- Reproduced the failure and confirmed the emitted indexed-mesh draw uses `index_format: "uint32"` because `DvzIndex` is 32-bit in this branch.
- Updated the indexed-mesh and indexed-primitive scene tests to expect `uint32` instead of stale `uint16`.

## Validation

- `cmake --build build --target dvztest_scene -j4`
- `./build/testing/dvztest_scene test_scene_mesh_indexed_default_color_emits_draw_indexed`
- `./build/testing/dvztest_scene test_scene_indexed_primitive_emits_draw_indexed`
- `git diff --check`

## Current state

- The indexed draw tests now match the scene converter's current command stream encoding.
