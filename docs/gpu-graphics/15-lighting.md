# 15. Lighting

**Your program at the end of this chapter: 714 C lines. The raw Vulkan equivalent: around 2150 lines, a rough estimate. The textured cube now responds to light.**

![A checkerboard cube with ambient, diffuse, and specular light.](../assets/gpu-graphics/15-lighting.webp)

Continue from chapter 14's textured cube. Surface color describes how a material reflects light; a **normal** is a direction perpendicular to the surface and tells the lighting calculation which way that surface faces. You will add face normals and calculate ambient, diffuse, and specular terms in the fragment shader. The texture upload, descriptors, reload path, depth, and mouse controls stay in place.

## Add face normals

Replace `Vertex` with:

```c
typedef struct
{
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;
```

The 24 vertices already split the cube at face boundaries for UVs. Use that same split for normals: every vertex on one face has the same outward direction. Sharing one normal across adjacent faces would smooth the edge; duplicating the records preserves the cube's hard creases. Replace `VERTICES` with this complete array; keep chapter 14's `INDICES`:

```c
static const Vertex VERTICES[24] = {
    // Back face.
    {{0.65f, -0.65f, -0.65f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    // Front face.
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, 0.65f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    // Bottom face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.65f, -0.65f, 0.65f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    // Top face.
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    // Right face.
    {{0.65f, -0.65f, 0.65f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, -0.65f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // Left face.
    {{-0.65f, -0.65f, -0.65f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, 0.65f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, -0.65f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
};
```

In `create_pipeline()`, replace the UV attribute declaration with these normal and UV declarations. Position remains at location 0, normal takes location 1, and UV moves to location 2:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
```

## Extend the material uniform

The material buffer from chapter 12 already occupies set 0 binding 0. Extend its C structure with a second aligned `vec4` rather than introducing another data path:

```c
typedef struct
{
    float tint[4];
    float lighting[4];
} Material;

_Static_assert(sizeof(Material) == 32, "the aligned material block must match two shader vec4 values");
```

Replace the local initializer in `create_material()` with:

```c
    Material material = {.tint = {1.0f, 1.0f, 1.0f, 1.0f}, .lighting = {0.16f, 0.84f, 0.35f, 32.0f}};
```

The four `lighting` components are ambient strength, diffuse strength, specular strength, and specular exponent. Both members are `vec4`-sized, so C and GLSL retain the simple `std140` agreement introduced in chapter 12. The buffer remains initialized once before drawing; the descriptor at binding 0 automatically covers the new `sizeof(Material)` range when the pipeline is created.

## Fit the matrices into 128 bytes

Replace `Push` with:

```c
typedef struct
{
    mat4 projection;
    mat4 model_view;
} Push;
```

Add the size assertion after the material assertion:

```c
_Static_assert(sizeof(Push) == 128, "the lighting push block must fit in 128 bytes");
```

Vulkan guarantees at least 128 bytes of push constants. Two 4 × 4 float matrices use exactly that amount. They remain per-draw push data, while the material and lighting coefficients remain persistent uniform data. Sending projection and model-view separately lets the shaders perform lighting in view space, where the eye is at the origin. The light position is a shader constant editable with **R**; a movable light could become another aligned member of the material uniform.

In `draw()`, keep the camera and arcball updates and `clip_correction`. Replace the two local intermediate matrices and the three `multiply_mat4()` calls after that correction with:

```c
    multiply_mat4(mvp.view, mvp.model, push.model_view);
    multiply_mat4(clip_correction, mvp.proj, push.projection);
```

The existing `sizeof(Push)` declaration and `sizeof(push)` upload now cover 128 bytes. Keep `VK_SHADER_STAGE_VERTEX_BIT` in both places: only the vertex shader reads the push block. The fragment shader receives interpolated view-space positions and normals.

## Shade each fragment

Replace `shader.vert` with:

```glsl
#version 450

layout(push_constant) uniform Push { mat4 projection; mat4 model_view; } push_data;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 view_position;

void main()
{
    vec4 position = push_data.model_view * vec4(in_position, 1.0);
    view_position = position.xyz;
    normal = mat3(push_data.model_view) * in_normal;
    uv = in_uv;
    gl_Position = push_data.projection * position;
}
```

`model_view` transforms positions into the camera's coordinate system before projection. Its upper-left 3 × 3 part rotates normals. This works because this program uses rotation and translation without nonuniform scale. Under nonuniform scale, transforming a normal by that same matrix can make it cease to be perpendicular to the surface; the correct normal matrix is the inverse transpose of the model-view matrix's linear 3 × 3 part. Translation never belongs in a normal because a direction has no position.

Replace `shader.frag` with:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Material { vec4 tint; vec4 lighting; } material;
layout(set = 0, binding = 1) uniform sampler2D tex;
layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 view_position;
layout(location = 0) out vec4 out_color;

void main()
{
    vec3 n = normalize(normal);
    vec3 l = normalize(vec3(2.0, 2.0, 0.0) - view_position);
    vec3 v = normalize(-view_position);
    vec3 h = normalize(l + v);
    float diffuse = max(dot(n, l), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), material.lighting.w) : 0.0;
    vec3 albedo = texture(tex, uv).rgb;
    vec3 color = albedo * material.tint.rgb * (material.lighting.x + material.lighting.y * diffuse) + vec3(specular * material.lighting.z);
    out_color = vec4(color, 1.0);
}
```

All lighting vectors are in view space. The light is at `(2, 2, 0)`; the eye is at `(0, 0, 0)`, so the view direction is `-view_position`. Keeping position, normal, light direction, and view direction in one space makes their dot products meaningful. Normalize the interpolated normal again because interpolation does not preserve unit length.

The ambient coefficient `material.lighting.x` gives surfaces a baseline brightness. The diffuse term uses the positive part of the normal/light dot product. Blinn-Phong specular uses the halfway vector between light and view directions, with `material.lighting.w` controlling highlight width. Specular is suppressed when the face points away from the light. Because the texture is concretely `VK_FORMAT_R8G8B8A8_SRGB`, sampling decodes its RGB texels to linear values before this arithmetic. The sRGB color attachment encodes the shader's linear output for display.

## Build and run

Keep `main.c`, `shader.vert`, and `shader.frag` in the project directory from chapter 1. Run these commands there, so the program can find the shader files.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse --png chapter15.png
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe --png chapter15.png
    .\build\Release\vkcourse.exe
    ```

Compare the image with chapter 14: the checkerboard remains, but the visible faces now have different brightness. Drag the cube and watch the faces move through the light. Scroll and pan to check that the highlight still uses the camera's actual view-space position. The offscreen initial pose remains deterministic and reports `validation errors: 0`.

!!! tip "Try it"

    1. Set the C initializer's ambient value `0.16f` to `0.0f`, rebuild, and inspect the contribution of unlit faces.
    2. Change the C initializer's specular exponent `32.0f` to `4.0f` for a broad highlight or `128.0f` for a tight one, then rebuild.
    3. Move the shader's light from `(2, 2, 0)` to `(-2, 2, 0)`. The bright side should change without a C rebuild.

??? info "Under the hood: values between shader stages"

    Raw Vulkan declares the same vertex formats, attribute locations, and push-constant range. The rasterizer interpolates vertex outputs before invoking the fragment shader. vklite records the pipeline declarations and push update; it does not choose the lighting model or reconcile mismatched coordinate spaces.

!!! warning "When it goes wrong"

    A push-range validation error usually means one of the shader files still declares the old block, or C and GLSL disagree about its size. A face that stays bright while turning away from the light suggests an untransformed normal. A moving highlight that disagrees with zoom or pan suggests mixing world-space and view-space values. Keep the light, eye direction, surface position, and normal in one space.

## Checkpoint

1. Why does the lighting calculation need a position before projection?
2. How does the view-space choice keep the push block at 128 bytes?
3. Why do cube faces need separate normals, and when is an inverse-transpose normal matrix necessary?

??? example "Full current listing"

    ```c
    --8<-- "examples/c/vulkan/step15.c"
    ```

??? example "Current shader.vert"

    ```glsl
    --8<-- "examples/c/vulkan/step15/shader.vert"
    ```

??? example "Current shader.frag"

    ```glsl
    --8<-- "examples/c/vulkan/step15/shader.frag"
    ```
