> **Execution Status**
> - **Status:** `LONG-TERM VISION`
> - **Updated on:** `2026-05-27`
> - **Purpose:** capture ambitious future directions for Datoviz without freezing APIs.
> - **Scope:** renderer, runtime, scientific-visualization, scale, and ecosystem directions beyond
>   the v0.4 release slice.

# Datoviz Long-Term Ambition


## North Star

Datoviz should aim to become a GPU-native scientific visualization engine with the performance of a
modern rendering engine, the scientific depth of established visualization systems, the interactive
feel of modern plotting tools, and the portability expected from Vulkan/WebGPU-era software.

The long-term goal is not only to draw plots. It is to provide an open, modern, high-performance
visual-computing kernel that can power exploratory analysis, native applications, Python frontends,
remote renderers, publications, demonstrations, and future scientific workflows.


## Strategic Pillars

1. **Quality**: physically expressive lighting, materials, transparency, volume rendering, text, and
   color management.
2. **Scale**: out-of-core data, level of detail, streaming, sparse resources, remote rendering, and
   eventually multi-GPU or distributed paths.
3. **Interactivity**: picking, probing, selections, linked views, controllers, measurements,
   annotations, live updates, and low latency.
4. **Scientific depth**: fields, tensors, volumes, meshes, particles, graphs, trajectories,
   molecules, uncertainty, and domain resources.
5. **Portability**: Vulkan, WebGPU/WASM, headless/offscreen, native apps, Python, browser, and
   backend-neutral GSP integration.
6. **Reproducibility**: declarative scenes, deterministic fixtures, image/readback tests, capture,
   export, and visual regression tooling.
7. **Extensibility**: custom visuals, custom shaders, plugins, compute pipelines, and renderer
   extension points without compromising the core C API.


## Rendering Quality Directions

### Physically Based Rendering

Long-term Datoviz should support a material system that can represent scientific geometry with
modern real-time rendering quality:

- metallic/roughness materials, normal maps, clearcoat, transmission, anisotropy, and subsurface
  approximations when useful;
- HDR lighting, tone mapping, image-based lighting, and color-managed output;
- material presets for surfaces such as tissue, glass, fluids, rock, metal, particles, and
  molecular structures;
- compatibility with simpler Phong/matte models for portable GSP scenes.

PBR should be treated as a Datoviz renderer capability first, with GSP-level material semantics kept
portable and capability-gated.

### Ray Tracing And Path Tracing

A future ray-tracing path could provide:

- Vulkan hardware ray tracing where available;
- compute-based or offline fallback paths for selected scene types;
- hybrid raster plus ray-traced shadows, ambient occlusion, reflections, and refraction;
- progressive static-scene refinement for publication or cinematic output;
- scientific volumetric path-tracing experiments.

This should not be a v0.4/v0.5 requirement. It is a long-horizon renderer capability that should not
force the scene API to become renderer-specific.

### Advanced Volume Rendering

Volume rendering should become a flagship scientific feature area:

- GPU ray marching, transfer functions, gradient lighting, preintegration, and adaptive sampling;
- bricked/sparse volumes, empty-space skipping, progressive loading, and multiresolution pyramids;
- multi-volume compositing and volume/mesh/label overlays;
- volume picking, probing, slicing, clipping, and transfer-function interaction;
- cinematic smoke, cloud, plasma, medical, microscopy, and simulation examples.

### Transparency And Compositing

Scientific scenes often require many transparent layers. Future work should preserve room for:

- weighted blended order-independent transparency;
- depth peeling or moment/stochastic transparency where justified;
- correct mesh/image/volume compositing within a panel;
- mixed transparent surfaces, labels, slices, and postprocess effects.


## Massive-Scale Data Directions

### Out-Of-Core And Progressive Rendering

Datoviz should eventually handle data that is larger than RAM or GPU memory:

- chunked arrays, sparse textures, bricked volumes, tiled images, and point-cloud pages;
- asynchronous upload queues and explicit residency management;
- progressive refinement and partial-valid rendering;
- cache policy hooks usable from Python/GSP without turning Datoviz into a full data server.

### Level Of Detail Everywhere

LOD should become a cross-cutting capability rather than a special case:

- hierarchical points and splats;
- mesh simplification and screen-space error metrics;
- path and trajectory decimation;
- image and volume pyramids;
- adaptive glyph density for vector/tensor fields;
- temporal LOD for time-varying data.

### Remote And Distributed Visualization

Long-term remote use cases include:

- headless Datoviz server processes on workstations or HPC nodes;
- streamed raster/video output to browser, notebook, or native clients;
- server-side picking/probing and interaction requests;
- future multi-GPU or cluster rendering for very large scenes;
- explicit latency, throughput, and frame-pacing telemetry.


## Scientific Visual Computing Directions

Datoviz should keep generic visual families lean while adding semantic resources that lower to those
families. High-value long-term areas include:

- vector fields: arrows, streamlines, stream tubes, particles, LIC, probes, and animated flow;
- tensor fields: ellipsoid glyphs, eigenvectors, invariants, stress/diffusion visualization;
- unstructured grids: cells, boundary extraction, cut planes, isosurfaces, and cell picking;
- graph/network resources: topology, layout, identity, selection, edge bundling, and labels;
- trajectory and track resources: identity over time, trails, events, fading history, and playback;
- molecular resources: atoms, bonds, residues, chains, ribbons, surfaces, and density overlays;
- uncertainty and ensembles: intervals, covariance, distributions, probabilistic volumes, and
  ensemble reductions;
- segmentation and label volumes: categorical scales, label picking, region overlays, and tables.

Most of these should start as semantic resources or Python/GSP-level composites, then promote proven
generic pieces into Datoviz C.


## Compute-Integrated Visualization

GPU compute should become a practical implementation capability for visualization tasks:

- reductions, histograms, filtering, resampling, and colormap mapping;
- marching cubes, contour extraction, streamline integration, and particle advection;
- graph layout, clustering, and sampling;
- picking acceleration, selection masks, and region queries;
- compute-to-render dataflow with CPU/WebGPU fallbacks where possible.

The semantic API should not require users to know whether a CPU, Vulkan compute, WebGPU compute, or
precomputed path is selected.


## Interaction, Analysis, And Tools

Long-term Datoviz should support scientific interaction beyond camera control:

- point, pixel, mesh-face, segment, path, label, atom, cell, voxel, and volume probes;
- lasso, rectangle, frustum, nearest-neighbor, and semantic selection;
- clipping planes, slicing widgets, rulers, measurements, annotations, and readouts;
- linked panels, linked cameras, crosshairs, brushing, and dashboard-style interactions;
- deterministic request/readback paths suitable for tests and remote clients.

VisPy2 should own the Python UX for these tools, while Datoviz should provide efficient native
components and backend requests.


## Export, Reproducibility, And Publication

Datoviz should be excellent at raster, video, and reproducible renderer output:

- high-DPI PNG, transparent-background PNG, EXR/HDR when useful, and video capture;
- deterministic scene fixtures and image/readback regression tests;
- serialized frame plans or scene snapshots for conformance and debugging;
- visual debugging overlays for resources, passes, picking ids, and LOD state.

Publication-oriented PDF/SVG/vector export should remain a GSP/Matplotlib responsibility. Datoviz
can contribute raster layers and exact screenshots when a scene uses GPU-only features.


## Backend And Platform Directions

The internal architecture should keep the scene-to-runtime split backend-portable:

```text
Scene / app API
  -> frame plan
  -> DRP2 or successor protocol stream
  -> backend runtime
  -> Vulkan / WebGPU / headless / future backends
```

Likely long-term targets include Vulkan, WebGPU/WASM, headless/offscreen, remote servers, native
applications, Python bindings, and browser clients. Metal or other APIs should be considered only if
there is a clear maintenance and ecosystem reason.


## Relationship With GSP And VisPy2

Datoviz should expose its power through two routes:

1. **GSP/VisPy2 route** for portable Python scientific visualization, common visuals, notebooks,
   dashboards, backend selection, and Matplotlib publication export.
2. **Direct Datoviz route** for advanced rendering, maximum performance, native embedding,
   experimental features, custom visuals, low-level runtime control, and conformance fixtures.

The GSP core should stay portable. Datoviz-specific features should appear as capability-gated
extensions instead of forcing all renderers to imitate Datoviz internals.


## API Detail Policy For This Vision

This document intentionally avoids freezing concrete C structs, Python class names, DRP2 command
schemas, shader ABIs, or timeline commitments. Future implementation proposals should be written only
when there is enough prototype evidence to decide:

- whether the feature belongs in Datoviz C, GSP, VisPy2, Matplotlib, or an external package;
- which parts are semantic API and which parts are backend capabilities;
- what tests, fixtures, and examples would prove the feature works;
- how the feature degrades on weaker or non-interactive backends.


## Open Questions

- Which renderer extensions should be named and stabilized first for GSP discovery?
- Which high-end rendering features are important enough to justify native C API exposure?
- How much out-of-core scheduling belongs in Datoviz versus Python/GSP data loaders?
- Which compute algorithms should be built in, and which should be left to custom shaders or
  external preprocessing?
- What visual-regression infrastructure is needed before high-quality rendering features can evolve
  safely?
