# Component Targets

Status: build and packaging roadmap.

This note preserves the useful component-target split direction without making it a v0.4 release
blocker.


## Product Model

The monorepo should remain one repository and one release pipeline, while exposing consumable CMake
targets:

1. `datoviz::core`: `common`, `ds`, `fileio`, `math`, `thread`;
2. `datoviz::vk`: `vk`, `vklite`;
3. `datoviz::canvas`: `input`, `window`, `stream`, `video`, `canvas`;
4. `datoviz::drp2`: portable DRP2 protocol, streams, validation, and runtime client API;
5. `datoviz::webgpu`: planned WebGPU runtime;
6. `datoviz::scene`: high-level scene API built on DRP2/runtime contracts;
7. `datoviz::datoviz`: aggregate convenience target.


## Dependency Rules

1. `core` has no graphics/runtime dependencies.
2. `vk` depends on `core`, not on `canvas`.
3. `canvas` depends on `core` and `vk`.
4. `drp2` depends on `core`; public headers expose no Vulkan, canvas, window, or platform types.
5. `webgpu` depends on `drp2` and optionally integrates with canvas through adapters.
6. `scene` depends on DRP2/runtime contracts, not raw backend internals.
7. WASM builds include portable `drp2` and `scene`, excluding `vk` and native `canvas`.


## Remaining Work

Completed baseline: layered targets, exports, build toggles, component runners, and the aggregate
target exist.

Open follow-up:

1. CI profiles for core-only, vk-only, canvas-stack, aggregate, and future wasm;
2. out-of-tree package-consumer CTest/CI smokes for component imports;
3. component-owned installed header sets;
4. public-header probes for accidental transitive dependencies;
5. reverse-dependency lint checks;
6. README/build docs showing component-first consumption.


## Guardrails

1. Keep the aggregate target available.
2. Add package-consumer smokes before tightening install ownership.
3. Do not use packaging cleanup to reshuffle source modules without a behavior reason.
4. Do not let DRP2 or scene public headers leak backend-native types.
