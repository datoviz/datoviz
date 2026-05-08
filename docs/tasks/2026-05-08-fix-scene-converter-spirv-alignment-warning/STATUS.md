# Status

- Task: fix the macOS `-Wcast-align` warning in `src/scene/converter.c` when emitting precompiled SPIR-V.
- Started: 2026-05-08
- State: done

## Plan

1. Remove the unsafe SPIR-V pointer cast by making the DRP2 shader-module path accept raw bytes and by copying to aligned storage before Vulkan use.
2. Rebuild the affected targets to confirm the warning is gone.
3. Run `git diff --check` and record any residual risks.

## Update

- Switched the DRP2 SPIR-V command path from `const uint32_t*` to raw byte pointers so `scene/converter.c` no longer needs an alignment-unsafe cast.
- Added aligned runtime copying before `dvz_shader()` consumes SPIR-V bytes.
- Validation: `cmake --build build --target datoviz_scene dvztest_integration -j4` passed with no `-Wcast-align` warning.
- Validation: `direnv exec . just test scene` passed (`83/83` tests).
- Validation: `git diff --check` passed.
