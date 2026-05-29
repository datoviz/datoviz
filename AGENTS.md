# AGENTS.md - Datoviz v0.4-dev Agent Guide

This is the mandatory entry point for Codex and other automation agents working in this
repository. Keep this file short: it should answer what must never be violated, what parts of
the repo are active, which commands to run, and where to read next.

Detailed rules live in [agents/rules/](agents/rules/).


## Non-Negotiable Rules

1. Do not treat v0.3 compatibility as a constraint on this branch. Prefer architecture,
   correctness, and maintainability for v0.4-dev.
2. Do not delete existing comments during refactors. Update comments when they become stale.
3. Do not commit changes inside the `data` submodule unless the user explicitly approves that
   submodule commit or pointer update in the current turn.
4. Treat `M data`, `? data`, or any staged `data` gitlink update as a stop sign until explicitly
   approved.
5. Do not stage or commit generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`,
   `*.so`, `*.dll`, `*.npy`, `*.npz`, or `.DS_Store` unless the user explicitly approves those
   exact files in the current turn.
6. Before committing, run `git status --short` and verify the staged set excludes unapproved
   `data`, vendored runtime libraries, generated files, and large binary assets.
7. Always run `git diff --check` before finalizing code changes.
8. Keep `docs/` as legacy v0.3 public documentation. Put active v0.4 design notes,
   specifications, implementation plans, and architecture records under `spec/` or `agents/`
   according to the routing rules below.

More detail: [agents/rules/REPO_HYGIENE.md](agents/rules/REPO_HYGIENE.md).


## Current Branch Map

Active modules currently built into `libdatoviz` by default:

`common`, `ds`, `fileio`, `geom`, `math`, `thread`, `input`, `window`, `canvas`,
`stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`.

The active runtime path is:

`scene` frame plans -> `drp2` command streams -> `vklite` runtime -> `canvas`/`stream`
frame execution -> optional `app` presentation.

Treat the low-level graphics stack (`vk`, `vklite`, `canvas`, `stream`, `video`, `window`) as the
runtime foundation. Do not create parallel presentation, frame-stream, renderer, or Vulkan wrapper
paths unless the user explicitly asks for a new architecture.

Scaffolding modules such as `color`, `wasm`, richer text/gui layers, and broader renderer/client
layers should stay untouched unless the task explicitly activates them.


## Start Here

1. Read [agents/now/START.md](agents/now/START.md) for the current execution summary and
   branch-specific guardrails.
2. Read [agents/now/RELEASE.md](agents/now/RELEASE.md) for feature-freeze, release-candidate,
   validation, packaging, and final-release work.
3. Use [agents/README.md](agents/README.md) to find completed phase records, imminent work, and
   subsystem-specific handoffs.
4. Use [spec/scene/README.md](spec/scene/README.md) and the closest `spec/` subtree for durable
   v0.4 semantics and architecture.


## Workflow

1. Inspect local state first with `git status --short`.
2. Read the nearest existing code and tests before changing implementation.
3. Make focused changes inside the owning module boundary.
4. Add or update focused tests for behavioral changes, lifetime fixes, bounds checks, or
   cross-module contracts.
5. Run the narrowest relevant validation loop, then broader validation when the blast radius
   warrants it.
6. Run `git diff --check`.
7. Before any commit, inspect `git status --short` and the staged diff.


## Build And Test

Run from the repository root:

```sh
just clean
just build
just test [filter]
```

Prefer narrow validation while iterating, such as `just test drp2`, `just test scene`, or the
relevant `dvztest_*` target when available. Keep `dvztest` as the unified broad runner.

For Vulkan/GLFW/Metal paths on macOS, use the repo environment:

```sh
direnv exec . just test [filter]
```

More detail: [agents/rules/BUILD_TEST.md](agents/rules/BUILD_TEST.md).


## Code Boundaries

Public headers live in `include/datoviz/`. Internal implementation lives in `src/`. Shared internal
helpers live in `src/common` and are reachable through module include paths.

Use public includes for public API:

```c
#include "datoviz/math.h"
```

Use shared internal includes for implementation:

```c
#include "_alloc.h"
```

Cross-module low-level utilities belong in `src/common`; do not duplicate them across modules.
Only install headers under `include/datoviz/`.

More detail: [agents/rules/BUILD_TEST.md](agents/rules/BUILD_TEST.md).


## C Coding Rules

Follow the project C style in touched files:

1. Document every new module-level function with a short Doxygen-style docstring.
2. Separate neighboring top-level definitions with three blank lines.
3. Keep public symbols under the `dvz_` prefix; internal helpers should be `static`, `_dvz_`, or
   module-local names.
4. Prefer `dvz_calloc`, `dvz_free`, `dvz_memcpy`, `dvz_memset`, `dvz_fprintf`, and
   `dvz_vfprintf` over raw libc allocation, copy/fill, and stdio calls.
5. Avoid file-scope mutable state. Keep runtime and test-control state in owning objects.
6. Treat warnings, undefined behavior risks, ownership ambiguity, partial-initialization leaks, and
   bounds errors as defects.

More detail: [agents/rules/C_CODING.md](agents/rules/C_CODING.md).


## Graphics Safety

For `vk`, `vklite`, `canvas`, `stream`, `drp2`, `scene`, and `app` changes:

1. Distinguish owned and borrowed Vulkan handles explicitly.
2. Do not destroy, begin, end, reset, or submit borrowed handles unless the API contract grants that
   ownership.
3. Track command-buffer recording state at the owner level when possible.
4. Do not transition destroyed images or borrowed images with unknown layout/ownership.
5. Recycle or destroy transient per-frame runtime objects; live paths must not accumulate them.
6. Run Vulkan validation smoke tests for changes touching resources, command buffers, frame
   lifetimes, render targets, swapchains, or synchronization.

More detail: [agents/rules/GRAPHICS_SAFETY.md](agents/rules/GRAPHICS_SAFETY.md).


## Scene And DRP2

The active scene slice is real v0.4 implementation, not future scaffolding. Scene should emit
frame plans and DRP2 streams; native runtime execution should flow through `vklite` and borrowed
canvas frames without scene owning swapchains, command-buffer lifecycle, or sinks.

For scene visual/shader work:

1. Read [spec/scene/implementation/VISUAL_SHADER_REFACTOR.md](spec/scene/implementation/VISUAL_SHADER_REFACTOR.md).
2. Run `just shader-abi-check` when changing scene GLSL/WGSL, shader registry entries, pipeline
   bind/layout rules, or visual shader ABI docs.
3. Keep family-specific behavior behind visual descriptors or lowering helpers. Do not add concrete
   visual-family checks to generic visual, render-emission, or pipeline plumbing.

More detail: [agents/rules/SCENE_DRP2.md](agents/rules/SCENE_DRP2.md).


## Documentation Routing

Use `spec/` for durable v0.4 design, semantics, and architecture. Use `agents/` for execution
status, handoff notes, automation plans, and completed implementation records.

Completed agent plans should not remain in active queues. When work tracked under `agents/now/` or
`agents/soon/` is done, remove the active plan, move or rewrite the final record under
`agents/done/`, and update README/index links.

More detail: [agents/rules/REPO_HYGIENE.md](agents/rules/REPO_HYGIENE.md).


## Pre-Commit Checklist

Before committing:

1. `git diff --check`
2. Relevant build/test command for the touched slice
3. `git status --short`
4. `git diff --cached --stat`
5. Confirm the staged set excludes unapproved `data`, `libs/vulkan/`, runtime libraries, generated
   binaries, and unrelated user changes.
