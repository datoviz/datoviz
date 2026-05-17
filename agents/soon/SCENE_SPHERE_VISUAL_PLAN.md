# Scene Sphere Visual Plan

> **Execution Status**
> - **Status:** `ACTIVE / PARTIALLY IMPLEMENTED`
> - **Updated on:** `2026-05-17`
> - **Purpose:** track the remaining work for the v0.4 sphere impostor visual and its
>   integration with materials, G-buffer output, SSAO, and antialiased borders.

## Current State

The core sphere visual slice is already present in the codebase:

1. `DVZ_VISUAL_TYPE_SPHERE` exists and is wired into the scene runtime.
2. `dvz_sphere()`, `dvz_sphere_mode()`, and the typed position/color/size setters exist.
3. GLSL color, A2C, and G-buffer shader variants are already registered and emitted.
4. Scene tests cover sphere emit execution, mode retention, and SSAO usage.
5. The interactive GLFW sphere SSAO example already exercises the retained visual.

The remaining work is therefore narrower than the original pickup plan.


## Decision

Sphere should be a standalone retained visual family in v0.4, not a marker shape and not a mesh
shape helper.

The v0.3 implementation had the right conceptual split:

1. 2D markers are their own visual family with many planar glyph shapes.
2. Sphere is its own visual family with one semantic shape: a 3D sphere.
3. Sphere rendering is shader-defined: the fragment shader reconstructs the sphere surface from a
   point sprite, computes normals, optionally applies lighting/texturing, and writes sphere-surface
   depth.

Keep that split in v0.4. Add `DVZ_VISUAL_TYPE_SPHERE` and a typed public constructor/setter API
instead of extending marker or primitive semantics.


## v0.3 Reference

Use `v0.3.5` as the behavioral reference, not as a compatibility constraint:

1. `include/datoviz/scene/visuals/sphere.h`
2. `src/scene/visuals/sphere.c`
3. `src/scene/glsl/graphics_sphere.vert`
4. `src/scene/glsl/graphics_sphere.frag`

The v0.3 vertex payload was intentionally small:

```c
struct DvzSphereVertex
{
    vec3 pos;
    DvzColor color;
    float size;
};
```

The v0.3 flags covered texturing, lighting, pixel-size mode, and equirectangular texture
projection. The core untextured lit path is already stable enough in v0.4; the remaining
porting work is mainly feature completion and documentation alignment.


## Target v0.4 Visual Shape

First public slice:

```c
DvzVisual* dvz_sphere(DvzScene* scene, uint32_t flags);
dvz_visual_set_data(sphere, "position", positions, count);
dvz_visual_set_data(sphere, "color", colors, count);
dvz_visual_set_data(sphere, "radius", radii, count);
```

Keep sphere data upload on the generic visual API. Typed sphere entry points are reserved for
behavioral configuration such as `dvz_sphere_mode()`, not position/color/radius payloads.

Current first-slice flags:

1. `DVZ_SPHERE_FLAGS_NONE`
2. `DVZ_SPHERE_FLAGS_LIGHTING`
3. `DVZ_SPHERE_FLAGS_SIZE_PIXELS`, if and when pixel-sized spheres are wired through the
   runtime/shader path

Still deferred:

1. `DVZ_SPHERE_FLAGS_TEXTURED`
2. `DVZ_SPHERE_FLAGS_EQUAL_RECTANGULAR`


## Material And Lighting

Use the current v0.4 scene material layer instead of reviving v0.3's sphere-private material
uniforms.

Sphere should be a lit material-capable visual family:

1. default kind: `DVZ_MATERIAL_KIND_LIT`;
2. shared fields: alpha mode, opacity, ambient, diffuse, specular, shininess, light direction, depth
   cueing;
3. sphere-specific optional fields later: texture projection mode, texture binding, and size unit.

The first implementation already reuses the existing `DvzSceneMaterialState` and
`DvzSceneMaterialParams` path used by primitive/mesh lighting. Add fields only when the sphere
shader genuinely needs data that is not already represented there.


## Shader Strategy

Use a point-list topology for the first GLSL path, as v0.3 did.

Vertex shader:

1. read sphere center, color, and size;
2. transform center through the active panel MVP;
3. compute `gl_PointSize` from projected sphere diameter;
4. pass center, color, radius, and camera/view data to the fragment shader.

Fragment shader:

1. map `gl_PointCoord` to a `[-1, +1]` disc coordinate;
2. discard fragments outside the unit sphere silhouette;
3. reconstruct the front-facing sphere normal analytically;
4. reconstruct the sphere surface position;
5. write `gl_FragDepth` from the projected sphere surface position;
6. shade with the shared material lighting path.

Do not use a tessellated mesh sphere for the core visual. The whole point of this family is efficient
atom/particle-scale rendering with analytic normals and compact per-sphere data.


## Antialiased Borders

Analytic antialiasing is required in the first implementation. Hard disc discard produces visibly
jagged borders and makes sphere clouds look lower quality than the surrounding scene.

Use a derivative-based coverage ramp around the unit-disc implicit function:

```glsl
vec2 coord = 2.0 * gl_PointCoord - 1.0;
float r2 = dot(coord, coord);
float edge = 1.0 - r2;
float aa = fwidth(r2);
float coverage = smoothstep(0.0, aa, edge);

if (coverage <= 0.0)
    discard;
```

Apply coverage to the output alpha at the silhouette while keeping interior opacity unchanged.
Depth should still come from the reconstructed sphere surface for every covered fragment. Do not
move or bias depth to fake a soft edge.

First slice policy:

1. analytic border antialiasing is always enabled;
2. no public knob yet;
3. keep a small internal scale constant if tuning is needed after visual smoke tests;
4. consider alpha-to-coverage/MSAA later, after the shader coverage path is already good.


## G-Buffer And SSAO

Sphere should participate in normal/depth G-buffer techniques from the start.

Add sphere pass capabilities:

1. color;
2. depth;
3. normal/depth;
4. EDL depth source where appropriate;
5. SSAO receiver.

The G-buffer fragment shader should use the same analytic sphere reconstruction as the color pass
and write:

1. encoded/reconstructed normal from the sphere surface;
2. linear or clip-depth output consistent with the current G-buffer convention;
3. `gl_FragDepth` from the projected sphere surface position.

This is the key improvement needed for a useful SSAO example. A dense cloud of sphere impostors
should show contact shadows and cavity darkening far more clearly than the current simple
height-field mesh example.


## Remaining Work

Recommended follow-up commits:

1. Wire `DVZ_SPHERE_FLAGS_SIZE_PIXELS` through the runtime and shaders, or remove it if the
   feature is no longer desired.
2. Port the remaining texture/equirectangular sphere support from v0.3.
3. Update the public visuals docs so they match the actual v0.4 implementation and API.
4. Add or refresh focused regression tests for the remaining sphere-specific feature slices.

## Notes

1. Keep sphere separate from marker and primitive semantics.
2. Reuse the existing material path rather than introducing a sphere-private material system.
3. Preserve the current shader/runtime split: retained sphere data in scene state, shader-defined
   impostor reconstruction in the GLSL path, and SSAO/G-buffer integration through the scene
   pipeline.


## Validation

For the remaining sphere work:

```text
just build
just test scene
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```
