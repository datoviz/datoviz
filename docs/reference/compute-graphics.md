# Compute And Graphics

Status: experimental v0.4 slice.

Datoviz v0.4 includes a narrow compute-to-render path for early testing. The goal is one practical
GPU dataflow:

```text
scene buffer with STORAGE | VERTEX usage
  -> scene compute pass writes the buffer
  -> DRP2 ResourceBarrier declares compute-write to render-read ordering
  -> normal render pass consumes the same buffer as vertex input
```

This is not a general custom render-shader API, a general compute framework, or a CUDA/CuPy Python
interop contract.

The scene-compute constructors are an experimental authoring surface; emitted frame-plan and DRP2
objects are advanced/unstable runtime contracts. Applications should not couple ordinary visual
code to the lower packet representation.

## Supported Slice

- `DvzSceneCompute` stores shader source, dispatch size, and explicit scene-buffer bindings.
- `dvz_figure_add_compute()` schedules compute before the figure render path.
- DRP2 emits compute pipeline, compute pass, dispatch, `ResourceBarrier`, and render consumption.
- `ResourceBarrier` currently covers storage-buffer writes consumed as vertex input or copy source.
- The WebGPU runner validates the portable fixture subset, including compute and `ResourceBarrier`.
- The native runtime has focused compute-to-render command-generation and vklite execution tests.
- `examples/c/showcases/gpu_particle_smoke.c` is the C gallery proof for the scene compute path.

## Unsupported Or Deferred

- async compute queues and timer-driven compute independent from rendering;
- automatic multi-pass compute scheduling;
- compute-written textures as the public v0.4 path;
- reductions, atomics-heavy algorithms, and deterministic reduction policy;
- CUDA external-memory interop as a supported portable feature;
- CuPy/Python interop;
- custom render shaders for built-in visuals.

## Validation

Portable contract and scene lowering:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
./build/testing/dvztest scene/frame-plan-emit/drp2_compute_assisted
./build/testing/dvztest scene/frame-plan-emit/runtime_compute_two_frames
./build/testing/dvztest scene/scene-graph/compute_point_position_buffer_emits_drp2
```

Native Vulkan compute-to-render proof was recorded on 2026-06-17 with:

```bash
./build/testing/dvztest scene/frame-plan-emit/runtime_compute_two_frames_glsl_executes
./build/testing/dvztest vklite/compute_1
./build/testing/dvztest vklite/technique_compute_graphics
./build/examples/c/showcases/gpu_particle_smoke --png
```

The corresponding test functions are
`test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes`, `test_vklite_compute_1`, and
`test_technique_compute_graphics`. The showcase artifact was retained as
`build/release-evidence/gpu_particle_smoke.png`.

If the shell cannot create a Vulkan instance, the native GPU execution case may skip with a Vulkan
initialization diagnostic. That does not invalidate the portable DRP2/WebGPU fixture proof, but it
does mean that particular run is not fresh native GPU evidence. The recorded proof does not change
the feature status: this remains a narrow experimental scene-compute/DRP2 interoperability slice,
not a general compute framework.

For exact current signatures, see the generated [scene API](c-api/scene.md),
[frame-plan API](c-api/frame-plan.md), and [DRP2 API](c-api/drp2.md). The first is the authoring
surface; the latter two are contributor/runtime surfaces.
