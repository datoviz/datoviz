# Datoviz

<p class="dvz-home-lead">GPU visualization for large, dynamic scientific data.</p>

Datoviz is an open-source C engine with direct Python and NumPy bindings. Build interactive 2D and 3D scenes, native applications, and offscreen renderers on Linux, macOS, and Windows.

Datoviz gives you an explicit retained scene model rather than a high-level plotting interface. Use it when data volume, frequent updates, interaction, or native integration require direct rendering control.

<nav class="dvz-home-actions" aria-label="Primary actions">
<a class="dvz-home-action-primary" href="start/">Get started <span aria-hidden="true">→</span></a>
<a href="examples/">Explore examples</a>
<a href="https://github.com/datoviz/datoviz">GitHub</a>
</nav>

<div class="dvz-gallery-media dvz-gallery-media--video dvz-home-hero" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_protein/" aria-label="Open the protein visualization example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_protein.poster.webp" alt="Interactive 3D protein visualization rendered with Datoviz" loading="eager">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true" poster="assets/gallery/v0.4/showcases/showcases_protein.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_protein.mp4" type="video/mp4">
  </video>
</div>


## See it in action { .dvz-home-heading }

<div class="dvz-showcase-grid">
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_point_cloud/" aria-label="Open the point cloud example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp" alt="A dense colorized outdoor point cloud" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true" poster="assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_point_cloud.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_point_cloud/">Point cloud</a></h3>
<p>Large colorized LiDAR data with fly navigation and depth enhancement.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_cortical_activity/" aria-label="Open the human cortical activity example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_cortical_activity.poster.webp" alt="Human cortical activity projected onto a brain mesh" loading="lazy">
</div>
<h3><a href="examples/gallery/showcases/showcases_cortical_activity/">Human cortical activity</a></h3>
<p>Time-varying MEG activity projected onto a human cortical surface.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_scientific_plotting/" aria-label="Open the scientific plotting example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_scientific_plotting.webp" alt="A scientific plotting workflow with histograms, traces, axes, and annotations" loading="lazy">
</div>
<h3><a href="examples/gallery/showcases/showcases_scientific_plotting/">Scientific plotting</a></h3>
<p>Histograms, uncertainty bands, stacked traces, axes, and annotations.</p>
</article>
<article class="dvz-showcase">
<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <a class="dvz-gallery-media-target" href="examples/gallery/showcases/showcases_textured_planet/" aria-label="Open the textured planets example"></a>
  <img class="dvz-gallery-poster" src="assets/gallery/v0.4/showcases/showcases_textured_planet.poster.webp" alt="Textured Earth with orbital debris in a star field" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none" aria-hidden="true" poster="assets/gallery/v0.4/showcases/showcases_textured_planet.poster.webp">
    <source data-src="assets/gallery/v0.4/showcases/showcases_textured_planet.mp4" type="video/mp4">
  </video>
</div>
<h3><a href="examples/gallery/showcases/showcases_textured_planet/">Textured planets</a></h3>
<p>Textured worlds, orbital debris, and responsive 3D navigation.</p>
</article>
</div>

<p class="dvz-text-links"><a href="examples/">Explore the complete gallery →</a></p>


## Start building { .dvz-home-heading }

<div class="dvz-language-grid">
<section>
<h3>Python + NumPy</h3>
<p>Install the published package, pass NumPy arrays directly to Datoviz, and open your first interactive scene.</p>
<pre><code>python -m pip install --pre datoviz==0.4.0rc2</code></pre>
<p class="dvz-text-links"><a href="start/quickstart/">Python Quickstart →</a> <a href="start/install/">Installation details</a></p>
</section>
<section>
<h3>C or C++</h3>
<p>Build the native library for desktop applications, embedding, offscreen rendering, and lower-level integration.</p>
<pre><code>target_link_libraries(my_app PRIVATE datoviz::datoviz)</code></pre>
<p class="dvz-text-links"><a href="start/first-c-program/">First C program →</a> <a href="how-to/c-integration/">C/C++ integration</a></p>
</section>
</div>


## Why Datoviz { .dvz-home-heading }

<div class="dvz-feature-grid" markdown="1">
<section markdown="1">

### Large, dynamic data

Update GPU-backed points, images, meshes, volumes, text, and annotations without rebuilding the scene.

</section>
<section markdown="1">

### 2D and 3D together

Combine plots, images, guides, cameras, lighting, point clouds, meshes, and volumes in one scene model.

</section>
<section markdown="1">

### Native and embeddable

Render in native windows or offscreen, embed the C engine, capture images and video, or replay render streams.

</section>
</div>


## Current scope { .dvz-home-heading }

<p class="dvz-home-status"><span class="dvz-status dvz-status--supported">Supported</span> The native Vulkan scene API is the primary path. <span class="dvz-status dvz-status--experimental">Experimental</span> Browser WebGPU supports a selected subset. <span class="dvz-status dvz-status--advanced">Advanced</span> DRP2 and lower-level runtime APIs are unstable integration surfaces.</p>

<p class="dvz-text-links"><a href="reference/platform-support/">Platform support →</a> <a href="reference/feature-status/">Feature status</a> <a href="start/choose-your-layer/">Choose your layer</a></p>

<nav class="dvz-home-actions dvz-home-actions--final" aria-label="Get started with Datoviz">
<a class="dvz-home-action-primary" href="start/">Get started with Datoviz <span aria-hidden="true">→</span></a>
</nav>
