# Datoviz — GPU visualization for large scientific data

**Datoviz is an open-source, cross-platform visualization engine written in C, with direct
Python/NumPy bindings.** It uses Vulkan to render large, dynamic datasets in
interactive, high-quality 2D and 3D scenes on Linux, macOS, and Windows.

Datoviz is desktop-first, with native windows, offscreen rendering, application embedding, and
built-in GUI support through [Dear ImGui](https://github.com/ocornut/imgui). An experimental
[WebGPU/WASM subset](reference/webgpu-subset.md) brings selected scenes to the browser.

<p class="dvz-home-meta">MIT license <span aria-hidden="true">·</span> C API and Python/NumPy
<span aria-hidden="true">·</span> Linux, macOS, and Windows <span aria-hidden="true">·</span>
Desktop-first</p>

<nav class="dvz-home-actions" aria-label="Primary actions">
<a class="dvz-home-action-primary" href="start/">Get started <span aria-hidden="true">→</span></a>
<a href="examples/">Browse the gallery</a>
<a href="https://github.com/datoviz/datoviz">View on GitHub</a>
</nav>

<p class="dvz-home-status"><strong>v0.4.0rc2:</strong> the active release candidate is published on PyPI and GitHub. The native Vulkan scene API is the primary supported path; browser WebGPU and advanced/unstable facilities remain experimental. See the <a href="releases/v0.4.0rc2/">RC2 release notes</a>, <a href="reference/feature-status/">feature status</a>, or the <a href="/v0.3/">legacy v0.3 documentation</a>.</p>

<div class="dvz-gallery-media dvz-gallery-media--video dvz-home-hero" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_protein/" aria-label="Open the protein visualization example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" alt="Interactive 3D protein visualization rendered with Datoviz" loading="eager">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_protein.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_protein.mp4" type="video/mp4">
  </video>
</div>


## :material-image-multiple-outline: What can you build? { .dvz-home-heading }

Datoviz combines scientific visuals, interaction, annotation, and native application controls in
the same retained scene model.

<div class="dvz-showcase-grid">
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_textured_planet/" aria-label="Open the textured planets and orbital debris example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_textured_planet.poster.webp" alt="Textured Earth with orbital debris in a star field" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_textured_planet.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_textured_planet.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_textured_planet/">Textured planets</a></h3>
<p>Textured worlds, a star field, orbital debris, and 3D navigation.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_point_cloud/" aria-label="Open the point cloud example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp" alt="A dense colorized outdoor point cloud" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_point_cloud.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_point_cloud/">Point cloud</a></h3>
<p>Large colorized LiDAR data with fly navigation and depth enhancement.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_choropleth/" aria-label="Open the U.S. state choropleth example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_choropleth.webp" alt="U.S. state population density shown as a choropleth map" loading="lazy">
</div>
<h3><a href="examples/gallery/showcases/showcases_choropleth/">U.S. state choropleth</a></h3>
<p>Population density mapped across contiguous U.S. state polygons.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_cortical_activity/" aria-label="Open the human auditory cortical activity example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_cortical_activity.poster.webp" alt="Human cortical activity projected onto a rotating brain surface" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_cortical_activity.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_cortical_activity.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_cortical_activity/">Human cortical activity</a></h3>
<p>Time-varying MEG activity projected onto a rotating cortical surface.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_galaxy/" aria-label="Open the density-wave galaxy example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_galaxy.poster.webp" alt="An animated spiral galaxy rendered from colored particles" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_galaxy.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_galaxy.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_galaxy/">Density-wave galaxy</a></h3>
<p>An animated spiral galaxy built from a density-wave particle model.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_gpu_particle_smoke/" aria-label="Open the GPU particle smoke example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_gpu_particle_smoke.poster.webp" alt="GPU-driven colored particles forming an animated smoke plume" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_gpu_particle_smoke.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_gpu_particle_smoke.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_gpu_particle_smoke/">GPU particle smoke</a></h3>
<p>Compute-driven particles rendered through native compute and graphics interop.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_scientific_plotting/" aria-label="Open the Scientific Plotting Workflow example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_scientific_plotting.webp" alt="A scientific plotting workflow with histograms, traces, axes, and annotations" loading="lazy">
</div>
<h3><a href="examples/gallery/showcases/showcases_scientific_plotting/">Scientific Plotting Workflow</a></h3>
<p>Histograms, uncertainty bands, stacked traces, axes, and annotations.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_brain_volume/" aria-label="Open the Allen mouse brain example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_brain_volume.poster.webp" alt="Allen mouse brain volume rendering with anatomical mesh overlays" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true"
         poster="assets/gallery/v0.4/showcases/showcases_brain_volume.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_brain_volume.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_brain_volume/">Allen mouse brain</a></h3>
<p>An anatomical RGBA volume with slices and structure meshes.</p>
</article>
</div>


## :material-code-braces: Start with your language { .dvz-home-heading }

<div class="dvz-language-grid">
<section>
<h3>Python + NumPy</h3>
<p>Create retained scenes and pass NumPy arrays directly to Datoviz functions. The API keeps the
same explicit <code>dvz_*</code> vocabulary as C.</p>
<p class="dvz-text-links"><a href="start/quickstart/">Python quickstart →</a> <a href="reference/ctypes/">Python API</a></p>
</section>
<section>
<h3>C or C++</h3>
<p>Use the native C library for desktop applications, embedding, offscreen rendering, capture, and
lower-level runtime integration. The public API uses C linkage and is callable from C++.</p>
<p class="dvz-text-links"><a href="start/first-c-program/">First C program →</a> <a href="how-to/c-integration/">C/C++ integration</a></p>
</section>
</div>

Prebuilt wheels containing the Python binding and native runtime are published and validated for supported Linux, macOS, and Windows targets. Install the exact release candidate with `python -m pip install --pre datoviz==0.4.0rc2`, then follow the [installation instructions](start/install.md).


## :material-flask-outline: Built for scientific applications { .dvz-home-heading }

<div class="dvz-feature-grid" markdown="1">
<section markdown="1">

### :material-database-outline: Large, dynamic data

Update points, images, sampled fields, meshes, volumes, text, and annotations without rebuilding
the entire scene.

</section>
<section markdown="1">

### :material-axis-arrow: First-class 2D and 3D

Compose axes, panels, paths, images, and guides alongside cameras, lighting, meshes, point clouds,
and volumes.

</section>
<section markdown="1">

### :material-cursor-default-click-outline: Interaction and composition

Combine linked panels, panzoom and 3D controllers, picking, queries, labels, colorbars, and native
GUI controls.

</section>
<section markdown="1">

### :material-application-export: Native integration and output

Render in windows or offscreen, embed the C engine, capture screenshots, export video, and record
or replay render streams.

</section>
</div>


## :material-layers-triple-outline: Where Datoviz fits { .dvz-home-heading }

Datoviz v0.4 is an explicit rendering engine, not a Matplotlib-like plotting frontend. Most users
work with retained figures, panels, visuals, and controllers; experienced integrators can use the
protocol and runtime layers directly.

<ol class="dvz-architecture" aria-label="Datoviz software layers">
<li>
<strong>High-level plotting</strong>
<span>Developing GSP and VisPy 2 interfaces outside Datoviz v0.4 — use the scene API below today</span>
</li>
<li>
<strong><a href="start/">Retained scene API</a></strong>
<span>Figures, panels, visuals, controllers, annotations, and queries</span>
</li>
<li>
<strong><a href="advanced/drp2-command-streams/">Datoviz Rendering Protocol v2 (DRP2)</a></strong>
<span>Backend-neutral, WebGPU-shaped command streams, validation, and replay</span>
</li>
<li>
<strong><a href="advanced/runtime-internals/">Rendering runtimes</a></strong>
<span>Native Vulkan execution and the experimental browser WebGPU subset</span>
</li>
</ol>

Source builds expose switches for compatible core, Vulkan, canvas, DRP2, scene, app, and GUI
components. The native WebGPU build option is separate from the browser WebGPU/WASM toolchain. See
[Choose your layer](start/choose-your-layer.md) and [Build options](reference/build-options.md).


## :material-check-decagram-outline: Platform and maturity { .dvz-home-heading }

| Surface | v0.4 position |
| --- | --- |
| Native Vulkan scene API | <span class="dvz-status dvz-status--supported">Supported</span> Primary path, with feature-specific status documented separately |
| Linux, macOS, and Windows | <span class="dvz-status dvz-status--supported">Supported targets</span> Wheel and source-build requirements vary by platform |
| Python | <span class="dvz-status dvz-status--supported">Supported</span> Direct generated binding with documented NumPy adaptation |
| Dear ImGui | <span class="dvz-status dvz-status--supported">Supported</span> Built-in native desktop GUI support |
| WebGPU/WASM | <span class="dvz-status dvz-status--experimental">Experimental</span> Browser subset for promoted examples, not native feature parity |
| DRP2 and lower-level runtime APIs | <span class="dvz-status dvz-status--advanced">Advanced/unstable</span> Integration surfaces |

Review [Platform support](reference/platform-support.md), [Feature status](reference/feature-status.md),
and [Project status](reference/project-status.md) before adopting an experimental or
backend-specific feature.


## :material-arrow-right-circle-outline: Continue { .dvz-home-heading }

<nav class="dvz-continue-links" aria-label="Documentation links">
<a href="start/install/">Install</a>
<a href="start/quickstart/">Quickstart</a>
<a href="examples/">Examples</a>
<a href="how-to/">How-To guides</a>
<a href="reference/">Reference</a>
<a href="ai-agents/">AI agents</a>
<a href="https://github.com/datoviz/datoviz">GitHub</a>
</nav>

For human-led coding-assistant guidance, see the [AI-assisted workflow](start/ai-workflow.md). For
autonomous code generation, use the stricter [AI-agent contract](ai-agents.md). Project
acknowledgements are recorded in the
[repository credits](https://github.com/datoviz/datoviz#license-and-credits).
