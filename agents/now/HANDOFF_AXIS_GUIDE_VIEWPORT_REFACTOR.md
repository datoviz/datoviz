# Axis, Guide, Viewport Refactor Handoff

Status: active architecture refactor lane, started on 2026-06-30.

Read this before changing 2D axes, grid lines, guide lines/spans, panel DATA coordinates, View2D,
aspect-ratio handling, plot/panel viewport logic, or GSP-facing guide behavior.


## Source Plan

Durable plan:
[../../plans/AXIS_GUIDE_VIEWPORT_REFACTOR_PLAN.md](../../plans/AXIS_GUIDE_VIEWPORT_REFACTOR_PLAN.md)

Related GSP readiness:
[../../spec/api/GSP_BACKEND_READINESS.md](../../spec/api/GSP_BACKEND_READINESS.md)


## Completed Checkpoints

Committed on `v0.4-dev`:

1. `0a6717ac3` `plan: define axis guide viewport refactor`
   - Added the aggressive API/ABI-breaking refactor plan.

2. `555aca425` `scene: centralize generated axis guide visual policy`
   - Added `src/scene/annotation/generated_visual_policy.h`.
   - Moved axis marks, axis text, axis grid, guide fill, guide line, and guide outline attachment
     defaults onto semantic generated-visual roles.
   - Guide descriptor `z_layer` now behaves as an offset from the semantic role layer.
   - Removed the `features/guide_spans` local z workaround.

3. `bb07751e7` `scene: add panel frame snapshot for guides`
   - Added internal `DvzPanelFrameSnapshot` in `src/scene/core/_scene.h`.
   - Added resolver in `src/scene/core/panel_frame_snapshot.c`.
   - Migrated guide line/span upload and label placement to use snapshot visible domains and
     plot/panel pixel rectangles.

4. `3cac186cd` `scene: drive axis visuals from panel snapshot`
   - Migrated axis grid/tick/spine geometry to one per-frame panel snapshot for plot view,
     controller extent, visible data domain, and pixel sizing.

5. `420bf87a4` `scene: drive axis text from panel snapshot`
   - Migrated axis text layout to the same snapshot for controller extent and panel-pixel
     conversion.
   - Removed the stale `_axis_visual_to_pixels()` parallel geometry helper.


## Current Invariants

Generated visual ordering is semantic, not example-driven:

```text
guide fill < axis grid < default data < guide line/outline < axis marks < axis text
```

Axis/grid/guide generated visuals should not introduce new hard-coded local `z_layer`,
`coord_space`, `controller_mode`, depth, or alpha defaults. Extend generated role policy instead.

Axis visuals, axis text, and guide upload now share `DvzPanelFrameSnapshot`. Do not reintroduce
ad-hoc calls to `_scene_panel_panzoom_extent()`, `_scene_panel_pixel_rect()`,
`dvz_panel_plot_rect_px()`, or `dvz_panel_visible_domain()` in those paths unless the snapshot is
missing required data and the plan is updated.


## Validation Already Run

From Datoviz root:

```sh
just test axis
just test interaction
just example-c features/guide_spans --png
git diff --check
```

The generated `feature_guide_spans.png` was inspected and removed. Grid lines remain visible
through transparent spans; points stay above spans; outlines and labels remain visible.

From `../GSP_API`:

```sh
PYTHONPATH=/home/cyrille/GIT/Viz/datoviz:. .venv/bin/pytest -q \
  tests/test_datoviz_v04_probe.py \
  tests/test_datoviz_v04_protocol_renderer.py \
  tests/test_matplotlib_guides.py \
  tests/test_matplotlib_guide_query.py

PYTHONPATH=/home/cyrille/GIT/Viz/datoviz:. .venv/bin/python \
  tools/probe_datoviz_guide_axis.py \
  --out artifacts/visual_qa/s030/datoviz-guide-axis-proof.json
```

The pytest slice passed with `126 passed`. The guide-axis probe passed API-side without capture.
Its generated artifact was removed. GSP still reports rendered placement as blocked when capture is
not requested.


## Next Checkpoints

Recommended next commit:

1. Extend generated role policy to include clip and viewport policy.
2. Replace frame-plan pointer special cases for axis grid and other generated adornments with role
   metadata or a narrow compatibility bridge.
3. Validate with:

```sh
just test axis
just test scene_graph
just example-c features/guide_spans --png
git diff --check
```

Recommended follow-up commit:

1. Decide whether panel background, colorbar adornments, legend, scalebar, and overlay cards should
   join the same generated-role table now or after RC1.
2. If included, migrate one group at a time and keep existing rendered behavior.
3. Add tests that assert role-derived layer, controller, coord-space, clip, viewport, depth, and
   alpha policy for each migrated group.

Recommended GSP-facing commit:

1. Re-run the GSP guide-axis probe with `--capture` only in an environment known to support Datoviz
   offscreen capture.
2. If rendered placement is proven, update GSP evidence in `../GSP_API` in that repo, not in
   Datoviz.
3. Do not claim GSP guide-query or panel-title support; the current probe still classifies those as
   unsupported/blocked.


## Stop Signs

1. Do not stage or commit the `data` submodule gitlink unless explicitly approved in the current
   turn.
2. Do not stage generated screenshots such as `feature_guide_spans.png`.
3. Do not preserve v0.3 API/ABI behavior if it conflicts with the v0.4 generated-role/snapshot
   architecture.
4. Keep the active runtime path unified:

```text
scene frame plans -> drp2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```
