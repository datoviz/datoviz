# Scene Code Split Pickup

> **Execution Status**
> - **Status:** `SOON / STRUCTURAL CLEANUP`
> - **Updated on:** `2026-05-28`
> - **Purpose:** point agents at the durable scene source-split roadmap without duplicating it.

Use [`../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md)
for source-split progress. Use
[`../../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](../../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md)
for the final architecture target: explicit typed metadata, registry-driven visual-family
operations, descriptor-driven runtime emission, coarse reusable CMake layers, and final done
criteria.


## Pickup Order

1. Continue low-risk derived payload extraction from `scene_emit/uploads.c`.
2. Continue annotation/domain helper splits where generated visuals and public object state are mixed.
3. Eliminate normal untyped descriptor inference: make all active scene/query render paths emit
   explicit `DvzFramePlanVisualMeta`, then delete or quarantine the current `desc_legacy.c` path.
4. Introduce the visual-family registry and migrate generic switches into family operations.
5. Continue visual-family payload/helper splits only when a clear owner boundary appears; the first
   descriptor/attribute split inside `src/scene/visuals/` is complete.
6. Tighten remaining query/interaction boundaries without changing picking semantics accidentally.
7. Split scene CMake targets into coarse reusable layers once the registry and family boundaries are
   clear; do not split per visual family unless a concrete consumer needs it.

Completed on 2026-05-28: FramePlan internals were split into lifecycle/facade, node helpers,
capabilities, diagnostics, upload resources, graph resources, node passes, graph passes,
dependencies, graph validation, graph helpers, and readback owner files. Render contracts were
split into facade, diagnostics, resources, and visual assembly. Runtime render emission was split
into shared helpers, bindings, draw emission, visual preparation, and pass emission. Core scene was
split into notify, format state, frame trace, panel geometry/layout, grid, controllers, and figure
emission owners. Follow-up slices also split visual descriptor-kind helpers, colormap annotation
ownership, scale-bar/colorbar/legend/text-font annotation ownership, scene domain buffers, and
field/polygon helpers, visual descriptor/attribute helpers, and query target/profile policy. The old
`src/scene/plan/` folder was removed; its ownership now lives in `src/scene/frame_plan/`,
`src/scene/scene_emit/`, and `src/scene/render_contract/`.


## Validation

Documentation-only updates need `git diff --check` and `git status --short`.

Code splits need `just build`, `git diff --check`, and `direnv exec . just test scene`. Add
`just spec-check` for DRP2 schema/fixture/portable-command changes and an offscreen or bounded GLFW
smoke for runtime resource, command-buffer, descriptor, render-target, or synchronization changes.
