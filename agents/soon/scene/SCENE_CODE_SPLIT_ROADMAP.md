# Scene Code Split Pickup

> **Execution Status**
> - **Status:** `SOON / STRUCTURAL CLEANUP`
> - **Updated on:** `2026-05-29`
> - **Purpose:** point agents at the durable scene source-split roadmap without duplicating it.

Use [`../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md)
for source-split progress. Use
[`../../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](../../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md)
for the final architecture target: explicit typed metadata, registry-driven visual-family
operations, descriptor-driven runtime emission, coarse reusable CMake layers, and final done
criteria.


## Pickup Order

1. Continue annotation/domain helper splits where generated visuals and public object state are
   mixed; field dirty propagation and scalar sampling are already split out.
2. Continue query-family ownership in family `visuals/*/query.c` files; move remaining scratch
   geometry, non-item result decoding outside the standard item-id path, and unsupported-policy
   decisions into family owners while keeping queueing, freshness, executor lifecycle, metadata
   completeness checks, shared scratch helpers, standard item-id decoding, standard item-target
   eligibility, native/sample target fallback policy, and readback scheduling generic.
3. Eliminate normal untyped descriptor inference: point/pixel/marker/splat/primitive/image/labels
   and textured-mesh WGSL fallback paths now resolve typed metadata labels. Continue by deleting or
   quarantining `desc_untyped_compat.c` behind an explicit compatibility-only path once fixture or
   import callers are audited.
4. Continue low-risk payload/helper extraction from scene-emission support only where the extracted
   owner builds family data or cache payloads without taking over FramePlan ordering.
5. Continue visual-family payload/helper splits only when a clear owner boundary appears; the first
   descriptor/attribute split inside `src/scene/visuals/` is complete, and the active registry
   already owns lowering, bounds, pass caps, bind, pipeline, shader, and draw hooks.
6. Tighten remaining query/interaction boundaries without changing picking semantics accidentally.
7. Split scene CMake targets into coarse reusable layers once the registry and family boundaries are
   clear; do not split per visual family unless a concrete consumer needs it.

Completed through 2026-05-29: FramePlan internals were split into lifecycle/facade, node helpers,
capabilities, diagnostics, upload resources, graph resources, node passes, graph passes,
dependencies, graph validation, graph helpers, and readback owner files. Render contracts were
split into facade, diagnostics, resources, and visual assembly. Runtime render emission was split
into shared helpers, bindings, draw emission, visual preparation, and pass emission. Core scene was
split into notify, format state, frame trace, panel geometry/layout, grid, controllers, and figure
emission owners. Follow-up slices also split visual descriptor-kind helpers, colormap annotation
ownership, scale-bar/colorbar/legend/text-font annotation ownership, scene domain buffers, and
field/polygon helpers, visual descriptor/attribute helpers, query target/profile policy, upload
support helpers, panel drawable/viewport helpers, and narrow helper declarations formerly exposed
through `_scene.h`. Query now also has shared scratch helpers, standard item-id decoding, standard
item-target eligibility, native/sample target fallback policy, and vector/segment/path family
decode ownership. FramePlan owns render-metadata completeness checks, render contracts reject
missing typed metadata unless explicit compatibility is enabled, WGSL fallback resolves typed
metadata labels for point, pixel, marker, splat, primitive, image/labels, and textured mesh,
`domain/field_dirty.c` owns sampled-field dirty propagation, and `domain/field_sample.c` owns
scalar sampled-field interpretation. The old `src/scene/plan/` folder was removed; its
ownership now lives in `src/scene/frame_plan/`, `src/scene/scene_emit/`, and
`src/scene/render_contract/`. Last focused validation for the split was `git diff --check`,
`just build`,
`direnv exec . just test scene/query` (`40/40`), `direnv exec . just test fields` (`47/47`),
focused WGSL typed-fallback and FramePlan static-render filters, earlier
`direnv exec . just test scene/frame-plan` (`55/55`), earlier
`direnv exec . just test scene-graph` (`158/158`), focused sampled-field update filters, and
`direnv exec . just test app-offscreen` (`76/76`). The earlier broad query readback failures are
not current blockers.

Standalone candidates to assess during the next split are `frame_plan/`, `render_contract/`,
`query/`, `text/`, `domain/`, and `visuals/registry/`. Keep `scene_emit/`, `runtime/`,
`techniques/`, and `app/` as orchestration/runtime layers even if they become separate CMake
targets.


## Validation

Documentation-only updates need `git diff --check` and `git status --short`.

Code splits need `just build`, `git diff --check`, and `direnv exec . just test scene`. Add
`just spec-check` for DRP2 schema/fixture/portable-command changes and an offscreen or bounded GLFW
smoke for runtime resource, command-buffer, descriptor, render-target, or synchronization changes.
