# v0.4 Experimental Showcases

> **Example status:** experimental or stretch
> **Target:** C examples, generated fixtures, or backend-limited demos
> **Data:** synthetic or explicitly cached/prepared data
> **Validation:** smoke and clearly documented unsupported pieces

These examples may appear in v0.4 only with visible experimental status. They should not block the
release.


## `webgpu_browser_subset`

Browser/WebGPU proof over the shared DRP2/scene semantics. Keep the promise narrow: point,
primitive, image, and maybe basic mesh. Unsupported commands and backend differences must be
documented in the runnable example or generated fixture.


## `animation_video_export`

Simple animation and deterministic capture/video proof. Use frame callbacks, offline clocks, and
bounded export. Defer camera-path authoring helpers and rich transition APIs unless another release
example needs them.


## `datetime_axis`

Experimental datetime-axis proof over compact numeric data coordinates and UTC tick labels. The
current public C entry is `examples/c/features/datetime_axis.c`; keep it visibly experimental until
datetime coordinate and formatting policy is stable.


## `splat_cloud`

Optional dense translucent point-cloud showcase over the retained splat visual. The current public C
entry is `examples/c/visuals/splat.c`; keep it experimental until blend/depth policy and fixture
coverage are stable. Full Gaussian-splat asset pipelines remain later.


## `cpu_fluid_or_particles`

CPU-side fluid or particle advection stretch over existing dynamic image, point, and path updates.
GPU particles and Gray-Scott should wait for scene-level compute resources.


## `dense_streaming_2d`

One sustained-update 2D example is enough for v0.4. Choose DAQ or physiology, keep the data
synthetic, and prove partial updates, linked X panzoom, and readable axes/text without committing to
the full dashboard/ring-buffer policy.


## `spatial_omics`

Large point/pixel stress and selection demo inspired by napari/spatial biology workflows. Keep
napari integration external; Datoviz should prove rendering, coloring, basic selection, and bounded
data scale.


## `mouse_brain_slice`

Narrow brain-slice example with image or volume slice, colorbar, and simple GUI controls. The full
atlas explorer stays in v0.5.
