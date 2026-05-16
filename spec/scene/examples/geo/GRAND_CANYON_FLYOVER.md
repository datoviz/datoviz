# Grand Canyon Flyover

> **Agent Pickup**
> - **Category:** `geo`
> - **Implementation target:** Geographic or globe/terrain example with a minimal deterministic mode and optional real assets.
> - **Data policy:** Prefer public datasets with cache metadata; include a synthetic fallback for offline development.
> - **Preprocessing:** Required for real datasets; specify download, projection, tiling, simplification, and cache outputs.
> - **Validation:** Smoke command, camera/interaction checklist, and visual checks for projection or coordinate correctness.


## Purpose

This example specifies a Datoviz v0.4 Python demo showing a cinematic flyover above a real-world terrain patch, ideally the Grand Canyon. It is intended as a high-value test of the upcoming v0.4 scene architecture without depending on the exact final Python API.

The example should work out of the box. If the required terrain bundle is not present in the local Datoviz cache, the script should download it from the `datoviz/data` GitHub repository or a stable release asset.

The example should not be a full GIS viewer. It should be a visually compelling, compact, deterministic rendering demo that exercises:

- large indexed mesh rendering;
- texture sampling on a 3D mesh;
- 3D camera animation;
- scene-level resource management;
- near/far depth handling for real-world scale data;
- optional user interaction on top of an animated camera;
- optional video/export pipeline later.

## Working title

**Grand Canyon Flyover**

Suggested file name:

```text
GRAND_CANYON_FLYOVER.py
```

Suggested generated documentation/gallery title:

```text
Grand Canyon Flyover
```

## Visual result

The viewer should open a 3D scene containing a textured terrain mesh representing a recognizable canyon landscape. The camera should smoothly fly above the terrain, following either:

1. a predefined flight path through the canyon, or
2. a circular/orbital path around the terrain patch.

The default mode should be the predefined flyover path, because it better stresses camera animation and produces a more cinematic result.

The scene should feel like a lightweight flight over a landscape, not like a scientific wireframe demo. The expected rendering should include:

- shaded terrain relief;
- a high-resolution terrain texture or generated color relief texture;
- smooth camera motion;
- perspective projection;
- depth testing;
- optional fog/haze;
- optional sky gradient or skybox;
- optional river polyline overlay.

## Why this example matters for Datoviz v0.4

This example is useful because it forces the scene API to support a common but nontrivial scientific visualization pattern: a large textured surface with camera animation.

It should exercise the following v0.4 concepts:

- `Scene` as the owner of panels, visuals, resources, and animations;
- one 3D `Panel` with a perspective camera;
- a textured indexed `MeshVisual` or equivalent generic visual;
- CPU-side resources mapped to GPU buffers and textures;
- dirty resource upload through the scene/DRP layer;
- one opaque render pass with a depth attachment;
- optional overlay pass for path/river/debug visuals;
- scene-level animation scheduling for the camera;
- possible future extension to terrain LOD or tiled rendering.

The implementation should remain compatible in spirit with the v0.4 scene design: high-level scene objects emit DRP commands; the example should not directly depend on Vulkan, command buffers, or backend-specific details.

## Data source strategy

### Runtime rule

The runtime example should not query GIS web services directly. That would make the demo fragile, slow, and dependent on external APIs.

Instead, the runtime script should download a preprocessed terrain bundle from `datoviz/data` if it is not already cached locally.

Recommended runtime data bundle name:

```text
grand_canyon_terrain_v1.npz
```

or, if the texture is large and should remain separate:

```text
grand_canyon_terrain_v1/
    metadata.json
    terrain.npz
    texture.png
```

The example should support both a bundled `.npz` texture array and a separate PNG texture, but the first implementation may choose only one.

### Upstream data

The data bundle should be generated offline from public geographic data. Good upstream candidates include:

- USGS 3DEP / The National Map DEM data;
- USGS Grand Canyon / Colorado River corridor DEM or DSM datasets;
- NASA SRTM as a lower-resolution fallback;
- public-domain or permissively licensed orthophoto/satellite imagery when available.

The preprocessing pipeline should be documented but not run by default. The final runtime example should only consume Datoviz-ready cached files.

### Texture policy

A true orthophoto or satellite texture would look best, but licensing and API stability can be problematic. Therefore the recommended v1 asset should include a generated terrain texture derived from the elevation model itself:

- elevation-based color ramp;
- slope-based shading;
- ambient occlusion-like hillshade;
- optional river/valley accent;
- optional subtle noise to avoid flat gradients.

This ensures the example is reproducible, compact, and redistributable.

A later version may replace or supplement this texture with real orthophoto imagery.

## Datoviz data bundle format

The preferred runtime asset is a single compressed NumPy archive:

```text
grand_canyon_terrain_v1.npz
```

It should contain:

```text
positions       float32, shape (N, 3)
normals         float32, shape (N, 3)
texcoords       float32, shape (N, 2)
indices         uint32,  shape (M,)
texture         uint8,   shape (H, W, 4)
path_positions  float32, shape (K, 3), optional
path_targets    float32, shape (K, 3), optional
river_positions float32, shape (R, 3), optional
bounds          float32, shape (2, 3)
metadata        JSON string or UTF-8 bytes, optional
```

The data should already be normalized into Datoviz-friendly world coordinates.

Suggested coordinate convention:

```text
x: east-west local coordinate, centered around 0
y: north-south local coordinate, centered around 0
z: elevation, vertically exaggerated if desired
```

Suggested units:

```text
x, y, z: arbitrary scene units, derived from meters but normalized
```

The horizontal terrain extent should be normalized to approximately:

```text
x, y in [-1, +1] or [-2, +2]
```

The vertical elevation should be scaled for visual clarity, for example:

```text
z in approximately [-0.2, +0.4]
```

The original real-world units and scaling factors should be stored in `metadata`.

## Mesh generation

The terrain should be represented as a regular grid converted into an indexed triangle mesh.

For a height map of shape `(ny, nx)`:

- create `nx * ny` vertices;
- create two triangles per grid cell;
- use `uint32` indices;
- generate UV coordinates in `[0, 1] x [0, 1]`;
- compute vertex normals from the height field.

Recommended default asset size:

```text
height map: 512 x 512 or 1024 x 1024
vertices:   262k to 1M
triangles:  approximately 0.5M to 2M
texture:    2048 x 2048 RGBA, if possible
```

The v1 target should be visually impressive but still run on typical developer GPUs. A 512 x 512 mesh is acceptable for the first implementation; 1024 x 1024 is a better stress test if performance remains good.

## Runtime behavior

At startup, the script should:

1. determine the Datoviz cache directory;
2. check whether `grand_canyon_terrain_v1.npz` exists;
3. download it from `datoviz/data` if missing;
4. validate the archive contents;
5. create the scene;
6. create one full-window 3D panel;
7. create the terrain visual;
8. upload mesh buffers and texture resources;
9. initialize the camera;
10. start the camera flyover animation.

The example should not require command-line arguments.

It may accept optional arguments for development:

```text
--asset PATH              use a local terrain bundle
--mode flyover|orbit      camera mode
--wireframe               debug terrain topology
--no-texture              use shaded material only
--screenshot PATH         save one frame and exit
--frames N                run for N frames and exit
```

## Scene structure

The scene should conceptually contain:

```text
Scene
└── Panel3D
    ├── Camera3D
    ├── Terrain mesh visual, opaque stage
    ├── Optional river/path line visual, overlay or opaque stage
    └── Optional debug visuals
```

The exact Python API is not fixed. The implementation agent should map this structure to the actual v0.4 API available at implementation time.

Pseudo-structure:

```python
scene = dvz.Scene()
panel = scene.panel_3d()

camera = panel.camera_3d()
camera.set_perspective(fov=55, near=0.01, far=20.0)

terrain = scene.visual("mesh")
terrain.set_positions(positions)
terrain.set_normals(normals)
terrain.set_texcoords(texcoords)
terrain.set_indices(indices)
terrain.set_texture(texture)
terrain.set_material(...)
panel.add(terrain)

scene.animate(camera, update_camera_along_path)
scene.run()
```

This is illustrative only. The final implementation should use the actual v0.4 Python API.

## Terrain visual requirements

The terrain visual should support:

- indexed triangle mesh rendering;
- position attribute;
- normal attribute;
- texture coordinate attribute;
- sampled 2D RGBA texture;
- depth testing enabled;
- back-face culling optional;
- simple directional lighting;
- optional ambient term;
- optional fog based on distance.

Minimum shader/material behavior:

```text
base_color = sample(texture, texcoord)
normal_world = normalize(normal)
light = ambient + diffuse * max(dot(normal_world, light_dir), 0)
output_color = base_color * light
```

The terrain should render correctly even if `normals` are missing. In that case the implementation may either:

- compute normals on the CPU at load time;
- use a simpler unlit texture material;
- or derive approximate normals in the shader if a height texture is available.

## Camera animation

The camera should fly along a smooth path above the terrain.

Preferred path representation in the data bundle:

```text
path_positions: float32[K, 3]
path_targets:   float32[K, 3]
```

If path arrays are missing, the script should generate a default circular orbit around the terrain bounds.

The default flyover path should:

- start outside or above one side of the terrain;
- move along the canyon axis;
- keep the camera above the ground;
- look slightly ahead along the path;
- avoid clipping through the terrain;
- loop seamlessly if possible.

Interpolation should be smooth, for example:

- Catmull-Rom spline;
- cubic Bézier segments;
- or simple smoothstep interpolation between control points.

Pseudo-code:

```python
def update_camera(t):
    u = (t * speed) % 1.0
    pos = sample_spline(path_positions, u)
    target = sample_spline(path_targets, u)
    camera.set_position(pos)
    camera.set_target(target)
    camera.set_up((0, 0, 1))
```

The camera should use a perspective projection:

```text
fov:       45-60 degrees
near:      small enough for close terrain, e.g. 0.005 to 0.02 in normalized units
far:       large enough for full terrain, e.g. 10 to 50 in normalized units
up vector: +Z
```

## Interaction

The default should be animated flyover.

User controls should be simple:

```text
Space: pause/resume animation
O:     switch orbit mode
F:     switch flyover mode
R:     reset camera animation
W:     toggle wireframe/debug mesh, if supported
T:     toggle texture, if supported
```

Mouse interaction may either:

- temporarily override the animated camera with an arcball/orbit controller;
- or be disabled while the camera animation is playing.

The preferred behavior:

- animation runs by default;
- user drag pauses animation and enables orbit inspection;
- pressing `F` resumes flyover mode.

An optional ImGui panel may expose:

```text
Camera mode: flyover / orbit
Speed
Vertical exaggeration
Fog strength
Texture enabled
Wireframe enabled
Light direction
```

The example should remain usable without ImGui if GUI support is not ready.

## Rendering pipeline expectations

The minimal rendering graph should contain one main render pass:

```text
MAIN_COLOR pass
    color target: swapchain or offscreen color texture
    depth target: depth24/depth32 equivalent
    visuals: terrain mesh, optional river/path lines
```

The terrain visual belongs to the opaque stage.

Optional future passes:

```text
SKY pass       background gradient or skybox
MAIN_COLOR     terrain and opaque geometry
OVERLAY        debug path, labels, UI-independent overlays
```

This example should not require multipass rendering in v1, but it should be compatible with the scene framegraph design.

## Texture and sampler requirements

The texture should be uploaded as a scene texture resource.

Recommended sampler:

```text
min filter: linear
mag filter: linear
mip filter: linear, if mipmaps are supported
address U/V: clamp-to-edge
anisotropy: enabled if available
```

If mipmap generation is not yet supported in v0.4, the first version may use a single texture level.

## Coordinate and scaling details

The preprocessing step should normalize real geospatial coordinates into local scene coordinates.

Recommended preprocessing logic:

1. load DEM in projected coordinates, preferably meters;
2. crop a rectangular canyon region;
3. resample to target grid size;
4. subtract horizontal center;
5. divide horizontal coordinates by the larger horizontal extent;
6. subtract median elevation;
7. divide elevation by the same horizontal scale;
8. apply optional vertical exaggeration, e.g. `1.5x` to `3x`;
9. store scaling metadata.

The result should avoid huge world coordinates. Datoviz should receive compact float32 positions.

Metadata should include:

```json
{
  "name": "Grand Canyon terrain",
  "source": "USGS / NASA / generated texture, exact source to be filled in",
  "license": "public domain or compatible, exact license to be filled in",
  "original_crs": "...",
  "bounds_original": [xmin, ymin, xmax, ymax],
  "elevation_units": "meters",
  "horizontal_scale": 12345.0,
  "vertical_exaggeration": 2.0,
  "grid_shape": [ny, nx],
  "texture_shape": [height, width, 4]
}
```

## Offline preprocessing pipeline

The offline preprocessing script is not part of the runtime example, but should eventually live in the data repository or tools repository.

Suggested script name:

```text
prepare_grand_canyon_terrain.py
```

Suggested dependencies for preprocessing only:

```text
rasterio
pyproj
numpy
scipy or scikit-image
Pillow
```

Preprocessing steps:

1. download or locate source DEM;
2. optionally download or locate orthophoto imagery;
3. crop to selected bounds;
4. reproject/resample DEM and texture to a common grid;
5. fill small nodata holes;
6. smooth only if necessary;
7. compute normals;
8. build indexed triangle mesh;
9. generate UV coordinates;
10. generate or resample texture;
11. define camera path control points;
12. save `grand_canyon_terrain_v1.npz`;
13. write metadata.

The preprocessing script may also generate a small preview image for documentation.

## Fallback mode

If the terrain bundle cannot be downloaded, the example should fail gracefully with a clear message.

A lightweight fallback synthetic terrain may be included, but only as a last resort. The fallback should not silently replace the real-data demo in normal operation.

Acceptable fallback:

- generate a procedural heightfield using fractal noise;
- generate a synthetic terrain texture;
- print a warning that the real Grand Canyon dataset is unavailable.

The fallback should still exercise the same mesh/texture/camera pipeline.

## Performance expectations

Target performance on a modern discrete GPU:

```text
512x512 terrain:   interactive at 60 FPS
1024x1024 terrain: interactive, ideally >30 FPS
texture:           2048x2048 RGBA
startup time:      dominated by download on first run, fast after caching
```

The example should avoid per-frame CPU uploads. After startup, terrain buffers and texture should remain static. Per-frame updates should normally be limited to camera uniforms and GUI state.

## Validation checks

At runtime, the implementation should validate:

- `positions` exists and has shape `(N, 3)`;
- `indices` exists and is one-dimensional;
- `texcoords` exists and has shape `(N, 2)` if texture is enabled;
- `texture` exists and is `uint8` with shape `(H, W, 3)` or `(H, W, 4)`;
- index values are within `[0, N)`;
- no NaN or inf values are present in positions;
- bounds are finite;
- camera near/far values are consistent with terrain scale.

If validation fails, the script should raise a concise, actionable error.

## Screenshot expectations

The example should be suitable for the Datoviz gallery. A representative screenshot should show:

- a perspective view from above or inside the canyon;
- clearly visible terrain relief;
- textured surface with non-flat coloring;
- no excessive clipping;
- no wireframe/debug overlay by default.

Suggested camera for gallery screenshot:

```text
position: elevated and offset along canyon axis
target:   central canyon region
fov:      50 degrees
```

## Possible extensions

Future versions may add:

- terrain LOD with multiple decimated meshes;
- tiled terrain streaming;
- real orthophoto texture;
- atmospheric scattering or distance fog;
- river mesh or animated water strip;
- shadow map or screen-space ambient occlusion;
- video export using Datoviz recording infrastructure;
- picking on terrain to display elevation and coordinates;
- path editor for the camera route.

These are not required for v1.

## Minimum acceptance criteria

The example is considered successful if:

1. it runs from a clean checkout after installing Datoviz v0.4 Python bindings;
2. it downloads the terrain bundle automatically if missing;
3. it displays a textured 3D terrain mesh;
4. the camera animates smoothly along a flyover or orbit path;
5. depth testing works correctly;
6. the scene remains interactive;
7. there are no per-frame terrain buffer uploads;
8. a screenshot can be captured deterministically for the gallery.

## Implementation notes for the agent

Do not hard-code assumptions about the final v0.4 Python API. Use the available v0.4 API at implementation time, but preserve the conceptual structure:

```text
Scene -> Panel3D -> Camera3D -> Textured indexed mesh visual -> Animation
```

Keep the example concise. The runtime script should focus on loading the prepared data and rendering it. GIS preprocessing belongs outside the example.

Avoid making the demo depend on heavyweight GIS packages at runtime. Runtime dependencies should ideally be limited to:

```text
numpy
Pillow or imageio, only if texture is stored as PNG
Datoviz Python bindings
```

The resulting example should be both a beautiful demo and a practical architectural test for Datoviz v0.4.
