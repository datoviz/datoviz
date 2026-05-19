# Scene Sphere Render Modes Follow-Up

> **Execution Status**
> - **Status:** `IMPLEMENTED / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** preserve validation and quality follow-ups for fast and raycast sphere impostor
>   modes.


## Current Status

The mode API and raycast impostor path have landed. The durable public contract lives in
[`../../../spec/scene/visuals/SPHERE.md`](../../../spec/scene/visuals/SPHERE.md).

Keep this note for execution details that may still matter while tuning quality:

1. fast impostor and raycast impostor are modes of one sphere visual family;
2. both modes use the same retained payload: center position, color, radius, material state, and
   optional future texture state;
3. mode switching should remain cheap and should not require rebuilding user data buffers;
4. G-buffer behavior must match the color path for depth, normal, and SSAO input quality.


## Remaining Mode Work

Recommended follow-up commits:

1. Add or refresh live example controls that compare `FAST_IMPOSTOR` and `RAYCAST_IMPOSTOR`
   without recreating the scene.
2. Keep a close-up foreground sphere in the example so perspective-correct raycast depth and normal
   behavior are visible.
3. Validate the raycast path with MSAA and alpha-to-coverage enabled. Raycast improves hit
   correctness, while MSAA/A2C remains the final silhouette-quality path.
4. If the uniform mode branch becomes too divergent, split shader variants through the existing
   scene shader descriptor mechanism:
   - sphere fast color;
   - sphere raycast color;
   - sphere fast G-buffer;
   - sphere raycast G-buffer.
5. Add visual-regression notes or image baselines if example screenshots become part of the release
   gate.


## Shader Notes

Fast mode reconstructs the normal from `gl_PointCoord` and writes depth from the reconstructed
front sphere surface. It is compact and suitable for small or medium spheres.

Raycast mode still uses rasterization, not Vulkan hardware ray tracing. Per fragment, it
reconstructs the camera ray, intersects that ray with the view-space sphere, writes exact surface
depth, and feeds the same material and G-buffer outputs as the color path.

Large near-camera spheres may eventually need a quad-billboard path if point-size limits become a
runtime constraint.


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
