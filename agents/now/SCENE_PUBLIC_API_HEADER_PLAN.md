# Scene Public API Header Plan

> **Execution Status**
> - **Status:** `HEADER DRAFT LANDED; IMPLEMENTATION FOLLOW-UP`
> - **Updated on:** `2026-05-11`
> - **Purpose:** record the current public scene header split and identify which declared API groups
>   still need implementation.


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
10. panel backgrounds, z-layer ordering, and fixed-vs-controller-applied visual attachments.

Drafted in public headers but not yet implemented in `src/scene`:

1. interaction policy objects,
2. pick/probe request execution and polling,
3. retained hover state,
4. selection objects and link channels,
5. pinned readouts,
6. font/text retained objects,
7. annotation retained objects.

Partially implemented / semantic bookkeeping only:

1. colorbars are retained scene objects but do not yet render ticks/labels,
2. scale/colormap state is consumed by image shader paths, but broader mapped attributes are not yet
   wired,
3. primitive/mesh shading has a small uniform-buffer path, not a general material system.


## Next Header Rules

1. Keep `scene.h` as the umbrella include; add to focused `include/datoviz/scene/*.h` subheaders
   instead of growing the umbrella directly.
2. Do not add generic public binding APIs yet; keep typed setters for each retained object family.
3. Treat `interaction.h`, `text.h`, and `annotation.h` as draft contracts until their first source
   implementations and tests land.
4. When a declared API is implemented, add focused tests in `src/scene/tests/test_scene.c` before
   broadening the surface.
5. Keep `spec/scene/api/API_SURFACE.md` as policy, but make installed headers the source of truth
   for names that already exist.


## Recommended Implementation Order

1. Interaction core without GPU picking first: object allocation/lifetime, panel binding, selection,
   link keys, and result queues that tests can populate deterministically.
2. Pick/probe plumbing through the scene -> DRP2 readback path for points/images, with request ids and
   stale-result rejection.
3. Text/annotation retained-object bookkeeping, still without glyph rendering if necessary.
4. Rendered colorbar/text/annotation realization once the retained object model has tests.
5. Expand scale binding beyond images only after the first interaction/probe path needs it.
