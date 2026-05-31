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

The baseline smoke showcase now lives in `examples/c/showcases/gpu_particle_smoke.c`. Later GPU
particle work should cover broader custom-shader variants: alternate integrators, trails
accumulation, texture or mesh emission, richer UI parameters, and retained scene task scheduling.
