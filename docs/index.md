# Datoviz — High-performance 2D and 3D scientific visualization

**Datoviz is an open-source, cross-platform visualization engine written in C, with direct Python
bindings designed for NumPy arrays.** Powered by Vulkan, it renders large, interactive scientific
datasets at high visual quality on Linux, macOS, and Windows. Two-dimensional and three-dimensional
visualization are both first-class use cases.

Datoviz is desktop-first and deliberately more explicit than a Matplotlib-like plotting library.
Its retained scene API gives applications direct control over visuals, data, interaction, layout,
GPU resources, and reproducible output. Built-in GUI support uses
[Dear ImGui](https://github.com/ocornut/imgui); an experimental WebGPU/WASM path brings a growing
subset of the same scenes to the browser.

<div class="dvz-home-facts">
<span>MIT open source</span>
<span>C + Python/NumPy</span>
<span>Vulkan</span>
<span>Linux · macOS · Windows</span>
<span>First-class 2D + 3D</span>
<span>Dear ImGui</span>
</div>

<div class="dvz-home-actions">
<a class="md-button md-button--primary" href="start/">Get started</a>
<a class="md-button" href="examples/">Browse examples</a>
<a class="md-button" href="https://github.com/datoviz/datoviz">View on GitHub</a>
</div>

Release packages are designed for a one-command `pip install datoviz`: wheels bundle the Python
binding and native runtime for supported Linux, macOS, and Windows targets. Datoviz v0.4 is
currently preparing its first release candidate, so use the exact command from the
[installation page](start/install.md) until a public package is announced.

!!! info "v0.4 release status"

    The native scene API is the primary release-facing path. WebGPU and selected advanced
    facilities remain experimental; check [Feature status](reference/feature-status.md) before
    adopting them.

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video" style="margin:1.5rem 0 1.5rem;">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_protein/" aria-label="Protein visualization"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" alt="Interactive protein visualization rendered with Datoviz" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" aria-label="Protein visualization preview">
    <source data-src="assets/gallery/v0.4/showcases/showcases_protein.mp4" type="video/mp4">
  </video>
</div>


## One engine, three layers

Datoviz is modular from the rendering core upward. Use the retained scene layer for normal
applications, or work closer to the protocol and runtime when integration requires it.

<div class="dvz-layer-grid">
<div class="dvz-layer-card">
<strong>Scene</strong>
<span>Retained figures, panels, visuals, controllers, axes, annotations, queries, and frame planning.</span>
</div>
<div class="dvz-layer-arrow" aria-hidden="true">→</div>
<div class="dvz-layer-card">
<strong>DRP2</strong>
<span>A backend-neutral, WebGPU-shaped rendering protocol for validation, replay, and portable command streams.</span>
</div>
<div class="dvz-layer-arrow" aria-hidden="true">→</div>
<div class="dvz-layer-card">
<strong>Rendering runtimes</strong>
<span>The core Vulkan engine handles native rendering, windows, offscreen targets, capture, and streaming; WebGPU execution is experimental.</span>
</div>
</div>

The CMake build exposes separate switches for the core, Vulkan runtime, canvas, DRP2, scene, app,
GUI, and WebGPU components, so native applications can enable only the layers they need. See
[Choose your layer](start/choose-your-layer.md) for API guidance and
[Build options](reference/build-options.md) for the complete module list.


## Start here

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/install/">
<strong>Install</strong>
<span>Choose the right setup path for Python, C/C++, macOS, Linux, or Windows.</span>
</a>
<a class="dvz-nav-card" href="start/quickstart/">
<strong>Quickstart</strong>
<span>Render 10,000 interactive points and learn the core scene workflow.</span>
</a>
<a class="dvz-nav-card" href="examples/">
<strong>Browse examples</strong>
<span>Explore working visuals, features, runtime examples, and scientific showcases.</span>
</a>
<a class="dvz-nav-card" href="how-to/c-integration/">
<strong>Use from C or C++</strong>
<span>Build native applications against the installed Datoviz library.</span>
</a>
</div>


## A complete first example

Create a scene, attach point data to a panel, bind pan and zoom, then open the window.

```python
# doctest: skip -- MkDocs expands the source include after this checker runs.
--8<-- "examples/docs/quickstart.py"
```

![10 000 randomly colored points in an interactive Datoviz window](assets/gallery/v0.4/start/start_scatter.webp)

See the annotated [Quickstart](start/quickstart.md) or [use Datoviz from C or C++](how-to/c-integration.md).


## Why Datoviz?

- **Large, dynamic data**: dense points, sampled fields, images, meshes, volumes, text, and
  scientific annotations remain interactive through GPU-backed rendering.
- **Application-ready output**: render in native windows or offscreen, embed the C library, capture
  screenshots, export video, and record or replay render streams.
- **Interaction and composition**: combine linked panels, cameras, 2D and 3D controllers, picking,
  queries, axes, colorbars, labels, and Dear ImGui controls.
- **Direct APIs**: use the native C API from C or C++, or call the generated Python binding directly
  with documented NumPy array adaptation.

Datoviz is part of the [VisPy](https://vispy.org/) ecosystem and is the flagship interactive GPU
backend for the developing VisPy 2 and
[Graphics Server Protocol](https://github.com/vispy/GSP_API) architecture. That project owns the
higher-level plotting layer; Datoviz v0.4 remains the explicit rendering engine underneath it.


## Gallery highlights

<div class="grid cards" markdown="1">

<div class="card" markdown="1">

**[Point Cloud](examples/gallery/showcases/showcases_point_cloud.md)**

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_point_cloud.md" aria-label="Point Cloud"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp" alt="Dense point cloud rendered in Datoviz" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp" aria-label="Point Cloud preview">
    <source data-src="assets/gallery/v0.4/showcases/showcases_point_cloud.mp4" type="video/mp4">
  </video>
</div>

Large 3D datasets with depth, color, and interactive navigation.

</div>

<div class="card" markdown="1">

**[Brain Volume](examples/gallery/showcases/showcases_brain_volume.md)**

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_brain_volume.md" aria-label="Brain Volume"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_brain_volume.poster.webp" alt="Brain volume rendering with mesh overlay" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="assets/gallery/v0.4/showcases/showcases_brain_volume.poster.webp" aria-label="Brain Volume preview">
    <source data-src="assets/gallery/v0.4/showcases/showcases_brain_volume.mp4" type="video/mp4">
  </video>
</div>

Volumes, slices, transparent geometry, and scientific context.

</div>

<div class="card" markdown="1">

**[Wind Field](examples/gallery/showcases/showcases_wind_field.md)**

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_wind_field.md" aria-label="Wind Field"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_wind_field.poster.webp" alt="Wind field visualization with color mapped data" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="assets/gallery/v0.4/showcases/showcases_wind_field.poster.webp" aria-label="Wind Field preview">
    <source data-src="assets/gallery/v0.4/showcases/showcases_wind_field.mp4" type="video/mp4">
  </video>
</div>

Scalar fields, vector overlays, colorbars, and probe-style workflows.

</div>

<div class="card" markdown="1">

**[Choropleth](examples/gallery/showcases/showcases_choropleth.md)**

[![Choropleth map rendered with Datoviz](assets/gallery/v0.4/showcases/showcases_choropleth.webp)](examples/gallery/showcases/showcases_choropleth.md)

Retained composites, labels, color scales, and responsive layouts.

</div>

</div>


## Go further

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/choose-your-layer/">
<strong>Choose your layer</strong>
<span>Compare Python, C/C++, WebGPU, exact raw calls, and higher-level plotting tools.</span>
</a>
<a class="dvz-nav-card" href="how-to/">
<strong>How-To guides</strong>
<span>Learn focused tasks such as axes, colorbars, picking, animation, capture, and offscreen output.</span>
</a>
<a class="dvz-nav-card" href="reference/">
<strong>Reference</strong>
<span>Look up visual families, attributes, C API pages, platform support, and project status.</span>
</a>
<a class="dvz-nav-card" href="reference/project-status/">
<strong>Project status</strong>
<span>See release maturity, supported paths, experimental features, and current limitations.</span>
</a>
</div>

For coding-assistant guidance, see the [AI-assisted workflow](start/ai-workflow.md). Project
acknowledgements are recorded in the [repository credits](https://github.com/datoviz/datoviz#license-and-credits).
