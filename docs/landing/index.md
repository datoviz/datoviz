---
hide:
  - navigation
  - title
  - toc
---

<style>
.md-header,
.md-tabs,
.md-footer,
.md-sidebar {
    display: none;
}

.md-main__inner,
.md-content,
.md-content__inner {
    max-width: none;
    margin: 0;
    padding: 0;
}

.md-content__inner::before {
    display: none;
}
</style>

<main class="dvz-public-landing" markdown="1">

<nav class="dvz-public-nav" aria-label="Landing navigation">
<a class="dvz-public-nav__brand" href="./">Datoviz</a>
<span>v0.4 RC</span>
<a href="../examples/">Examples</a>
<a href="../">Docs</a>
<a href="https://github.com/datoviz/datoviz">GitHub</a>
</nav>

<section class="dvz-public-hero" markdown="1">

<div class="dvz-public-hero__copy" markdown="1">

<p class="dvz-public-kicker">C-first scientific GPU visualization</p>

# Datoviz

A native rendering engine for large scientific scenes: retained visuals, precise interaction,
replayable command streams, and a Vulkan-first runtime with low-level Python integration.

Datoviz is the engine layer in the GSP/VisPy2 direction. The high-level plotting layer is still work
in progress; today you can use Datoviz directly from C or through the generated Python `ctypes`
binding.

<div class="dvz-public-actions" markdown="1">
[View Examples](../examples/showcases.md){ .md-button .md-button--primary }
[Read the Docs](../start/index.md){ .md-button }
</div>

<div class="dvz-public-proof" markdown="1">
<span>Scene API</span>
<span>DRP2</span>
<span>Vulkan</span>
<span>WebGPU preview</span>
</div>

</div>

<div class="dvz-public-hero__media" markdown="1">

<div class="dvz-public-panel dvz-public-panel--large" markdown="1">
![Datoviz colormap texture](../images/color_texture.png)
<strong>Retained GPU visuals</strong>
<span>images · meshes · volumes · points · text</span>
</div>

<div class="dvz-public-panel-row" markdown="1">
<div class="dvz-public-panel" markdown="1">
<strong>10M+</strong>
<span>interactive marks target</span>
</div>
<div class="dvz-public-panel" markdown="1">
<strong>v0.4</strong>
<span>Release candidate</span>
</div>
</div>

</div>

</section>

<section class="dvz-public-showcases" markdown="1">

## Visual Proof Targets

<div class="dvz-public-card-grid" markdown="1">

<a class="dvz-public-card" href="../examples/gallery/showcases/showcases_point_cloud/" markdown="1">
<span>01</span>
<strong>Dense LiDAR</strong>
<em>Large point clouds with direct color, depth, and metric navigation.</em>
</a>

<a class="dvz-public-card" href="../examples/gallery/showcases/showcases_protein/" markdown="1">
<span>02</span>
<strong>Molecular Arcball</strong>
<em>Clustered spheres and interactive 3D inspection for real scientific data.</em>
</a>

<a class="dvz-public-card" href="../examples/gallery/showcases/showcases_brain_volume/" markdown="1">
<span>03</span>
<strong>Brain Volume</strong>
<em>Volume data, slices, transparent mesh overlays, and annotations.</em>
</a>

<a class="dvz-public-card" href="../examples/gallery/showcases/showcases_wind_field/" markdown="1">
<span>04</span>
<strong>Weather Field</strong>
<em>Scalar fields, vector overlays, colorbars, and probe readback.</em>
</a>

<a class="dvz-public-card" href="../examples/gallery/showcases/showcases_choropleth/" markdown="1">
<span>05</span>
<strong>Choropleth</strong>
<em>Retained polygon composites with labels, color scales, and resizing proof.</em>
</a>

<a class="dvz-public-card" href="../reference/webgpu-subset/" markdown="1">
<span>06</span>
<strong>WebGPU Preview</strong>
<em>Browser rendering experiments backed by the shared scene contract.</em>
</a>

</div>

</section>

<section class="dvz-public-bands" markdown="1">

<div markdown="1">

## Engine Layer

Use Datoviz directly for native C APIs, embedding, offscreen rendering, replayable render streams,
backend work, and exact control over retained GPU resources.

</div>

<div markdown="1">

## Python Boundary

Datoviz v0.4 owns one generated Python binding: `datoviz` accepts documented NumPy arrays, while
`datoviz.raw` exposes the same binding's exact pointer/count call form. High-level plotting belongs
to GSP/VisPy2, with Datoviz as a backend when that layer is ready.

</div>

<div markdown="1">

## Release Status

v0.4 is a release candidate. Supported and experimental surfaces stay explicitly
labelled in the docs and examples.

</div>

</section>

</main>
