# Scene Sphere Visual Plan

> **Execution Status**
> - **Status:** `PICKUP PLAN`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the v0.4 port of the v0.3 ray-traced sphere impostor visual and its
>   integration with materials, G-buffer output, SSAO, and antialiased borders.


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
projection. Port those incrementally after the core untextured lit path is stable.


## Target v0.4 Visual Shape

First public slice:

```c
DvzVisual* dvz_sphere(DvzScene* scene, uint32_t flags);
int dvz_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, const vec3* pos);
int dvz_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, const DvzColor* color);
int dvz_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, const float* size);
```

Keep the API typed. Do not introduce a generic material or binding API just to support sphere.

Recommended first flags:

1. `DVZ_SPHERE_FLAGS_NONE`
2. `DVZ_SPHERE_FLAGS_LIGHTING`
3. `DVZ_SPHERE_FLAGS_SIZE_PIXELS`, only if the projection math remains simple in the first port

Defer texture flags until the core visual, G-buffer path, and SSAO example are validated:

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

The first implementation can reuse the existing `DvzSceneMaterialState` and
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


## Implementation Order

Recommended commits:

1. Add the retained visual type, public declarations, internal attribute layout, and basic tests
   that frame-plan metadata recognizes sphere as its own family.
2. Add GLSL color-pass shaders with lighting and analytic antialiased silhouettes.
3. Add runtime shader/pipeline selection and a bounded offscreen smoke that renders several
   overlapping spheres without SSAO.
4. Add sphere G-buffer shaders and visual pass capabilities.
5. Extend SSAO tests so sphere visuals feed the normal/depth path.
6. Add a new interactive GLFW example with many spheres, arcball auto-rotation, SSAO controls, and
   material/size controls.
7. Port texture/equirectangular support if the untextured lit path is stable and the example proves
   useful.


## Validation

For the first implementation slice:

```text
just build
just test scene
./build/examples/c/<new_sphere_example> 2
git diff --check
```

For G-buffer/SSAO support:

```text
just test test_scene_ssao
just test test_app_offscreen_<sphere_ssao_test_name>
./build/examples/c/<new_sphere_ssao_example> 2
git diff --check
```

