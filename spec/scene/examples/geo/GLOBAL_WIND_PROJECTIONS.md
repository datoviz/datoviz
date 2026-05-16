# Global Climate Wind Field with Projection-Aware Vector Glyphs

> **Agent Pickup**
> - **Category:** `geo`
> - **Implementation target:** Geographic or globe/terrain example with a minimal deterministic mode and optional real assets.
> - **Data policy:** Prefer public datasets with cache metadata; include a synthetic fallback for offline development.
> - **Preprocessing:** Required for real datasets; specify download, projection, tiling, simplification, and cache outputs.
> - **Validation:** Smoke command, camera/interaction checklist, and visual checks for projection or coordinate correctness.


## Example name

`GLOBAL_WIND_PROJECTIONS`

## Goal

Create a Python example for the Datoviz v0.4 branch that renders a global climate wind field with antialiased vector glyphs on top of a projected geographic map.

The example should be visually appealing, scientifically credible, and useful as a stress test for:

- a future `VectorFieldVisual` or `GlyphFieldVisual` type;
- dense antialiased vector rendering;
- nonlinear transform chains;
- geographic map projections;
- projection-aware vector orientation;
- time-dependent resource updates;
- optional compute/render integration for particle advection;
- level-of-detail and density control for large vector fields.

The exact Datoviz v0.4 Python API is not finalized. This document therefore specifies behavior, data flow, and rendering requirements rather than exact API calls.

---

## High-level description

The example displays a global near-surface wind field from climate or reanalysis data. Wind vectors are shown as smooth antialiased arrows over a world map. The user can switch between several geographic projections and animate the wind field over time.

The core visual challenge is that both **sample positions** and **wind vectors** must be transformed consistently under nonlinear geographic projections.

For each grid point:

- the position `(longitude, latitude)` is transformed to projected coordinates;
- the vector `(u, v)`, representing eastward and northward wind components, is transformed using the local differential of the projection;
- the resulting screen-space or projected vector is rendered as an antialiased arrow glyph.

This is not just a scatter plot with arrows: it should explicitly test projection-aware vector-field rendering.

---

## Suggested screenshot / expected visual result

The default view should show:

- a global map in an equirectangular or orthographic projection;
- a subtle ocean/land background or simple coastline overlay;
- a wind-speed scalar field as a smooth semi-transparent background image;
- thousands of antialiased arrows showing wind direction;
- arrows colored by wind speed;
- a small GUI panel with projection and animation controls.

The final result should look like a climate-science visualization, not a generic synthetic vector field.

---

## Dataset

### Preferred dataset

Use a small preprocessed ERA5 10 m wind subset, stored in `datoviz/data` and downloaded automatically if missing from the local cache.

ERA5 variables:

- `u10`: eastward 10 m wind component, in m/s;
- `v10`: northward 10 m wind component, in m/s.

The example must not require a live Copernicus/ECMWF API request at runtime. The runtime example should only download a curated, lightweight file from the Datoviz data repository.

### Suggested file

```text
climate/era5_global_wind_10m_24h_2deg.npz
```

### Suggested content

```text
lon: float32[nx]
lat: float32[ny]
u10: float32[nt, ny, nx]
v10: float32[nt, ny, nx]
time: optional float64[nt] or string[nt]
land_mask: optional uint8[ny, nx]
coastlines: optional packed polyline arrays
metadata: optional JSON string
```

Recommended dimensions:

```text
nx = 180 or 360
ny = 91 or 181
nt = 24 or 48
```

This is large enough to look real, but small enough to load quickly.

### Acceptable fallback dataset

If the ERA5 subset is not available, the implementation may generate a synthetic but climate-like global wind field with:

- trade-wind bands;
- polar jets;
- low-pressure vortices;
- smooth temporal evolution.

However, the preferred implementation should use real preprocessed data.

---

## Data loading requirements

The example should:

1. Determine a local cache directory using the usual Datoviz example-data convention.
2. Check whether the `.npz` file exists locally.
3. Download it from the Datoviz data GitHub repository if missing.
4. Load the arrays with NumPy.
5. Validate array shapes and dtypes.
6. Convert data to `float32` if necessary.
7. Compute wind speed if not already present:

```python
speed = np.sqrt(u10**2 + v10**2)
```

The implementation should avoid hard failures when optional arrays such as coastlines or land masks are missing.

---

## Scene layout

Use a single main panel.

Recommended initial size:

```text
window: 1280 x 800
panel: full window, with small GUI overlay
```

The panel should use a 2D camera for projected map coordinates.

For the orthographic mode, either:

- keep a 2D projected-disk representation; or
- use a 3D camera and render the field on a sphere if the v0.4 API already makes that straightforward.

The simpler and preferred first implementation is a 2D projected representation.

---

## Projection modes

The example should expose at least these projection modes:

### 1. Equirectangular

Baseline projection.

```text
x = lon
 y = lat
```

This mode is useful for debugging and verifying that the raw wind vectors are correctly rendered.

### 2. Mercator

Nonlinear cylindrical projection.

```text
x = lon
 y = log(tan(pi/4 + lat/2))
```

Latitude must be clamped to avoid singularities near the poles, for example to `[-85 deg, +85 deg]`.

This mode strongly tests nonlinear transforms and vector rescaling near high latitudes.

### 3. Orthographic

Globe-like projection onto a disk.

Inputs:

```text
central longitude: lon0
central latitude: lat0
```

Only the visible hemisphere should be shown. Points on the far side of the globe must be clipped or hidden.

This mode is the most visually impressive and is a good stress test for clipping, projection boundaries, and local vector orientation.

### Optional later projections

- Lambert azimuthal equal-area;
- Robinson;
- Mollweide;
- polar stereographic.

These are not required for the first version.

---

## Projection-aware vector transformation

This is the key requirement of the example.

Wind vectors are given as eastward/northward components:

```text
u = eastward component
v = northward component
```

For nonlinear projections, arrows must not be rendered by simply projecting the base point and then drawing `(u, v)` unchanged.

Instead, the implementation must transform the vector through the local differential of the projection.

### Recommended finite-difference method

For each point `(lon, lat)`:

1. Project the base point:

```text
p0 = project(lon, lat)
```

2. Project two nearby points:

```text
pe = project(lon + eps_lon, lat)
pn = project(lon, lat + eps_lat)
```

3. Compute local projected basis vectors:

```text
east_basis  = (pe - p0) / eps_lon
north_basis = (pn - p0) / eps_lat
```

4. Transform the wind vector:

```text
projected_vector = u * east_basis + v * north_basis
```

5. Normalize, clamp, or scale according to the selected rendering mode.

The implementation may use analytical Jacobians instead, but the finite-difference method is simpler and projection-agnostic.

### Numerical robustness

The implementation should handle:

- invalid projections;
- points outside the visible orthographic hemisphere;
- antimeridian discontinuities;
- latitudes near Mercator singularities;
- very small or very large projected vectors;
- NaNs in the source dataset.

Invalid arrows should be hidden, not drawn as corrupt geometry.

---

## Vector glyph rendering

### Required visual behavior

The vector field should render arrows with:

- smooth antialiased edges;
- stable screen-space shaft width;
- configurable arrow length scale;
- optional normalization of arrow lengths;
- color mapped from wind speed;
- alpha blending suitable for dense fields;
- correct clipping at projection boundaries.

### Preferred implementation

Use an instanced glyph approach.

Each vector is one instance with attributes such as:

```text
position: projected vec2
vector: projected vec2
speed: float
color: optional vec4
valid: optional uint or float mask
```

A small canonical glyph should be expanded in the shader into an arrow.

Two acceptable approaches:

1. **Geometry-based arrow glyph**
   - shaft and head represented by a small triangle mesh or quad set;
   - instanced per vector;
   - antialiasing via shader distance to edges or MSAA.

2. **SDF arrow glyph**
   - one quad per arrow;
   - fragment shader computes signed distance to an arrow shape;
   - alpha computed with `smoothstep` and screen-space derivatives;
   - best visual quality and most flexible.

The SDF approach is preferred if feasible.

### Channels to pressure

The example should naturally map to visual channels:

```text
position: attribute vec2 or vec3
vector: attribute vec2
speed: attribute float
color: attribute or colormap lookup
scale: constant float
width: constant float
opacity: constant float
```

The visual should work with both constant and attribute channels where appropriate.

---

## Background scalar field

The example should optionally render wind speed as a smooth background image.

Requirements:

- compute `speed = sqrt(u^2 + v^2)` for each frame;
- project or resample the scalar field into the current map projection;
- display as a semi-transparent image or texture;
- use a perceptually reasonable colormap;
- allow hiding the background to inspect arrows alone.

For the first implementation, it is acceptable to display the speed background only in equirectangular mode and hide it in other projections, but the preferred version should support all projections.

---

## Coastlines and graticule

The example should include at least one geographic reference layer.

Minimum:

- draw a latitude/longitude graticule in the selected projection.

Preferred:

- also draw simplified coastlines loaded from the dataset or a small bundled file.

The graticule is useful because it clearly shows projection distortion and validates the transform pipeline.

Coastline rendering should use antialiased line rendering if available.

---

## Animation

The wind field should animate over time.

Recommended behavior:

- time index advances automatically while playing;
- playback loops over all time frames;
- interpolation between frames is optional;
- GUI exposes play/pause and time index slider.

On each frame change, update the vector resources:

```text
u10[t], v10[t] -> projected arrow positions/vectors/colors
```

The example should keep CPU work reasonable by:

- decimating the grid before projection;
- updating only when the time index or projection changes;
- avoiding unnecessary reallocations.

---

## Interaction

### Required controls

Expose an ImGui control panel with:

```text
Projection: Equirectangular / Mercator / Orthographic
Play/Pause
Time index
Vector density / stride
Arrow scale
Normalize arrows: on/off
Show speed background: on/off
Show graticule: on/off
Show coastlines: on/off, if available
Colormap selection, if available
```

### Orthographic controls

For orthographic mode, support at least one of:

- sliders for central longitude and latitude;
- mouse drag to rotate the globe center.

Mouse-driven rotation is preferred, but sliders are sufficient for the first implementation.

### Picking / hover

Optional but useful:

- hovering an arrow displays longitude, latitude, `u`, `v`, and speed;
- nearest-vector lookup can be CPU-side.

---

## Optional particle overlay

An optional advanced layer may show particles advected by the wind field.

This would further stress compute/render integration.

Behavior:

- particles are initialized at random valid lon/lat positions;
- each frame, they sample the wind field and move forward;
- particles wrap in longitude and clamp or reset near the poles;
- particles are rendered as small fading points or short trails.

Two implementation paths are acceptable:

1. CPU advection for simplicity.
2. GPU compute advection if the v0.4 compute/scene API is ready.

If implemented on GPU, the particle update should be a compute pass whose output buffer is consumed directly by a render pass without CPU readback.

---

## Performance targets

The example should run interactively on a modern laptop or desktop GPU.

Suggested default density:

```text
vector stride: 3 or 4 grid cells
visible arrows: 2,000 to 8,000
```

Stress-test mode:

```text
visible arrows: 20,000 to 50,000
```

The GUI should allow increasing density to stress the renderer.

Target frame rate:

- at least 60 FPS for default density on a decent GPU;
- smooth interaction during projection changes and animation;
- graceful degradation at very high density.

---

## Expected implementation structure

The Python example should roughly follow this structure:

```text
1. Import NumPy and Datoviz.
2. Resolve cache path and download dataset if missing.
3. Load lon, lat, u10, v10 arrays.
4. Build a decimated lon/lat grid.
5. Initialize scene, panel, camera, GUI, and visuals.
6. Create resources for:
   - projected arrow positions;
   - projected arrow vectors;
   - wind speed or color;
   - optional scalar background texture;
   - graticule/coastline polylines.
7. On projection/time/density change:
   - recompute projected positions;
   - transform vectors through projection differential;
   - update dirty resources.
8. On every frame:
   - advance time if playing;
   - update resources when needed;
   - draw scene.
```

The exact calls should follow the final Datoviz v0.4 Python API.

---

## Pseudocode for projection update

```python
def update_projected_vectors(t, projection, stride, params):
    lon_s = lon_grid[::stride, ::stride]
    lat_s = lat_grid[::stride, ::stride]
    u_s = u10[t, ::stride, ::stride]
    v_s = v10[t, ::stride, ::stride]

    p0, valid0 = project(lon_s, lat_s, projection, params)
    pe, valide = project(lon_s + eps_lon, lat_s, projection, params)
    pn, validn = project(lon_s, lat_s + eps_lat, projection, params)

    east = (pe - p0) / eps_lon
    north = (pn - p0) / eps_lat

    vec = u_s[..., None] * east + v_s[..., None] * north
    speed = np.sqrt(u_s * u_s + v_s * v_s)

    valid = valid0 & valide & validn & np.isfinite(vec).all(axis=-1)

    if normalize_arrows:
        vec = normalize(vec) * normalized_length
    else:
        vec = vec * arrow_scale
        vec = clamp_length(vec, max_length)

    positions = p0[valid]
    vectors = vec[valid]
    speeds = speed[valid]

    position_resource.update(positions.astype(np.float32))
    vector_resource.update(vectors.astype(np.float32))
    speed_resource.update(speeds.astype(np.float32))
```

---

## Visual correctness tests

The implementation should be checked visually in the following situations:

### Equirectangular

- arrows have expected east/north orientation;
- no discontinuity except at the dateline;
- graticule appears rectangular.

### Mercator

- high-latitude deformation is visible;
- arrows remain smooth and finite;
- polar regions are clipped or masked correctly.

### Orthographic

- only one hemisphere is visible;
- arrows near the limb are clipped or hidden cleanly;
- rotating the central longitude changes the visible hemisphere;
- graticule curves correctly;
- vector orientation remains tangent to the projected globe.

---

## Datoviz architecture pressure points

This example is intended to pressure the following v0.4 architecture elements.

### Scene resources

The vector field uses CPU-side numerical resources that are updated when time or projection changes. The scene layer should track dirty resources and emit the necessary buffer or texture updates.

### Visual channels

The vector field naturally requires multiple attribute channels and constants:

- position attribute;
- vector attribute;
- speed/color attribute;
- global scale constant;
- glyph width constant;
- opacity constant.

### Transform chains

The example should encourage explicit support for nonlinear geographic transform chains, ideally with WGSL code generation for projection functions.

### Vector transform semantics

The example should make clear that transforming a vector field is not the same as transforming point positions. A future scene API may need a first-class distinction between:

```text
point transform: p -> f(p)
vector transform: v -> J_f(p) v
```

where `J_f(p)` is the local Jacobian of the transform at position `p`.

### Framegraph and compute

The optional particle overlay should be implemented as a compute pass feeding a render pass when the compute API is ready.

### DRP backend

The rendered result should remain backend agnostic and compatible with the DRP/WebGPU-style object model.

---

## Non-goals

The first implementation does not need to:

- download raw ERA5 data directly from Copernicus;
- implement a full GIS stack;
- support arbitrary shapefiles;
- provide exact meteorological cartography;
- implement all map projections;
- handle extremely large climate datasets out of core.

This is a visualization and architecture stress test, not a climate-data processing framework.

---

## Acceptance criteria

The example is considered successful if:

1. It runs out of the box after downloading the small cached dataset.
2. It displays a real or realistic global wind field.
3. It renders antialiased vector arrows with good visual quality.
4. It supports at least equirectangular, Mercator, and orthographic projections.
5. Vector directions are transformed consistently under nonlinear projections.
6. The GUI allows projection, time, density, and scaling changes.
7. The implementation avoids hard dependency on a still-unstable exact v0.4 API shape.
8. The code is clear enough to serve as a reference for a future Datoviz vector-field visual.

---

## Suggested future extensions

- Add animated particle advection in a compute shader.
- Add streamlines generated from the same wind field.
- Add global temperature or pressure as an alternative scalar background.
- Add multi-panel comparison of projections.
- Add polar stereographic projection for climate applications.
- Add support for vector fields on curvilinear grids.
- Add video export using Datoviz offscreen rendering.
