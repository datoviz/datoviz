# GPU Particle System Compute Example

> **Example status:** informative pressure test
> **Target:** Python compute/render example
> **Data:** deterministic generated particles
> **Validation:** bounded smoke, fixed seed, movement/readback or screenshot checks

## Summary

Build a GPU particle-system example where a compute pass updates a persistent particle storage
buffer and the render pass consumes that same buffer directly. The default visual is an analytic
galaxy/vortex rather than true all-pairs N-body, so the example scales to large particle counts
while staying focused on Datoviz compute-buffer-to-render flow.

See also [particle system design](../../proposals/active/PARTICLE_SYSTEM_DESIGN.md).

Status note on 2026-05-27: this GPU-driven version remains deferred until scene-level compute and
compute-written render inputs are first-class. A v0.4 stretch particle/fluid showcase should use
CPU-side bounded updates to existing point, path, or image visuals instead of claiming GPU particle
simulation support.

## User-Visible Result

- Dark 3D scene with a bright rotating core, spiral/vortex streaks, and semi-transparent particles.
- Default particle count around `200_000`; stress preset up to `1_000_000`.
- Color based on radius, speed, age, or particle group.
- Additive or premultiplied alpha blending.
- Arcball/orbit camera if available; top-down fallback is acceptable.
- Controls for pause, reset, presets, particle size, simulation parameters, and optional mouse
  attraction/repulsion.

## Feature Pressure Points

- Storage buffer written by compute and read by rendering in the same frame.
- Framegraph dependency from compute pass to render pass.
- Dynamic per-frame uniform or push-constant updates.
- Transparent particle blending at high instance counts.
- Optional single-buffer storage+vertex usage; ping-pong fallback if required by backend rules.
- Reset/reseed as one CPU upload, not CPU animation.
- Optional trails through accumulation targets or path semantics.

## Required Data And Resources

No external data is required. Initial particles are generated once with a fixed seed:

```text
r = R * sqrt(random())
theta = 2*pi*random()
z = normal(0, disk_thickness)
x = r*cos(theta)
y = r*sin(theta)
velocity = tangent_direction * sqrt(GM / (r + softening)) + noise
```

Recommended particle layout:

```text
position_age  vec4  # xyz position, w age or phase
velocity_mass vec4  # xyz velocity, w mass or kind
color_size    vec4  # rgba or rgb + size multiplier
```

Minimum layout:

```text
position vec4
velocity vec4
```

Recommended default parameters:

```text
particle_count      = 200_000
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

Presets: calm galaxy, chaotic vortex, nebula cloud, and accretion disk.

## Scene Shape And Runtime Behavior

Frame shape:

```text
Pass 1: COMPUTE particle_update
    read/write particles
    read params
    dispatch ceil(N / workgroup_size), 1, 1

Pass 2: RENDER particles
    read particles
    draw N point sprites, billboards, or short velocity segments
```

If one buffer cannot safely be storage and render input:

```text
compute particles_a -> particles_b
render particles_b
swap(a, b)
```

Simulation model:

- Softened central attraction.
- Tangential velocity relaxation for disk rotation.
- Damping.
- Vertical confinement.
- Optional procedural turbulence.
- Optional mouse attractor/repulsor projected onto the `z=0` plane.
- Optional deterministic respawn for particles escaping a maximum radius.

Rendering options:

| Path | Use |
|---|---|
| Point sprites / billboard particles | Preferred default |
| Instanced quads | Fallback when point size or point coordinates are limited |
| Velocity line segments | Attractive fallback and trail-like mode |

Controls:

```text
Space pause/resume
R reset particles
1-4 switch presets
+/- particle size
Left drag attract
Right drag repel
Wheel zoom or dolly
```

## Minimal Implementation Target

- One scene and one 3D panel.
- Deterministic CPU initialization and one upload.
- One particle storage buffer, or ping-pong pair if required.
- One compute shader implementing the analytic force model.
- One render visual reading compute-updated particles directly.
- Additive blending with finite alpha scaling.
- Pause, reset, and at least two presets.
- No per-frame CPU readback or CPU-side particle integration.

## Validation / Acceptance Criteria

- Particles move continuously under compute.
- Rendering consumes the GPU-updated particle buffer directly.
- Reset performs one CPU upload; animation remains GPU-driven.
- Parameter changes affect subsequent frames.
- No NaN/inf positions after a bounded smoke run with fixed seed.
- At least `100k` particles are interactive on a normal discrete GPU.
- Optional `1M` stress mode remains usable on high-end hardware.
- The framegraph clearly orders compute before render.

## Links

- [Shared example policies](../POLICIES.md)
- [Particle system design](../../proposals/active/PARTICLE_SYSTEM_DESIGN.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [Path visual](../../visuals/PATH.md)
- [Tube visual](../../visuals/TUBE.md)
- [DRP2 specs](../../../drp2/)
