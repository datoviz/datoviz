# Status

- Task: fix macOS build failure in `src/scene/scene.c` background color initialization.
- Started: 2026-05-07
- State: in progress

## Plan

1. Patch the `DvzColor` array initialization in `dvz_panel_set_background_color()` so it compiles on macOS.
2. Rebuild the affected target and run a focused scene validation if available.
3. Record the validation result, current state, and any residual risks.

## Update

- Fixed the invalid `DvzColor` array initialization by using per-element brace initializers.
- Validation: `cmake --build build --target datoviz_scene -j4` passed.
- Validation: `cmake --build build --target dvztest_scene -j4` passed.
- Validation: `./build/testing/dvztest_scene` completed, but `test_panzoom_zoom_wheel` failed and appears unrelated to this build fix.
- Validation: `git diff --check` passed.
