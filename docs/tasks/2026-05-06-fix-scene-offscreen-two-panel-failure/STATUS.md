# Status

- Task: fix the failing `test_app_offscreen_two_panel_points_light_both_halves` scene/offscreen test.
- Date: 2026-05-06
- State: completed

## Plan

1. Reproduce the failure in the narrowest relevant test binary and inspect the failing assertions/data path. ✅
2. Identify and implement the smallest correct fix in scene/runtime/test code. ✅
3. Re-run focused validation, then broader relevant validation if practical. ✅

## Work completed

- Reproduced the failure directly and traced it to the active scene -> DRP2 -> runtime path ignoring panel rectangles in runtime render-pass execution.
- Extended scene frame-plan render/clear nodes to carry normalized panel rectangles.
- Extended DRP2 begin-render-pass commands to carry normalized target sub-regions plus an explicit clear/load behavior.
- Updated the runtime emitter so the first panel pass clears the target and later panel passes preserve prior contents while rendering only inside each panel region.
- Updated the vklite runtime to:
  - configure render passes for clear vs load correctly,
  - apply per-panel viewport/scissor rectangles,
  - include color-attachment read access when transitioning targets used with `LOAD`.
- Strengthened regression coverage by asserting the emitted DRP2 render-pass regions for multi-panel scene emission.
- Made the offscreen app test a bit more frame-robust while still asserting that both panel colors appear in the final output.

## Validation

- `git diff --check` ✅
- `cmake --build build --target dvztest_scene` ✅
- `cmake --build build --target dvztest` ✅
- `direnv exec . ./build/testing/dvztest_scene test_scene_multiple_panels_multiple_point_visuals_emit` ✅
- `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_two_panel_points_light_both_halves` ✅
- `direnv exec . just test scene` ✅ (`52/52` passed)

## Current state

- Multi-panel scene rendering now carries panel regions through to the runtime path.
- The previously failing offscreen two-panel app test now passes in isolation and in the unified scene suite.
