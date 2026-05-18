# Scene Public API Header Plan

> **Execution Status**
> - **Status:** `COMPLETED HEADER SPLIT RECORD`
> - **Updated on:** `2026-05-18`
> - **Purpose:** preserve the public scene header split history after the broad header-drafting
>   task landed.
> - **Current location:** `agents/done/`; current public API work should start from
>   [../../spec/scene/api/API_SURFACE.md](../../spec/scene/api/API_SURFACE.md),
>   [../now/V0_4_NEXT_STEPS.md](../now/V0_4_NEXT_STEPS.md), and focused `agents/soon/` plans.


## Current State

The first public scene header split now exists:

1. `include/datoviz/scene.h` remains the umbrella header and contains scene, figure, panel, visual,
   buffer, primitive, mesh, image, and texture convenience APIs.
2. `include/datoviz/scene/types.h` and `include/datoviz/scene/enums.h` contain shared public value
   types and enums.
3. Focused public subheaders now exist for:
   - `field.h` — sampled-field descriptors, updates, and visual field binding,
   - `scale.h` — scale, colormap, colorbar, and visual scale binding,
   - `interaction.h` — interaction/picking/selection/link/readout draft API,
   - `text.h` — font/text draft API,
   - `annotation.h` — annotation draft API,
   - `panzoom.h` / `arcball.h` — active panel controller APIs,
   - `frame_plan.h` — active frame-plan and DRP2 emitter APIs.
4. API-shape examples are present under `spec/scene/examples/` for mesh selection, image probe,
   scale/colorbar/annotation, and sampled fields.

This means the old “draft headers before implementation” task is complete. Future work should not
create another broad header sketch unless a concrete implementation slice exposes a gap.


## Implemented Versus Drafted

Implemented and covered by focused scene tests:

1. scene / figure / panel ownership,
2. frame plans and DRP2 emission,
3. point, primitive, mesh, path-as-line/strip, and image visuals,
4. retained visual data and partial updates,
5. scene-owned index/uniform-capable buffers for primitive/mesh slices,
6. sampled fields and image field binding, including partial texture-region updates,
7. scale/colormap core and image colormap scale binding,
8. colorbar retained object bookkeeping,
9. panzoom and arcball controllers,
10. panel backgrounds, z-layer ordering, and fixed-vs-controller-applied visual attachments,
11. interaction policy, selection, link-channel, pick/probe queue, hover-state, and pinned-readout
    bookkeeping,
12. first GPU-backed request execution for point picking and image probing through
    `dvz_figure_process_requests()`,
13. font/text and annotation retained-object bookkeeping.

Drafted in public headers but not yet implemented in `src/scene`:

1. rendered glyph/text output,
2. rendered annotation contributions,
3. rendered colorbar ticks/labels,
4. broad mapped attributes beyond the current image scale/colormap path,
5. selection highlight rendering and richer link-driven visual updates,
6. mesh/object picking and richer probe payloads.

Partially implemented / semantic bookkeeping only:

1. colorbars are retained scene objects but do not yet render ticks/labels,
2. scale/colormap state is consumed by image shader paths, but broader mapped attributes are not yet
   wired,
3. primitive/mesh shading has a small uniform-buffer path, not a general material system,
4. text and annotations are retained scene objects but do not yet emit glyph/overlay rendering work,
5. pick/probe execution is real but narrow: point and image only, auxiliary streams, RGBA-style
   payload/readback, and limited coordinate/value metadata.


## Next Header Rules

1. Keep `scene.h` as the umbrella include; add to focused `include/datoviz/scene/*.h` subheaders
   instead of growing the umbrella directly.
2. Do not add generic public binding APIs yet; keep typed setters for each retained object family.
3. Treat `interaction.h`, `text.h`, and `annotation.h` as first-slice contracts: object lifetime and
   queues are implemented, but rendered output and richer semantics still need tests before use.
4. When a declared API is implemented, add focused tests in `src/scene/tests/test_scene.c` before
   broadening the surface.
5. Keep `spec/scene/api/API_SURFACE.md` as policy, but make installed headers the source of truth
   for names that already exist.


## Historical Implementation Order

The sequence below is retained as context for the header split and first implementation slices. It
is not the current active execution order.

1. Native 3D/depth/arcball pressure before adding more API surface.
2. Manual interactive examples for the implemented point/image pick-probe path.
3. Safety/hygiene review of scene request queues, retained object lifetimes, and app trace/status code.
4. Rendered colorbar/text/annotation realization once the native runtime pressure is stable.
5. Expand scale binding beyond images only when a concrete visual or interaction path needs it.
6. Widen pick/probe payloads and mesh targets after the current narrow request architecture has been
   validated interactively.
