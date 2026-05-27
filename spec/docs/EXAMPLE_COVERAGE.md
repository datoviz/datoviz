# Example Coverage Plan

Datoviz v0.4 documentation uses examples as executable release proof. Every public visual and
public feature should have a focused C example that demonstrates only that visual or feature and the
minimum surrounding setup needed to run it.

The aspirational gallery direction is recorded in
[`../scene/examples/EXAMPLE_NORTH_STAR.md`](../scene/examples/EXAMPLE_NORTH_STAR.md). Use that
document to choose screenshot, video, and showcase targets before reducing them to concrete release
fixtures and current implementation work.


## Example Principles

1. One public visual family gets one minimal C example.
2. One public feature gets one minimal C example.
3. A minimal example should avoid unrelated visual polish.
4. A showcase may compose many features, but it does not satisfy the minimal-example requirement.
5. Every example should be linked from the relevant reference and how-to pages.
6. Examples should have stable identifiers that can be used by docs, tests, release notes, and LLM
   retrieval.


## Suggested Source Layout

```text
examples/c/
  visuals/
  features/
  techniques/
  showcases/
  runtime/
  drp2/
```

WebGPU examples and fixtures may keep their existing browser-oriented layout under `examples/webgpu/`
while being indexed by the same documentation manifest.


## Visual Examples

Required dedicated visual examples:

| Visual | Example | Notes |
| --- | --- | --- |
| Point | `examples/c/visuals/point.c` | retained point data, color, size |
| Pixel | `examples/c/visuals/pixel.c` | image-like point grid or pixel visual semantics |
| Marker | `examples/c/visuals/marker.c` | marker shape, size, color |
| Primitive | `examples/c/visuals/primitive.c` | topology-parametric primitive |
| Segment | `examples/c/visuals/segment.c` | independent line segments |
| Path | `examples/c/visuals/path.c` | ordered path/polyline data |
| Image | `examples/c/visuals/image.c` | 2D sampled field or image texture |
| Mesh | `examples/c/visuals/mesh.c` | indexed geometry and 3D controller |
| Sphere | `examples/c/visuals/sphere.c` | impostor spheres |
| Volume | `examples/c/visuals/volume.c` | retained 3D sampled field |
| Text | `examples/c/visuals/text.c` | narrow text or text block, depending on public surface |
| Labels | `examples/c/visuals/labels.c` | integer labels field if first-class in v0.4 |
| Polygon | `examples/c/visuals/polygon.c` | only if polygon helpers are public release surface |

Adornments and scene features such as axes, colorbars, scale bars, annotations, overlays, and
controllers belong under feature examples, not visual examples, unless the public API explicitly
presents them as visual families.


## Feature Examples

Required or high-priority feature examples:

| Feature Area | Examples |
| --- | --- |
| Scene basics | `examples/c/features/scene_basic.c` |
| Figure and panel | `examples/c/features/panel_single.c` |
| Grid layout | `examples/c/features/panel_grid.c` |
| Multi-panel rendering | `examples/c/features/panel_multi.c` |
| Linked panels | `examples/c/features/panel_linked.c` |
| Retained data update | `examples/c/features/update_visual_data.c` |
| Partial data update | `examples/c/features/update_partial.c` |
| 2D sampled field | `examples/c/features/sampled_field_2d.c` |
| 3D sampled field | `examples/c/features/sampled_field_3d.c` |
| Colormap scale | `examples/c/features/colormap_scale.c` |
| Continuous colorbar | `examples/c/features/colorbar.c` |
| Categorical legend | `examples/c/features/legend_categorical.c` if supported |
| 2D axes | `examples/c/features/axes_2d.c` |
| Axis labels and ticks | `examples/c/features/axis_labels.c` |
| Scale bar | `examples/c/features/scalebar.c` |
| Text block | `examples/c/features/text_block.c` |
| Annotation label | `examples/c/features/annotation_label.c` |
| Overlay card | `examples/c/features/overlay_card.c` |
| Panzoom controller | `examples/c/features/controller_panzoom.c` |
| Arcball controller | `examples/c/features/controller_arcball.c` |
| Fly controller | `examples/c/features/controller_fly.c` |
| Turntable controller | `examples/c/features/controller_turntable.c` |
| Point picking | `examples/c/features/pick_point.c` |
| Image probing | `examples/c/features/probe_image.c` |
| Label probing | `examples/c/features/probe_labels.c` |
| Selection | `examples/c/features/selection.c` |
| Hover readout | `examples/c/features/pick_hover.c` |
| Mesh material | `examples/c/features/material_mesh.c` |
| Lighting | `examples/c/features/lighting.c` |
| Depth behavior | `examples/c/features/depth.c` |

The query, pick, probe, and selection examples should be treated as normal first-class examples once
the current API overhaul lands.


## Runtime Examples

Runtime examples document how a program is hosted or executed:

| Runtime Feature | Example |
| --- | --- |
| GLFW window | `examples/c/runtime/window_glfw.c` |
| Offscreen rendering | `examples/c/runtime/offscreen.c` |
| PNG capture | `examples/c/runtime/capture_png.c` |
| Frame callback | `examples/c/runtime/frame_callback.c` |
| Immediate or continuous rendering | `examples/c/runtime/continuous.c` |
| Hosted Qt integration | existing Qt example path, linked from docs when supported |


## Technique Examples

Technique examples may compose a small number of visuals and runtime features to demonstrate a
rendering technique:

| Technique | Example |
| --- | --- |
| Transparency | `examples/c/techniques/transparency.c` |
| Weighted blended OIT | `examples/c/techniques/wboit.c` |
| MSAA | `examples/c/techniques/msaa.c` |
| Eye-dome lighting | `examples/c/techniques/edl.c` |
| SSAO | `examples/c/techniques/ssao.c` |
| Depth cueing | existing or `examples/c/techniques/depth_cue.c` |
| Scatter with axes | existing or `examples/c/techniques/scatter_axes.c` |
| Bounds or overlay diagnostics | existing bounds/overlay examples |


## DRP2 And Portability Examples

| Area | Example |
| --- | --- |
| Raw triangle | existing raw triangle DRP2 example |
| Record DVZR | existing `record_dvzr` tool/example |
| Replay DVZR | existing `replay_dvzr` tool/example |
| WebGPU point | WebGPU fixture or runnable browser example |
| WebGPU image | WebGPU fixture or runnable browser example |
| WebGPU primitive | WebGPU fixture or runnable browser example |
| WebGPU mesh | when included in the supported experimental subset |


## Showcase Examples

Showcases are allowed to be attractive, composed, and domain-flavored. They should not be minimal
or exhaustive.

Candidate showcases:

| Showcase | Demonstrates |
| --- | --- |
| LiDAR | large point cloud, colormap, controller, performance |
| Brain image and labels | image, labels, probing, colorbar |
| Molecule or protein | mesh/sphere, material, lighting |
| Volume slice | volume, sampled field, slicing, probe |
| Multi-panel dashboard | panels, linked views, axes, colorbars |
| Mesh technique demo | mesh plus SSAO, EDL, or MSAA |
| Annotation/readout demo | picking, overlay cards, label annotations |


## Example Metadata

Every documented example should eventually have machine-readable metadata. The format may live in a
central manifest or near each example, but it should contain these fields:

```yaml
id: visual.point
title: Point visual
kind: visual
source: examples/c/visuals/point.c
status: supported
features:
  - scene
  - point
backends:
  - native
screenshot: screenshots/visuals/point.png
tests:
  - just test scene
docs:
  reference: reference/visual-families/index.md#point
  how_to: how-to/add-a-visual.md
```

This metadata should support:

1. generated example galleries;
2. feature coverage reports;
3. release checklists;
4. stable LLM retrieval;
5. checks for missing examples, screenshots, or reference pages.


## Minimal Example Rules

1. Prefer one source file per example.
2. Keep setup explicit and easy to copy.
3. Use the fewest visuals necessary.
4. Avoid hidden global state.
5. Include a short top-of-file comment describing the demonstrated visual or feature.
6. Keep validation commands in the corresponding docs or metadata, not as stale comments in code.
7. If an example requires optional runtime support, label the backend and platform constraints.
