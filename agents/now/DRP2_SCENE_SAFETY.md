> **Execution Status**
> - **Status:** `ACTIVE HARDENING PLAN`
> - **Created on:** `2026-04-30`
> - **Purpose:** make the active DRP2/scene implementation more robust while it is being wired into
>   the existing canvas, stream/sink, and vklite stack.
> - **Current priority:** fix concrete C safety risks first, then tighten borrowed-frame and
>   vklite lifecycle contracts with focused tests and static/dynamic analysis.

# DRP2/Scene Safety Hardening Plan

This document tracks the safety and quality pass for the current DRP2/scene implementation.

The implementation plan remains in `agents/now/SCENE_DRP2_IMPLEMENTATION.md`. The executable DRP2
contract remains in `spec/drp2/`.


## Goals

1. Remove likely undefined behavior in the active DRP2/scene C code.
2. Make allocation, growth, and partial-initialization paths fail cleanly.
3. Prevent stale pointers into growable object arrays after append/reallocation.
4. Clarify runtime-owned vs borrowed Vulkan/frame resources.
5. Add regression coverage for failure, lifetime, and boundary cases.
6. Keep static and dynamic analysis practical for the active focused runners.


## Non-Goals

1. Do not redesign the DRP2 command surface while hardening implementation details.
2. Do not reopen broad `vk`/`vklite` public boundary work without a concrete safety issue.
3. Do not activate unrelated scaffold modules.
4. Do not import v0.3 APIs or compatibility constraints.


## Current Tooling Notes

The old v0.3 tree is present under `v0.3/`, but the active v0.4-dev root already carries the useful
analysis wiring:

1. `CMAKE_EXPORT_COMPILE_COMMANDS` is enabled by default.
2. `DVZ_ENABLE_ASAN_IN_DEBUG`, `DVZ_SANITIZER`, and sanitizer flags are wired through CMake.
3. `just asan`, `just msan`, and `just tsan` create sanitizer builds.
4. `just atest [filter]` runs the ASan build through `testing/dvztest`.
5. `just analyze` runs `run-clang-tidy` over build compile commands.
6. `just cppcheck` exists, but still targets older paths such as `cli/` and `tests/`.
7. `just valgrind` exists as a generic wrapper.

The first tooling task is to adapt the existing recipes to the focused v0.4 runners where useful,
especially `dvztest_drp2` and `dvztest_scene`, rather than importing another v0.3 build flow.


## Phase 0 - Baseline

Run from the repository root:

1. `just build`
2. `just test drp2`
3. `just test scene`
4. `just spec-check`
5. `git diff --check`

Record failures before editing code. Treat baseline failures as constraints for later validation.


## Phase 1 - Mechanical C Safety

Start with low-risk correctness fixes in `src/drp2` and `src/scene`:

1. Guard growable-array capacity doubling before multiplication.
2. Avoid assigning `dvz_realloc()` directly to the only owned pointer.
3. Check `count + size + 1` arithmetic in JSON builders before comparing against capacity.
4. Check base64 and byte-size arithmetic before allocation or serialization.
5. Replace recoverable allocation assertions in public constructors/builders with clean
   `NULL`/`false` returns.

Initial hotspots:

1. `src/drp2/stream.c`
2. `src/scene/frame_plan.c`
3. `src/scene/converter.c`
4. shared patterns inside `src/drp2/runtime.c`


## Phase 2 - Public Failure Semantics

Define and enforce a simple rule:

1. public create/append/emit/execute functions return failure for recoverable runtime conditions;
2. internal helpers use `ANN`/`ASSERT` only for invariants that indicate a caller bug;
3. partial initialization must clean up everything already created.

Add tests for invalid inputs and allocation-adjacent behavior where the API can expose it without
global fault injection.


## Phase 3 - Object-Table Pointer Lifetime Audit

Audit every pointer into `Drp2RuntimeState.objects` and `Drp2VkliteState.objects`.

Rules:

1. do not keep `Drp2Object*` or `Drp2VkliteObject*` across a call that may append, destroy, compact,
   grow, or reallocate the table;
2. reacquire by stable id after `_add_object()`, `_vklite_add()`, or any helper that may grow state;
3. do not assume destroyed slots are still semantically safe unless they are immediately reset.


## Phase 4 - Borrowed Frame/Vulkan Lifecycle

Write and enforce an ownership matrix for:

1. runtime-owned buffers, images, views, shaders, pipelines, descriptors, samplers, and commands;
2. borrowed `DvzStreamFrame` image, image view, and command buffer;
3. temporary command wrappers around borrowed command buffers;
4. runtime-created textures copied into borrowed frame targets.

Test cases should cover:

1. invalid borrowed frame rejection;
2. repeated frame-target attach with the same texture id;
3. attaching a frame target over a previously runtime-owned texture;
4. executing a stream after attach failure;
5. runtime destroy after partial attach or partial execute;
6. copy-to-frame before and after source texture destruction.


## Phase 5 - Static And Dynamic Analysis

Preferred analysis sequence:

1. `just analyze` once `just build` has produced `build/compile_commands.json`.
2. focused `clang-tidy` on touched files when full analysis is too noisy.
3. `cppcheck --enable=warning,style,performance,portability src/drp2 src/scene include/datoviz/drp2 include/datoviz/scene`.
4. `just asan` followed by `just atest drp2` and `just atest scene` for CPU-heavy checks.
5. Vulkan validation-layer smoke tests for changes touching borrowed frames, command buffers,
   render targets, synchronization, or canvas integration.

If a tool is unavailable or too noisy, record the reason and fall back to focused tests.


## Phase 6 - Validation Gates

For small CPU-only safety changes:

1. `git diff --check`
2. `just build`
3. `just test drp2`
4. `just test scene`
5. `just spec-check`

For changes touching vklite/canvas/stream boundaries, also run relevant graphics smoke tests and, when
practical, Vulkan validation-layer checks.


## Active Findings

Initial code review found these concrete first-pass risks:

1. `src/drp2/stream.c` grows command and JSON buffers without overflow-safe arithmetic.
2. `src/scene/frame_plan.c` has the same grow/realloc and JSON arithmetic pattern.
3. `src/scene/converter.c` computes base64 output sizes with unchecked multiplication/addition.
4. `src/drp2/runtime.c` already has safer capacity growth in several places, but still needs an
   object-pointer lifetime audit around `_vklite_add()` and borrowed frame target replacement.
5. v0.4-dev analysis recipes exist, but `just cppcheck` should be treated as legacy-broad until it is
   narrowed or supplemented for active `src/drp2` and `src/scene` work.
