# Later Compute And Custom Shader Scenarios

> **Example status:** later planning bundle
> **Target:** scene-level compute/custom material design pressure
> **Data:** deterministic generated inputs
> **Validation:** fixed seed, bounded smoke, readback or screenshot checks

Do not force these through ad hoc DRP2-only examples. They should wait for scene-level resource,
compute, material, and framegraph semantics.


## `gray_scott`

Needs persistent ping-pong fields or storage textures, compute-to-render dependencies, brush/input
uniforms, reset/preset controls, and rendering the compute output without bypassing scene semantics.


## `mandelbrot`

Needs scene custom fullscreen visual/material API, user uniforms, event-driven parameter updates,
double-single or high-precision parameter policy, progressive refinement, and optional HUD text.


## `gpu_particles`

Needs compute-written buffers consumed by render passes, persistent buffer reuse, barriers,
additive/transparent particle shaders, optional trails accumulation, and per-frame UI parameters.
