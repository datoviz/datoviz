> **Implementation Status**
> - **Status:** `STRATEGIC BACKLOG`
> - **Verified on:** `2026-05-08`
> - **Role:** long-horizon direction for DRP2, WebGPU runtime work, and eventual scene bring-up
> - **Current branch status:** not the immediate execution plan; active priority is the native
>   scene -> DRP2 -> vklite/canvas slice
> - **Execution note:** the actionable spec-phase entry point is `agents/now/DRP2_SPEC.md`
> - **Implementation note:** the active repo-local implementation guide is
>   `agents/now/V0_4_NEXT_STEPS.md`

# DRP2 / WebGPU / Scene Roadmap

This file is intentionally strategic.
It should describe direction and phase ordering, not act as the active coding checklist.


## First Objective

Reach a point where Datoviz has:

1. a stable DRP2 contract,
2. a native runtime that executes that contract,
3. a browser runtime over WebGPU that executes the same contract,
4. enough conformance parity to let a future scene layer target DRP2 rather than backend internals.


## Ground Rules

1. DRP/WebGPU contract is the source of truth.
2. Public DRP headers must not expose `Vk*` types.
3. Runtime semantics and validation are backend-agnostic and shared across native/browser implementations.
4. Vulkan remains the first native backend implementation target.
5. Keep performance measurable at every milestone.
6. Browser support is a first-class requirement, not a later port.
7. The future Scene API in C/WASM must target DRP and browser JS runtime, not low-level C runtime internals.
8. Do not start scene rewrite until renderer milestones are green.
9. Native escape hatches (Vulkan/CUDA interop) live outside DRP and must be explicitly
   opt-in, clearly marked as advanced, and capability-gated.
10. Power-user requirements are first-class:

    1.  Explicit memory management and allocation policy control for large datasets.
    2.  Deterministic headless/offscreen rendering with readback for reproducibility.
    3.  Compute is mandatory for v1 (not optional).
    4.  FP64 format support is required where hardware exposes it; capabilities must
      report supported precision and formats.
    5. Public profiling API for timing and performance counters.
    6. External memory/sync interop is generic and extensible beyond CUDA.
    7. Borrowed vs owned resource lifetimes are explicit and validated.
    8. Thread-safety guarantees are documented and enforced.
    9. Deterministic compute/reduction behavior is available when requested.
    10. WGSL is the portable DRP shader language; runtimes may additionally accept
        GLSL or SPIR-V behind capability flags without changing the portable contract.
    11. Memory budget reporting, OOM handling, and leak detection are mandatory.
    12. Data layout, alignment, and stride guarantees are documented and validated.


## Current Baseline Snapshot

1. The active low-level graphics stack (`vk`, `vklite`, `window`, `canvas`, `stream`, `video`) has
   undergone a substantial February 2026 refactor and is currently the stable runtime foundation.
2. `drp2` and `scene` are now active default-build modules with a working first vertical slice:
   scene/frame-plan emission -> DRP2 command stream -> vklite runtime -> canvas/stream execution.
3. Built-in scene visuals currently implemented are `point`, `primitive`, and `image`.
4. Per-panel runtime viewport/scissor and controller-driven panel transforms are already live on the
   native path.
5. The next native pressure target is a minimal `mesh` + depth slice before broader browser parity
   or long-tail visual expansion.


## Target Architecture

1. `DRP v2` (WebGPU-shaped command/data contract).
2. `semantic core` (backend-agnostic object model, validation rules, capability model).
3. `native runtime (C)` (DRP executor for native targets, using semantic core).
4. `browser runtime (JS)` (DRP executor over browser WebGPU API, using same DRP semantics).
5. `backend_vulkan` (platform implementation details only, behind native runtime interface).
6. `wasm bridge` (C/WASM scene-side emission of DRP commands to JS runtime transport).
7. `native interop` (advanced APIs for Vulkan/CUDA interop).
8. `memory manager` (explicit allocation policies, large-resource handling).
9. `profiling` (public API for timing and counters).
10. `shader pipeline` (WGSL for portable DRP; optional GLSL/SPIR-V ingestion).
11. `tests` split by layer:
   1. DRP contract tests.
   2. Semantic/validation conformance tests shared across runtimes.
   3. Native backend translation and end-to-end rendering tests.
   4. Browser runtime conformance tests.
   5. Performance benchmarks.


## Phase Order

This document remains backlog-oriented, but the recommended ordering has changed:

1. native 3D baseline first,
2. early WebGPU feasibility second,
3. broader browser/runtime parity later,
4. long-tail scene growth after the contract survives both native 3D and browser pressure.

## P0 - DRP2 Contract Freeze

### Goal
Freeze a narrow DRP2 contract and the minimum conformance material needed to implement it safely.

### Deliverables
1. `spec/drp2/` contract set
2. reduced schema set aligned with the contract
3. first fixture corpus definition
4. `spec/scene/` consumer requirements

### Exit Criteria
1. DRP2 minimal scope is frozen.
2. No public DRP2 concept requires backend type leakage.
3. The first implementation tasks are obvious and bounded.


## P1 - Runtime Semantic Core

### Goal
Implement backend-agnostic DRP2 validation and runtime semantics before backend-specific execution.

### Responsibilities
1. Object registry with typed handles and generation counters.
2. State tracking for resource and pass compatibility.
3. Validation rules for command order and object usage.
4. Command recording abstraction for backend submission.
5. Explicit owned vs borrowed resource lifetimes with validation.
6. Thread-safety rules for core runtime APIs (documented and tested).
7. Deterministic compute/reduction mode with validation hooks.
8. Memory budget reporting and OOM/eviction handling with explicit errors.

### Deliverables
1. semantic validation layer
2. handle/object registry
3. structured error reporting

### Tests
1. Handle lifetime tests (use-after-destroy, stale generation).
2. Validation tests (invalid bind, incompatible pipeline, illegal pass ops).
3. Deterministic replay tests from DRP fixture streams.

### Exit Criteria
1. Runtime tests pass with a mock backend.
2. Public runtime-facing headers remain backend-agnostic.


## P2 - Native Vulkan Runtime

### Goal
Map runtime operations to Vulkan efficiently while staying behind backend interface.

### Scope
1. Adapter/device/queue mapping.
2. Buffer/texture allocation mapping via existing allocator work.
3. Pipeline/bind layout/shader mapping.
4. Command buffer recording and submission.
5. Minimal swapchain/offscreen path for presentable output.
6. Deterministic headless rendering path with readback.
7. Native GLSL/SPIR-V ingestion paths behind capability flags (WGSL remains the portable source).

### Deliverables
1. Vulkan backend implementation module.
2. Translation layer tests for runtime-op -> Vulkan-op behavior.
3. Initial integration with existing `canvas/window/stream` path where relevant.

### Tests
1. Headless triangle render + image checksum.
2. Upload buffer -> draw -> readback assertions.
3. Validation-on tests run clean for supported platforms.
4. Deterministic headless readback hash is stable across runs.
5. Deterministic compute/reduction fixtures (native).

### Exit criteria
1. DRP fixture can render a known image through runtime + Vulkan backend.
2. No direct DRP/Vulkan cross-contamination in public contract headers.


## P3 - Native Interop

### Goal
Provide Vulkan/CUDA interop without contaminating DRP or browser paths.

### Scope
1. Capability query API (`DVZ_CAP_NATIVE_VK`, `DVZ_CAP_CUDA_INTEROP`, etc.).
2. Export/import buffers and images.
3. Export/import semaphores/fences for explicit cross-API sync.
4. Explicit ownership rules for Datoviz-owned export and user-owned import.
5. Opt-in access (for example `DVZ_ENABLE_NATIVE`) and clear API labeling.
6. Generic external memory/sync interop path (extensible beyond CUDA).
7. Versioned structs and ABI stability notes for native interop.
8. Explicit interop support targets for PyTorch and CuPy through standard exchange protocols
   (DLPack and CUDA Array Interface).
9. Stream/synchronization semantics required for framework interop are defined at API level and
   capability-gated.

### Deliverables
1. Public headers under `include/datoviz/native.h` and `include/datoviz/native/*`.
2. Dedicated `src/native/` module with Vulkan-backed implementation.
3. Platform support reporting (Linux/Windows supported; macOS may report unsupported).
4. Documentation for ownership and synchronization rules.
5. High-level protocol support statement for DLPack and CUDA Array Interface integration points
   (detailed wire-format spec remains separate).

### Tests
1. Capability gating tests with clear unsupported paths.
2. Buffer export/import roundtrip with explicit semaphore sync.
3. Image export/import roundtrip with explicit semaphore sync.
4. End-to-end framework roundtrips:
   1. Datoviz -> CuPy -> Datoviz
   2. Datoviz -> PyTorch -> Datoviz
5. Negative tests for ownership/sync misuse across framework boundaries.

### Exit criteria
1. Interop APIs are stable and capability-gated.
2. Interop tests pass on supported platforms.
3. No DRP headers include Vulkan types.
4. Framework interop contract is validated for both DLPack and CUDA Array Interface paths.


## P4 - Browser WebGPU Runtime

### Goal
Run the same DRP v2 fixtures in browser via a JS runtime implemented on top of WebGPU.

Updated sequencing note:

The first browser pass should now happen earlier than this old backlog wording implied. The active
branch plan is to attempt a narrow feasibility pass soon after the first native `mesh`/depth slice,
before transparency, axes/text, and many additional visual families harden native-only assumptions.

### Scope
1. JS DRP decoder/dispatcher with identical command semantics.
2. WebGPU object mapping:
   1. GPUBuffer/GPUTexture/GPUSampler
   2. bind groups/layouts
   3. render pipeline and pass encoding
3. Capability query and error mapping aligned with DRP validation codes.
4. Minimal transport path for WASM -> JS DRP command submission.

### Deliverables
1. Browser runtime module under a dedicated path (for example `web/` or `js/`).
2. Conformance runner in headless browser (or CI browser environment).
3. DRP fixture replay parity report: native vs browser.
4. Explicit portability-gap report for any native assumptions exposed by:
   depth state, viewport/scissor handling, shader ingestion, or readback semantics.

### Tests
1. DRP conformance fixtures replay in browser.
2. Deterministic image/hash checks where feasible.
3. Error-code parity tests between native and browser runtimes.

### Exit criteria
1. Minimal draw fixtures pass in browser runtime.
2. Capability and validation behavior matches DRP spec expectations.
3. WASM transport can feed DRP commands to JS runtime.

Practical first browser subset:

1. `point`
2. `primitive`
3. `image`
4. one minimal `mesh` scene with depth testing


## P5 - Contract Parity And Renderer v1 Slice

### Goal
Reach a practical renderer baseline suitable for stabilization.

### Scope
1. Dynamic viewport/scissor.
2. Multiple bind groups/descriptor sets.
3. Texture sampling and basic sampler states.
4. Compute pass (mandatory).
5. Explicit synchronization model represented in runtime semantics.
6. Thread-safe submission model documented for power users.

### Deliverables
1. Expanded DRP command coverage documentation.
2. Backend support matrix by feature.
3. Additional end-to-end fixtures (multiple pipelines/resources).

### Tests
1. Conformance fixtures per feature.
2. Regression suite for previously fixed lifecycle/sync bugs.
3. Cross-platform smoke runs (Linux first, then macOS/Windows where applicable).

### Exit criteria
1. Green end-to-end suite for agreed v1 feature set.
2. Known unsupported features are explicitly flagged in capability model.


## P6 - Performance And Stabilization

### Goal
Ensure architecture remains high-performance and operationally stable.

### Performance work
1. Add microbenchmarks:
   1. command decode
   2. command record
   3. submission overhead
   4. buffer update throughput
2. Add macrobenchmarks:
   1. frame CPU time
   2. frame GPU time (where available)
   3. frame variance
3. Add benchmark baselines and regression thresholds.
4. Public profiling API exposes timing and counters for power users.

### Reliability work
1. Stress tests for create/destroy churn.
2. Long-run replay tests.
3. Error injection tests (invalid DRP streams, device capability mismatch).
4. OOM/eviction and resource leak detection tests.

### Exit criteria
1. Benchmarks tracked in CI artifacts.
2. Agreed performance thresholds documented.
3. Renderer declared "stabilized v1".
4. Profiling API is documented and stable.


## P7 - Scene Bring-Up

### Goal
Start new standalone scene API as a pure DRP consumer.

### Rules
1. Scene layer depends on DRP/runtime client APIs only.
2. Scene layer does not include backend-internal headers.
3. Scene uses capability checks from runtime contract, not Vulkan constants.
4. Browser path is mandatory: C Scene API compiled to WASM sends DRP to JS runtime.

### First slice
1. Minimal panel/view abstraction.
2. One primitive pipeline (for example points or mesh-lite).
3. One native end-to-end scene example using DRP path.
4. One browser end-to-end scene example (WASM scene -> JS runtime -> WebGPU).


## P8 - Post-v1 Memory Features

### Goal
Add advanced memory and streaming capabilities once v1 is stabilized.

### Scope
1. Zero-copy streaming and persistent mapped buffers for large datasets.
2. Sparse/virtualized buffers and textures for out-of-core rendering.
3. Advanced memory placement and aliasing policies for large allocations.
4. Multi-GPU device selection, affinity, and explicit device group workflows.

### Deliverables
1. Memory manager extensions and capability reporting for sparse/virtual resources.
2. Streaming tests for long-running data ingestion without stalls.

### Exit criteria
1. Streaming and sparse resource tests pass on supported platforms.


## Execution Rules

Use this workflow for every task:

1. Pick one task ID from the checklist.
2. Ask Codex for:
   1. exact file changes
   2. tests to add/update
   3. acceptance commands
3. Require Codex to run relevant tests and report failures exactly.
4. Merge only if acceptance criteria are met.


## Agent Rules

1. Always select exactly one task ID from the backlog before coding.
2. Confirm scope: list files to change, tests to add/update, and acceptance commands.
3. Keep patches small and reviewable (prefer 5-15 files).
4. Run the relevant tests and report failures verbatim; if tests cannot be run,
   state why explicitly.
5. Do not proceed to the next milestone until the current gate is green.
6. If a task’s acceptance criteria are not met, do not mark it done.


## Task Sizing Rules

1. One PR/task should be 1 logical change, ideally 5 to 15 files.
2. Never combine spec changes and broad refactors without tests.
3. Prefer adding adapters over sweeping renames.
4. Keep behavioral changes accompanied by fixture updates.


## Suggested Backlog

1. `T001`: Create DRP v2 spec skeleton and command taxonomy.
2. `T002`: Add DRP versioning fields and compatibility checks.
3. `T003`: Implement DRP parser/encoder with roundtrip tests.
4. `T004`: Add DRP validation error code framework.
5. `T005`: Define capability schema for precision/format requirements (including FP64).
6. `T006`: Define data layout/alignment/stride rules and validation.
7. `T007`: Define WGSL ingestion rules and validation for DRP shader modules.
8. `T008`: Introduce semantic-core object registry with generation-safe handles.
9. `T009`: Add mock backend interface and semantic-core unit tests.
10. `T010`: Implement native runtime command dispatcher for minimal draw path.
11. `T011`: Build Vulkan backend adapter for buffer lifecycle.
12. `T012`: Build Vulkan backend adapter for texture/sampler lifecycle.
13. `T013`: Build Vulkan backend adapter for render pipeline and draw.
14. `T014`: Add first headless native end-to-end DRP fixture rendering test.
15. `T015`: Add native interop public headers, capability flags, and opt-in gating.
16. `T016`: Implement native interop buffer/image export/import with sync primitives.
17. `T017`: Add native interop tests with platform capability coverage.
18. `T018`: Add native interop protocol bridge for DLPack + CUDA Array Interface (capability-gated).
19. `T019`: Add PyTorch/CuPy roundtrip tests and ownership/sync misuse negatives.
20. `T020`: Add native GLSL/SPIR-V ingestion behind capability flags.
21. `T021`: Add compute pass path (mandatory) with basic fixtures.
22. `T022`: Add deterministic compute/reduction fixtures and validation mode.
23. `T023`: Document and test thread-safety guarantees for runtime and submission.
24. `T024`: Add memory budget reporting and OOM/eviction handling tests.
25. `T025`: Add browser JS runtime DRP decoder and minimal draw fixture.
26. `T026`: Add WASM -> JS DRP transport API (minimal command stream).
27. `T027`: Add capability model and unsupported-feature reporting with native/browser parity tests.
28. `T028`: Add present path integration where needed with canvas/window (native).
29. `T029`: Add browser conformance replay suite for shared DRP fixtures.
30. `T030`: Add profiling API and timing/counter exposure.
31. `T031`: Add microbenchmark harness and baseline capture (native) + browser timing smoke metrics.
32. `T032`: Stabilization pass (bug fixes + docs + compatibility notes).


## Post-v1 Backlog

1. `P001`: Add zero-copy streaming and persistent mapped buffer support.
2. `P002`: Add sparse/virtualized buffer and texture resource support.
3. `P003`: Add advanced memory placement/aliasing policies with validation.
4. `P004`: Add multi-GPU device selection and explicit affinity workflows.


## Acceptance Gates

1. Gate A (after M1): DRP v2 contract and parser stable.
2. Gate B (after M2): Runtime semantic validation stable with mock backend.
3. Gate C (after M3): First Vulkan-backed rendering fixture stable.
4. Gate D (after M4): Native interop escape hatch stable and capability-gated.
5. Gate E (after M5): First browser WebGPU runtime fixture stable.
6. Gate F (after M6): Feature-complete v1 renderer slice with native/browser contract parity.
7. Gate G (after M7): Performance and reliability stabilized.
8. Gate H (after M8 start): Scene API work can begin without low-level leakage.
9. Gate I (after M9): Advanced memory features are available for power users.


## Quality Checklist

1. Public API changes documented.
2. Tests added or updated.
3. No new dependency from high-level modules to backend internals.
4. Build passes on primary platform.
5. Logs/errors are actionable.
6. No unchecked TODOs without linked task ID.


## Risks And Mitigation

1. Risk: DRP contract churn causes constant rewrites.
   1. Mitigation: freeze minimal v1 contract early and version strictly.
2. Risk: runtime duplicates Vulkan logic inefficiently.
   1. Mitigation: keep runtime semantic-focused; push platform specifics to backend.
3. Risk: Codex introduces large unreviewable patches.
   1. Mitigation: enforce small task IDs and acceptance gates.
4. Risk: performance regressions hidden during refactor.
   1. Mitigation: add benchmarks before final feature expansion.
5. Risk: native and browser semantics diverge.
   1. Mitigation: shared DRP fixtures and parity tests are mandatory before stabilization.
6. Risk: WASM bridge becomes ad-hoc and unstable.
   1. Mitigation: define minimal versioned transport API early and test with fixture replay.
7. Risk: Native escape hatches leak backend internals into DRP.
   1. Mitigation: keep interop in a dedicated native module, capability-gated, and
      explicitly excluded from DRP headers and tests.


## Definition Of Done

1. DRP v2/WebGPU-shaped contract is versioned and documented.
2. Native runtime executes DRP v2 with strict validation and deterministic behavior.
3. Browser JS runtime executes DRP v2 over WebGPU with conformance parity on agreed fixture set.
4. Vulkan backend passes renderer conformance fixtures for native targets.
5. Native interop escape hatch (Vulkan/CUDA) is implemented and capability-gated.
6. WASM bridge path (C/WASM -> JS runtime -> WebGPU) is operational on minimal scene example.
7. Compute is available and validated in the v1 renderer slice.
8. FP64 capability reporting is present and respected where supported.
9. Public profiling API is available for power users.
10. Thread-safety guarantees are documented and tested.
11. Deterministic headless/offscreen rendering with readback is stable.
12. Deterministic compute/reduction mode is available for power users.
13. Memory budget reporting and OOM handling are validated.
14. Data layout, alignment, and stride guarantees are documented and enforced.
15. Performance baseline is established and tracked (native, plus browser smoke metrics).
16. Renderer API/behavior is stable enough to start standalone Scene API development.
