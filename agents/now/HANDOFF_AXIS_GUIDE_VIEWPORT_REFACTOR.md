# Axis, Guide, Viewport Refactor Handoff

Status: completed architecture refactor record; use as guardrails before related changes.

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
   - Added `src/scene/core/generated_visual_policy.h`.
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

6. `1fae2aa27` `Store generated visual roles on attachments`
   - Moved generated visual role metadata onto `DvzPanelAttach`.
   - Switched frame-plan clip/viewport routing for migrated generated visuals to read attachment
     role policy instead of scene object pointers.

7. `f2eb157ed` `Route generated visuals by attachment roles`
   - Migrated panel backgrounds/borders, colorbar visuals, legend visuals, scale bars, overlay
     cards, and bounds overlays onto generated attachment roles.
   - Propagated generated text roles to lowered glyph visuals.
   - Removed axis/colorbar/legend pointer scans from `scene_emit/panel.c`.
   - Updated the durable plot viewport contract to forbid pointer-derived generated routing.

8. `864f726cc` `Move generated visual policy to core`
   - Moved generated visual policy from `annotation/` to `src/scene/core/`.
   - Updated annotation, interaction, emit, visual, and test includes to use the neutral scene-level
     policy home.

9. `d08c75b0a` `Add explicit visual attach rect policy`
   - Added public `DvzVisualAttachDesc.clip_rect` and `.viewport_rect` selectors.
   - Stored explicit clip/viewport routing on `DvzPanelAttach` and propagated it through normal
     visuals, composites, generated-role helpers, and lowered glyph visuals.
   - Added a frame-plan metadata regression test for explicit rect routing.
   - Added an architecture source guard that rejects generated/adornment pointer scans in
     `scene_emit/panel.c`.


## Current Invariants

Generated visual ordering is semantic, not example-driven:

```text
guide fill < axis grid < default data < guide line/outline < axis marks < axis text
```

Generated/adornment visuals should not introduce new hard-coded local `z_layer`, `coord_space`,
`controller_mode`, clip, viewport, depth, or alpha defaults. Extend generated role policy instead
and attach through `_scene_panel_add_generated_visual()`.

Frame-plan clip/viewport routing for generated visuals is attachment metadata, not pointer identity.
Do not reintroduce emit-time scans over axis, guide, colorbar, legend, panel chrome, scale-bar,
overlay, or bounds-overlay object fields. Text visuals lowered to glyphs must inherit the generated
role from their source text attachment.

For ordinary visuals, `DvzVisualAttachDesc.clip_rect` and `.viewport_rect` are the public escape
hatch when the default AUTO routing is too implicit. Use generated roles for semantic adornments;
use explicit attach rects for custom overlays, composites, or helpers that should remain ordinary
visual attachments.

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

After `1fae2aa27` and `f2eb157ed`:

```sh
cmake --build build --target dvztest
direnv exec . just test axis
direnv exec . just test interaction
direnv exec . just test fields
direnv exec . just test scene/scene-graph
git diff --check
```

The original failing WGSL fixtures are covered by `scene/scene-graph` and pass.

After `d08c75b0a`:

```sh
cmake --build build --target dvztest
python3 -m pytest -q testing/test_scene_architecture_source_guard.py
direnv exec . just test scene/scene-graph
direnv exec . just test axis
direnv exec . just test interaction
direnv exec . just test fields
python3 tasks/spec_check.py
git diff --check
```

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

Datoviz-side architecture work in this lane is complete for the current release blocker. Remaining
direct `dvz_panel_add_visual()` call sites were audited: public/default data visuals, plot
bars/bands, view-projected helpers, and generic text glyph sync remain intentional. If any future
generated adornment is added, give it an explicit `DvzGeneratedVisualRole` before touching
frame-plan routing, or use explicit `DvzVisualAttachDesc` rects when it is not a semantic generated
role.

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
