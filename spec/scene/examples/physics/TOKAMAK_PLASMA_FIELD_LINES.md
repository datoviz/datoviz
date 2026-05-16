# Tokamak Plasma Field Lines

> **Agent Pickup**
> - **Category:** `physics`
> - **Implementation target:** Scientific domain example with staged implementation, starting from deterministic or prepared data.
> - **Data policy:** Synthetic or prepared-cache first slice; real public data may be a second stage with license notes.
> - **Preprocessing:** Document any data conversion script, output schema, coordinate normalization, and cache validation.
> - **Validation:** Smoke run plus domain-specific visual, picking/probe, and performance acceptance criteria.

## Summary

Build a fusion/plasma scene example that visualizes a tokamak equilibrium with transparent flux
surfaces, helical magnetic field lines, a scalar slice or surface coloring, optional tracer motion,
and linked radial profiles. Stage 1 should use deterministic synthetic toroidal geometry driven by
major/minor radius, safety-factor profile, flux surfaces, and radial scalar profiles; later prepared
equilibrium data can export the same field-line, surface, slice, and profile cache arrays. The first
practical slice should render one 3D arcball viewport with flux-surface meshes, colored field-line
paths, one scalar cross-section, and selected-line or surface readout. Validate with smoke execution,
plasma-specific visual checks, picking/probe checks, and staged performance criteria.


## Example Name

`TOKAMAK_PLASMA_FIELD_LINES`


## Purpose

Specify a Datoviz v0.4 showcase example for fusion and plasma-physics visualization. The example
renders a tokamak equilibrium as transparent flux surfaces, helical magnetic field lines, scalar
plasma slices, animated tracers, and linked radial profile panels.

This example should not be just a decorative collection of 3D curves. Its scientific identity is:

```text
tokamak equilibrium -> magnetic field lines -> flux surfaces -> scalar plasma fields -> profile/probe links
```


## Why This Example Exists

This example fills a gap in the showcase set: fusion plasma geometry and 3D field-line
visualization.

It should pressure:

1. long 3D path rendering,
2. transparent mesh surfaces,
3. scalar colormaps on slices or surfaces,
4. arcball camera interaction,
5. linked 3D selection and 2D radial profiles,
6. optional particle/tracer animation along paths,
7. visibility and LOD controls for many field lines,
8. deterministic synthetic scientific geometry that can later be replaced by prepared real data.


## Recommended Default Scenario

Use a synthetic axisymmetric tokamak equilibrium.

Stage 1 does not need EFIT, GEQDSK, or another real equilibrium format. A deterministic analytic
model is enough to create a visually credible tokamak:

- major radius `R0`,
- minor radius `a`,
- toroidal angle `phi`,
- poloidal angle `theta`,
- safety-factor profile `q(r)`,
- nested normalized flux surfaces `psi_n`,
- scalar plasma fields such as temperature and density.

The visual result should clearly show helical magnetic field lines wrapping around a torus and
nested surfaces representing magnetic flux surfaces.


## User-Facing Scenario

The default scene should show:

- a dark or neutral background,
- several transparent nested toroidal flux surfaces,
- 20-100 helical magnetic field lines,
- one colored poloidal cross-section or scalar slice,
- optional vessel/limiter outline,
- arcball camera navigation,
- selected field-line or flux-surface highlight,
- a linked radial profile panel for temperature, density, and safety factor.

The strongest screenshot should read immediately as fusion/plasma visualization.


## Scene Layout

Recommended layout:

```text
+------------------------------------------------------------------+
| 3D tokamak viewport                                               |
| transparent flux surfaces, field lines, scalar slice, tracer dots |
+------------------------------------------------------------------+
| radial profile panel                                              |
| temperature / density / safety factor vs normalized radius         |
+------------------------------------------------------------------+
```

Minimum viable version:

1. one 3D panel with arcball interaction,
2. toroidal flux-surface meshes,
3. helical field-line paths,
4. one scalar cross-section or surface color,
5. selected line or surface readout.

Preferred fuller version:

1. linked radial profile panel,
2. animated tracer particles moving along selected lines,
3. flux-surface selection,
4. show/hide controls for surfaces, slice, vessel, and tracers,
5. color modes for `psi_n`, `q`, temperature, or field-line length.


## Data Strategy

### Stage 1: Deterministic Synthetic Equilibrium

The first implementation should work without external data.

Generate field lines and flux surfaces from a simple analytic toroidal model:

```text
R = R0 + r * cos(theta)
Z = r * sin(theta)
x = R * cos(phi)
y = R * sin(phi)
z = Z
```

For a field line on a flux surface of radius `r`:

```text
theta(phi) = theta0 + phi / q(r)
q(r) = q0 + (q_edge - q0) * (r / a)^2
```

This produces deterministic helical paths with a clear safety-factor interpretation.


### Stage 2: Prepared Equilibrium Cache

A later preparation script may read real equilibrium or field-line data and export a compact cache.

Suggested cache layout:

```text
~/.cache/datoviz/tokamak/synthetic_equilibrium/
  metadata.json
  field_line_position_f32.bin    # n_lines x n_samples x 3
  field_line_offset_u32.bin      # optional packed path offsets
  field_line_scalar_f32.bin      # psi_n, q, or line scalar
  flux_surface_position_f32.bin  # mesh vertices
  flux_surface_index_u32.bin     # mesh triangle indices
  flux_surface_scalar_f32.bin    # psi_n or temperature
  slice_position_f32.bin         # optional poloidal slice vertices
  slice_index_u32.bin
  slice_scalar_f32.bin
  profile_radius_f32.bin
  profile_temperature_f32.bin
  profile_density_f32.bin
  profile_q_f32.bin
```

Recommended metadata:

```text
case_name
source
R0
a
n_lines
n_line_samples
n_flux_surfaces
q0
q_edge
field_line_layout
surface_layout
profile_units
```

Runtime should consume prepared arrays. It should not require a real equilibrium parser.


## Synthetic Geometry Defaults

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

Flux surfaces:

- generate torus meshes at several normalized radii,
- use low to moderate opacity,
- color by `psi_n` or temperature,
- keep the outermost surface slightly more visible for orientation.

Field lines:

- seed lines at several radii and poloidal phases,
- sample several toroidal turns,
- color by `psi_n`, `q`, or local scalar,
- highlight the selected field line with larger width or brighter color.

Poloidal slice:

- generate a vertical plane or cross-section at one toroidal angle,
- color by a scalar field such as temperature,
- use this as the easiest first scalar-field surface.


## Scalar Field Model

Use simple radial profiles:

```text
rho = r / a
temperature(rho) = T_edge + (T_core - T_edge) * (1 - rho^2)^alpha
density(rho)     = n_edge + (n_core - n_edge) * (1 - rho^2)^beta
q(rho)           = q0 + (q_edge - q0) * rho^2
psi_n            = rho^2
```

The scalar values should be deterministic and normalized for stable colormap ranges.


## Visual Encodings

3D tokamak panel:

```text
flux surfaces     transparent mesh surfaces
field lines       colored 3D paths
slice             scalar-colored mesh or image-like surface
vessel outline    optional path/mesh overlay
tracer particles  small points animated along selected field lines
selection         brighter line/surface and profile marker
```

Default color modes:

- field lines by `q` or `psi_n`,
- flux surfaces by `psi_n`,
- slice by temperature.

The profile panel should mark the selected normalized radius when a line or surface is selected.


## Interactivity

Required MVP interactions:

1. arcball/orbit camera,
2. select or cycle highlighted field line,
3. show/hide flux surfaces,
4. show/hide scalar slice,
5. color mode selector,
6. selected object readout.

Preferred interactions:

1. click field line or flux surface to select `psi_n`,
2. animate tracer particles along field lines,
3. adjust number of visible field lines,
4. adjust flux-surface opacity,
5. select radial profile marker to update highlighted surface,
6. reset camera.


## Picking And Readout

Picking should identify field-line or flux-surface identity when available.

Readout should include:

```text
object type
field line id or surface id
psi_n
rho
q
temperature
density
line length or sampled turns
```

The selected line or surface should update:

- 3D highlight state,
- radial profile marker,
- optional tracer animation target.


## Radial Profile Panel

The linked profile panel should show:

```text
x = normalized radius rho or psi_n
y = temperature / density / q
```

At minimum, show one profile curve and a vertical selected-radius marker. A fuller version may show
multiple profiles with separate axes or normalized values.

Selecting a profile location should highlight the nearest flux surface or field-line radius in the
3D panel.


## Tracer Animation

Tracer particles should move along precomputed field-line samples:

```text
sample_index(t) = (sample_index0 + speed * t) mod n_line_samples
```

The animation should update only tracer point positions, not the static field-line paths.

Optional tracer modes:

- one tracer on the selected line,
- many tracers on selected flux surface,
- all visible field lines with sparse tracers.


## FramePlan Shape

### Static Setup

Initial frame:

```text
UploadNode  -> field-line path vertices and colors
UploadNode  -> flux-surface mesh vertices, indices, and colors
UploadNode  -> scalar slice geometry and colors
UploadNode  -> radial profile panel geometry
RenderNode  -> 3D tokamak panel
RenderNode  -> profile panel
```


### Selection Change

When a line or surface is selected:

```text
UploadNode  -> line/surface highlight resources or style state
UploadNode  -> profile selected-radius marker
RenderNode  -> affected panels
```

No full geometry reupload should be required for selection-only changes.


### Tracer Animation Frame

When tracers are animated:

```text
UploadNode  -> tracer point positions
RenderNode  -> 3D tokamak panel
```

Static field-line and flux-surface geometry should remain unchanged.


### Visibility Or Density Change

When the number of visible lines or surface opacity changes:

```text
UploadNode  -> visibility/style buffers, if available
UploadNode  -> reduced path geometry only if the visual cannot hide subsets cheaply
RenderNode  -> 3D tokamak panel
```


## DRP2 Command Categories

The example is expected to require:

- buffers for 3D path vertices, mesh vertices, indices, scalar colors, and tracer points,
- draw commands for path, mesh/primitive, point, and profile-panel visuals,
- dynamic uploads for selection highlights and tracer positions,
- panel transform updates from the arcball controller,
- optional readback/pick requests for line or surface selection,
- optional capture/video commands through the app/canvas layer.


## Implementation Notes

The first C implementation can stay focused:

1. generate synthetic field lines and flux surfaces on the CPU,
2. render field lines as paths,
3. render flux surfaces with low opacity if supported, or as wireframe paths if transparency is
   not ready,
4. render one scalar slice or skip it until mesh scalar coloring is stable,
5. add tracer animation and profile linkage after static geometry is correct.

The implementation should keep synthetic geometry generation separate from rendering setup so a
prepared equilibrium cache can replace it later.


## Key Pressure On The Scene Spec

This example checks that Datoviz v0.4 can express a fusion visualization workflow where:

- long 3D paths, transparent surfaces, scalar slices, and profile plots share one physical state,
- selected 3D objects can map to radial profile coordinates,
- tracer animation updates small dynamic resources while field geometry remains static,
- visibility and density controls can handle many path objects,
- synthetic analytic data can later be replaced by prepared equilibrium data without changing scene
  semantics.
