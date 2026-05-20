# CFD Vorticity And Particle Advection

## Summary

Example name: `CFD_VORTICITY_ADVECTION`

Build a CFD scene showing a von Karman vortex street as coupled scalar, vector, particle, path,
and probe views. Stage 1 uses deterministic synthetic flow. A later stage may load a prepared CFD
cache. The first slice should render one 2D panzoom flow panel with an animated vorticity image,
advected tracer particles, a circular cylinder, play/pause, and hover readout for velocity and
vorticity.

The example is distinct from reaction-diffusion, standalone particles, and geoscience wind-field
showcases: one flow state drives vorticity, tracers, optional streamlines, probes, and diagnostics.

## Feature Pressure

- Dynamic image or sampled-field updates for vorticity.
- Dynamic point buffers for 20k+ passive tracers.
- Optional path updates for trails or streamlines.
- Static obstacle geometry reused across frames.
- Shared animation time for field, particles, and diagnostics.
- Probe overlays that update without reuploading the full field.
- A future GPU-compute path that does not change scene semantics.

## Data And Resources

Stage 1 requires no external data. Generate a deterministic wake:

- domain `[0, 8] x [-1.5, 1.5]`;
- defaults: `nx = 512`, `ny = 192`, `n_particles = 20000`;
- cylinder center `(1.4, 0.0)`, radius `0.22`;
- uniform inflow, circular obstacle mask, alternating Gaussian vortices, approximate induced
  velocity, and clamped velocities near vortices/obstacle.

Logical arrays:

```text
velocity        float32[ny, nx, 2]
vorticity       float32[ny, nx]
obstacle_mask   uint8[ny, nx]
particles       float32[n_particles, 2]
particle_age    float32[n_particles]
```

The exact flow formula is not normative. The required property is a deterministic coherent
vortex-shedding pattern suitable for visual and interaction tests.

Stage 2 may load:

```text
~/.cache/datoviz/cfd/vortex_street/
  metadata.json
  velocity_f32.bin       # nt x ny x nx x 2
  vorticity_f32.bin      # nt x ny x nx
  obstacle_mask_u8.bin   # ny x nx
  diagnostics_f32.bin    # optional nt x n_metrics
```

`metadata.json` records source, case name, dimensions, domain bounds, `dt`, Reynolds number,
array layouts, and diagnostic names. Runtime falls back to Stage 1 generation when the cache is
missing.

## Scene And Runtime Behavior

Recommended layout:

```text
+--------------------------------------------------------------+
| flow panel: vorticity image + obstacle + particles + paths    |
+--------------------------------------------------------------+
| optional diagnostics: kinetic energy, enstrophy, probe speed  |
+--------------------------------------------------------------+
```

Encodings:

| Element | Visual behavior |
|---|---|
| Vorticity | signed scalar image with symmetric diverging colormap |
| Particles | small partially transparent points, optionally speed/age colored |
| Trails/streamlines | optional translucent `path` visuals |
| Obstacle | filled primitive or mesh circle |
| Probe | crosshair, velocity arrow, and numeric annotation |
| Diagnostics | path/point plots linked to animation time |

Tracer particles use bilinear velocity sampling or direct analytic evaluation. RK2 integration is
preferred. Particles leaving the domain or entering the obstacle respawn near the inflow boundary
with deterministic jitter.

Controls should cover case selection, play/pause, time, speed, particle count, trails, trail
length, streamlines, color mode, vorticity scale, probe visibility, particle reset, and capture.

Probe readout reports:

```text
x, y, u, v, speed, vorticity, inside_obstacle, time
```

Frame-plan shape:

- static setup uploads vorticity, particles, obstacle, optional paths, and diagnostics;
- animation frames update vorticity, particles, optional trails, and diagnostic cursors only;
- probe movement updates crosshair/arrow/annotation overlays and optional probe-history points;
- obstacle, axes, colorbar, and static style resources are not reuploaded each frame.

Expected DRP2 categories: image/sampled-field resources, dynamic buffers, image/point/path/
primitive draws, panzoom updates, optional pick/readback requests, and optional app/canvas capture.

Future GPU compute may keep velocity/vorticity and particles in GPU resources, with compute nodes
feeding image and point draws. Radius-bearing streamlines should lower to [`tube`](../../visuals/TUBE.md),
not a CFD-specific renderer.

## Minimal Target

1. Generate deterministic CPU-side vortex-street fields.
2. Render one 2D panzoom panel.
3. Animate the vorticity image and tracer point buffer.
4. Draw the circular obstacle.
5. Add play/pause and a hover probe.
6. Add diagnostics only after the core animation is stable.

If generation is too expensive, reduce resolution or particle count before changing the scene
architecture.

## Validation

- Smoke-run the example with a bounded frame count.
- Confirm alternating red/blue vortices shed downstream of the cylinder.
- Confirm particles advect coherently and respawn deterministically.
- Confirm probe values match the flow state used by particles and image rendering.
- Confirm probe-only updates do not trigger full field reupload.
- Confirm static obstacle/style resources remain stable across animation frames.
- Report frame rate and rendered particle count for the default configuration.

Shared policies: [`FRAME_PLAN.md`](../../pipeline/FRAME_PLAN.md), [`SCALES.md`](../../semantics/SCALES.md),
[`PATH.md`](../../visuals/PATH.md), and [`TUBE.md`](../../visuals/TUBE.md).
