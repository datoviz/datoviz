> **Execution Status**
> - **Status:** `ACTIVE SPEC / FIXTURE CONTRACT`
> - **Updated on:** `2026-03-23`
> - **Purpose:** Keep the DRP2 `2.0` contract executable and disciplined before any runtime or scene
>   implementation starts.
> - **Current branch priority:** The low-level graphics stack remains the main code priority, but
>   the DRP2 spec/fixture lane is now real, runnable, and worth keeping current because it has
>   become an executable contract rather than a planning note.

# DRP2 Spec Phase

This document describes the active DRP2 spec/fixture phase for higher-level Datoviz work.


## Objective

Keep a small DRP2 renderer contract strong enough to support future scene layers and a future
browser runtime, without coupling the contract to Vulkan internals.


## Current Status

The DRP2 spec is no longer just an outline.

What is now in place:

1. authoritative prose for the active DRP2 `2.0` command surface in `spec/drp2/COMMANDS.md`
2. aligned machine-readable active schemas in `spec/drp2/schema/`
3. explicit active vs deferred boundary in the DRP2 schema docs
4. lifetime/state and error-selection rules in `LIFETIMES.md` and `ERRORS.md`
5. fixture format, fixture schema, and runner contract under `spec/drp2/fixtures/`
6. a runnable Python validator in `tools/drp2_fixture_runner.py`
7. an active conformance corpus exercised by `just spec-check`

Active DRP2 `2.0` surface now includes:

1. buffers and textures
2. command encoders and passes
3. lightweight render/compute pipelines
4. vertex/index buffer binding
5. lightweight bind groups
6. lightweight bind-group layouts
7. copy commands and queue submission

What remains intentionally deferred:

1. pipeline layouts
2. shader modules
3. samplers
4. texture views
5. indirect draws/dispatch
6. richer backend-facing pipeline/shader semantics


## Validation Snapshot

Verified on this revision:

1. `python3 tools/drp2_fixture_runner.py --json`
2. `.venv/bin/pytest -q testing/test_drp2_fixture_runner.py`
3. `just spec-check`

Current executable DRP2 corpus status:

1. `42/42` fixtures passing
2. `14` focused runner tests passing


## In Scope

1. DRP2 human-readable Layer 1 contract
2. error model
3. capability model
4. versioning rules
5. machine-readable schemas
6. initial conformance fixtures
7. scene pressure-test requirements


## Explicitly Out Of Scope

1. browser runtime implementation
2. wasm transport implementation
3. scene runtime implementation
4. public production headers under `include/datoviz/`
5. native interop API design
6. performance and profiling API design


## Acceptance Criteria

1. `spec/drp2/` has a coherent indexed structure.
2. The minimal command set is frozen for first implementation work.
3. `spec/scene/` defines consumer requirements without backend leakage.
4. The first fixture list is defined.
5. The spec is narrow enough that implementation can proceed incrementally.


## Recommended Next Step

The most sensible next DRP2 step is not another large object-graph promotion.

Recommended next slice:

1. tighten `SetBindGroup.dynamic_offsets` semantics end to end
2. let bind-group layout entries declare whether a buffer binding uses dynamic offsets
3. validate dynamic-offset count and ordering in `SetBindGroup`
4. add positive and negative fixtures for missing, extra, and misordered dynamic offsets
5. add explicit destruction-safety negatives for bind-group layouts and pipelines

Reasoning:

1. the active contract already has pipelines, bind groups, and bind-group layouts
2. `dynamic_offsets` is still present but under-specified
3. this next slice deepens the current active model instead of reopening deferred object families


## Recommended Task Order

1. keep `spec/drp2/COMMANDS.md`, `LIFETIMES.md`, `ERRORS.md`, and active schemas aligned
2. prefer extending the existing executable fixture corpus over adding prose-only rules
3. keep deferred object families deferred unless the runner can validate them meaningfully
4. use `just spec-check` as the default validation gate for DRP2 spec changes
5. only start runtime experiments once a slice is strong enough that fixtures, schemas, and prose all
   agree on the same contract


## Guardrails

1. Prefer deleting speculative scope over carrying it forward.
2. Do not design the future scene API and DRP2 at the same time.
3. Do not allow public DRP2 definitions to mention `Vk*` types.
4. Do not treat prototype headers under `spec/*/prototypes/` as source of truth.
5. Do not promote deferred commands unless their semantics can be validated by the active fixture
   runner without inventing backend-specific behavior.
