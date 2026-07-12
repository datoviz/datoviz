# Datoviz

**Datoviz is a GPU-powered visualization engine for scientific data.** It is built for
interactive 2D and 3D scenes that outgrow ordinary plotting: dense points, images, meshes, volumes,
text, annotations, linked panels, controllers, capture, and replayable render streams.

Datoviz v0.4 is the **engine layer**: use it directly when you want explicit control over scene
objects, GPU-backed rendering, native integration, and reproducible visual output.

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video" style="margin:1.5rem 0 1.5rem;">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_protein/" aria-label="Protein visualization"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" alt="Interactive protein visualization rendered with Datoviz" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" aria-label="Protein visualization preview">
    <source data-src="assets/gallery/v0.4/showcases/showcases_protein.mp4" type="video/mp4">
  </video>
</div>


## Why Datoviz?

- ⚡ **Large interactive scenes**: explore point clouds, sampled fields, meshes, volumes, and
  scientific annotations with GPU-backed rendering.
- 🧩 **Retained scene model**: create figures, panels, visuals, cameras, controllers, and adornments
  as explicit objects instead of one-shot plotting calls.
- 🖥️ **Native first**: use the C API for applications, embedding, offscreen rendering, screenshots,
  video export, and low-level runtime integration.
- 🐍 **Python without a wrapper stack**: call the generated `datoviz` binding directly with
  documented NumPy array adaptation.
- 🌐 **Browser experiments**: try the experimental WebGPU/WASM path for selected examples that share
  the same scene model as the native runtime.


## Visual proof

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


## Start here

Pick the entry point that matches how you want to use Datoviz.

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/install/">
<strong>Install</strong>
<span>Choose the right setup path for Python, C/C++, macOS, Linux, or Windows.</span>
</a>
<a class="dvz-nav-card" href="start/quickstart/">
<strong>Quickstart</strong>
<span>Follow the annotated walkthrough and learn scene, panel, visual, and interaction basics.</span>
</a>
<a class="dvz-nav-card" href="start/choose-your-layer/">
<strong>Choose your layer</strong>
<span>Compare Python, C/C++, WebGPU, exact raw calls, and higher-level plotting tools.</span>
</a>
<a class="dvz-nav-card" href="examples/">
<strong>Examples</strong>
<span>Browse working visuals, features, runtime examples, and scientific showcases.</span>
</a>
<a class="dvz-nav-card" href="how-to/">
<strong>How-To guides</strong>
<span>Learn focused tasks such as axes, colorbars, picking, animation, capture, and offscreen output.</span>
</a>
<a class="dvz-nav-card" href="reference/">
<strong>Reference</strong>
<span>Look up visual families, attributes, C API pages, platform support, and project status.</span>
</a>
</div>


## Code example

The core workflow is straightforward: create a scene, add a panel, attach visual data, bind interaction,
then run or capture.

```python
--8<-- "examples/docs/quickstart.py"
```

![10 000 randomly colored points in an interactive Datoviz window](assets/gallery/v0.4/start/start_scatter.webp)

See [Quickstart](start/quickstart.md) for the annotated walkthrough, [Use from C or C++](how-to/c-integration.md)
for native integration, or [AI-assisted workflow](start/ai-workflow.md) when you want an LLM to help
generate Datoviz code.


## Acknowledgements

Datoviz v0.4 was developed with substantial AI-assisted engineering support from
[OpenAI Codex](https://openai.com/codex/). All changes were directed, reviewed, and validated by the
project maintainer. See the [project credits](https://github.com/datoviz/datoviz#license-and-credits)
for the full acknowledgement.
