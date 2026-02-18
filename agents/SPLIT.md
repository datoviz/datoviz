# Datoviz v0.4-dev Monorepo Target Split Plan

This document defines a concrete plan to split Datoviz into independently consumable targets while
staying in a single monorepo.

Primary decision:
1. Keep one GitHub repository.
2. Publish multiple CMake targets/packages from that repository.
3. Keep an optional batteries-included aggregate target for users who want everything.


## Goals

1. Let users adopt only the layer they need:
   1. thin Vulkan wrappers (`vk` + `vklite`)
   2. batteries-included canvas stack
   3. future DRP2/WebGPU runtime
   4. future scene layer
2. Prevent accidental coupling across layers.
3. Preserve fast internal refactors through monorepo co-location.
4. Support desktop and wasm deliverables with clear boundaries.


## Non-goals

1. Splitting into multiple repositories.
2. Immediate large-scale source tree reorganization before boundaries are enforced in CMake/API.
3. Reintroducing v0.3 compatibility constraints.


## Target product model (monorepo)

Define the following installable targets and public include entry points.

1. `datoviz::core`
   1. Modules: `common`, `ds`, `fileio`, `math`, `thread`
   2. No graphics runtime dependency
   3. Public umbrella: `include/datoviz/core.h` (new)
2. `datoviz::vk`
   1. Modules: `vk`, `vklite`
   2. Thin Vulkan wrappers for advanced Vulkan users
   3. Public umbrellas: `include/datoviz/vk.h`, `include/datoviz/vklite.h`
3. `datoviz::canvas`
   1. Modules: `input`, `window`, `stream`, `video`, `canvas`
   2. Depends on: `datoviz::core`, `datoviz::vk`
   3. Public umbrellas: `include/datoviz/canvas.h`, `include/datoviz/window.h`, `include/datoviz/stream.h`,
      `include/datoviz/video.h`, `include/datoviz/input.h`
4. `datoviz::drp2` (planned)
   1. Modules: new DRP2 spec/protocol/runtime
   2. Platform-neutral contract; no Vulkan symbols in public headers
   3. wasm-portable by design
5. `datoviz::webgpu` (planned)
   1. WebGPU renderer runtime on desktop
   2. Depends on `datoviz::drp2`
   3. May optionally integrate with `datoviz::canvas` for presentation/capture workflows
6. `datoviz::scene` (planned)
   1. High-level scene API built on DRP2/WebGPU contract
   2. Desktop and wasm-facing public API
7. `datoviz::datoviz` (aggregate convenience target)
   1. Links all active layers
   2. Mirrors current batteries-included behavior


## Dependency rules (must hold)

1. `core` has no dependency on `vk`, `canvas`, `drp2`, `webgpu`, or `scene`.
2. `vk` depends on `core`, not on `canvas`.
3. `canvas` depends on `core` and `vk`; never the reverse.
4. `drp2` depends on `core`; no direct public dependency on Vulkan or platform window APIs.
5. `webgpu` depends on `drp2` and optionally `core`; canvas integration is adapter-level and optional.
6. `scene` depends on `drp2` (+ runtime adapter), not on raw `vk` internals.
7. wasm build includes `drp2` and `scene` (and any wasm-specific runtime bridge), excluding `vk`/`canvas`.


## Public header strategy

1. Keep `include/datoviz/datoviz.h` as an aggregate umbrella only.
2. Add product-specific umbrellas and document them as the default includes:
   1. `datoviz/core.h`
   2. `datoviz/vk.h`, `datoviz/vklite.h`
   3. `datoviz/canvas.h`
   4. `datoviz/drp.h` (v1/current) then `datoviz/drp2.h` when introduced
   5. `datoviz/scene.h`
3. Ensure each target installs only the headers it owns, plus shared/common headers needed by its public
   ABI contract.


## CMake packaging plan

## Phase A - Build graph split (no functional behavior change)

1. Keep existing module object libraries.
2. Introduce intermediate layered libraries:
   1. `add_library(datoviz_core SHARED|STATIC ...)`
   2. `add_library(datoviz_vk_layer SHARED|STATIC ...)`
   3. `add_library(datoviz_canvas_layer SHARED|STATIC ...)`
3. Link these layered libs from current object modules.
4. Keep current `datoviz` target as aggregate linking the layered targets.

Acceptance criteria:
1. `just build` succeeds.
2. Existing `dvztest` still builds/runs with aggregate target.
3. No public API break required in this phase.


## Phase B - Exported package targets

1. Add proper export/install rules:
   1. `install(TARGETS datoviz_core datoviz_vk_layer datoviz_canvas_layer datoviz ...)`
   2. `install(EXPORT DatovizTargets NAMESPACE datoviz:: ...)`
2. Add `DatovizConfig.cmake` and version file with component support.
3. Map exported names:
   1. `datoviz::core`
   2. `datoviz::vk`
   3. `datoviz::canvas`
   4. `datoviz::datoviz`
4. Ensure transitive include dirs and compile definitions are correct per component.

Acceptance criteria:
1. External sample project can `find_package(datoviz CONFIG COMPONENTS vk)` and link only `datoviz::vk`.
2. External sample project can link only `datoviz::canvas` and receive required transitives.
3. Aggregate package still works.


## Phase C - Build toggles and CI matrix

1. Add options:
   1. `DVZ_BUILD_CORE=ON`
   2. `DVZ_BUILD_VK=ON`
   3. `DVZ_BUILD_CANVAS=ON`
   4. `DVZ_BUILD_DRP2=OFF` (until implemented)
   5. `DVZ_BUILD_WEBGPU=OFF` (until implemented)
   6. `DVZ_BUILD_SCENE=OFF` (until implemented)
2. Gate subdirectories and tests by these options.
3. Define CI matrix jobs:
   1. core-only
   2. vk-only (+ core)
   3. canvas stack (+ core + vk)
   4. aggregate
   5. wasm profile (when drp2/scene path exists)

Acceptance criteria:
1. Each profile configures and builds independently.
2. Tests skip or run based on enabled components with explicit diagnostics.


## Phase D - Test architecture split

1. Keep unified `dvztest` for integration.
2. Add component test runners:
   1. `dvztest_core`
   2. `dvztest_vk`
   3. `dvztest_canvas`
   4. `dvztest_integration` (existing aggregate behavior)
3. Register module tests under component runners by ownership.
4. Keep graphics tests in unsandboxed Vulkan-capable path as already required.

Acceptance criteria:
1. Users building only `vk` can run only `vk` tests without canvas/video build requirements.
2. Integration runner preserves current end-to-end coverage.


## Phase E - DRP2/WebGPU/Scene onboarding

1. Create new source roots with strict dependencies:
   1. `src/drp2/`
   2. `src/webgpu/`
   3. `src/scene/` (new implementation, not scaffold placeholders)
2. Add public headers:
   1. `include/datoviz/drp2.h` + `include/datoviz/drp2/*`
   2. `include/datoviz/webgpu.h` + `include/datoviz/webgpu/*`
   3. `include/datoviz/scene.h` + `include/datoviz/scene/*` (as API stabilizes)
3. Enforce that DRP2 public API is backend-agnostic and wasm-safe.
4. Keep canvas integration with webgpu renderer optional and adapter-based.

Acceptance criteria:
1. Desktop can consume `webgpu` with or without canvas adapter path.
2. wasm path builds `drp2` + `scene` without linking `vk` or `canvas`.


## Repository layout guidance (monorepo)

Keep current top-level layout; add target-level grouping and packaging metadata.

1. Keep:
   1. `src/` per-module source
   2. `include/datoviz/` public headers
   3. `testing/` shared test infrastructure
2. Add:
   1. `cmake/packages/` for config/export templates
   2. `testing/components/` for component-specific test runners
   3. optional `examples/<component>/` samples for external consumers


## Migration sequence (recommended order)

1. Land Phase A (layered targets) with zero API changes.
2. Land Phase B (install/export component targets).
3. Land Phase C (build toggles + CI profiles).
4. Land Phase D (component test runners + integration preservation).
5. Land Phase E incrementally as DRP2/WebGPU/Scene modules mature.


## Current status (as of February 15, 2026)

Completed:
1. Phase A foundation:
   1. Layered libraries and aliases are in place (`datoviz_core`, `datoviz_vk_layer`,
      `datoviz_canvas_layer`, plus `datoviz::core`, `datoviz::vk`, `datoviz::canvas`,
      `datoviz::datoviz`).
   2. Aggregate `datoviz` target remains available and builds successfully.
2. Phase B foundation:
   1. Export/install wiring is in place (`DatovizTargets`, package config, version file).
   2. Component checks exist in `DatovizConfig.cmake` for `core`, `vk`, `canvas`, `datoviz`.
3. Phase C foundation:
   1. Build options exist: `DVZ_BUILD_CORE`, `DVZ_BUILD_VK`, `DVZ_BUILD_CANVAS`,
      `DVZ_BUILD_DRP2`, `DVZ_BUILD_WEBGPU`, `DVZ_BUILD_SCENE`.
   2. Subdirectories/tests are gated by component toggles with dependency guard checks.
4. Phase D:
   1. Component runners are implemented: `dvztest_core`, `dvztest_vk`, `dvztest_canvas`,
      `dvztest_integration`.
   2. Legacy `dvztest` integration runner remains available for compatibility.

Partially complete / remaining from A-D:
1. CI matrix is not yet updated to run all component profiles/runners.
2. Automated out-of-tree package-consumer smoke tests are not yet integrated into CTest/CI.
3. Header install ownership is still broad; component-specific header install sets remain to do.


## Next execution steps

1. CI matrix integration (highest priority):
   1. Update `.github/workflows/test.yml` to run profile jobs:
      core-only, vk-only (+core), canvas stack (+core+vk), aggregate.
   2. In each profile, run the matching runner:
      `dvztest_core`, `dvztest_vk`, `dvztest_canvas`, `dvztest_integration`.
2. Package-consumer smoke tests:
   1. Add CTest out-of-tree consumer checks for `core`, `vk`, and `canvas` package components.
   2. Wire these into CI to catch export/include/transitive-link regressions.
3. Package/header ownership tightening:
   1. Move from broad `install(DIRECTORY include/)` toward component-owned header install sets.
   2. Keep aggregate install behavior while component boundaries are enforced.
4. Docs follow-up:
   1. Update `README.md` and `docs/discussions/BUILD.md` with component build and consumption examples.


## Risk controls

1. API drift risk:
   1. Add per-component public-header probe tests.
2. Hidden coupling risk:
   1. Add include/lint checks forbidding reverse dependencies (for example, `vk` including `canvas/*`).
3. Packaging regression risk:
   1. Add out-of-tree consumer smoke tests in CI for each exported component.
4. Feature matrix complexity risk:
   1. Keep aggregate target as compatibility bridge during transition.


## Immediate implementation backlog

1. Introduce layered targets (`core`, `vk`, `canvas`) in `src/CMakeLists.txt`.
2. Add exported target aliases and install/export rules for component consumption.
3. Add CMake options to build subsets and guard module subdirectories.
4. Split test executable wiring into component runners while keeping `dvztest` integration.
5. Update README/BUILD docs with component-first consumption examples.


## Done criteria for this plan

1. A user can consume only `datoviz::vk` without building/installing canvas/video/window.
2. A user can consume only `datoviz::canvas` (and required transitives) without scene/DRP2/webgpu.
3. A user can consume only future `datoviz::drp2` + `datoviz::scene` for wasm workflows.
4. Aggregate `datoviz::datoviz` remains available for batteries-included users.
5. All of the above are delivered from one monorepo and one release pipeline.
