# Tokamak Plasma Field Lines

## Summary

Example name: `TOKAMAK_PLASMA_FIELD_LINES`

Build a fusion/plasma scene that visualizes a tokamak equilibrium with transparent flux surfaces,
helical magnetic field lines, scalar slice or surface coloring, optional tracer motion, and linked
radial profiles. Stage 1 uses deterministic synthetic toroidal geometry. A later stage may load a
prepared equilibrium cache. The first slice should render one 3D arcball viewport with flux-surface
meshes, colored field-line paths, one scalar cross-section, and selected-line or surface readout.

The scientific identity is:

```text
tokamak equilibrium -> field lines -> flux surfaces -> scalar fields -> profile/probe links
```

## Feature Pressure

- Long 3D `path` rendering for helical magnetic field lines.
- Transparent mesh surfaces with depth and OIT participation.
- Scalar colormaps on slices or surfaces.
- Arcball camera interaction over dense 3D geometry.
- Linked 3D selection and 2D radial profile markers.
- Small dynamic point updates for optional tracer animation.
- Visibility and LOD controls for many line/surface objects.

## Data And Resources

Stage 1 uses an analytic toroidal model:

```text
R = R0 + r * cos(theta)
Z = r * sin(theta)
x = R * cos(phi)
y = R * sin(phi)
z = Z

theta(phi) = theta0 + phi / q(r)
q(r) = q0 + (q_edge - q0) * (r / a)^2
```

Recommended defaults:

```text
R0 = 2.0
a = 0.65
q0 = 1.0
q_edge = 4.0
n_flux_surfaces = 6
n_field_lines = 64
n_line_samples = 512
n_phi_turns = 5 to 8
```

Scalar profiles:

```text
rho = r / a
temperature(rho) = T_edge + (T_core - T_edge) * (1 - rho^2)^alpha
density(rho)     = n_edge + (n_core - n_edge) * (1 - rho^2)^beta
q(rho)           = q0 + (q_edge - q0) * rho^2
psi_n            = rho^2
```

Stage 2 may load:

```text
~/.cache/datoviz/tokamak/synthetic_equilibrium/
  metadata.json
  field_line_position_f32.bin    # n_lines x n_samples x 3
  field_line_offset_u32.bin      # optional packed path offsets
  field_line_scalar_f32.bin
  flux_surface_position_f32.bin
  flux_surface_index_u32.bin
  flux_surface_scalar_f32.bin
  slice_position_f32.bin         # optional poloidal slice
  slice_index_u32.bin
  slice_scalar_f32.bin
  profile_radius_f32.bin
  profile_temperature_f32.bin
  profile_density_f32.bin
  profile_q_f32.bin
```

`metadata.json` records case/source, `R0`, `a`, line/surface counts, safety-factor parameters,
array layouts, and profile units. Runtime should not require a real equilibrium parser.

## Scene And Runtime Behavior

Recommended layout:

```text
+--------------------------------------------------------------+
| 3D tokamak: flux surfaces, field lines, slice, tracers        |
+--------------------------------------------------------------+
| radial profile panel: temperature / density / q vs radius     |
+--------------------------------------------------------------+
```

Encodings:

| Element | Visual behavior |
|---|---|
| Flux surfaces | transparent meshes, color by `psi_n` or temperature |
| Field lines | colored 3D paths, color by `q`, `psi_n`, or local scalar |
| Slice | scalar-colored mesh or image-like poloidal cross-section |
| Vessel/limiter | optional path or mesh overlay |
| Tracers | small points animated along selected field-line samples |
| Selection | brighter line/surface plus profile marker |

The active field-line target is [`path`](../../visuals/PATH.md). Future radius-bearing lines should
use [`TUBE.md`](../../visuals/TUBE.md), not a tokamak-specific renderer.

Required interactions: arcball/orbit camera, select/cycle highlighted field line, show/hide flux
surfaces, show/hide scalar slice, color-mode selector, and selected-object readout. Preferred
interactions add click selection, tracers, visible-line count, surface opacity, profile selection,
and reset camera.

Readout reports object type/id, `psi_n`, `rho`, `q`, temperature, density, and line length or
sampled turns. Selection updates the 3D highlight, radial profile marker, and optional tracer
target.

Tracer animation samples precomputed paths:

```text
sample_index(t) = (sample_index0 + speed * t) mod n_line_samples
```

Only tracer point positions update during tracer animation.

Frame-plan shape:

- static setup uploads field-line paths, flux-surface meshes, scalar slice, and profile geometry;
- selection updates highlight/style resources and profile marker only;
- tracer frames update tracer point positions only;
- visibility/density changes update visibility/style buffers when possible, or reduced path
  geometry only when necessary.

Expected DRP2 categories: path, mesh, point, and profile buffers/draws; dynamic selection/tracer
uploads; arcball panel transforms; optional pick/readback requests; optional capture/video.

## Minimal Target

1. Generate synthetic field lines and flux surfaces on the CPU.
2. Render field lines as [`path`](../../visuals/PATH.md).
3. Render flux surfaces with low opacity, or wireframe paths until transparency is stable.
4. Render one scalar slice or skip it until mesh scalar coloring is stable.
5. Add selected-line/surface readout.
6. Add tracer animation and profile linkage after static geometry is correct.

Keep geometry generation separate from rendering setup so a prepared cache can replace it later.

## Validation

- Smoke-run the example with a bounded frame count.
- Confirm the view reads as nested tokamak flux surfaces plus helical field lines.
- Confirm scalar colors and profile marker agree with selected `rho`/`psi_n`.
- Confirm selection-only updates do not reupload full line/surface geometry.
- Confirm tracer frames update only tracer point buffers.
- Confirm arcball interaction remains smooth with the default line/surface counts.
- Report line count, sample count, triangle count, and frame rate.
