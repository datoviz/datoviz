# Particle System Design

> Status: proposal.
> Scope: GPU-updated particle state, render views, optional tracks, and CUDA/CuPy producer
> integration for the scene -> FramePlan -> DRP2 -> runtime path.


## Purpose

Particle systems should be modeled as dynamic simulation state with one or more render views, not as
only another static visual family.

The core use case is efficient animation where particle positions, velocities, ages, and related
attributes are updated on the GPU and rendered without CPU readback or per-frame CPU uploads. The
producer may be a Datoviz-owned compute shader or an external CUDA/CuPy kernel writing shared GPU
memory.

This proposal complements:

1. [../examples/compute/PARTICLES.md](../examples/compute/PARTICLES.md), which describes the
   gallery/example
   pressure test;
2. [../integration/CUPY_CUDA_INTEROP.md](../integration/CUPY_CUDA_INTEROP.md), which defines the
   preferred Vulkan-owned external-memory route for CuPy;
3. [../../../agents/soon/SCENE_VECTOR_VISUALS_PLAN.md](../../../agents/soon/SCENE_VECTOR_VISUALS_PLAN.md),
   which defines `path`, `streamline`, and `tube` as trajectory/geometry renderers.


## Taxonomy

Use these terms consistently:

1. `point`, `pixel`, `sphere`, and future billboard/splat modes render independent particle-like
   samples.
2. `path`, `streamline`, `track`, and `tube` render authored or derived trajectory geometry.
3. `particles` owns dynamic simulation state and exposes render views over that state.

Tracks and tubes do not replace particles. They are optional consumers of particle history,
velocity, or selected particle trajectories.


## Core Model

A particle system owns a persistent GPU state buffer and lightweight per-frame parameters.

Minimum state:

```text
struct Particle {
    vec4 position_age;   // xyz = position, w = age, phase, or life
    vec4 velocity_mass;  // xyz = velocity, w = mass, kind, or group
};
```

Common extended state:

```text
struct Particle {
    vec4 position_age;
    vec4 velocity_mass;
    vec4 color_size;     // rgba, rgb + size, or scalar payload
};
```

The buffer should be created with usage flags that allow the active producer and render views:

1. storage read/write for compute-shader simulation;
2. vertex or instance input for point/sphere/billboard rendering when practical;
3. read-only storage input for shaders that need packed particle structs;
4. external-memory export when CUDA/CuPy is the producer.


## Producer Modes

### Datoviz Compute Producer

Datoviz owns the simulation shader, state buffer, parameter buffer, and scheduling.

One-frame shape:

```text
Pass 1: COMPUTE particle_update
    read/write: particles
    read:       params
    dispatch:   ceil(count / workgroup_size), 1, 1

Pass 2: RENDER particle_view
    read:       particles
    draw:       count particles
```

This mode should be the portable first target because it maps to DRP2 compute and can align with
future WebGPU support.

### CUDA/CuPy Producer

CuPy should write a Datoviz/Vulkan-owned exportable buffer, not require Vulkan to import an ordinary
CuPy memory-pool allocation.

Ownership and ordering:

1. Datoviz creates a buffer with graphics-compatible usage flags.
2. Datoviz exports external-memory metadata and an external synchronization primitive.
3. CUDA imports the memory and exposes a CuPy array view.
4. CuPy kernels write the buffer on a known stream.
5. CUDA signals completion.
6. Datoviz waits before rendering the associated frame.

Double or triple buffering should be supported when CUDA and Vulkan work need to overlap. The
single-buffer path is acceptable for a first correctness slice when synchronization is explicit and
measured.


## Render Views

Particle systems should expose render views rather than force one visual type.

Recommended first views:

1. `point` or `pixel` view for maximum throughput;
2. billboard or soft splat view for glowing particles and density clouds;
3. `sphere` view for atoms, cells, or 3D particles requiring depth and lighting;
4. velocity streak view, rendering short segments derived from position and velocity.

The view should not own the simulation state. It should bind the particle state buffer as an
attribute, instance input, or read-only storage resource depending on the selected shader path.


## Tracks And Trails

Three trail modes are useful and should remain distinct.

### Velocity Streaks

Render a short segment directly from current state:

```text
p0 = position
p1 = position - velocity * trail_scale
```

This is cheap, needs no history buffer, and is usually the best first track-like view for dense
particle systems.

### History Tracks

Maintain a ring buffer of recent particle positions:

```text
history[history_length][particle_count]
```

Each frame, compute or CuPy writes the current position into the next history slot. Rendering can
then draw faded segments or line strips by age. This preserves particle identity over a short window
without CPU readback, but memory cost scales with `history_length * particle_count`.

### Accumulation Trails

Render into an accumulation target, fade the previous frame, then draw current particles.

This produces attractive motion trails with low geometry cost. It is not a true per-particle track
and should not be used for picking or identity-preserving analysis.


## Streamlines, Tubes, And Selected Tracks

Future `streamline` and `tube` visuals should be used for selected trajectories, authored paths, or
lower-density high-quality outputs.

Do not make true tube meshes the default rendering mode for every particle in a dense simulation.
For high-density systems, prefer points, billboards, velocity streaks, or ribbon/strip modes. Tube
meshes are appropriate for selected particles, bundled paths, quality screenshots, or scientific
trajectory subsets where normals, lighting, and SSAO matter.


## API Direction

Names are provisional, but the public shape should make producer, state, and view ownership clear:

```c
DvzParticles* dvz_particles(DvzScene* scene, uint32_t count, uint32_t flags);

int dvz_particles_set_state(DvzParticles* particles, const void* data, uint32_t count);
int dvz_particles_set_params(DvzParticles* particles, const DvzParticleParams* params);
int dvz_particles_set_compute(DvzParticles* particles, const DvzParticleComputeDesc* desc);
int dvz_particles_set_external_producer(
    DvzParticles* particles, const DvzExternalProducerDesc* desc);

DvzVisual* dvz_particles_as_points(DvzParticles* particles, uint32_t flags);
DvzVisual* dvz_particles_as_spheres(DvzParticles* particles, uint32_t flags);
DvzVisual* dvz_particles_as_streaks(DvzParticles* particles, const DvzParticleStreakDesc* desc);
DvzVisual* dvz_particles_as_tracks(DvzParticles* particles, const DvzParticleTrackDesc* desc);
```

The important design rule is that `DvzParticles` owns state and scheduling, while the returned
visuals are views that may be attached to panels like ordinary visuals.


## Scene And Runtime Requirements

The scene layer needs these capabilities before a polished public particle API is stable:

1. persistent scene buffers that can be used as storage and render input;
2. FramePlan nodes that express compute-to-render dependencies for buffers;
3. resource transitions/barriers for compute-written buffers consumed by render passes;
4. stable runtime resource ids across frames so buffers are reused rather than recreated;
5. external-buffer registration and synchronization for CUDA/CuPy producer mode;
6. parameter-buffer updates that do not force particle-state uploads;
7. optional history-buffer ownership and ring-index update semantics;
8. visual pass capabilities for additive, source-over, WBOIT, and depth-tested views.


## Validation Targets

Initial tests:

1. compute shader writes particle positions and a point view renders them without CPU readback;
2. repeated frames reuse the same state buffer and runtime resource ids;
3. parameter-only updates do not re-upload particle state;
4. velocity streak view draws from current position and velocity data;
5. history ring buffer advances without unbounded allocation growth;
6. CuPy/CUDA external producer path writes a Vulkan-owned buffer that Datoviz renders after explicit
   synchronization when CUDA support is available.

Representative commands once implementation exists:

```text
just build
just test test_scene_particles
just test test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer
just test scene
git diff --check
```


## Implementation Order

1. Promote scene-level compute-written storage buffers consumed by render views.
2. Add a low-level particle example using the existing point or pixel visual path.
3. Add a particle state object that owns buffer lifetime and per-frame parameter updates.
4. Add point/billboard and sphere render views over the same state.
5. Add velocity streaks as the first track-like view.
6. Add optional history tracks with bounded ring-buffer storage.
7. Add CUDA/CuPy-facing Python wrappers around Vulkan-owned exported buffers.
8. Add selected-particle streamline/tube export after the vector visual lane has the required
   path/tube primitives.
