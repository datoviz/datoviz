# Datoviz v0.4-dev Next Steps

This document is a detailed, high-level execution plan to reach the first major objective:

1. Build and stabilize a high-performance renderer based on a new DRP version shaped by WebGPU semantics.
2. Ensure this architecture also works in the browser by running a WebGPU runtime in JavaScript.
3. Only after stabilization, build a standalone Scene API on top of that DRP/WebGPU contract with no dependency on low-level backend internals.

The plan is organized so each step can be executed by Codex in small, testable increments.


## Ground rules

1. DRP/WebGPU contract is the source of truth.
2. Public DRP headers must not expose `Vk*` types.
3. Runtime semantics and validation are backend-agnostic and shared across native/browser implementations.
4. Vulkan remains the first native backend implementation target.
5. Keep performance measurable at every milestone.
6. Browser support is a first-class requirement, not a later port.
7. The future Scene API in C/WASM must target DRP and browser JS runtime, not low-level C runtime internals.
8. Do not start scene rewrite until renderer milestones are green.


## Current baseline snapshot (branch reality)

1. Core modules are actively refactored: `vk`, `vklite`, `window`, `canvas`, `stream`, `video`.
2. High-level scene/visual layers are currently thin/placeholders in this branch.
3. `datoviz` is currently built as one shared library from modular object components.
4. Existing tests are primarily low-level/core and run through `testing/dvztest.c`.


## Target architecture (for this phase)

1. `DRP v2` (WebGPU-shaped command/data contract).
2. `semantic core` (backend-agnostic object model, validation rules, capability model).
3. `native runtime (C)` (DRP executor for native targets, using semantic core).
4. `browser runtime (JS)` (DRP executor over browser WebGPU API, using same DRP semantics).
5. `backend_vulkan` (platform implementation details only, behind native runtime interface).
6. `wasm bridge` (C/WASM scene-side emission of DRP commands to JS runtime transport).
7. `tests` split by layer:
   1. DRP contract tests.
   2. Semantic/validation conformance tests shared across runtimes.
   3. Native backend translation and end-to-end rendering tests.
   4. Browser runtime conformance tests.
   5. Performance benchmarks.


## Milestone plan

## M0 - Planning freeze and scaffolding

### Goal
Freeze a clear implementation path and create scaffolding that lets Codex work incrementally.

### Deliverables
1. `spec/DRP_V2.md` skeleton (object model, command taxonomy, lifecycle states).
2. `agents/WEBGPU_RENDERER_TASKS.md` checklist with task IDs.
3. New source folders (or clear naming convention) for:
   1. semantic core
   2. native runtime
   3. browser runtime
   4. wasm bridge
4. Test harness placeholders for DRP parser/validator and cross-runtime fixtures.

### Exit criteria
1. Team agrees DRP v2 scope for first renderer slice.
2. Build still passes.
3. Placeholder tests are wired into CI/test runner.


## M1 - DRP v2 contract (minimal but strict)

### Goal
Define a small, stable DRP subset required to draw a basic frame.

### Scope (minimum)
1. Device/queue selection.
2. Buffer create/upload/destroy.
3. Shader module and basic render pipeline creation.
4. Texture/view/sampler minimal subset.
5. Command encoding:
   1. begin pass / end pass
   2. set pipeline
   3. set vertex/index buffers
   4. draw / draw indexed
6. Resource lifetime and error model.

### Deliverables
1. DRP message structs/enums in `include/datoviz/drp/*`.
2. Versioned binary/text representation rules.
3. Validation spec section with explicit error codes.

### Tests
1. Roundtrip encode/decode tests for each message kind.
2. Fuzz-ish malformed payload tests for parser robustness.
3. Deterministic conformance fixtures under `testing/fixtures/drp_v2/`.

### Exit criteria
1. DRP tests pass in isolation.
2. No Vulkan symbol leaks in DRP public headers.


## M2 - Runtime core (backend-agnostic)

### Goal
Implement a runtime that consumes DRP v2 and enforces API semantics independent of Vulkan details.

### Responsibilities
1. Object registry with typed handles and generation counters.
2. State tracking for resource and pass compatibility.
3. Validation rules for command order and object usage.
4. Command recording abstraction for backend submission.

### Deliverables
1. Runtime module (new `src/...` namespace dedicated to DRP execution).
2. Runtime API boundary that backends implement.
3. Structured error reporting mapped to DRP validation codes.

### Tests
1. Handle lifetime tests (use-after-destroy, stale generation).
2. Validation tests (invalid bind, incompatible pipeline, illegal pass ops).
3. Deterministic replay tests from DRP fixture streams.

### Exit criteria
1. Runtime tests pass with a mock backend.
2. Runtime has zero direct Vulkan includes in public-facing runtime headers.


## M3 - Native Vulkan backend adapter (first functional native path)

### Goal
Map runtime operations to Vulkan efficiently while staying behind backend interface.

### Scope
1. Adapter/device/queue mapping.
2. Buffer/texture allocation mapping via existing allocator work.
3. Pipeline/bind layout/shader mapping.
4. Command buffer recording and submission.
5. Minimal swapchain/offscreen path for presentable output.

### Deliverables
1. Vulkan backend implementation module.
2. Translation layer tests for runtime-op -> Vulkan-op behavior.
3. Initial integration with existing `canvas/window/stream` path where relevant.

### Tests
1. Headless triangle render + image checksum.
2. Upload buffer -> draw -> readback assertions.
3. Validation-on tests run clean for supported platforms.

### Exit criteria
1. DRP fixture can render a known image through runtime + Vulkan backend.
2. No direct DRP/Vulkan cross-contamination in public contract headers.


## M4 - Browser JS runtime over WebGPU

### Goal
Run the same DRP v2 fixtures in browser via a JS runtime implemented on top of WebGPU.

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

### Tests
1. DRP conformance fixtures replay in browser.
2. Deterministic image/hash checks where feasible.
3. Error-code parity tests between native and browser runtimes.

### Exit criteria
1. Minimal draw fixtures pass in browser runtime.
2. Capability and validation behavior matches DRP spec expectations.
3. WASM transport can feed DRP commands to JS runtime.


## M5 - Feature-complete renderer v1 slice (native + browser contract parity)

### Goal
Reach a practical renderer baseline suitable for stabilization.

### Scope
1. Dynamic viewport/scissor.
2. Multiple bind groups/descriptor sets.
3. Texture sampling and basic sampler states.
4. Optional compute pass (if needed for immediate roadmap).
5. Explicit synchronization model represented in runtime semantics.

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


## M6 - Performance pass and stabilization

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

### Reliability work
1. Stress tests for create/destroy churn.
2. Long-run replay tests.
3. Error injection tests (invalid DRP streams, device capability mismatch).

### Exit criteria
1. Benchmarks tracked in CI artifacts.
2. Agreed performance thresholds documented.
3. Renderer declared "stabilized v1".


## M7 - Scene API kickoff (only after M6)

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


## Codex execution protocol (important)

Use this workflow for every task:

1. Pick one task ID from the checklist.
2. Ask Codex for:
   1. exact file changes
   2. tests to add/update
   3. acceptance commands
3. Require Codex to run relevant tests and report failures exactly.
4. Merge only if acceptance criteria are met.


## Task sizing rules for Codex

1. One PR/task should be 1 logical change, ideally 5 to 15 files.
2. Never combine spec changes and broad refactors without tests.
3. Prefer adding adapters over sweeping renames.
4. Keep behavioral changes accompanied by fixture updates.


## Suggested task backlog (ordered)

1. `T001`: Create DRP v2 spec skeleton and command taxonomy.
2. `T002`: Add DRP versioning fields and compatibility checks.
3. `T003`: Implement DRP parser/encoder with roundtrip tests.
4. `T004`: Add DRP validation error code framework.
5. `T005`: Introduce semantic-core object registry with generation-safe handles.
6. `T006`: Add mock backend interface and semantic-core unit tests.
7. `T007`: Implement native runtime command dispatcher for minimal draw path.
8. `T008`: Build Vulkan backend adapter for buffer lifecycle.
9. `T009`: Build Vulkan backend adapter for texture/sampler lifecycle.
10. `T010`: Build Vulkan backend adapter for render pipeline and draw.
11. `T011`: Add first headless native end-to-end DRP fixture rendering test.
12. `T012`: Add browser JS runtime DRP decoder and minimal draw fixture.
13. `T013`: Add WASM -> JS DRP transport API (minimal command stream).
14. `T014`: Add capability model and unsupported-feature reporting with native/browser parity tests.
15. `T015`: Add present path integration where needed with canvas/window (native).
16. `T016`: Add browser conformance replay suite for shared DRP fixtures.
17. `T017`: Add microbenchmark harness and baseline capture (native) + browser timing smoke metrics.
18. `T018`: Stabilization pass (bug fixes + docs + compatibility notes).


## Acceptance gates by phase

1. Gate A (after M1): DRP v2 contract and parser stable.
2. Gate B (after M2): Runtime semantic validation stable with mock backend.
3. Gate C (after M3): First Vulkan-backed rendering fixture stable.
4. Gate D (after M4): First browser WebGPU runtime fixture stable.
5. Gate E (after M5): Feature-complete v1 renderer slice with native/browser contract parity.
6. Gate F (after M6): Performance and reliability stabilized.
7. Gate G (after M7 start): Scene API work can begin without low-level leakage.


## Quality checklist (for every merged task)

1. Public API changes documented.
2. Tests added or updated.
3. No new dependency from high-level modules to backend internals.
4. Build passes on primary platform.
5. Logs/errors are actionable.
6. No unchecked TODOs without linked task ID.


## Risks and mitigation

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


## Definition of done for "WebGPU renderer first objective"

1. DRP v2/WebGPU-shaped contract is versioned and documented.
2. Native runtime executes DRP v2 with strict validation and deterministic behavior.
3. Browser JS runtime executes DRP v2 over WebGPU with conformance parity on agreed fixture set.
4. Vulkan backend passes renderer conformance fixtures for native targets.
5. WASM bridge path (C/WASM -> JS runtime -> WebGPU) is operational on minimal scene example.
6. Performance baseline is established and tracked (native, plus browser smoke metrics).
7. Renderer API/behavior is stable enough to start standalone Scene API development.
