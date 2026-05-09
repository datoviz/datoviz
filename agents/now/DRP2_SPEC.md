> **Execution Status**
> - **Status:** `ACTIVE DRP2 EXECUTION NOTE`
> - **Updated on:** `2026-05-08`
> - **Purpose:** Keep the DRP2 `2.0` contract executable and disciplined while implementation work
>   proceeds.
> - **Current branch priority:** DRP2 is now both an executable spec lane and an active C
>   implementation/runtime lane. Keep prose, schemas, fixtures, stream code, runtime behavior, and
>   scene emitters aligned.

# DRP2 Spec Phase

This document describes the active DRP2 spec/fixture work queue for higher-level Datoviz work.
Normative DRP2 protocol rules live under `spec/drp2/`.


## Objective

Keep a small DRP2 renderer contract strong enough to support future scene layers and a future
browser runtime, without coupling the contract to Vulkan internals.

The current implementation plan now specifically pressures DRP2 with:

1. one minimal indexed 3D visual (`mesh`) on the native runtime,
2. per-panel depth attachments and depth/stencil state,
3. viewport/scissor and panel-relative UBO usage,
4. an early browser/WebGPU feasibility replay,
5. later transparency and picking paths.


## Current Status

The DRP2 spec is no longer just an outline, and DRP2 is no longer spec-only.

What is now in place:

1. authoritative prose for the active DRP2 `2.0` command surface in `spec/drp2/COMMANDS.md`
2. aligned machine-readable active schemas in `spec/drp2/schema/`
3. explicit active vs deferred boundary in the DRP2 schema docs
4. lifetime/state and error-selection rules in `LIFETIMES.md` and `ERRORS.md`
5. fixture format, fixture schema, and runner contract under `spec/drp2/fixtures/`
6. a runnable Python validator in `tools/drp2_fixture_runner.py`
7. an active conformance corpus exercised by `just spec-check`
8. a C command-stream and JSON/debug serialization implementation in `src/drp2`
9. semantic validation plus a native vklite-backed runtime
10. scene pressure fixtures emitted from C and tested against committed JSON

Active DRP2 `2.0` surface now includes:

1. buffers and textures
2. command encoders and passes
3. lightweight render/compute pipelines
4. vertex/index buffer binding
5. lightweight bind groups
6. lightweight bind-group layouts
7. dynamic buffer offsets on bind-group bindings
8. shader modules
9. samplers
10. texture views for bind-group sampling
11. copy commands and queue submission
12. `QueueSubmit.readbacks` and `QueueSubmitReply`
13. destruction-safety negatives for all resource types (buffers, textures, texture views,
    samplers, bind groups, bind-group layouts, shader modules, render and compute pipelines)
14. conservative submitted-work lifetime coverage for all resource types
15. explicit bounded-range requirement for buffer-backed bind-group entries
16. pipeline-rebind validation for later bind-group and draw commands

What remains intentionally deferred:

1. pipeline layouts
2. indirect draws/dispatch
3. explicit resource barriers
4. richer backend-facing pipeline/shader semantics
5. protocol-visible fences and completion-based destruction

Pressure areas expected next:

1. richer render-pipeline fixed-function state validation for the native 3D slice,
2. fixture coverage that combines viewport/scissor, depth state, and draw sequencing,
3. browser-oriented replay checks for the currently active command subset,
4. multi-pass sequencing pressure from transparency work,
5. readback and identity-routing pressure from scene picking flows.


## Validation Snapshot

Verified on this revision:

1. `python3 tools/drp2_fixture_runner.py --json`
2. `.venv/bin/pytest -q testing/test_drp2_fixture_runner.py`

Current executable DRP2 corpus status:

1. `119/119` fixtures passing
2. focused runner tests must be kept in lockstep with fixture tag growth

Current rule for upcoming work:

1. do not widen the spec only because a future visual family might want it,
2. do widen the spec when `mesh`, browser replay, transparency, or picking expose a concrete gap
   that the active fixture runner can validate meaningfully.


## In Scope

1. DRP2 human-readable Layer 1 contract
2. error model
3. capability model
4. versioning rules
5. machine-readable schemas
6. initial conformance fixtures
7. scene pressure-test requirements


## Explicitly Out Of Scope

1. full browser runtime implementation
2. wasm transport implementation
3. a second scene runtime implementation path
4. public production headers under `include/datoviz/`
5. native interop API design
6. performance and profiling API design

Clarification:

Small browser/WebGPU feasibility experiments are now expected to pressure the contract, but this
file is still not the execution checklist for browser implementation work. Keep the active spec lane
focused on contract quality and executable conformance material.


## Acceptance Criteria

1. `spec/drp2/` has a coherent indexed structure.
2. The minimal command set is frozen for first implementation work.
3. `spec/scene/` defines consumer requirements without backend leakage.
4. The first fixture list is defined.
5. The spec is narrow enough that implementation can proceed incrementally.


## Implementation Plan

The first implementation sequencing plan is complete and recorded in
`agents/done/SCENE_DRP2_IMPLEMENTATION.md`. Current implementation sequencing is tracked in
`agents/now/V0_4_NEXT_STEPS.md`.

Keep this file focused on the active DRP2 contract and fixture lane. Do not duplicate module
bring-up order here.

Spec maintenance rules:

1. keep `spec/drp2/COMMANDS.md`, `LIFETIMES.md`, `ERRORS.md`, and active schemas aligned,
2. prefer extending the existing executable fixture corpus over adding prose-only rules,
3. keep deferred object families deferred unless the runner can validate them meaningfully,
4. use `just spec-check` as the default validation gate for DRP2 spec changes.

Near-term spec work should concentrate on:

1. depth-attachment and depth-state fixtures required by the first `mesh` slice,
2. viewport/scissor and panel-region sequencing fixtures where scene emission now relies on them,
3. browser-portability review of shader/module and pipeline assumptions,
4. keeping transparency and picking requirements explicit but deferred until their first executable
   fixture slice is ready.


## Guardrails

1. Prefer deleting speculative scope over carrying it forward.
2. Do not design the future scene API and DRP2 at the same time.
3. Do not allow public DRP2 definitions to mention `Vk*` types.
4. Do not treat prototype headers under `spec/*/prototypes/` as source of truth.
5. Do not promote deferred commands unless their semantics can be validated by the active fixture
   runner without inventing backend-specific behavior.
