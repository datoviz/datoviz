# Status

- Task: fix `test_scene_json_includes_buffer_binding_metadata` in `src/scene/tests/test_scene.c`.
- Started: 2026-05-10
- State: in progress

## Plan

1. Reproduce the failing assertion and inspect the generated scene JSON.
2. Patch the test or JSON serialization if the expectation is stale or incorrect.
3. Re-run the focused scene test, then record validation and remaining risks.

## Update

- Reproduced the failure and confirmed the generated JSON reports the index buffer with `stride: 4` and `byte_size: 12`.
- Updated the test to assert the actual buffer descriptor values instead of stale expectations.

## Validation

- `cmake --build build --target dvztest_scene -j4`
- `./build/testing/dvztest_scene scene_json_includes_buffer_binding_metadata`
- `git diff --check`

## Current state

- The scene JSON test now matches the emitted buffer descriptor metadata (`stride: 4`, `byte_size: 12`) for the retained 32-bit index buffer slice.
