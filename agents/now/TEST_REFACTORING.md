# Test Refactoring Handoff

Status: ready for next checkpoint. Updated: 2026-05-31.

This is an active execution note for the next agent, not durable architecture.


## Current State

DRP2 test refactoring has landed in small commits. `src/drp2/tests/test_drp2.c` is now a
registration-only entry point around 200 lines. Split files include:

- `stream.c`
- `recording.c`
- `render_pass.c`
- `runtime_lifecycle.c`
- `runtime_validation.c`
- `vklite_runtime.c`
- `test_drp2_helpers.c/.h`

Recent validation:

```sh
cmake --build build --target dvztest_drp2 dvztest
./build/testing/dvztest_drp2 --group runtime-validation
./build/testing/dvztest_drp2 --group vklite-runtime
just test-drp2-contract
git diff --check
```

The direct `vklite-runtime` group may skip GPU cases on machines where Vulkan instance creation
fails; the CPU DRP2 contract lane passed `83/83`.


## Next Preferred Work

Do not continue by over-splitting DRP2 immediately. The next useful hotspot is:

```text
src/scene/tests/scene_visuals.c
```

It is the largest remaining test file and should be split into coherent visual-family modules.

Recommended first checkpoint:

1. Inspect `scene_visuals.c` groups, helper functions, and registration style.
2. Identify the smallest self-contained visual-family block.
3. Extract any shared helpers only if they are needed by the first split.
4. Move one coherent block into a new `src/scene/tests/*.c` file.
5. Keep test names and registration behavior unchanged.
6. Reconfigure/build if needed because tests are globbed.
7. Run the narrow scene test filter for the moved group, then `git diff --check`.
8. Stage only the scene test split and commit.

My preference: start with the least dependency-heavy visual family rather than the largest one.
Avoid mixing scene semantic changes with this refactor.


## Hygiene

Before committing:

```sh
git status --short
git diff --cached --stat
```

Do not stage `data`, `libs/vulkan/`, generated binaries, or unrelated user changes.
