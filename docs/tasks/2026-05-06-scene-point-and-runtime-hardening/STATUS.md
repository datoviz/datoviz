# Task Status

- Task: scene point-slice completion and scene/DRP2 runtime-boundary hardening
- Date: 2026-05-06
- Status: validation passed; commits in progress

## Plan

1. Verify latest commits and current repo state against active docs. ✅
2. Update v0.4 next-step docs if code has moved beyond the recorded state. ✅
3. Inspect current scene/drp2/runtime code and tests for step 1 and step 4 gaps. ✅
4. Implement the highest-value point-slice and runtime-boundary hardening changes. ✅
5. Run focused validation and record results. ✅

## What changed

- Refreshed `agents/now/V0_4_NEXT_STEPS.md` so it reflects already-landed repeated point-update and
  multi-panel point-emission work.
- Tightened scene point-attribute validation and error reporting in `src/scene/scene.c` and
  documented the point-attribute contract in `include/datoviz/scene.h`.
- Added focused scene regressions for unsupported attributes, mismatched attribute counts, missing
  full allocation before range updates, and a two-panel offscreen smoke path.
- Hardened runtime buffer downloads with semantic-state bounds checks in `src/drp2/runtime.c` and
  added a focused DRP2 regression.

## Validation

- `cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DDVZ_BUILD_SCENE=ON -DDVZ_BUILD_DRP2=ON -DDVZ_BUILD_APP=ON`
- `cmake --build build`
- `just test scene` -> `47/47` passed
- `just test drp2` -> `75/75` passed
- `git diff --check` -> passed

## Notes

- The existing `build/` cache was stale (`DVZ_BUILD_APP=ON` with `DVZ_BUILD_SCENE=OFF`), so validation
  required regenerating CMake with scene enabled before building.
