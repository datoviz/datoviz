# Datoviz

**Datoviz is a GPU-powered visualization engine for scientific data.** It is built for
interactive 2D and 3D scenes that outgrow ordinary plotting: dense points, images, meshes, volumes,
text, annotations, linked panels, controllers, capture, and replayable render streams.

Datoviz v0.4 is the **engine layer** in the broader GSP/VisPy2 direction. Use it directly when you
want explicit control over scene objects and GPU-backed rendering; use GSP/VisPy2 for the future
high-level plotting layer when that work matures.

<a href="examples/gallery/showcases/showcases_protein/"><img src="assets/gallery/v0.4/showcases/showcases_protein.webp" alt="Interactive protein visualization rendered with Datoviz" style="width:100%;border-radius:8px;margin:1.5rem 0 1.5rem;display:block;"></a>


## Why Datoviz?

- ⚡ **Large interactive scenes**: explore point clouds, sampled fields, meshes, volumes, and
  scientific annotations with GPU-backed rendering.
- 🧩 **Retained scene model**: create figures, panels, visuals, cameras, controllers, and adornments
  as explicit objects instead of one-shot plotting calls.
- 🖥️ **Native first**: use the C API for applications, embedding, offscreen rendering, screenshots,
  video export, and low-level runtime integration.
- 🐍 **Python without a wrapper stack**: call the generated `datoviz` binding directly with
  documented NumPy array adaptation; use `datoviz.raw` only for exact C-shaped calls.
- 🌐 **Browser experiments**: try the experimental WebGPU/WASM path for selected examples that share
  the same scene model as the native runtime.


## Visual Proof

<div class="grid cards" markdown="1">

<div class="card" markdown="1">

**[Point Cloud](examples/gallery/showcases/showcases_point_cloud.md)**

[![Dense point cloud rendered in Datoviz](assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp)](examples/gallery/showcases/showcases_point_cloud.md)

Large 3D datasets with depth, color, and interactive navigation.

</div>

<div class="card" markdown="1">

**[Brain Volume](examples/gallery/showcases/showcases_brain_volume.md)**

[![Brain volume rendering with mesh overlay](assets/gallery/v0.4/showcases/showcases_brain_volume.poster.webp)](examples/gallery/showcases/showcases_brain_volume.md)

Volumes, slices, transparent geometry, and scientific context.

</div>

<div class="card" markdown="1">

**[Wind Field](examples/gallery/showcases/showcases_wind_field.md)**

[![Wind field visualization with color mapped data](assets/gallery/v0.4/showcases/showcases_wind_field.poster.webp)](examples/gallery/showcases/showcases_wind_field.md)

Scalar fields, vector overlays, colorbars, and probe-style workflows.

</div>

<div class="card" markdown="1">

**[Choropleth](examples/gallery/showcases/showcases_choropleth.md)**

[![Choropleth map rendered with Datoviz](assets/gallery/v0.4/showcases/showcases_choropleth.webp)](examples/gallery/showcases/showcases_choropleth.md)

Retained composites, labels, color scales, and responsive layouts.

</div>

</div>


## Where It Fits

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/choose-your-layer/">
<strong>Choose your layer</strong>
<span>Decide between Datoviz, raw bindings, C/C++, WebGPU experiments, and the GSP/VisPy2 direction.</span>
</a>
<a class="dvz-nav-card" href="reference/python-direct-engine/">
<strong>Python binding</strong>
<span>Use `import datoviz as dvz` with NumPy arrays and documented type adaptation.</span>
</a>
<a class="dvz-nav-card" href="how-to/c-integration/">
<strong>C and C++ integration</strong>
<span>Embed Datoviz directly, manage native windows, render offscreen, and integrate with applications.</span>
</a>
<a class="dvz-nav-card" href="reference/webgpu-subset/">
<strong>WebGPU subset</strong>
<span>Understand the experimental browser runtime, supported examples, and current boundaries.</span>
</a>
</div>


## Start Here

<div class="dvz-nav-grid">
<a class="dvz-nav-card" href="start/install/">
<strong>Install</strong>
<span>Choose the right setup path for Python, C/C++, macOS, Linux, or Windows.</span>
</a>
<a class="dvz-nav-card" href="start/quickstart/">
<strong>Quickstart</strong>
<span>Follow the annotated walkthrough and learn scene, panel, visual, and interaction basics.</span>
</a>
<a class="dvz-nav-card" href="examples/">
<strong>Examples</strong>
<span>Browse working visuals, features, runtime examples, and scientific showcases.</span>
</a>
<a class="dvz-nav-card" href="how-to/create-a-scene/">
<strong>How-To Guides</strong>
<span>Learn focused tasks such as axes, colorbars, picking, animation, capture, and offscreen output.</span>
</a>
<a class="dvz-nav-card" href="reference/feature-status/">
<strong>Feature status</strong>
<span>Check what is supported, experimental, advanced/unstable, deferred, or external/GSP-owned.</span>
</a>
<a class="dvz-nav-card" href="reference/">
<strong>Reference</strong>
<span>Look up visual families, attributes, C API pages, platform support, and project status.</span>
</a>
</div>


## API Sketch

The core workflow is explicit: create a scene, add a panel, attach visual data, bind interaction,
then run or capture.

```python
import numpy as np
import datoviz as dvz

N = 10_000
pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
pos[:, 2] = 0
color = np.full((N, 4), (80, 180, 255, 255), dtype=np.uint8)
diameter = np.full(N, 5, dtype=np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

panzoom = dvz.dvz_panzoom(scene, None)
dvz.dvz_panel_bind_controller(panel, panzoom, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

points = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data(points, "position", pos)
dvz.dvz_visual_set_data(points, "color", color)
dvz.dvz_visual_set_data(points, "diameter_px", diameter)
dvz.dvz_panel_add_visual(panel, points, None)

dvz.run(scene, figure, title="Datoviz")
```

![10 000 blue points in an interactive Datoviz window](assets/gallery/v0.4/start/start_scatter.webp)

See [Quickstart](start/quickstart.md) for the annotated walkthrough, [Use from C or C++](how-to/c-integration.md)
for native integration, or [AI-assisted workflow](start/ai-workflow.md) when you want an LLM to help
generate Datoviz code.
