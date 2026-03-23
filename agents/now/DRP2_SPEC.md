> **Execution Status**
> - **Status:** `READY AFTER LOW-LEVEL CLEANUP IS STABLE ENOUGH`
> - **Updated on:** `2026-03-23`
> - **Purpose:** Define the first DRP2 contract without starting runtime or scene implementation.
> - **Current branch priority:** The active immediate priority is still `vk`/`vklite` boundary and
>   lifecycle cleanup. This file exists so the next higher-level spec phase starts with a narrow,
>   disciplined scope instead of another broad brainstorming pass.

# DRP2 Spec Phase

This document describes the next actionable spec-only phase for higher-level Datoviz work.


## Objective

Freeze a small DRP2 renderer contract that is strong enough to support future scene layers and a
future browser runtime, without coupling the contract to Vulkan internals.


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


## Recommended Task Order

1. tighten `spec/drp2/LAYER1.md` until the first command set is stable
2. align schemas with that reduced command set
3. define symbolic error codes and capability flags
4. define the first fixture corpus
5. validate the contract against a small set of scene use cases
6. only then start runtime experiments


## Guardrails

1. Prefer deleting speculative scope over carrying it forward.
2. Do not design the future scene API and DRP2 at the same time.
3. Do not allow public DRP2 definitions to mention `Vk*` types.
4. Do not treat prototype headers under `spec/*/prototypes/` as source of truth.
