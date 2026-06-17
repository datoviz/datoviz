# How-To Documentation Writing Plan

Status: active v0.4 rewrite plan.

## Goal

Rewrite `docs/how-to/` as the task and decision layer for Datoviz v0.4. The Examples section is
the executable source of truth; How-To pages explain how to adapt canonical examples to real user
tasks.

Do not write long standalone programs in How-To pages. Link to generated gallery pages and
canonical `examples/c/...` sources instead.


## Page Contract

Every How-To page should use this shape unless the task clearly needs a shorter variant:

```markdown
# Task title

What this solves.

## Use this when

Short decision guidance.

## Minimal sequence

3-8 ordered calls or steps. Use small snippets only for the calls being explained.

## Canonical examples

Links to generated gallery detail pages and source files.

## Important details

Ownership, lifetime, coordinate, async, backend, or validation caveats.

## Common mistakes

Short bullets.

## See also

Related examples and reference pages.
```

Rules:

1. Stay task-focused.
2. Link to minimal examples and generated gallery pages instead of duplicating full source.
3. Include validation commands where relevant.
4. Call out ownership, lifetime, coordinate, async, and backend limitations.
5. Reuse generated gallery screenshots when the task has a visual result.
6. Do not add `<!-- TODO: Python -->` markers.
7. Do not expose old v0.3 Pythonic APIs as current v0.4 APIs.


## Final How-To Navigation

```text
Core Workflow
  create-a-scene.md
  create-a-window.md
  render-offscreen.md
  add-a-visual.md
  update-visual-data.md
  animation.md

Data To Visuals
  choose-a-visual-family.md
  coordinate-systems.md
  transforms-and-scales.md
  use-colormaps.md
  use-sampled-fields.md

Layout
  create-multiple-panels.md
  link-panels.md
  axes.md
  adornments.md
  add-annotations.md

Interaction
  use-panzoom.md
  3d-navigation.md
  input-events.md
  pick-and-probe.md
  probe-fields.md
  select-items.md

Rendering
  configure-cameras.md
  lighting-and-materials.md
  rendering-techniques.md
  profile-performance.md

Output
  capture-an-image.md
  video-export.md
  replay-dvzr.md

Integration
  c-integration.md
  use-python.md
  use-raw-ctypes.md
  embed-in-qt.md
  deploy-to-web.md

Diagnostics
  debug-rendering.md
  debug-webgpu.md
  diagnose-platform.md
```


## Canonical Example Mapping

Use `examples/c/MANIFEST.yaml` and generated `docs/examples/gallery/` pages as source of truth.
Common mappings:

| How-To | Canonical examples |
| --- | --- |
| create scene | `features/basic_scene.c`, `features/panel_single.c` |
| window/app | `features/app_glfw.c`, `start/scatter.c` |
| offscreen/capture | `features/offscreen_capture.c` |
| add visual | `visuals/point.c`, `visuals/marker.c`, visual family examples |
| update data | `features/update_visual_data.c`, `features/update_partial.c`, `features/visibility.c` |
| panels/layout | `features/panel_grid.c`, `features/panel_multi.c`, `features/panel_linked.c` |
| axes/adornments | `features/axes_2d.c`, `features/axis_labels.c`, `features/colorbar.c`, `features/scalebar.c`, `features/legend_categorical.c` |
| panzoom/controllers | `features/panzoom.c`, `features/controller_arcball.c`, `features/controller_turntable.c`, `features/controller_fly.c`, `features/controller_orbit_camera.c` |
| input/pick/probe/select | `features/input_events.c`, `features/picking.c`, `features/image_probe.c`, `features/selection_pixel.c`, `features/selection_sphere.c`, `features/selection_mesh_instances.c` |
| rendering | `features/lighting.c`, `features/material_mesh.c`, `features/technique_*.c`, `features/alpha_blending.c` |
| animation/video | `features/timer_animation.c`, `features/animation_tracks.c`, `features/video_export.c` |
| diagnostics/replay | `features/record_replay.c`, `features/json_export.c`, `reference/webgpu-subset.md` |


## Validation

Documentation-only:

```sh
git diff --check
python -m mkdocs build --strict
```

If `mkdocs` is unavailable, run `git diff --check` and inspect `git status --short`.
