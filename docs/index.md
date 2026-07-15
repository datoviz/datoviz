# Datoviz — High-performance 2D and 3D scientific visualization

**Datoviz is an open-source, cross-platform visualization engine written in C, with direct Python
bindings designed for NumPy arrays.** Powered by Vulkan, it targets large, dynamic GPU-resident
datasets on Linux, macOS, and Windows, with high-quality 2D and 3D rendering as equal priorities.

Datoviz is desktop-first and deliberately more explicit than a Matplotlib-like plotting library.
With its retained scene API, you create figures and visuals once, attach arrays, then update only
what changes. Applications keep direct control over interaction, layout, GPU resources, and
reproducible output. Built-in GUI support uses
[Dear ImGui](https://github.com/ocornut/imgui); an experimental WebGPU/WASM path brings a growing
subset of the same scenes to the browser. See the current
[browser subset](reference/webgpu-subset.md) and its limits.

<div class="dvz-home-facts">
<span>MIT open source</span>
<span>C + Python/NumPy</span>
<span>Vulkan</span>
<span>Linux · macOS · Windows</span>
<span>First-class 2D + 3D</span>
<span>Dear ImGui</span>
</div>

<div class="dvz-home-actions">
<a class="md-button md-button--primary" href="start/install/">Install</a>
<a class="md-button" href="start/quickstart/">Python quickstart</a>
<a class="md-button" href="examples/">Browse examples</a>
</div>

Prebuilt wheels containing the Python binding and native runtime have been validated for supported
Linux, macOS, and Windows targets. They will provide the one-command `pip install datoviz` path when
the first public release candidate is published. Until then, follow the exact current instructions
on the [installation page](start/install.md); see [platform support](reference/platform-support.md)
for graphics and architecture requirements.

!!! info "v0.4 release status"

    The native Vulkan scene API is the primary supported v0.4 path, with feature-specific status
    recorded in [Feature status](reference/feature-status.md). Browser WebGPU and facilities marked
    advanced/unstable remain experimental.

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video" style="margin:1.5rem 0 1.5rem;">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_protein/" aria-label="Protein visualization"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" alt="Interactive protein visualization rendered with Datoviz" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" aria-label="Protein visualization preview">
    <source data-src="assets/gallery/v0.4/showcases/showcases_protein.mp4" type="video/mp4">
  </video>
</div>


## Choose your path

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/quickstart/">
<strong>Python + NumPy</strong>
<span>Render 10,000 interactive points and learn the retained scene workflow.</span>
</a>
<a class="dvz-nav-card" href="start/first-c-program/">
<strong>C or C++</strong>
<span>Build a native first program, then integrate the installed C library with CMake.</span>
</a>
<a class="dvz-nav-card" href="reference/webgpu-subset/">
<strong>Browser WebGPU</strong>
<span>Try promoted live examples and understand the experimental browser subset.</span>
</a>
<a class="dvz-nav-card" href="advanced/runtime-internals/">
<strong>Engine internals</strong>
<span>Explore DRP2, Vulkan execution, recording, replay, and backend portability.</span>
</a>
</div>


## One engine, three layers

Datoviz is modular from the rendering core upward. Most applications use the retained scene layer;
specialized integrations can work closer to the protocol and runtime.

<div class="dvz-layer-grid">
<div class="dvz-layer-card">
<strong><a href="start/what-is-datoviz/">Scene</a></strong>
<span>Retained figures, panels, visuals, controllers, axes, annotations, queries, and frame planning.</span>
</div>
<div class="dvz-layer-arrow" aria-hidden="true">→</div>
<div class="dvz-layer-card">
<strong><a href="advanced/drp2-command-streams/">Datoviz Rendering Protocol v2 (DRP2)</a></strong>
<span>A backend-neutral, WebGPU-shaped contract for validation, replay, and portable command streams.</span>
</div>
<div class="dvz-layer-arrow" aria-hidden="true">→</div>
<div class="dvz-layer-card">
<strong><a href="advanced/runtime-internals/">Rendering runtimes</a></strong>
<span>The core Vulkan engine handles native rendering, windows, offscreen targets, capture, and streaming; browser WebGPU execution is experimental.</span>
</div>
</div>

Source builds expose switches for compatible core, Vulkan, canvas, DRP2, scene, app, and GUI
components. The native WebGPU build option is separate from the experimental browser WebGPU/WASM
toolchain. See [Choose your layer](start/choose-your-layer.md) for API guidance and
[Build options](reference/build-options.md) for the module list and dependencies.


## A first Python scene

This complete example creates a scene, attaches point data to a panel, binds pan and zoom, then
opens a native window.

```python
# doctest: skip -- MkDocs expands the source include after this checker runs.
--8<-- "examples/docs/quickstart.py"
```

![10 000 randomly colored points in an interactive Datoviz window](assets/gallery/v0.4/start/start_scatter.webp)

See the annotated [Quickstart](start/quickstart.md) or [use Datoviz from C or C++](how-to/c-integration.md).


## Built for scientific applications

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

### [Point Cloud](examples/gallery/showcases/showcases_point_cloud.md)

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

### [Brain Volume](examples/gallery/showcases/showcases_brain_volume.md)

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

### [Scientific Plotting](examples/gallery/showcases/showcases_scientific_plotting.md)

![A composed 2D scientific figure with axes, traces, bands, and annotations](assets/gallery/v0.4/showcases/showcases_scientific_plotting.webp)

First-class 2D axes, traces, uncertainty bands, annotations, and linked layouts.

</div>

<div class="card" markdown="1">

### [GUI Controls](examples/gallery/features/features_gui_controls.md)

![Dear ImGui controls updating a Datoviz point visual](assets/gallery/v0.4/features/features_gui_controls.webp)

Native Dear ImGui controls connected to retained visual data and application state.

</div>

</div>


## Go further

<div class="dvz-nav-grid">
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
<a class="dvz-nav-card" href="https://github.com/datoviz/datoviz">
<strong>Source and contributions</strong>
<span>Read the MIT-licensed source, report issues, and follow development on GitHub.</span>
</a>
</div>

For coding-assistant guidance, see the [AI-assisted workflow](start/ai-workflow.md). Project
acknowledgements are recorded in the [repository credits](https://github.com/datoviz/datoviz#license-and-credits).
