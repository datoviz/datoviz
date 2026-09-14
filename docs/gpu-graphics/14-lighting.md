# 14. Lighting

**Your program at the end of this chapter: 683 C lines. The raw Vulkan equivalent: around 2050 lines, a rough estimate. The textured cube now responds to light.**

![A checkerboard cube with ambient, diffuse, and specular light.](../assets/gpu-graphics/14-lighting.webp)

Continue from chapter 13's textured cube. Surface color tells us what color a face has; a normal tells us which way it faces. We will add face normals and calculate ambient, diffuse, and specular lighting in the fragment shader. The texture upload, descriptors, reload path, depth, and mouse controls stay in place.

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

The 24 vertices already split the cube at face boundaries for UVs. Use that same split for normals: every vertex on one face has the same outward normal. Replace `VERTICES` with this complete array; keep chapter 13's `INDICES`:

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

## Fit the matrices into 128 bytes

Replace `Push` and add the size assertion immediately below it:

```c
typedef struct
{
    mat4 projection;
    mat4 model_view;
} Push;

_Static_assert(sizeof(Push) == 128, "the lighting push block must fit in 128 bytes");
```

Vulkan guarantees at least 128 bytes of push constants. Two 4 × 4 float matrices use exactly that amount. We send the projection and model-view matrices separately, then work in view space: the eye is at the origin, and the light has a fixed position relative to the viewer. The light position will be a shader constant, editable with **R**. A light that moves independently in world space would need another data source, such as a uniform buffer, or a different packing scheme.

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

`model_view` transforms positions into the camera's coordinate system before projection. Its upper-left 3 × 3 part rotates normals. This works for the rotation and translation in this program; nonuniform scale would need the inverse transpose of that 3 × 3 matrix. Translation never belongs in a normal, which describes a direction.

Replace `shader.frag` with:

```glsl
#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;
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
    float specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), 32.0) : 0.0;
    vec3 albedo = texture(tex, uv).rgb;
    vec3 color = albedo * (0.16 + 0.84 * diffuse) + vec3(specular * 0.35);
    out_color = vec4(color, 1.0);
}
```

All lighting vectors are in view space. The light is at `(2, 2, 0)`; the eye is at `(0, 0, 0)`, so the view direction is `-view_position`. Normalize interpolated normals again because interpolation does not preserve their length.

The ambient `0.16` term gives surfaces a small baseline brightness. The diffuse term uses the positive part of the normal/light dot product. Blinn-Phong specular uses the halfway vector between light and view directions, with exponent `32.0` controlling highlight width. We suppress specular when the face points away from the light. The sampled sRGB texture supplies linear RGB values for these calculations; the renderer's sRGB color attachment encodes the output for display.

## Build and run

Keep `main.c`, `shader.vert`, and `shader.frag` in the project directory from chapter 1. Run these commands there, so the program can find the shader files.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse --png chapter14.png
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe --png chapter14.png
    .\build\Release\vkcourse.exe
    ```

Compare the image with chapter 13: the checkerboard remains, but the visible faces now have different brightness. Drag the cube and watch the faces move through the light. Scroll and pan to check that the highlight still uses the camera's actual view-space position. The offscreen initial pose remains deterministic and reports `validation errors: 0`.

!!! tip "Try it"

    1. Set the ambient value `0.16` to `0.0`, save `shader.frag`, and press **R** to see the contribution of unlit faces.
    2. Change the specular exponent `32.0` to `4.0` for a broad highlight or `128.0` for a tight one, then reload.
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
    --8<-- "examples/c/vulkan/step14.c"
    ```

??? example "Current shader.vert"

    ```glsl
    --8<-- "examples/c/vulkan/step14/shader.vert"
    ```

??? example "Current shader.frag"

    ```glsl
    --8<-- "examples/c/vulkan/step14/shader.frag"
    ```
