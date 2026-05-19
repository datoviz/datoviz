# CFD Vorticity And Particle Advection

> **Agent Pickup**
> - **Category:** `physics`
> - **Implementation target:** Scientific domain example with staged implementation, starting from deterministic or prepared data.
> - **Data policy:** Synthetic or prepared-cache first slice; real public data may be a second stage with license notes.
> - **Preprocessing:** Document any data conversion script, output schema, coordinate normalization, and cache validation.
> - **Validation:** Smoke run plus domain-specific visual, picking/probe, and performance acceptance criteria.

## Summary

Build a CFD visualization scene example showing a von Karman vortex street as a coupled scalar and
particle-advection view: vorticity image, velocity-driven tracers, obstacle geometry, optional
streamlines, and probes or diagnostics. Stage 1 should use deterministic synthetic flow generated
from an analytic or semi-analytic vortex-street model, with a later prepared-cache path for real CFD
frames using velocity, vorticity, obstacle, and diagnostics arrays. The first practical slice should
create one 2D panzoom flow panel, animate the vorticity field and tracer particles, render the
circular cylinder, provide play/pause, and expose a hover probe for velocity and vorticity. Validate
with smoke execution, fluid-specific visual checks, probe checks, and staged performance criteria.


## Example Name

`CFD_VORTICITY_ADVECTION`


## Purpose

Specify a Datoviz v0.4 showcase example for computational fluid dynamics visualization. The example
renders a time-dependent 2D flow as a coupled scalar/vector-field scene:

- vorticity as a scalar image field,
- velocity-driven tracer particles,
- optional streamlines or pathlines,
- obstacle geometry,
- field probes and linked diagnostics.

This example should not duplicate the existing compute or particle showcases. Its identity is the
fluid-dynamics relationship between one velocity field and several derived views.


## Relationship To Nearby Examples

This example is distinct from:

- `GRAY_SCOTT.md`, which is a scalar reaction-diffusion compute example,
- `PARTICLES.md`, which is a standalone particle simulation,
- `SHOWCASE_WIND_FIELD.md`, which is a global geoscience vector-field visualization.

The differentiator here is:

```text
flow state -> vorticity image + advected tracers + streamlines + probes + diagnostics
```

The example may eventually use GPU compute, but the first version can be a deterministic CPU-side
or cached-data animation as long as it exercises the scene resources and update pattern.


## Recommended Default Scenario

Use a von Karman vortex street behind a circular cylinder.

Visual target:

```text
inflow --->  O  ~~~~~ alternating red/blue vortices downstream
             cylinder
```

Rationale:

- it is immediately recognizable as CFD,
- it has a clear scalar field: signed vorticity,
- it has a clear vector field: velocity,
- particles make the flow structure legible,
- the obstacle gives the scene physical context,
- the result is visually strong in a still screenshot and in animation.


## User-Facing Scenario

The default view should show a rectangular flow domain with:

- a dark or neutral background,
- a circular cylinder near the left side,
- a diverging vorticity field, red for one sign and blue for the other,
- thousands of small tracer particles moving downstream,
- optional streamlines seeded upstream,
- a probe cursor that reports velocity, speed, and vorticity,
- a small diagnostics panel showing kinetic energy and enstrophy over time.

The scene should look like a compact CFD post-processing tool rather than an abstract particle demo.


## Scene Layout

Recommended layout:

```text
+------------------------------------------------------------------+
| CFD flow panel                                                    |
| vorticity image + obstacle + particles + optional streamlines      |
+------------------------------------------------------------------+
| diagnostics panel                                                 |
| kinetic energy / enstrophy / probe history                        |
+------------------------------------------------------------------+
```

Minimum viable version:

1. one 2D flow panel with panzoom,
2. animated vorticity image,
3. animated tracer particles,
4. circular obstacle geometry,
5. play/pause time animation,
6. hover probe for velocity/vorticity.

Preferred fuller version:

1. vorticity image field,
2. particle positions updated every frame,
3. fading particle trails or pathlines,
4. optional streamlines for the current velocity frame,
5. linked diagnostics panel,
6. interactive controls for particle count, trail length, and field strength.


## Data Strategy

### Stage 1: Deterministic Synthetic Flow

The first implementation should work without external data.

Use a deterministic analytic or semi-analytic vortex-street generator:

- uniform inflow from left to right,
- circular obstacle mask,
- alternating Gaussian vortices shed downstream,
- weak background perturbation,
- velocity induced by approximate vortex fields,
- vorticity sampled on a regular grid,
- tracer particles integrated through the velocity field.

This avoids implementing a full Navier-Stokes solver while still producing a scientifically
recognizable flow.


### Stage 2: Prepared CFD Cache

A later preparation script may generate or load real CFD frames and export a compact cache.

Suggested cache layout:

```text
~/.cache/datoviz/cfd/vortex_street/
  metadata.json
  velocity_f32.bin       # nt x ny x nx x 2
  vorticity_f32.bin      # nt x ny x nx
  obstacle_mask_u8.bin   # ny x nx
  diagnostics_f32.bin    # optional nt x n_metrics
```

Recommended metadata:

```text
source
case_name
nx
ny
nt
domain_xmin
domain_xmax
domain_ymin
domain_ymax
dt
reynolds_number
velocity_layout
vorticity_layout
diagnostic_names
```

The runtime example should load prepared arrays and interpolate frames if available. It should fall
back to the deterministic generator if the cache is missing.


## Synthetic Flow Model

The synthetic model should generate a visually plausible wake.

Logical field state:

```text
grid_x          float32[nx]
grid_y          float32[ny]
velocity        float32[ny, nx, 2]
vorticity       float32[ny, nx]
obstacle_mask   uint8[ny, nx]
particles       float32[n_particles, 2]
particle_age    float32[n_particles]
```

Recommended defaults:

```text
nx = 512
ny = 192
n_particles = 20_000
domain = [0, 8] x [-1.5, 1.5]
cylinder_center = (1.4, 0.0)
cylinder_radius = 0.22
```

For each animation time `t`, generate alternating vortex centers downstream:

```text
x_k = cylinder_x + wake_offset + k * spacing - convection_speed * phase
y_k = sign(k) * wake_amplitude
strength_k = sign(k) * vortex_strength
```

The vorticity field can be approximated as a sum of signed Gaussian blobs plus a near-cylinder
shear layer. The velocity field can be approximated by uniform inflow plus softened point-vortex
induced velocities. Clamp extreme velocities near vortex centers and inside the obstacle.

The exact formula is not normative. The important requirement is a deterministic, coherent
vortex-shedding pattern suitable for testing visualization.


## Particle Advection

Particles should be passive tracers advected by the velocity field:

```text
dx/dt = u(x, y, t)
dy/dt = v(x, y, t)
```

Use bilinear sampling from the velocity grid or direct analytic evaluation. RK2 integration is a
good first choice:

```text
k1 = velocity(p, t)
k2 = velocity(p + 0.5 * dt * k1, t + 0.5 * dt)
p_next = p + dt * k2
```

Particles that leave the domain should be respawned near the inflow boundary with deterministic
jitter. Particles entering the obstacle should also be respawned or pushed to a valid position.

Optional trails:

- store the previous `N` positions per displayed trail particle,
- render trails through the active [`path`](../../visuals/PATH.md) visual,
- fade trail alpha with age.

Trails should be optional because they increase buffer size and update cost.


## Visual Encodings

Flow panel:

```text
vorticity image   signed scalar field with diverging colormap
particles         small points, white/cyan or speed-colored
trails            translucent paths, optional
streamlines       thin paths seeded upstream; future tubes can follow `TUBE.md`
obstacle          filled primitive or mesh circle
probe             crosshair and velocity arrow
```

Vorticity colormap:

- negative vorticity: blue/cyan,
- near zero: dark gray or neutral,
- positive vorticity: orange/red,
- symmetric color domain around zero.

Particle styling:

- small point size,
- partial opacity,
- optional color by local speed or age,
- avoid obscuring the vorticity field.


## Diagnostics Panel

The linked diagnostics panel should be optional in the first implementation but is part of the
preferred full version.

Suggested metrics:

```text
kinetic_energy(t) = mean(u^2 + v^2)
enstrophy(t)      = mean(vorticity^2)
probe_speed(t)    = speed at current probe location
```

The panel should share the animation time cursor with the flow panel. Selecting a time in the
diagnostics panel may eventually seek the flow animation.


## Controls

Recommended controls:

```text
Case:             Vortex street / lid-driven cavity / synthetic turbulence / cached CFD
Play:             checkbox
Time:             slider
Speed:            slider
Particles:        slider or preset
Trails:           checkbox
Trail length:     slider
Streamlines:      checkbox
Color by:         Vorticity / Speed / Particle age
Vorticity scale:  slider
Probe:            checkbox
Reset particles:  button
Export clip:      button or command-line option
```


## Picking And Probe Readout

Hover probe should map pointer position to flow-domain coordinates and report:

```text
x, y
u, v
speed
vorticity
inside_obstacle
time
```

The probe may show:

- a crosshair,
- a small velocity arrow,
- a numeric annotation,
- a point in the diagnostics panel recording probe speed over time.

The probe should sample the same flow state used by particles and vorticity rendering.


## FramePlan Shape

### Static Setup

Initial frame:

```text
UploadNode  -> vorticity image or sampled field
UploadNode  -> particle positions and colors
UploadNode  -> obstacle geometry
UploadNode  -> optional streamline/trail geometry
UploadNode  -> diagnostics panel data
RenderNode  -> flow panel
RenderNode  -> diagnostics panel, if enabled
```


### Animated Frame

Each animation frame updates the current flow state and particles:

```text
UploadNode  -> vorticity image or sampled field for current frame
UploadNode  -> particle positions
UploadNode  -> optional trail/pathline vertices
UploadNode  -> diagnostics time cursor
RenderNode  -> flow panel
RenderNode  -> diagnostics panel, if enabled
```

The obstacle geometry, static axes, colorbar, and most style resources should not be re-uploaded
each frame.


### Probe Frame

When the probe moves:

```text
UploadNode  -> probe crosshair, arrow, and annotation geometry
UploadNode  -> optional probe-history point in diagnostics panel
RenderNode  -> affected panels
```

No full field reupload should be required for a probe-only update.


## Future GPU Compute Path

The first implementation may update the field and particles on the CPU, but the example is a natural
candidate for a later GPU compute path:

1. velocity/vorticity or simulation state in GPU textures,
2. particle storage buffer updated by compute,
3. image visual samples the vorticity texture,
4. point visual renders particles from the compute-written buffer,
5. framegraph dependency from compute pass to render pass.

This path should reuse the same visual semantics rather than introducing a parallel renderer.
Radius-bearing streamlines should lower to the future [`tube`](../../visuals/TUBE.md) visual rather
than becoming a CFD-specific renderer.


## DRP2 Command Categories

The example is expected to require:

- image or sampled-field resources for vorticity,
- buffers for particle positions, colors, optional trails, and diagnostics,
- repeated uploads for animated field and particle state,
- draw commands for image, point, path, and primitive/mesh visuals,
- panel transform updates for panzoom,
- optional readback/pick requests for probe handling,
- optional capture/video commands through the app/canvas layer.


## Implementation Notes

The first C implementation can be deliberately practical:

1. generate a deterministic synthetic vortex street,
2. update vorticity and particles on the CPU each frame,
3. upload one image field and one particle position buffer per frame,
4. draw a simple obstacle primitive,
5. add probe and diagnostics only after the core animation is stable.

If CPU generation becomes too expensive, reduce field resolution or particle count before changing
the architectural shape.


## Key Pressure On The Scene Spec

This example checks that Datoviz v0.4 can express a coupled scientific field workflow where:

- one flow state drives scalar images, particles, paths, and probes,
- animation updates dynamic resources without rebuilding static overlays,
- particles and field images stay synchronized in time,
- linked diagnostics panels can share the same animation clock,
- the same scene shape can later move from CPU updates to GPU compute.
