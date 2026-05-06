# Status

- Task: add a direct regression test for `dvz_visual_destroy()` while a live emitted stream exists.
- Date: 2026-05-06
- State: completed

## Plan

1. Inspect existing scene live-stream guard tests and locate the best fixture/helper pattern. ✅
2. Add a dedicated regression covering `dvz_visual_destroy()` before and after live stream destruction. ✅
3. Run the narrowest relevant validation and record results. ✅

## Work completed

- Added `test_scene_rejects_visual_destroy_while_emitted_stream_is_live()` in `src/scene/tests/test_scene.c`.
- Registered the new regression in the `test_scene()` suite.
- The new test explicitly verifies:
  - `dvz_visual_destroy()` logs the expected live-stream guard error while a stream is outstanding.
  - the visual remains intact after the rejected destroy attempt.
  - `dvz_visual_destroy()` succeeds after the stream is destroyed.

## Validation

- `git diff --check` ✅
- `cmake --build build --target dvztest_scene` ✅
- `direnv exec . ./build/testing/dvztest_scene test_scene_rejects_visual_destroy_while_emitted_stream_is_live` ✅
- `direnv exec . just test scene` ⚠️ unrelated existing failure in `test_app_offscreen_two_panel_points_light_both_halves`

## Current state

- Focused regression is in place and passing.
- No production code changes were required; this is test coverage only.
