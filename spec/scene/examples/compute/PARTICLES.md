# GPU Particle System Compute Example

> **Agent Pickup**
> - **Category:** `compute`
> - **Implementation target:** Shader/compute-oriented example, preferably runnable with deterministic defaults.
> - **Data policy:** Generated or synthetic data by default; optional assets must have a cache and fallback.
> - **Preprocessing:** Document shader inputs, generated buffers/textures, and any optional Python preparation script.
> - **Validation:** Bounded smoke run, deterministic seed, and visual/readback criteria for the simulation state.


## Summary

Build a GPU particle-system example that uses a compute pass to update a persistent particle
storage buffer and renders the same buffer as a dense galaxy, vortex, or nebula-like scene. The
default data should be generated deterministically: particle positions, velocities, colors, sizes,
and optional palette or preset values must work without external downloads. The first practical
slice should use an O(N) analytic force field with a safe particle count, direct compute-to-render
resource flow, and a simple camera before adding stress presets, trails, or mouse forces. Validate
with a bounded smoke run using a fixed seed, confirming that particles move, remain finite, render
non-empty frames, and avoid GPU-to-CPU copies during animation.


See also [../../proposals/active/PARTICLE_SYSTEM_DESIGN.md](../../proposals/active/PARTICLE_SYSTEM_DESIGN.md) for the
scene-level particle-system design that separates GPU-updated simulation state, render views,
CUDA/CuPy producers, and optional track/trail consumers.

## Purpose

This example demonstrates how a Datoviz v0.4 scene can run a dynamic particle simulation in a GPU compute shader and render the same particle buffer directly, without any GPU-to-CPU readback or CPU-mediated updates after initialization.

The example should be a visually attractive, standalone Python example that works out of the box once the Datoviz v0.4 Python API is finalized. The exact Python API is intentionally not specified here. The implementation agent should adapt the concepts below to the actual v0.4 API, which is expected to be close in spirit to Datoviz v0.3 and to the Datoviz scene/DRP model.

The primary goal is to test and demonstrate:

- compute pass execution every frame;
- a GPU storage buffer used as persistent simulation state;
- rendering directly from the buffer written by compute;
- framegraph dependencies between compute and render passes;
- dynamic per-frame uniforms or push constants;
- transparent particle rendering;
- interactive control of simulation parameters;
- optional reset/reseed without restarting the application;
- no CPU copy of the particle state during animation.

Suggested filename for the future example:

```text
examples/features/compute_particles.py
```

Suggested documentation/example name:

```text
GPU particle system with compute shader
```

---

## Recommended concept

Use a **GPU galaxy / vortex particle simulation** rather than a true all-pairs N-body simulation.

A full O(N²) N-body simulation is expensive, harder to scale, and not necessary to demonstrate compute-to-render interop. Instead, each particle evolves independently under a small number of analytic forces:

- central gravitational attraction;
- angular disk rotation;
- weak damping;
- optional procedural turbulence;
- optional mouse-controlled attractor or repulsor;
- optional vertical oscillation for a 3D look.

This produces a convincing animated galaxy-like or nebula-like particle cloud while remaining simple, stable, and fast.

The simulation should support a large number of particles, for example:

```text
N = 100_000   # safe default
N = 1_000_000 # performance/stress preset
```

The visual should be attractive enough for a gallery example:

- dark background;
- transparent additive or alpha-blended particles;
- color based on radius, speed, age, or particle group;
- smooth rotation and swirling motion;
- optional arcball camera for 3D inspection;
- optional trails using either motion blur, fading accumulation, or short line segments if supported.

---

## Why not a true N-body simulation?

A true direct N-body simulation computes all pairwise interactions:

```text
for each particle i:
    acceleration_i = sum_j gravity(i, j)
```

This is O(N²), which becomes impractical beyond a few thousand particles unless using tiling, shared memory, approximation, or Barnes-Hut-like methods. Those techniques are interesting, but they would make the example primarily about numerical algorithms rather than Datoviz v0.4 resource flow.

For this example, prefer an **analytic force-field particle system**:

```text
for each particle i:
    read particle[i]
    update velocity from global force field
    update position
    write particle[i]
```

This is O(N), easy to parallelize, visually strong, and ideal for testing storage-buffer compute plus rendering.

A smaller optional mode may implement direct N-body for `N <= 4096`, but it should not be the main path.

---

## High-level visual result

The window shows a dense animated cloud of particles forming a rotating galaxy, accretion disk, vortex, or nebula.

Desired appearance:

- central bright core;
- spiral arms or vortex-like streaks;
- thousands to millions of semi-transparent particles;
- particles colored by radius and speed;
- smooth animation at interactive frame rates;
- camera orbit around the particle cloud;
- optional mouse interaction that attracts or repels particles.

The initial particle distribution should be procedural, so no external data is required.

Optional data from `datoviz/data` may be used for a palette, background star field, or preset configuration file, but the example must work without downloading anything.

---

## Simulation state

Use a single GPU storage buffer containing an array of particles.

Recommended particle structure:

```text
struct Particle {
    vec4 position_age;   // xyz = position, w = age or random phase
    vec4 velocity_mass;  // xyz = velocity, w = mass or particle kind
    vec4 color_size;     // rgba or rgb + size multiplier
};
```

Minimum viable structure:

```text
struct Particle {
    vec4 position; // xyz + size or age
    vec4 velocity; // xyz + mass or kind
};
```

The same buffer should be:

- read and written by the compute shader as a storage buffer;
- read by the render pipeline as a vertex or instance buffer, or as a read-only storage buffer depending on what the v0.4 API supports.

The important test is that the particle buffer is produced by compute and consumed by rendering in the same frame without CPU transfer.

---

## Initialization

Initial particle positions should be generated on the CPU once, then uploaded to the GPU.

Use a flattened rotating disk distribution:

```text
r     = R * sqrt(random())
theta = 2*pi*random()
z     = normal(0, disk_thickness)
x     = r*cos(theta)
y     = r*sin(theta)
```

Initial velocity should approximate circular orbit:

```text
v_tangent = sqrt(GM / (r + softening))
velocity = tangent_direction * v_tangent
```

Add random perturbations:

```text
velocity += random_normal_3d * velocity_noise
position += random_normal_3d * position_noise
```

Particle color may be initialized from radius:

```text
inner particles: warm yellow / white
middle particles: orange / pink
outer particles: blue / cyan
```

If the render shader can compute color procedurally from position/speed, the color field may be omitted from the buffer.

---

## Simulation model

Use a stable analytic force model.

For each particle:

```text
r = length(position.xy)
radial_dir = normalize(position.xy)
tangent_dir = vec2(-radial_dir.y, radial_dir.x)
```

Central attraction:

```text
acc_xy += -GM * position.xy / pow(r*r + softening*softening, 1.5)
```

Disk rotation / swirl correction:

```text
target_speed = swirl_strength / sqrt(r + softening)
target_velocity_xy = tangent_dir * target_speed
acc_xy += rotation_coupling * (target_velocity_xy - velocity.xy)
```

Damping:

```text
velocity *= exp(-damping * dt)
```

Vertical confinement:

```text
acc_z += -z_stiffness * position.z - z_damping * velocity.z
```

Optional procedural turbulence:

```text
acc += turbulence_strength * curl_noise(position * noise_scale + time)
```

If true curl noise is too much work, use a cheap deterministic pseudo-random vector field or skip turbulence.

Mouse attractor/repulsor:

```text
d = position - mouse_world_position
acc += mouse_strength * normalize(d) / (dot(d, d) + mouse_softening)
```

Use negative `mouse_strength` for attraction and positive for repulsion, depending on convention.

---

## Recommended parameters

Default values should be stable and visually pleasing:

```text
particle_count      = 200_000
G                   = 1.0
central_mass        = 10.0
softening           = 0.05
swirl_strength      = 1.2
rotation_coupling   = 0.5
damping             = 0.01
z_stiffness         = 0.2
z_damping           = 0.03
turbulence_strength = 0.02
time_step           = 0.002 to 0.01
steps_per_frame     = 1
point_size          = 1.0 to 3.0
```

Presets:

```text
Calm galaxy:
    central_mass = 8.0
    swirl_strength = 1.0
    turbulence_strength = 0.005
    damping = 0.01

Chaotic vortex:
    central_mass = 12.0
    swirl_strength = 1.8
    turbulence_strength = 0.05
    damping = 0.005

Nebula cloud:
    central_mass = 3.0
    swirl_strength = 0.5
    turbulence_strength = 0.08
    damping = 0.02

Accretion disk:
    central_mass = 20.0
    swirl_strength = 2.0
    z_stiffness = 0.5
    damping = 0.015
```

---

## GPU resources

Required resources:

```text
particles: storage buffer, also render-readable
params:    uniform buffer or push constants
```

Optional resources:

```text
indirect_draw: indirect draw buffer, if the example supports GPU-side particle culling
palette:       1D texture or small uniform array
```

The simplest implementation uses a single particle buffer updated in place by compute.

If the backend/API does not allow a buffer to be both storage and vertex in the same frame without aliasing concerns, use ping-pong particle buffers:

```text
particles_a: read by compute, rendered from previous frame
particles_b: written by compute, rendered after compute
swap(a, b)
```

However, the preferred design is one storage buffer with appropriate usage flags and implicit framegraph resource transitions.

---

## Framegraph structure

Per frame:

```text
Pass 1: COMPUTE particle_update
    read/write: particles
    read:       params
    dispatch:   ceil(N / workgroup_size), 1, 1

Pass 2: RENDER main_color
    read:       particles
    draw:       N particles
```

If ping-pong buffers are required:

```text
Pass 1: COMPUTE particle_update
    read:  particles_a
    write: particles_b

Pass 2: RENDER main_color
    read:  particles_b

End of frame:
    swap particles_a and particles_b
```

Optional third pass for trails:

```text
Pass 0: RENDER fade_previous_frame
    render a translucent black full-screen quad into an accumulation target

Pass 1: COMPUTE particle_update

Pass 2: RENDER particles_additive
    render particles into accumulation target

Pass 3: RENDER composite
    display accumulation target to swapchain
```

This trail mode is optional. The first implementation should avoid it unless render-to-texture is already stable.

---

## Rendering approach

Use one of the following, depending on the current Datoviz v0.4 capabilities.

### Preferred: point sprites / billboard particles

Render each particle as a point or billboard sprite.

Each particle contributes:

```text
position.xyz
size
color
alpha
```

The vertex shader reads particle data and transforms it with the panel camera MVP matrix.

The fragment shader should render soft circular splats:

```text
alpha = smoothstep(1.0, 0.0, length(point_coord * 2.0 - 1.0))
color *= alpha
```

Use additive or premultiplied alpha blending for a glowing particle cloud.

### Fallback: tiny instanced quads

If point size is not supported or is limited, render each particle as a small camera-facing quad using instancing.

Per-instance data comes from the particle buffer.

A static quad vertex buffer contains six vertices or four indexed vertices.

### Fallback: line segments

Render short line segments from current position and velocity:

```text
p0 = position
p1 = position - trail_scale * velocity
```

This can look very good and avoids point-size limitations, but requires either shader-generated segment endpoints or a derived render buffer.

---

## Transparency

The visual should use blending.

Recommended modes:

```text
Additive glow:
    src_color = src-alpha
    dst_color = one
    op_color  = add

Alpha blending:
    src_color = src-alpha
    dst_color = one-minus-src-alpha
    op_color  = add
```

Additive blending is visually effective for dense particle clouds, but it can saturate quickly. The shader should keep per-particle alpha low:

```text
alpha = 0.02 to 0.15
```

For very dense examples, color and alpha should scale approximately with particle count:

```text
alpha *= sqrt(200000 / particle_count)
```

---

## Camera

Use a 3D camera by default.

Recommended initial camera:

```text
position = (0, -4, 2)
target   = (0,  0, 0)
up       = (0,  0, 1)
fov      = 45 degrees
near     = 0.01
far      = 100
```

The example should support arcball/orbit interaction if available in the scene API.

If 3D camera support is not ready, use a 2D top-down camera and render `position.xy` only. The compute simulation can still maintain `z` internally.

---

## Interaction

Minimum controls:

```text
Space: pause/resume
R:     reset particles
1-4:   switch presets
+/-:   increase/decrease particle size
```

Mouse interaction:

```text
Left drag:  attract particles toward cursor-projected world position
Right drag: repel particles
Wheel:      zoom or camera dolly, if supported
```

Optional GUI sliders:

```text
particle count preset
central mass
swirl strength
damping
turbulence strength
time step
steps per frame
point size
alpha scale
```

The mouse world position can be approximate. A top-down projection onto the `z=0` plane is sufficient.

---

## Compute shader pseudocode

The exact shader language should follow the backend/API available at implementation time. WGSL is preferred conceptually because the Datoviz v0.4 DRP model is WebGPU-aligned, but GLSL/SPIR-V may be used for Vulkan-only prototyping.

```wgsl
struct Particle {
    position_age: vec4<f32>,
    velocity_mass: vec4<f32>,
    color_size: vec4<f32>,
};

struct Params {
    dt: f32,
    time: f32,
    particle_count: u32,
    flags: u32,

    central_mass: f32,
    softening: f32,
    swirl_strength: f32,
    rotation_coupling: f32,

    damping: f32,
    z_stiffness: f32,
    z_damping: f32,
    turbulence_strength: f32,

    mouse_pos: vec4<f32>,       // xyz + enabled
    mouse_strength: f32,
    mouse_softening: f32,
    _pad0: vec2<f32>,
};

@group(0) @binding(0)
var<storage, read_write> particles: array<Particle>;

@group(0) @binding(1)
var<uniform> params: Params;

@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;
    if (i >= params.particle_count) {
        return;
    }

    var p = particles[i];
    var pos = p.position_age.xyz;
    var vel = p.velocity_mass.xyz;

    let r2 = dot(pos.xy, pos.xy) + params.softening * params.softening;
    let r = sqrt(r2);

    var acc = vec3<f32>(0.0, 0.0, 0.0);

    // Central softened gravity.
    acc.xy += -params.central_mass * pos.xy / (r2 * r);

    // Swirl/circular orbit relaxation.
    let radial = pos.xy / max(r, 1e-4);
    let tangent = vec2<f32>(-radial.y, radial.x);
    let target_speed = params.swirl_strength / sqrt(r + params.softening);
    let target_vel = tangent * target_speed;
    acc.xy += params.rotation_coupling * (target_vel - vel.xy);

    // Vertical confinement to keep a disk-like shape.
    acc.z += -params.z_stiffness * pos.z - params.z_damping * vel.z;

    // Optional mouse force.
    if (params.mouse_pos.w > 0.5) {
        let d = pos - params.mouse_pos.xyz;
        let d2 = dot(d, d) + params.mouse_softening;
        acc += params.mouse_strength * normalize(d) / d2;
    }

    // Simple damping.
    vel += params.dt * acc;
    vel *= exp(-params.damping * params.dt);
    pos += params.dt * vel;

    // Optional soft respawn if particle escapes too far.
    let max_radius = 8.0;
    if (length(pos) > max_radius) {
        // The real implementation should use a deterministic hash from i and time.
        pos = respawn_position(i, params.time);
        vel = respawn_velocity(pos, i, params.time);
    }

    p.position_age = vec4<f32>(pos, p.position_age.w + params.dt);
    p.velocity_mass = vec4<f32>(vel, p.velocity_mass.w);
    particles[i] = p;
}
```

The helper functions `respawn_position()` and `respawn_velocity()` are placeholders. The implementation can either omit respawning or use a simple deterministic hash function in the compute shader.

---

## Vertex shader pseudocode

```wgsl
struct Particle {
    position_age: vec4<f32>,
    velocity_mass: vec4<f32>,
    color_size: vec4<f32>,
};

struct Camera {
    mvp: mat4x4<f32>,
};

@group(0) @binding(0)
var<storage, read> particles: array<Particle>;

@group(0) @binding(1)
var<uniform> camera: Camera;

struct VSOut {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) speed: f32,
};

@vertex
fn main(@builtin(vertex_index) vertex_index: u32) -> VSOut {
    let p = particles[vertex_index];
    let pos = p.position_age.xyz;
    let vel = p.velocity_mass.xyz;

    var out: VSOut;
    out.position = camera.mvp * vec4<f32>(pos, 1.0);

    let r = length(pos.xy);
    let speed = length(vel);
    out.speed = speed;
    out.color = particle_palette(r, speed, p.color_size.w);
    return out;
}
```

If using instanced quads, use `instance_index` to select the particle and `vertex_index` to select the quad corner.

---

## Fragment shader pseudocode

For point sprites:

```wgsl
@fragment
fn main(in: VSOut, @builtin(point_coord) pc: vec2<f32>) -> @location(0) vec4<f32> {
    let q = pc * 2.0 - vec2<f32>(1.0, 1.0);
    let d = length(q);
    let a = smoothstep(1.0, 0.0, d);
    return vec4<f32>(in.color.rgb * a, in.color.a * a);
}
```

If `point_coord` or programmable point size is unavailable, use instanced quads and pass local quad coordinates explicitly.

---

## API-agnostic implementation outline

The Python example should follow this structure conceptually:

```text
1. Create app/window/canvas.
2. Create a scene and one panel.
3. Configure a 3D camera and orbit controller if available.
4. Generate initial particle data on CPU.
5. Create a GPU/scene resource for the particle buffer.
6. Create a uniform/push-constant resource for simulation parameters.
7. Create or register compute shader/pipeline.
8. Create or register particle render shader/pipeline/visual.
9. Build framegraph:
       compute pass writes particle buffer
       render pass reads particle buffer
10. On every frame:
       update time and input-dependent params
       dispatch compute pass
       render particles
11. On reset:
       regenerate initial particles on CPU and upload once.
```

The implementation should not assume final function names. It should use the actual v0.4 Python scene API available at implementation time.

---

## Expected Datoviz v0.4 features exercised

This example should exercise:

- storage buffer creation;
- buffer usage as compute storage and render input;
- compute shader module creation;
- compute pipeline creation;
- compute dispatch;
- render pipeline creation;
- transparent blending;
- framegraph pass ordering;
- per-frame parameter updates;
- camera matrices;
- interaction events;
- optional GPU/DRP resource lifetime reuse across frames.

This is complementary to a texture-based reaction-diffusion example. Reaction-diffusion tests compute-written textures sampled by rendering; this particle example tests compute-written buffers consumed by vertex/instance rendering.

---

## Validation criteria

The example is successful if:

1. Particles move continuously under the compute shader.
2. Rendering uses the GPU-updated particle buffer directly.
3. There is no per-frame CPU readback.
4. The framegraph clearly contains a compute pass before the render pass.
5. Changing simulation parameters affects the next frames immediately.
6. Resetting particles performs a one-time CPU upload, not continuous CPU animation.
7. The example remains interactive with at least 100k particles on a normal discrete GPU.
8. The visual result is attractive enough for a gallery/demo page.

Optional performance targets:

```text
100k particles: smooth on integrated or modest discrete GPU
1M particles:   smooth on high-end discrete GPU
```

---

## Possible variants

### Variant A: analytic galaxy / vortex

Recommended default.

Pros:

- simple O(N) compute;
- visually compelling;
- stable;
- works with 100k to 1M particles;
- excellent Datoviz compute-buffer-render test.

Cons:

- not a physically exact N-body simulation.

### Variant B: direct N-body mini demo

Use `N = 1024` to `4096` particles and compute pairwise gravity.

Pros:

- physically recognizable;
- good stress test for compute shader loops.

Cons:

- O(N²), unsuitable for large visual particle counts;
- harder to make visually dense;
- distracts from the main v0.4 rendering goal.

This can be an optional mode, not the default.

### Variant C: boids / flocking

Particles follow local alignment, cohesion, and separation rules.

Pros:

- interactive and organic;
- good dynamic particle system.

Cons:

- efficient neighbor search is nontrivial;
- naive all-pairs boids has the same scaling issue as N-body.

Avoid for the first compute particle example.

### Variant D: flow-field particles

Particles advect through an analytic vector field or curl noise.

Pros:

- extremely simple;
- very stable;
- beautiful with trails.

Cons:

- slightly less physically intuitive;
- best appearance often requires accumulation/trail rendering.

Good fallback if galaxy forces prove unstable.

---

## Final recommendation

Implement **Variant A: analytic galaxy / vortex** as the main `PARTICLES.md` example specification.

It provides the best balance:

```text
simple compute shader
large particle count
strong visual appeal
interactive camera and mouse control
no external data dependency
clear compute-buffer-to-render dataflow
excellent stress test for Datoviz v0.4 scene/DRP integration
```
