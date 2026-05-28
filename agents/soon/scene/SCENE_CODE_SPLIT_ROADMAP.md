# Scene Code Split Pickup

> **Execution Status**
> - **Status:** `SOON / STRUCTURAL CLEANUP`
> - **Updated on:** `2026-05-28`
> - **Purpose:** point agents at the durable scene source-split roadmap without duplicating it.

Use [`../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md)
as the source of truth before broad `src/scene` refactors.


## Pickup Order

1. Finish low-risk derived payload extraction from `plan/visual_lowering_uploads.c`.
2. Split render-contract resolution into facade, visual contracts, resource contracts, and
   diagnostics.
3. Split runtime render emission into pass setup, visual draws, binding selection, and draw-count
   resolution.
4. Split core scene ownership once plan/runtime churn is easier to inspect.
5. Split annotation/domain helpers where generated visuals and public object state are mixed.
6. Tighten query/interaction boundaries without changing picking semantics accidentally.

Completed on 2026-05-28: FramePlan internals were split into lifecycle/facade, node helpers,
capabilities, diagnostics, upload resources, graph resources, node passes, graph passes,
dependencies, graph validation, graph helpers, and readback owner files.


## Validation

Documentation-only updates need `git diff --check` and `git status --short`.

Code splits need `just build`, `git diff --check`, and `direnv exec . just test scene`. Add
`just spec-check` for DRP2 schema/fixture/portable-command changes and an offscreen or bounded GLFW
smoke for runtime resource, command-buffer, descriptor, render-target, or synchronization changes.
