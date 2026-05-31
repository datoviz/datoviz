Execution Status:

- Status: active v0.4 experimental-slice plan
- Updated on: 2026-05-31
- Purpose: define the minimal compute-to-graphics and native CUDA interop work that may ship in
  v0.4 without turning Datoviz into a general custom-shader or CUDA framework
- Scope: C-first native runtime, DRP2 compute/synchronization, WebGPU parity for the portable
  subset, optional CUDA SDK example

# Compute And Graphics Interop

The v0.4 release should include a small but real compute-to-render path. The target is not a broad
user shader API. The target is one gallery-quality scientific example where GPU compute produces a
buffer or texture that graphics consumes directly, with no CPU readback in the frame loop.


## v0.4 Target

Required experimental slice:

1. DRP2 can express compute work that writes GPU resources consumed by a later render pass.
2. The native vklite runtime executes the compute-to-render path with explicit synchronization.
3. The WebGPU fixture runner accepts the same portable command semantics for the supported subset.
4. A C gallery example demonstrates GPU-only compute plus graphics.
5. Documentation marks the feature as `experimental`.

Optional native advanced slice:

1. A CUDA SDK example may update a Vulkan-owned buffer and let Datoviz render it.
2. CUDA support is compiled only when the CUDA SDK is available.
3. The example skips cleanly when no CUDA device, matching Vulkan GPU, external memory, or external
   semaphore support is available.
4. Documentation marks this path as `advanced/unstable`, native-only, and outside WebGPU scope.

Deferred:

1. CuPy/Python interop;
2. a general public custom-shader framework;
3. out-of-core compute scheduling;
4. reductions, atomics-heavy algorithms, and deterministic parallel-reduction policy;
5. broad CUDA-owned pointer import as a supported contract.

Longer-term scene compute task scheduling is tracked separately in
[`../future/SCENE_COMPUTE_TASK_ROADMAP.md`](../future/SCENE_COMPUTE_TASK_ROADMAP.md). That roadmap
must lower retained scene tasks into `DvzFramePlan`; it must not introduce a second executable graph
beside FramePlan.


## DRP2 Synchronization Requirement

v0.4 should promote a minimal backend-agnostic synchronization object or command into active DRP2.
The first useful shape is a small `ResourceBarrier` command or equivalent ordered resource-state
marker.

Initial required hazards:

1. buffer `STORAGE` write to buffer `VERTEX` read;
2. buffer `STORAGE` write to buffer `COPY_SRC` read;
3. storage texture or compute-written texture to sampled/read use, if the first gallery example
   uses textures.

Rules:

1. no Vulkan types in the DRP2 command;
2. schema, prose, validation, fixtures, native execution, and WebGPU runner behavior move together;
3. WebGPU may treat the barrier as a validated ordering marker when the backend already provides
   the needed ordering;
4. native Vulkan maps the marker to an explicit pipeline barrier.


## Scene API Shape

The scene-facing compute API should be a narrow experimental layer above the DRP2 contract, not a
general custom-renderer framework. The intended user model is:

```text
scene buffer with VERTEX | STORAGE usage
  -> scene compute pass writes the buffer
  -> FramePlan emits ResourceBarrier
  -> normal scene visual reads the same buffer as vertex attributes
```

The first public object should be a `DvzSceneCompute` attached to a scene. It owns no window,
swapchain, command buffer, or backend handle. It stores:

1. an explicit shader format and custom compute shader source;
2. an optional entry point, defaulting to `main`;
3. a mutable dispatch size;
4. declared buffer bindings with read/write access;
5. ordering constraints, initially `compute before visual`.

The descriptor may carry the initial dispatch:

```c
DvzSceneComputeDesc desc = {
    .shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL,
    .shader_source = source,
    .dispatch = {x, y, z},
};
```

Dispatch is scene state, not immutable shader state. A setter such as
`dvz_scene_compute_set_dispatch(compute, x, y, z)` should mark the scene dirty so particle counts,
adaptive grids, and selection/query workloads may change between frames. Fixed dispatch remains the
common path; animation parameters such as time, `dt`, forces, and counts should usually be updated
through a small scene buffer instead.

Custom shader source is an intentional abstraction leak for compute. Built-in visuals continue to
hide shader code because they expose semantic marks. Custom compute asks Datoviz to run user-defined
GPU logic, so the API must identify the shader format and source explicitly. The leak should remain
contained to `DvzSceneCompute`; normal visuals should consume scene buffers through existing visual
attribute bindings.

The initial C example should therefore be a live GLFW scene example:

```text
DvzScene / DvzFigure / DvzPanel
  -> DvzSceneBuffer particles: VERTEX | STORAGE
  -> DvzVisual point attributes bound to particle-buffer offsets
  -> DvzSceneCompute updates the particle buffer every frame
  -> dvz_view_glfw() presents normal scene rendering
```


## Implemented Gallery Example

The implemented v0.4 showcase is `examples/c/showcases/gpu_particle_smoke.c`:

```text
storage buffer input/state
  -> compute shader updates position, velocity or age/color
  -> resource barrier
  -> point/instance render consumes the same GPU buffer
```

Why this is the best first example:

1. it is visually strong with a modest amount of code;
2. it is recognizable scientific visualization: flow, plasma, wind, ocean, or dynamical systems;
3. it uses storage buffers and point rendering, which are already aligned with DRP2 and WebGPU;
4. it is a natural native CUDA variant: CUDA updates the exported Vulkan buffer, Datoviz renders it;
5. it avoids storage-texture, mesh-normal, and reduction complexity in the first slice.

Secondary examples, after the particle path works:

1. reaction-diffusion rendered as a colormapped image;
2. compute-generated heightfield rendered as a lit mesh;
3. GPU histogram or density map once atomics and deterministic behavior are specified.


## Implementation Order

1. Update release status and public feature classification.
2. Promote minimal DRP2 synchronization semantics.
3. Add native vklite barrier execution and focused GPU tests.
4. Add or update WebGPU fixture-runner parity for the portable compute-to-render subset.
5. Add the narrow scene `DvzSceneCompute` API and lowering required by the particle example.
6. Keep `examples/c/showcases/gpu_particle_smoke.c` as the live GLFW gallery proof.
7. Add optional CUDA SDK example if the native external-memory path remains low-risk.
8. Publish documentation for support level, unsupported variants, and validation commands.


## Acceptance Criteria

The portable v0.4 slice is release-ready when:

1. native validation covers compute pass, synchronization, render consumption, and readback or image
   evidence;
2. WebGPU fixture/dashboard evidence covers the portable command stream or records a specific
   unsupported-feature diagnostic;
3. the gallery example has a captured artifact suitable for release notes;
4. documentation identifies `compute+graphics interop` as experimental;
5. `CUDA interop` is documented as optional native advanced support, not part of the portable
   contract.
