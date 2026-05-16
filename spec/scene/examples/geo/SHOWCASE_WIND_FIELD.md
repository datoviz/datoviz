# Showcase: Global Wind Field

> **Agent Pickup**
> - **Category:** `geo`
> - **Implementation target:** Geographic or globe/terrain example with a minimal deterministic mode and optional real assets.
> - **Data policy:** Prefer public datasets with cache metadata; include a synthetic fallback for offline development.
> - **Preprocessing:** Required for real datasets; specify download, projection, tiling, simplification, and cache outputs.
> - **Validation:** Smoke command, camera/interaction checklist, and visual checks for projection or coordinate correctness.


## Purpose

Create a polished scientific showcase that renders a large 2D vector field with arrows and
optional streamline overlays through the active Datoviz v0.4 scene -> DRP2 -> app path.

The first implementation should be runnable as a C GLFW example:

```text
examples/c/showcase_wind_field_glfw.c
```

This is a concrete showcase, not a new API proposal. It should use existing scene visuals first and
make the absence of a dedicated vector-field visual visible through a clean local geometry generator.

## Relationship To Existing Specs

This file is the implementation-sized counterpart to `GLOBAL_WIND_PROJECTIONS.md`.

`GLOBAL_WIND_PROJECTIONS.md` describes the richer long-term target: projection-aware geographic
transforms, real ERA5 data, animation, and future `VectorFieldVisual` or `GlyphFieldVisual`
support. This showcase should start narrower:

1. one high-quality equirectangular global wind view;
2. synthetic climate-like data by default;
3. CPU-generated arrow geometry using existing visual families;
4. optional streamlines using the existing path visual;
5. a later data-loading seam for a prepared ERA5 `.npz` bundle.

## Scientific Story

The example should look like a compact climate-science visualization rather than a generic quiver
plot. It should show plausible global circulation:

1. trade-wind bands near the tropics;
2. stronger mid-latitude jet bands;
3. several vortices or pressure-system-like swirls;
4. wind speed encoded as a scalar background;
5. arrows colored by speed and oriented by local wind direction;
6. longer streamline traces that reveal flow structure.

The default view should be understandable in a screenshot without interaction.

## Visual Composition

Use one full-window 2D panel with a panzoom controller.

Recommended default window:

```text
1280 x 760
```

Coordinate convention:

```text
x = longitude normalized to [-1, +1]
y = latitude normalized to [-1, +1]
z = 0
```

Render layers, back to front:

1. a subtle background rectangle or image field for wind speed;
2. optional faint latitude/longitude reference lines generated as paths;
3. streamlines as colored `dvz_path` overlays;
4. arrows as generated triangle geometry or primitive triangles;
5. optional small title/status text later, once the text path is active.

Avoid GUI controls in the first version unless the existing GUI path is already needed for another
reason. A showcase screenshot matters more than a control panel.

## Data Model

The example should generate a regular grid:

```text
nx = 160
ny = 80
```

For each grid sample:

```text
lon:   float32
lat:   float32
u:     float32, eastward component
v:     float32, northward component
speed: float32
```

The synthetic field should be deterministic. Use fixed coefficients rather than random runtime
state, so screenshots and tests are repeatable.

Suggested procedural field:

1. zonal trade winds with opposite signs across latitude bands;
2. mid-latitude jet streams as Gaussian bands;
3. two or three analytic vortices;
4. a small meridional wave component to avoid purely horizontal arrows.

The generator should normalize speed to a stable display range and clamp arrows so local extreme
values cannot dominate the layout.

## Arrow Geometry

Do not wait for a dedicated vector visual.

Generate each arrow as CPU-side triangles and render it through the existing primitive or mesh path.
The geometry can be simple and still look good:

1. a narrow shaft rectangle made of two triangles;
2. a triangular head;
3. per-vertex color copied from the sample speed color;
4. one arrow per sampled vector after density decimation.

Recommended displayed arrow grid:

```text
arrow_nx = 80
arrow_ny = 40
arrow_count = 3200
vertices_per_arrow = 7 or 9
```

If the primitive visual is used, emit the arrow triangles as a triangle list. If the mesh visual is
more convenient for indexed geometry, use one mesh visual with generated indices.

Arrow sizing:

```text
base_length = 0.018 to 0.030 in normalized coordinates
length scales gently with speed
shaft_width = 0.0025 to 0.0045
head_length = 0.006 to 0.010
head_width = 2.5x to 3.5x shaft_width
```

The arrows should be numerous enough to demonstrate density, but sparse enough to be legible in a
single screenshot.

## Streamline Overlay

Streamlines are optional for the first commit of the runnable example, but the spec should guide the
implementation.

Generate a small set of RK2 or RK4-integrated paths over the same analytic field:

```text
streamline_count = 48 to 96
samples_per_line = 80 to 160
```

Seed positions should be distributed across latitude bands, with extra seeds near vortices.

Rendering:

1. use one `dvz_path` visual when grouped paths are supported by the active C API;
2. otherwise use one path visual per streamline only if the count remains low enough;
3. color streamlines by local speed or use a restrained off-white/blue overlay;
4. keep line widths modest so arrows remain readable.

If grouped path support is missing in the C v0.4 surface, the first implementation may skip
streamlines and leave a clear TODO near the local helper.

## Scalar Background

The scalar background should communicate wind speed and make the example scientific even before
arrows are inspected.

Preferred first implementation:

1. generate an RGBA texture from `speed`;
2. upload it as a sampled field;
3. render it with `dvz_image`.

Use a perceptually sensible multi-hue palette. Avoid a one-note blue or purple field. The map should
have enough contrast to reveal jets and vortices while leaving arrows readable.

## Future Real Dataset

The runnable showcase may start with synthetic data, but the code should be structured so the data
source can later be replaced by:

```text
data/climate/era5_global_wind_10m_24h_2deg.npz
```

Expected arrays:

```text
lon: float32[nx]
lat: float32[ny]
u10: float32[nt, ny, nx]
v10: float32[nt, ny, nx]
```

The first C example does not need `.npz` loading. A later Python or data-preparation step can
produce a compact C-friendly binary or generated header if needed.

## Interaction

The first version should support:

1. pan and zoom through the existing panzoom controller;
2. bounded smoke mode via an optional frame-count command-line argument;
3. screenshot capture when run in bounded mode if this matches nearby examples.

Optional later controls:

1. toggle arrows;
2. toggle streamlines;
3. adjust arrow density;
4. switch synthetic field presets;
5. animate synthetic time.

## FramePlan And DRP2 Pressure

This showcase should exercise the active stack without adding a parallel renderer:

1. image visual for the scalar speed field;
2. primitive or mesh visual for many CPU-generated arrow triangles;
3. path visual for streamline overlays when available;
4. panzoom transform propagation to all layers;
5. repeated live frames through `dvz_app_run`;
6. optional capture/readback from the same rendered path.

The example should not create Vulkan resources directly.

## Acceptance Criteria

The first runnable example is acceptable when:

1. it builds through the normal C example path;
2. it opens a GLFW window and renders a nonblank scientific wind-field view;
3. arrows are clearly directional and colored by speed;
4. the scalar background shows coherent bands and vortices;
5. panzoom works without desynchronizing image, arrows, and streamlines;
6. bounded mode exits cleanly after the requested number of frames;
7. the example uses only active v0.4 scene/app APIs;
8. no new visual family is required.

## Validation

Recommended validation when implementing the runnable example:

```bash
just build
just example-c showcase_wind_field_glfw
./build/examples/c/showcase_wind_field_glfw 120
git diff --check
```

For changes that touch scene, DRP2, or runtime emission, also run the narrow relevant test:

```bash
just test scene
```

## Follow-Up Path

After the first runnable showcase lands, the next useful increments are:

1. add grouped streamline support if the first version had to skip it;
2. add screenshot capture and gallery wiring;
3. replace synthetic data with a prepared ERA5-derived bundle;
4. add projection-aware Mercator or orthographic modes from `GLOBAL_WIND_PROJECTIONS.md`;
5. use the example to motivate a dedicated `VectorFieldVisual` only after the existing-visual
   implementation exposes concrete duplication or performance limits.
