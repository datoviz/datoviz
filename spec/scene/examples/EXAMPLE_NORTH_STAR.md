# Datoviz Example North Star

> **Status:** Aspirational direction
> **Updated on:** 2026-05-27
> **Scope:** public v0.4 documentation examples, gallery screenshots, and showcase videos
> **Purpose:** define the user-facing example ambition before filtering by current implementation
> status, release staging, or existing example inventory.

This document records the ideal example direction from the user's point of view. It is deliberately
not a release checklist and should not be read as proof that a feature already exists. Use it before
release staging, gap analysis, and implementation planning to keep the documentation target
ambitious.

The v0.4 gallery should make one argument quickly:

**Datoviz is a fast, modern, scientific GPU visualization engine for dense data, interactive
scenes, native applications, and portable rendering backends.**

Examples should not feel like API fragments or Vulkan demos. They should feel like credible
scientific scenes first, with code and reference links attached afterward.


## Gallery Principles

1. Lead with visual impact, then show the smallest useful code.
2. Use coherent scientific data or scientific-looking synthetic data, not random placeholder data.
3. Make scale, interactivity, and GPU rendering quality visible in screenshots and short videos.
4. Keep minimal examples as executable truth, but let showcases be composed and editorial.
5. Treat screenshots and videos as first-class documentation artifacts.
6. Separate aspiration from release promises; stage the final commitments elsewhere.


## Hero Showcases

These are the examples that should sell Datoviz in the first viewport of the public docs and in
release announcements.

| Showcase | Visual Story | Capabilities Communicated |
|---|---|---|
| Dense LiDAR Flythrough | Millions of colored 3D points, depth cues, fly camera, camera-path video. | Large data, interactive 3D navigation, point-cloud readability, performance confidence. |
| Interactive Brain Atlas | Translucent brain mesh, volume slice, region labels, linked 2D/3D panels. | Neuro/scientific scene composition, volumes, labels, transparency, probing. |
| Molecular Viewer | Atoms as shaded spheres, bonds as segments/cylinders, arcball, labels, selection. | High-quality scientific 3D, lighting/materials, sphere impostors, interaction. |
| Spatial Omics Explorer | Dense cells/spots, categorical clusters, microscopy underlay, linked selection. | Modern bio workflows, dense points, labels, linked views, hover/readout UI. |
| Weather Field Dashboard | Scalar field, streamlines, arrows, time animation, colorbar, linked panels. | 2D fields, vectors, animation, dashboard composition. |
| Volume Workbench | MRI/CT/simulation volume, slices, clipping, transfer controls, value probe. | 3D sampled fields, volume inspection, capture/video, probe workflows. |
| Large Time-Series Workbench | Many traces, linked x panzoom, annotations, sustained updates. | Dense 2D rendering, streaming updates, axes, operational analysis. |
| Scientific Multi-Panel Figure | Scatter, image, mesh, path, and time-series panels with shared selection. | Figure composition, linked panels, mixed visual families, explanatory objects. |
| Terrain Or Planet Flyover | Shaded surface, scalar coloring or texture, scale bar, lighting, camera path. | Cinematic 3D, surface meshes, camera animation, public-demo appeal. |
| Browser Preview | Point, image, primitive, and basic mesh running through a WebGPU/WASM subset. | Portability direction and shared scene semantics beyond native Vulkan. |


## Visual Family Gallery

Every public visual family should have a small, attractive, focused example. The goal is direct
discoverability: users should immediately understand when to use each visual.

| Visual Family | Ideal Example Theme |
|---|---|
| Point | Dense embedding, star field, particles, or LiDAR slice. |
| Pixel | Detector image, occupancy map, dense raster, or heatmap. |
| Marker | Categorical scatter with size, shape, hover, and selected items. |
| Primitive | Semantic glyphs, bars, quads, discs, or simple geometric marks. |
| Segment | Graph edges, measurement lines, vector stems, or error bars. |
| Path | Trajectories, streamlines, traces, GPS tracks, or neural paths. |
| Image | Microscopy, satellite tile, simulation scalar field, or camera frame. |
| Mesh | Shaded scientific surface with scalar attributes. |
| Sphere | Molecules, particles, stars, or atomistic simulation. |
| Volume | 3D scalar field with slicing, transfer, or clipping. |
| Text | Labels, axis titles, annotations, and readout text. |
| Labels | Segmentation mask, atlas regions, or categorical field. |
| Polygon | Regions of interest, filled contours, map regions, or shapes. |
| Vector/Arrow | Wind, flow, gradients, or cell motion, as a semantic direction visual. |


## Feature Gallery

Feature examples should show composition and workflow behavior, not just isolated API calls.

| Feature Area | Ideal User-Facing Example |
|---|---|
| Axes and ticks | Clean 2D scientific plot with panzoom and readable tick labels. |
| Colorbars | Image, mesh, or volume with continuous scalar scale. |
| Categorical legends | Segmented regions or clustered points with stable color identity. |
| Annotations | Pinned labels, callouts, measurement marks, and selected-item notes. |
| Scale bars | Microscopy/image scale bar and 3D scene scale indicator. |
| Linked panels | Click or probe one panel and update another panel. |
| Picking | Select point, marker, mesh item, region, or image pixel. |
| Probing | Hover over image, volume, or labels and show data value. |
| Selection | Highlight chosen data while dimming or de-emphasizing the rest. |
| Streaming updates | Live signal, evolving image, or changing point cloud. |
| Animation | Time-varying field, orbit camera, moving particles, or slice sweep. |
| Capture | Reproducible screenshot and video generation. |
| Layout | Grid, side-by-side comparison, inset panel, and reserved legend space. |
| Controllers | Panzoom, arcball, fly, and turntable demonstrated on memorable scenes. |


## Rendering Technique Gallery

Rendering-technique examples should use before/after or side-by-side comparisons whenever possible.
The feature should be visible without reading the caption.

| Technique | Best Demonstration |
|---|---|
| Eye-dome lighting | Dense point cloud readability before and after EDL. |
| SSAO | Mesh, molecule, or sphere cloud with stronger depth perception. |
| MSAA | Thin lines, mesh edges, and text before and after antialiasing. |
| Transparency | Overlapping anatomical regions, surfaces, or particles. |
| OIT/depth peeling | Correct translucent layering in a busy 3D scene. |
| Depth cueing | 3D scatter or path scene with distance readability. |
| Lighting/materials | Flat, lit, and polished material variants of the same mesh. |
| Clipping | Volume or mesh cutaway revealing internal structure. |
| Colormap choice | Same scalar field rendered with appropriate sequential/diverging maps. |


## Runtime And Integration Gallery

Runtime examples should answer whether Datoviz fits real software, not just whether it can draw.

1. Native interactive window.
2. Offscreen renderer.
3. PNG screenshot capture.
4. Video capture or deterministic animation export.
5. Frame callback and sustained update loop.
6. Embedding in another UI toolkit.
7. Headless batch rendering.
8. Command-stream recording and replay.
9. Minimal raw Python binding smoke test.
10. Experimental browser/WebGPU page.


## Tutorial Spine

Tutorials should be few, polished, and complete. They should teach workflows, while the galleries
provide breadth.

1. **First Scene:** colored points with axes and panzoom.
2. **Interactive 3D:** shaded mesh or molecule with arcball control.
3. **Image With Colorbar And Probe:** image, colormap, colorbar, and hover value.
4. **Multi-Panel Figure:** linked scatter, image, and path panels.
5. **Offscreen Capture:** reproducible screenshot and video generation.
6. **Large Data:** dense point cloud or raster with performance-oriented update path.


## How To Use This Document

This document is upstream of release planning. Use it in this order:

1. Pick the desired user impression and screenshot/video target here.
2. Classify the concrete release promise in
   [EXAMPLE_RELEASE_STAGING.md](EXAMPLE_RELEASE_STAGING.md).
3. Map the smallest executable proof in
   [FEATURE_FIXTURE_MATRIX.md](FEATURE_FIXTURE_MATRIX.md).
4. Apply gallery style choices from
   [GALLERY_VISUAL_IDENTITY.md](GALLERY_VISUAL_IDENTITY.md).
5. Record implementation gaps in
   [EXAMPLE_GAP_REPORT.md](EXAMPLE_GAP_REPORT.md).

When current capabilities fall short, prefer a reduced version of the same visual story over a
technically complete but visually forgettable replacement.
