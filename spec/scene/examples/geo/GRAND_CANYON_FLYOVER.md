# Grand Canyon Flyover

> **Example status:** informative pressure test
> **Target:** Python 3D terrain example
> **Data:** prepared terrain bundle with explicit synthetic fallback
> **Validation:** smoke, terrain, depth, camera, and screenshot checks

## Summary

Build a cinematic 3D flyover over a recognizable Grand Canyon terrain patch. Runtime consumes a
preprocessed Datoviz-ready terrain bundle from the cache or `datoviz/data`; it must not query GIS
services. The first v0.4 slice is one bounded true textured mesh with UVs, a mesh-bound RGBA
texture, lighting, and a perspective camera following a simple path. Generated color relief may be
the texture source, but baked vertex colors alone do not satisfy this example.

## User-Visible Result

- One full-window 3D scene with a textured terrain mesh.
- Shaded relief, depth testing, perspective camera, and smooth flyover animation.
- Optional fog/haze, sky background, river/path overlay, wireframe toggle, and orbit inspection.
- Default mode: animated flyover; user drag pauses animation and enables orbit inspection.

Controls:

```text
Space pause/resume animation
O orbit mode
F flyover mode
R reset camera animation
W wireframe/debug, if supported
T texture toggle, if supported
```

## Feature Pressure Points

- Large indexed triangle mesh with normals and UVs.
- Sampled 2D texture resource with linear filtering and optional mipmaps.
- Depth range and near/far behavior on real-world-scale normalized data.
- Static terrain resources with per-frame camera uniform updates only.
- Scene-level camera animation.
- Optional overlay pass for path, river, or debug visuals.

## Required Data And Resources

Preferred runtime asset:

```text
grand_canyon_terrain_v1.npz
```

or directory form:

```text
grand_canyon_terrain_v1/
    metadata.json
    terrain.npz
    texture.png
```

`.npz` content:

```text
positions float32[N, 3]
normals float32[N, 3]
texcoords float32[N, 2]
indices uint32[M]
texture uint8[H, W, 3 or 4]
path_positions float32[K, 3], optional
path_targets float32[K, 3], optional
river_positions float32[R, 3], optional
bounds float32[2, 3]
metadata JSON string or UTF-8 bytes, optional
```

Coordinate convention:

```text
x east-west local coordinate, centered near 0
y north-south local coordinate, centered near 0
z elevation, optionally vertically exaggerated
x,y roughly in [-1, +1] or [-2, +2]
z roughly [-0.2, +0.4]
```

Recommended asset scale:

```text
height map: 512 x 512 first slice, 1024 x 1024 stress
vertices: 262k to 1M
triangles: about 0.5M to 2M
texture: 2048 x 2048 RGBA if practical
```

The v1 texture should preferably be generated from elevation, slope, hillshade, AO-like shading,
river/valley accents, and subtle noise, avoiding redistribution issues from orthophotos. Metadata
should preserve source, license, CRS, original bounds, units, scale, vertical exaggeration, grid
shape, and texture shape.

Offline preprocessing may use USGS 3DEP/The National Map, USGS canyon datasets, or NASA SRTM. The
runtime example should not require GIS packages.

## Scene Shape And Runtime Behavior

Scene shape:

```text
Scene
└── Panel3D
    ├── Perspective camera
    ├── Opaque textured indexed terrain mesh
    ├── Optional river/path line visual
    └── Optional debug visuals
```

Startup:

1. Resolve or download the terrain bundle.
2. Validate array shapes, dtypes, indices, finite positions, and bounds.
3. Create one 3D panel.
4. Upload mesh buffers and texture once.
5. Initialize perspective camera and start flyover animation.

Camera:

```text
fov 45-60 degrees
near 0.005 to 0.02 normalized units
far 10 to 50 normalized units
up +Z
```

Path source:

- Prefer `path_positions` and `path_targets` from the bundle.
- If missing, generate a circular/orbital path around `bounds`.
- Interpolate smoothly with Catmull-Rom, cubic Bezier, or smoothstep segments.
- Keep camera above terrain and looking slightly ahead.

Terrain material minimum:

```text
base_color = sample(texture, texcoord)
light = ambient + diffuse * max(dot(normal_world, light_dir), 0)
output = base_color * light
```

If normals are missing, compute them at load time or fall back to unlit texture rendering.

## Minimal Implementation Target

- One terrain bundle or explicit procedural fallback.
- One full-window 3D panel.
- Indexed mesh with positions, indices, UVs, and a mesh-bound texture.
- Depth testing and simple directional lighting.
- Flyover or orbit path.
- Optional `--asset`, `--mode flyover|orbit`, `--wireframe`, `--no-texture`, `--screenshot`, and
  `--frames` development arguments.

## Validation / Acceptance Criteria

- Runs from a clean checkout after installing Datoviz v0.4 Python bindings.
- Downloads or loads the terrain bundle automatically if available.
- Displays a textured 3D terrain mesh with non-flat relief.
- Camera animates smoothly and avoids terrain clipping.
- Near/far values are consistent with terrain scale.
- Terrain buffers and texture are not uploaded per frame.
- Screenshot shows recognizable canyon-like relief, no wireframe/debug overlay by default, and no
  excessive clipping.
- Fallback, if used, clearly warns that the real Grand Canyon asset is unavailable while still
  exercising the same mesh/texture/camera path.

## Links

- [Shared example policies](../POLICIES.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
