# Scene Code Split Pickup

> **Execution Status**
> - **Status:** `SOON / STRUCTURAL CLEANUP`
> - **Updated on:** `2026-05-28`
> - **Purpose:** point agents at the durable scene source-split roadmap without duplicating it.

Use [`../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md)
as the source of truth before broad `src/scene` refactors.


## Pickup Order

1. Continue low-risk derived payload extraction from `plan/visual_lowering_uploads.c`.
2. Continue annotation/domain helper splits where generated visuals and public object state are mixed.
3. Continue visual descriptor/attribute splits inside `src/scene/visuals/`.
4. Tighten remaining query/interaction boundaries without changing picking semantics accidentally.

Completed on 2026-05-28: FramePlan internals were split into lifecycle/facade, node helpers,
capabilities, diagnostics, upload resources, graph resources, node passes, graph passes,
dependencies, graph validation, graph helpers, and readback owner files. Render contracts were
split into facade, diagnostics, resources, and visual assembly. Runtime render emission was split
into shared helpers, bindings, draw emission, visual preparation, and pass emission. Core scene was
split into notify, format state, frame trace, panel geometry/layout, grid, controllers, and figure
emission owners. Follow-up slices also split visual descriptor-kind helpers, colormap annotation
ownership, scale-bar/colorbar/legend/text-font annotation ownership, scene domain buffers, and
query target/profile policy.


## Validation

Documentation-only updates need `git diff --check` and `git status --short`.

Code splits need `just build`, `git diff --check`, and `direnv exec . just test scene`. Add
`just spec-check` for DRP2 schema/fixture/portable-command changes and an offscreen or bounded GLFW
smoke for runtime resource, command-buffer, descriptor, render-target, or synchronization changes.
