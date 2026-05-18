# Scene Example Prioritization

> **Execution Status**
> - **Status:** `ANALYSIS REPORT`
> - **Updated on:** `2026-05-17`
> - **Scope:** `spec/scene/examples`, current `examples/c`, and v0.4 scene/runtime capability
>   readiness
> - **Purpose:** rank example candidates by visual payoff, first-batch feasibility, 2D/3D balance,
>   and architecture pressure.


## Summary Recommendation

The first gallery-quality batch should not be 3D-only. The best near-term mix is:

1. `PROTEIN_ARCBALL_VIEWER`
2. `SHOWCASE_WIND_FIELD`
3. `LINKED_PANELS_PROBE_COLORBAR`
4. Allen/IBL brain volume plus transparent atlas mesh
5. LiDAR or dense point cloud with EDL
6. Dense raster, streaming DAQ, or physiology signal workbench
7. Embedding explorer pair: PD12M image LOD plus Wikipedia semantic atlas

This keeps the first batch balanced across 2D, 3D, volume, and dashboard workloads while pressuring
visuals, controllers, effects, interaction, data updates, and multi-pass runtime behavior. The
embedding pair is a follow-on dashboard showcase track: it is visually and conceptually strong, but
it depends on rendered text, overlay cards, large-data policies, and preprocessing/cache
conventions.

The current code is ahead of older gap notes: `dvz_sphere`, `dvz_volume`, EDL, MSAA, SSAO, depth
peeling-shaped passes, WBOIT, volume occlusion, fly/turntable controllers, and several polished C
GLFW examples already exist. The biggest remaining blockers for polished examples are rendered
text/axes/colorbars, vector/arrow visuals, broad picking/selection, scene-level compute/custom
materials, and large-data policies.


## Recommended First Batch

| Priority | Example | Why now | Main capabilities pressured | First MVP shape |
| ---: | --- | --- | --- | --- |
| 1 | `PROTEIN_ARCBALL_VIEWER` | Best immediate eye-candy from the current stack; a C example already exists. | Sphere impostors, mesh/ribbon, SSAO, MSAA, materials, GUI, arcball. | Polish the existing protein viewer as the flagship molecular demo; defer full ball-and-stick, molecular surface, labels, and picking. |
| 2 | `SHOWCASE_WIND_FIELD` | Best 2D visual showcase candidate with high readiness and no external data requirement. | Image scalar field, primitive arrow geometry, path streamlines, panzoom, vector-glyph gap. | Synthetic climate-like speed background with arrow field and optional streamlines; use primitives now, then replace with a vector visual later. |
| 3 | `LINKED_PANELS_PROBE_COLORBAR` | Strong scientific 2D UX built directly on existing image probe and multi-panel paths. | Shared sampled fields, image probe, linked panels, colorbar, annotation, text. | Two image panels sharing one field, crosshair/probe state, and a retained colorbar; rendering can start minimal while text/colorbar work lands. |
| 4 | Allen/IBL brain volume plus transparent mesh | Domain-specific and visually rich; current examples and data paths already exist. | Volume visual, transparent mesh, WBOIT/depth peel, volume occlusion, arcball, GUI. | Volume slice or MIP with selected atlas mesh overlay and opacity controls; defer region picking and full linked 2D subplot. |
| 5 | LiDAR or dense point cloud with EDL | High visual payoff and useful pressure on dense point rendering. | Point/pixel visuals, EDL, depth cueing, fly camera, large-buffer upload/update policy. | Combine real or prepared LiDAR with EDL/depth cue and a fly camera; keep LOD as a follow-up. |
| 6 | Dense raster, streaming DAQ, or physiology workbench | Keeps first batch honest for dense 2D operational views. | Pixel/path visuals, partial updates, ring-buffer behavior, linked x panzoom, overlays, axes/text. | Pick one synthetic-data example and focus on sustained updates plus clear 2D interaction. |
| 7 | Embedding explorer pair | AI-era showcase pair with one image/thumbnails path and one semantic/text path. | Image LOD, dense points, text labels, picking, selected cards, query/search, preprocessing bundles. | Start with PD12M mean-color image positions and Wikivecs colored points; defer thumbnail LOD and semantic query sidecar. |


## Broader Priority Order

| Priority | Example family | Readiness | Architecture pressure | Notes |
| ---: | --- | --- | --- | --- |
| 1 | Protein arcball viewer | High | Multi-pass 3D scene, SSAO, material controls, resource reuse. | Highest immediate visual payoff. |
| 2 | Global wind field | High | First-class vector semantics, projection transforms, 2D scene polish. | Best 2D showcase bet. |
| 3 | Linked probe/colorbar panels | Medium-high | Shared resources, probe routing, retained explanatory objects. | Exposes text/colorbar gap quickly. |
| 4 | Allen/IBL brain volume and mesh | Medium-high | Volume plus transparency plus occlusion. | Strong scientific identity; current examples exist. |
| 5 | LiDAR / dense point cloud EDL | Medium-high | Large point data, EDL, fly camera, performance. | Good benchmark-style demo. |
| 6 | Sphere SSAO cloud | Very high | Technique quality, sphere depth/normals, MSAA/SSAO composition. | Beautiful but less domain-specific. |
| 7 | Dense raster / DAQ / physiology | Medium | 2D streaming, partial updates, linked views, axes. | Important counterweight to 3D demos. |
| 8 | PD12M image embedding LOD | Medium | Image sprite LOD, large sampled fields, thumbnail packing, panzoom, picking. | Use only PD12M as the public dataset and keep all embedding/reduction work in preprocessing. |
| 9 | Wikipedia semantic embedding atlas | Medium | Dense points, label LOD, search/query, selected cards, overlay layout. | Use Wikivecs map coordinates first; add N-dimensional vectors and query sidecar later. |
| 10 | Toy DICOM / volume clipping | Medium | 3D sampled field, slices, crosshair, window/level, 4-panel layout. | Needs better slice semantics and colorbar/text. |
| 11 | Large labels segmentation | Medium-low | Integer label textures, random label colors, selection. | CPU RGBA fallback is possible but undersells the goal. |
| 12 | Earth / terrain / Mars flyover | Medium-low | Textured mesh/sphere, cubemap/skybox, asset cache, camera path. | Great gallery material after material textures land. |
| 13 | Gray-Scott / Mandelbrot / particles | Low for scene-first | Scene-level compute, custom shaders, ping-pong resources. | Very high eye-candy, but should not be forced through ad hoc DRP-only paths. |
| 14 | Tractography / tokamak / HEP | Low-medium | Ragged 3D paths, tubes, vector fields, transparency, picking. | Excellent later architecture-pressure demos. |


## Capability Matrix

| Capability | Current state | Examples unblocked or pressured | Next core work |
| --- | --- | --- | --- |
| Figures, panels, viewport/scissor, resize | Implemented. | All examples. | Keep multi-panel resize and effect composition covered by tests. |
| Offscreen and GLFW app runtime | Implemented. | First-batch screenshots, live demos, DVZR capture/replay. | Add repeatable gallery harness conventions. |
| GUI integration | Implemented in examples. | Protein, volume, LiDAR, transparent mesh, brain. | Keep GUI as example/runtime infrastructure, not scene core. |
| Point visual | Implemented. | LiDAR, dense raster fallback, galaxy, particles render fallback. | Validate very large counts and stable partial updates. |
| Pixel visual | Implemented. | Dense raster, spike plots, large point-like 2D data. | Stress test performance at v0.3 showcase scale. |
| Marker visual | Missing as a public first-class family. | Marker picking, polished scatter, selected events. | Add marker shapes, edge color/width, item identity, marker pick path. |
| Primitive visual | Implemented. | Wind arrows, bars, overlays, simple geometry. | Avoid overusing it as a permanent substitute for semantic visuals. |
| Mesh visual | Implemented. | Protein ribbon, brain atlas mesh, terrain, finite-element viewer. | Add mesh face/region picking and textured material slots. |
| Path visual | Partial: line/strip path exists. | Wind streamlines, DAQ traces, physiology, tractography fallback. | Add ragged/grouped paths, per-path identity, joins, width, and later tubes/ribbons. |
| Image visual | Implemented. | Wind scalar background, image probe, segmentation underlay, heatmaps. | Add direct GPU colormap/label paths and tiled/LOD image policy. |
| Sampled fields | Implemented for 2D image and 3D volume consumers. | Probe panels, texture update, volume, DICOM. | Broaden non-image consumers and strengthen 3D layout/update coverage. |
| Sphere visual | Implemented. | Protein atoms, sphere SSAO, crystal lattice. | Tune SSAO/material quality; add textured/equirectangular sphere later. |
| Volume visual | Implemented first slice. | Volume slice/offscreen, DICOM, Allen brain, volume clipping. | Add arbitrary slice plane semantics, richer transfer functions, and volume probes. |
| Vector glyphs/arrows | Missing as a semantic visual. | Wind, CFD, trajectories, cell motion, tokamak. | Implement a vector/arrow visual instead of relying on primitive triangles. |
| Text/glyph rendering | Bookkeeping only. | Axes, annotations, labels, colorbars, LaTeX/math, dashboards. | Highest-value missing capability for 2D and gallery polish. |
| Axes/ticks | Missing. | Path axes, linked panels, DAQ, physiology, market dashboard. | Build axis objects on top of text and panzoom domain state. |
| Colorbars/annotations/readouts | Retained bookkeeping exists; rendering incomplete. | Probe/colorbar panels, image probe readout, volume/DICOM, dashboards. | Render colorbar ramp, ticks, labels, anchored annotations, and pinned readouts. |
| Panzoom | Implemented. | Wind, probe panels, DAQ, physiology, market, image viewers. | Add shared x/y controller semantics and linked crosshair/probe state. |
| Arcball/camera/fly/turntable | Implemented. | Protein, brain, volume, LiDAR, terrain, sphere SSAO. | Add camera-path animation helpers for cinematic examples. |
| Materials/lighting | Partial but active. | Protein, sphere cloud, mesh, terrain, finite-element viewer. | Clarify material API and add texture slots for mesh/sphere paths. |
| EDL | Implemented. | LiDAR, dense point clouds. | Validate composition with large data and other panel techniques. |
| SSAO | Implemented. | Protein, sphere cloud, mesh demos. | Continue quality tuning and composition tests. |
| MSAA | Implemented. | Sphere edges, polished 3D. | Keep fallback/capability behavior explicit. |
| WBOIT/depth peeling | Implemented first slices. | Brain mesh, transparent mesh, protein surfaces. | Harden composition and consider full dual depth peeling when needed. |
| Volume occlusion | Implemented first slice. | Allen brain, embedded overlays inside volume. | Generalize beyond one narrow volume-occluder lane. |
| Picking/probing | Point picking and image probing are GPU-backed. | Marker picking, linked probe, brain, mesh selection. | Add marker, mesh face/region, label, path, and volume pick/probe payloads. |
| Selection/linking | Bookkeeping exists. | Mesh selection, brain region highlight, dashboards. | Propagate resolved selection into visual styling and linked panels. |
| FramePlan graph | Internal multi-pass shapes exist. | WBOIT, depth peel, EDL, SSAO, volume, future compute. | Expose scene-level virtual resources and pass dependencies. |
| Compute/custom shaders | DRP2-level only. | Gray-Scott, Mandelbrot, particles, custom postprocess. | Promote compute/custom material resources into scene semantics. |
| Large-data/LOD/ring buffers | Partial. | LiDAR, raster, DAQ, market, image embedding. | Add explicit visible-range, ring-buffer, sparse-update, and LOD policies. |


## Core Feature Priority

1. **Rendered text, axes, ticks, annotations, and colorbars.**
   This unlocks nearly every polished 2D example and improves 3D examples through labels, legends,
   readouts, and scale displays.

2. **First-class vector/arrow visual.**
   `SHOWCASE_WIND_FIELD` is the best near-term 2D showcase and should not permanently rely on
   manually built primitive triangles.

3. **Large point/pixel and partial-update policy.**
   LiDAR, dense raster, spatial omics, DAQ, and particle rendering all need confidence in buffer
   reuse, range updates, and high item counts.

4. **Selection and picking beyond points/images.**
   Marker, mesh region, label, path, and volume picking are the bridge from screenshots to real
   tools.

5. **Scene-level custom material and compute framegraph.**
   This should follow the first gallery batch. It unlocks Gray-Scott, Mandelbrot, particles, and
   richer postprocess work without bypassing scene semantics.

6. **Textured mesh/sphere/cubemap asset lane.**
   Earth, Grand Canyon, Mars, terrain, and richer molecular surface examples need texture-aware
   materials plus a practical cache/download policy.


## Notes On Example Staging

- `PROTEIN_ARCBALL_VIEWER` should remain the flagship current-stack 3D demo. It pressures real
  multi-pass rendering without requiring custom shaders or compute first.
- `SHOWCASE_WIND_FIELD` should be promoted early because it is 2D, visually rich, deterministic,
  and gives a concrete target for vector visuals.
- `LINKED_PANELS_PROBE_COLORBAR` should be used as the first explanatory-object pressure test:
  colorbar, annotation, text, and probe state all meet there.
- Allen/IBL brain work is a better near-term volume/transparency story than generic volume clipping
  because the current repository already has matching examples and data conventions.
- Gray-Scott, Mandelbrot, and particles are worth keeping high in the ambition stack, but they
  should wait for scene-level compute/custom resources rather than growing a parallel DRP-only
  example contract.
