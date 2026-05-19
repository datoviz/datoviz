# Scene Sphere Render Modes Plan

> **Execution Status**
> - **Status:** `IMPLEMENTED / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** preserve the rationale and validation guidance for the fast and raycast sphere
>   impostor modes.


## Current Status

The sphere mode API and raycast impostor path have landed. The durable contract now lives in
`spec/scene/visuals/SPHERE.md`.

Keep this note for shader rationale, validation commands, and future quality work around mode
comparison, MSAA/alpha-to-coverage, and example controls.


## Decision

Sphere should remain one visual family with selectable rendering modes:

1. `FAST_IMPOSTOR`: current point-coordinate reconstruction;
2. `RAYCAST_IMPOSTOR`: exact camera-ray/sphere intersection in the fragment shader.

Do not create a second public visual family for raycast spheres. The data model is the same:

1. center position;
2. color;
3. radius;
4. material;
5. optional texture fields later.

The render mode is an implementation and quality choice, not a different semantic object.


## Public API Shape

Add a small enum:

```c
typedef enum DvzSphereMode
{
    DVZ_SPHERE_MODE_FAST_IMPOSTOR = 0,
    DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR = 1,
} DvzSphereMode;
```

Add a typed setter:

```c
DVZ_EXPORT int dvz_sphere_mode(DvzVisual* visual, DvzSphereMode mode);
```

The setter should:

1. reject non-sphere visuals;
2. update retained sphere state;
3. mark the required material/visual uniform dirty;
4. avoid rebuilding buffers;
5. avoid destroying and recreating the visual.


## Implementation Strategy

Prefer one sphere shader pair with a uniform mode switch for the first implementation:

1. same vertex buffers;
2. same point-list topology;
3. same material bind group;
4. same color and G-buffer shader entry points;
5. one extra mode field in a sphere parameter uniform or an existing per-visual parameter block.

This keeps the example toggle cheap and makes it easy to compare modes interactively.

If the branch becomes too divergent or the compiler produces poor code, split into shader variants
later using the existing scene shader descriptor mechanism:

1. sphere fast color;
2. sphere raycast color;
3. sphere fast G-buffer;
4. sphere raycast G-buffer.

Do not start with duplicated pipelines unless the uniform mode switch proves inadequate.


## Fast Impostor Mode

Current mode:

1. vertex shader draws a point sprite sized from projected sphere radius;
2. fragment shader maps `gl_PointCoord` to a disc coordinate;
3. normal is reconstructed as `vec3(x, y, sqrt(1 - x*x - y*y))`;
4. surface depth is computed from the reconstructed view-space surface position.

Advantages:

1. cheap;
2. compact;
3. good enough for small and medium spheres;
4. simple G-buffer support.

Known limits:

1. not fully perspective-correct for large spheres close to the camera;
2. silhouette and depth can be subtly approximate;
3. antialiasing still needs MSAA/alpha-to-coverage for final quality.


## Raycast Impostor Mode

Raycast mode still uses rasterization. It is not Vulkan hardware ray tracing.

Per fragment:

1. reconstruct the camera ray through the current pixel in view space;
2. intersect that ray with the sphere defined by center and radius;
3. discard if there is no hit;
4. use the nearest positive hit distance;
5. compute exact view-space/world-space surface position;
6. compute exact normal;
7. project the hit position and write exact `gl_FragDepth`;
8. feed the existing material and depth-cue path;
9. write matching normal/depth data in the G-buffer shader.

Expected benefits:

1. better perspective correctness for large or near spheres;
2. more accurate depth and normals near silhouettes;
3. better SSAO input for close-up sphere clouds;
4. cleaner foundation for future texture projection and rim/reflection effects.

Costs:

1. more fragment math;
2. no scene-wide ray traversal;
3. overlap is still resolved by raster depth, which is appropriate for opaque spheres;
4. very large near-camera spheres may eventually need quad billboards instead of point sprites.


## Uniform Data

Raycast mode needs enough data to reconstruct rays.

Candidate additions:

1. inverse projection matrix;
2. inverse view matrix if world-space shading needs it;
3. viewport size or panel rect;
4. mode field;
5. optional near/far values if depth reconstruction helpers need them.

Prefer adding these to a sphere-specific parameter block if the common MVP uniform should remain
small and stable. If the same inverse matrices are needed by SSAO and other techniques, factor them
into the common scene/panel uniform instead.


## Shader Details

View-space ray reconstruction sketch:

```glsl
vec2 uv = (gl_FragCoord.xy - viewport_origin) / viewport_size;
vec2 ndc = uv * 2.0 - 1.0;
ndc.y = -ndc.y;
vec4 nearH = invProj * vec4(ndc, 0.0, 1.0);
vec4 farH = invProj * vec4(ndc, 1.0, 1.0);
vec3 ro = nearH.xyz / nearH.w;
vec3 rd = normalize(farH.xyz / farH.w - ro);
```

Sphere intersection in view space:

```glsl
vec3 oc = ro - centerView.xyz;
float b = dot(oc, rd);
float c = dot(oc, oc) - radius * radius;
float h = b * b - c;
if (h < 0.0)
    discard;
float t = -b - sqrt(h);
if (t <= 0.0)
    discard;
vec3 surfaceView = ro + t * rd;
```

The exact mapping may differ because Vulkan clip/depth conventions and the current Datoviz
projection transform already apply a `y` and `z` mapping. The implementation should add a focused
offscreen smoke rather than trusting the sketch blindly.


## Example Changes

Update `hello_sphere_ssao_glfw`:

1. add an ImGui combo or radio control: `Fast` / `Raycast`;
2. keep SSAO controls next to it because the G-buffer quality difference is most visible there;
3. keep material controls once the material model plan lands;
4. optionally add one larger foreground sphere so perspective differences are obvious.

The example should let the user toggle modes live without recreating the scene.


## Interaction With MSAA

Raycast mode improves surface correctness, not final silhouette antialiasing by itself.

For final edge quality:

1. use raycast mode for exact hit position/normal/depth;
2. use MSAA plus alpha-to-coverage for smooth opaque silhouettes;
3. avoid source-over blending for opaque sphere edges.

The sphere mode work and the MSAA work are complementary.


## Implementation Order

Recommended commits:

1. Add `DvzSphereMode`, retained sphere mode state, and `dvz_sphere_mode()`.
2. Add a sphere parameter uniform if needed for mode and inverse projection/viewport data.
3. Implement raycast mode in the color shader.
4. Implement the same mode in the G-buffer shader.
5. Add focused tests that both modes execute through the GLSL/vklite path.
6. Add live mode switching to `hello_sphere_ssao_glfw`.
7. Add a visual-quality note in the example comments explaining that MSAA remains the final edge
   quality upgrade.


## Validation

Focused validation:

```text
cmake --build build --target dvztest_scene hello_sphere_ssao_glfw -j 8
./build/testing/dvztest_scene test_scene_sphere_emit_glsl_executes
./build/testing/dvztest_scene test_scene_sphere_ssao_glsl_executes
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

If the shader mode becomes a variant rather than a uniform branch, also run shader-registry and
visual-pipeline focused tests.
