# Datoviz v0.4-dev Low-Level Consistency Plan

> **Status**
> - **Status:** `ACTIVE`
> - **Created on:** `2026-03-23`
> - **Predecessor phase:** `vk` / `vklite` ownership-boundary and wrapper-opacity cleanup is
>   effectively complete for the active surface.
> - **Current intent:** avoid reopening broad low-level boundary refactors unless a concrete API
>   pressure appears, and instead make the active low-level stack feel intentionally designed.

## Why this phase exists

The highest-value `vk` / `vklite` boundary work has now landed:

1. `vklite` no longer depends on vk-private headers.
2. The active wrapper-opacity pass is effectively complete.
3. `memory.h` now documents a stable allocator path versus advanced external-memory interop.
4. `queues.h` is documented as an intentional low-level queue-planning API, not an accidental leak.

That means the next leverage is no longer another ownership rewrite. The active goal is consistency:
naming, lifecycle semantics, and API shape across the low-level modules that already make up the
working v0.4 stack.


## Scope

In scope:

1. `canvas`
2. `stream`
3. `video`
4. `vk`
5. `vklite`

Out of scope:

1. Reopening broad `memory.h` or `queues.h` API changes without a concrete new pressure.
2. Activation of dormant higher-level layers (`renderer`, `scene`, `client`, `wasm`, `color`).
3. Backend feature expansion unrelated to consistency/lifecycle cleanup.


## Main objectives

1. Make object naming and lifecycle shape feel consistent across the active low-level stack.
2. Keep owner/borrower semantics explicit and predictable.
3. Preserve and expand focused regression coverage for repeated-submit, reset/recreate, and
   destroy-idempotence paths.
4. Keep the public low-level contract documented as it exists now, instead of letting comments drift
   behind the code again.


## Recommended work order

1. Audit naming and lifecycle verbs across the active low-level modules.
   Focus on mismatches between `create`, `init`, `wrap`, `destroy`, `free`, `recreate`, `reset`,
   `upload`, and `download`.
2. Identify the small set of semantics that should be uniform across modules.
   Examples:
   - wrapper allocation versus runtime-object creation
   - idempotent destroy behavior
   - partial-init unwind behavior
   - recreate/reset expectations
3. Fix one consistency slice at a time, end to end.
   Prefer narrow patches that align one concept across 2-3 modules over broad churn everywhere at
   once.
4. Keep focused validation current while doing this work.
   The highest-value tests remain repeated submit, recreate, and destroy-idempotence paths.
5. Update docs/comments when the code meaning changes so the public and internal contracts stay in
   sync.


## Immediate candidate slices

1. Lifecycle verb audit across `canvas`, `stream`, `video`, `vk`, and `vklite`.
2. Idempotent destroy and partial-init unwind audit for resource-owning wrappers.
3. Recreate/reset semantics audit for presentation, offscreen, and submission paths.
4. Naming consistency pass for wrapper allocation versus owned runtime creation.


## Decision notes carried from the previous phase

1. `include/datoviz/vk/queues.h` should be treated as an intentional public low-level
   queue-planning API for v0.4.
2. `include/datoviz/vk/memory.h` should remain public, but its stable core is the allocator/map/copy
   path; external-memory import/export is advanced interop surface, not the main contract.
3. `DvzBarriers` remains intentionally public as a builder/config type.
4. The default next move is no longer another boundary-hardening rewrite unless a specific pressure
   appears.


## Validation guidance

Use the narrowest relevant loop for the slice being changed:

1. `just build`
2. `direnv exec . just test vk`
3. `direnv exec . just test canvas`
4. `just test` when the change crosses multiple active modules

On Vulkan/presentation paths, prefer the repo environment (`direnv exec .`) and an unsandboxed
runtime context, as already noted elsewhere in the repo guidance.


## Exit criteria for this phase

1. The active low-level modules have more uniform lifecycle and naming conventions.
2. Focused lifecycle regression tests remain green while the cleanup proceeds.
3. The codebase feels less transitional and more like a deliberate v0.4 low-level API.
