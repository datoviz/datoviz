# Depth-Of-Field Postprocess

Status: proposal for an optional graph-backed postprocess technique.

DoF is a showcase tool, not a default scientific visualization style. It should remain disabled for
baseline visual comparisons, quantitative 2D plots, axes-heavy examples, colorbar/annotation-heavy
panels, and reference screenshots unless the example is specifically about postprocessing.


## Contract

DoF is a panel-scoped retained technique expanded into ordinary FramePlan graph resources and passes:

```text
scene color/depth -> signed CoC -> prefilter/downsample -> tile max -> bokeh gather ->
composite -> sharp overlays
```

The implementation must stay on the active route:

```text
scene state -> FramePlan graph -> DRP2 command stream -> vklite/WebGPU runtime
```

Do not add a Vulkan-only postprocess path, user-editable public framegraph, or browser-specific
scene semantics.


## Parameters

First public or internal parameter set:

1. mode: off, depth, tilt-shift, or hybrid;
2. quality: fast, medium, high, or ultra;
3. focus distance in eye-space units;
4. aperture or blur scale;
5. foreground/background blur limits;
6. optional screen-space focus point for showcase controls.

The first implementation should prefer a simple panel API or retained technique descriptor. Avoid
exposing low-level render graph details.


## Pipeline

Preferred high-quality path:

1. render data layer to `RGBA16F` color and sampled depth;
2. compute signed circle of confusion into `R16F` or `RG16F`;
3. prefilter/downsample color and CoC to half resolution;
4. compute tile-max CoC where supported;
5. run bokeh gather into half-resolution color;
6. composite into final panel target;
7. render annotations, overlays, labels, axes, guides, and UI after DoF so they stay sharp.

Signed CoC separates foreground and background blur and reduces bleeding across focus boundaries.


## Capability Fallback

Minimum useful capability set:

1. sampled render targets;
2. sampled depth or a defined depth-copy path;
3. `RGBA16F` color target or an explicit lower-quality fallback;
4. `R16F` or `RG16F` CoC target;
5. fullscreen render passes or compute dispatch for tile/gather stages.

Fallback ladder:

1. high: `RGBA16F` + `R16F/RG16F` + half-resolution bokeh + tile max;
2. medium: half-resolution gather without tile max;
3. low: `RGBA8` or tilt-shift approximation;
4. unsupported: disable and emit a deterministic diagnostic.

Diagnostics must distinguish unsupported formats, missing render-target sampling, missing sampled
depth, and undefined MSAA depth resolve.


## Ordering With Other Techniques

Recommended order:

```text
MSAA scene render, if enabled
  -> resolve color/depth if policy exists
  -> G-buffer/depth/normal preparation
  -> SSAO or EDL
  -> transparent resolve
  -> DoF
  -> tone mapping/gamma
  -> overlays/annotations
  -> present or readback
```

Rules:

1. SSAO and EDL should run before DoF.
2. Transparent WBOIT/depth-peel resolve should run before DoF for photographic output.
3. If MSAA depth resolve is undefined, DoF should force single-sample rendering or disable with a
   diagnostic.
4. Scientific transparency examples should usually disable DoF.


## First Implementation Target

The first useful slice:

1. one panel;
2. single-sample render targets;
3. `RGBA16F` scene color;
4. sampled depth;
5. `R16F` signed CoC;
6. half-resolution bokeh gather;
7. fullscreen composite;
8. GLSL native shaders;
9. late overlays unblurred;
10. one deterministic showcase example with DoF on/off comparison.

Defer physical camera modeling, per-object focus tracking, automatic focus picking, perfect MSAA
depth resolve, user-editable render graphs, and WebGPU parity until the native graph path is stable.


## Validation

Required coverage when implemented:

1. deterministic FramePlan expansion with expected target and pass order;
2. capability fallback tests;
3. DRP2 fixtures for transient targets, fullscreen shaders/pipelines, sampled inputs, and submit;
4. negative fixtures for unsupported formats or missing sampled render-target support;
5. visual regression showing focused geometry sharp, foreground/background blurred, overlays sharp,
   and output nonblank.
