# Status

- Task: normalize wheel sign handling at the backend boundary and remove the scene test branch.
- Started: 2026-05-07
- State: in progress

## Plan

1. Normalize wheel sign in the GLFW backend so higher layers see one convention.
2. Make the panzoom wheel helper and test use the same cross-platform expectation.
3. Rebuild and run the focused scene suite, then record the result.

## Update

- Normalized macOS wheel direction in `src/window/backend_glfw.c` before emitting pointer wheel
  events.
- Restored `test_panzoom_zoom_wheel` to a single positive/negative wheel expectation.
- Kept the helper coefficient positive on macOS so a positive wheel delta zooms in everywhere.
- Validation: `cmake --build build --target dvztest_scene -j4` passed.
- Validation: `./build/testing/dvztest_scene` passed all `76/76` scene tests.
- Validation: `git diff --check` passed.
