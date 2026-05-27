# Global Climate Wind Field with Projection-Aware Vector Glyphs

> **Example status:** informative pressure test
> **Target:** Python geographic vector-field example
> **Data:** prepared ERA5 cache with deterministic synthetic fallback
> **Validation:** smoke, projection, vector-orientation, and interaction checks

## Summary

Build a global 10 m wind visualization with projection-aware antialiased vector glyphs over a map.
The key requirement is that both sample positions and wind vectors transform correctly under
nonlinear projections. Runtime should not call live Copernicus/ECMWF APIs.

## User-Visible Result

- One main panel showing a global map in equirectangular, Mercator, or orthographic projection.
- Wind-speed background image where supported.
- Thousands of antialiased arrows colored by speed.
- Graticule and optional coastlines.
- GUI controls for projection, playback, time index, density, arrow scale, normalization,
  background, graticule, coastlines, and colormap.
- Orthographic mode supports central longitude/latitude sliders or mouse rotation.

## Feature Pressure Points

- Dense instanced glyph rendering with stable screen-space width.
- Nonlinear point projection plus local vector differential.
- Projection clipping and masking at poles, dateline, and orthographic limb.
- Attribute channels for position, vector, speed/color, and validity.
- Updated CPU-side resources when time, projection, density, or projection parameters change.
- Optional compute/render particle advection path later.

## Required Data And Resources

Preferred prepared dataset:

```text
climate/era5_global_wind_10m_24h_2deg.npz
```

Content:

```text
lon float32[nx]
lat float32[ny]
u10 float32[nt, ny, nx]  # eastward m/s
v10 float32[nt, ny, nx]  # northward m/s
time optional float64[nt] or string[nt]
land_mask optional uint8[ny, nx]
coastlines optional packed polyline arrays
metadata optional JSON string
```

Recommended dimensions:

```text
nx = 180 or 360
ny = 91 or 181
nt = 24 or 48
```

Fallback field should be deterministic and climate-like: trade-wind bands, polar jets,
low-pressure vortices, and smooth temporal evolution.

Derived scalar:

```text
speed = sqrt(u10^2 + v10^2)
```

## Scene Shape And Runtime Behavior

Scene shape:

```text
Window 1280 x 800
└── Full-window 2D projected map panel
    ├── Optional speed image
    ├── Graticule and optional coastlines
    ├── Instanced arrow glyphs
    └── Optional ImGui/hover overlay
```

Required projections:

| Projection | Notes |
|---|---|
| Equirectangular | Debug baseline: `x = lon`, `y = lat` |
| Mercator | Clamp latitude near `[-85, +85]` degrees |
| Orthographic | Globe-like disk with hidden far hemisphere |

Vector transform requirement:

```text
p0 = project(lon, lat)
pe = project(lon + eps_lon, lat)
pn = project(lon, lat + eps_lat)
east_basis  = (pe - p0) / eps_lon
north_basis = (pn - p0) / eps_lat
projected_vector = u * east_basis + v * north_basis
```

Invalid projections, NaNs, dateline artifacts, Mercator singularities, and orthographic far-side
points must be hidden rather than drawn as corrupt geometry.

Glyph attributes:

```text
position projected vec2/vec3
vector projected vec2
speed float
color optional vec4
valid optional mask
scale constant
width constant
opacity constant
```

Preferred glyph implementation: instanced SDF arrow quads. Geometry-based arrow meshes are
acceptable if antialiasing and density remain good.

Animation advances time index, loops, and updates resources only when time or projection state
changes. Default density should decimate to roughly `2k-8k` visible arrows; stress mode can expose
`20k-50k`.

## Minimal Implementation Target

- Prepared or synthetic global wind field.
- Static equirectangular or orthographic view.
- Speed background in at least equirectangular mode.
- Graticule or simple coastline/land-ocean reference.
- Projection-transformed arrows with speed coloring.
- GUI controls for projection, time, density, and scale.

## Validation / Acceptance Criteria

- Runs out of the box after resolving the small cached dataset or fallback.
- Displays a realistic global wind field with nonblank arrows.
- Supports equirectangular, Mercator, and orthographic projections.
- Vector orientation changes consistently under nonlinear projections.
- Mercator high latitudes are clipped/masked cleanly.
- Orthographic mode shows one hemisphere, hides far-side vectors, and curves the graticule.
- Projection/time/density changes update resources without unnecessary reallocations.
- Default density remains smooth on a modern laptop or desktop GPU.

## Links

- [Shared example policies](../POLICIES.md)
- [Transform pipeline](../../pipeline/TRANSFORM_PIPELINE.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
