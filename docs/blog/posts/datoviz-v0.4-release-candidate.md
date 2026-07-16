---
date: 2026-07-16T10:00:00+02:00
slug: datoviz-v0-4-release-candidate
categories:
  - Releases
---

# Datoviz v0.4 release candidate

I'm glad to announce the first release candidate of Datoviz v0.4. 🚀

Datoviz is an open-source GPU rendering engine for interactive scientific visualization. Written
in C and based on Vulkan and WebGPU, it is designed for 2D and 3D graphics, especially with large
datasets.

Version 0.4 is a major upgrade and the result of months of intensive work. This release candidate
is a public testing milestone, not the final v0.4 release. I would be very grateful for feedback
about installation problems, unclear APIs or documentation, broken examples, and behavior on
different platforms and GPUs.

<nav class="dvz-home-actions" aria-label="Release candidate actions">
<a class="dvz-home-action-primary" href="#try-datoviz-v04">Install RC1 <span aria-hidden="true">→</span></a>
<a href="../../releases/v0.4.0rc1/">Release notes</a>
<a href="../../examples/">Browse examples</a>
<a href="https://github.com/datoviz/datoviz/issues">Report an issue</a>
</nav>

<!-- more -->


## :material-image-multiple-outline: Datoviz in action { .dvz-home-heading }

A few examples showing the range of interactive 2D and 3D scientific visualization supported by
Datoviz.

<div class="grid cards" markdown="1">

<div class="card" markdown="1">

### [Rotating Earth](../../examples/gallery/showcases/showcases_textured_planet.md)

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <img class="dvz-gallery-poster" src="../../assets/gallery/v0.4/showcases/showcases_textured_planet.poster.webp" alt="Rotating textured Earth rendered in 3D" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="../../assets/gallery/v0.4/showcases/showcases_textured_planet.poster.webp" aria-label="Rotating textured Earth preview">
    <source data-src="../../assets/gallery/v0.4/showcases/showcases_textured_planet.mp4" type="video/mp4">
  </video>
</div>

</div>

<div class="card" markdown="1">

### [GPU particle smoke](../../examples/gallery/showcases/showcases_gpu_particle_smoke.md)

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <img class="dvz-gallery-poster" src="../../assets/gallery/v0.4/showcases/showcases_gpu_particle_smoke.poster.webp" alt="GPU particle smoke simulation" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="../../assets/gallery/v0.4/showcases/showcases_gpu_particle_smoke.poster.webp" aria-label="GPU particle smoke simulation preview">
    <source data-src="../../assets/gallery/v0.4/showcases/showcases_gpu_particle_smoke.mp4" type="video/mp4">
  </video>
</div>

</div>

<div class="card" markdown="1">

### [Scientific 2D workflow](../../examples/gallery/showcases/showcases_panel_linked_axes.md)

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <img class="dvz-gallery-poster" src="../../assets/gallery/v0.4/showcases/showcases_panel_linked_axes.poster.webp" alt="Linked scientific 2D panels with axes and traces" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="../../assets/gallery/v0.4/showcases/showcases_panel_linked_axes.poster.webp" aria-label="Scientific 2D workflow preview">
    <source data-src="../../assets/gallery/v0.4/showcases/showcases_panel_linked_axes.mp4" type="video/mp4">
  </video>
</div>

</div>

<div class="card" markdown="1">

### [Large point cloud](../../examples/gallery/showcases/showcases_point_cloud.md)

<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">
  <img class="dvz-gallery-poster" src="../../assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp" alt="Large colored 3D point cloud" loading="lazy">
  <video class="dvz-gallery-video" muted loop playsinline preload="none"
         poster="../../assets/gallery/v0.4/showcases/showcases_point_cloud.poster.webp" aria-label="Large 3D point cloud preview">
    <source data-src="../../assets/gallery/v0.4/showcases/showcases_point_cloud.mp4" type="video/mp4">
  </video>
</div>

</div>

</div>


Datoviz sits at a lower level than a high-level plotting library like Matplotlib. It is a rendering
engine with a C API and a generated Python binding. High-level plotting interfaces belong to the
experimental VisPy 2 and Graphics Server Protocol (GSP) work.


## :material-check-decagram-outline: What is in v0.4? { .dvz-home-heading }

Datoviz v0.3 already had a broad set of 2D and 3D visuals and basic interactivity. Version 0.4
brings together many features I had wanted Datoviz to support for years:

- order-independent transparency for translucent 3D meshes;
- 3D rendering techniques such as depth cueing, Eye-Dome Lighting, and screen-space ambient
  occlusion;
- multisample antialiasing;
- improved lighting and material controls for 3D meshes and spheres;
- more than one hundred examples covering most visuals and features in the library;
- an experimental WebGPU backend, with live browser versions of most examples;
- an experimental compute-to-render path, including CUDA and CuPy interoperability;
- item picking, selection, data probing, and GPU readback;
- terminal IPython integration;
- native Qt and PyQt hosting.

## :material-layers-triple-outline: Why v0.4 is different { .dvz-home-heading }

The visible features are only part of the story. The deeper change in v0.4 is architectural: the
scene, rendering protocol, GPU runtime, frame execution, and presentation layers now have clearer
boundaries.

This makes the library easier to extend without creating a separate rendering path for every new
platform or output mode. The specification-driven work behind this architecture, and the role
coding agents played in it, are the subject of
[a separate post](specifications-before-code-rebuilding-datoviz-with-coding-agents.md).


## :material-flask-outline: Release-candidate scope { .dvz-home-heading }

<div class="dvz-release-scope">
<section class="dvz-release-scope__primary">
<strong>Primary testing surface</strong>
<span>Vulkan core, retained scene, native app and offscreen rendering, Python binding</span>
</section>
<section class="dvz-release-scope__experimental">
<strong>Experimental</strong>
<span>WebGPU/WASM and compute-to-render support</span>
</section>
<section class="dvz-release-scope__optional">
<strong>Optional or external</strong>
<span>Qt and PyQt provider, VisPy 2 and GSP plotting work</span>
</section>
</div>

This release candidate does not imply that the API is fully stable. The architecture and public
surface are much closer to their intended v0.4 form, but feedback may still require changes, and
v0.5 may make further breaking changes where necessary.


## :material-arrow-right-circle-outline: Try Datoviz v0.4 { .dvz-home-heading }

The next step is to put this release in the hands of more users. External testing and feedback will
reveal problems that internal development cannot.

```sh
python -m pip install datoviz==0.4.0rc1
```

Start with the [installation guide](../../start/install.md), the
[quickstart](../../start/quickstart.md), or the [examples](../../examples/index.md). The detailed RC
scope and known limitations are in the [release notes](../../releases/v0.4.0rc1.md).

I would particularly appreciate feedback about:

- installation on a clean Linux, macOS, or Windows system;
- behavior on different GPUs and drivers;
- clarity and consistency of the C and Python APIs;
- missing or unclear documentation;
- the examples and gallery;
- IPython and Qt integration;
- the experimental WebGPU and compute paths;
- scientific use cases that Datoviz should support.

Please report problems in the [GitHub issue tracker](https://github.com/datoviz/datoviz/issues).
Include the operating system, GPU, driver, Python version, installation method, and a small
reproducer when possible.
