# 15. Lighting

*About 729 lines of C; raw Vulkan would need about 2150.*

![A checkerboard cube with ambient, diffuse, and specular light.](../assets/gpu-graphics/15-lighting.webp)

A flat-colored cube hides its shape; light reveals it. In this chapter each face gets brighter or darker depending on how it faces a light, with a highlight on the shiny side. Surface color says how a material reflects light; a **normal** is a direction perpendicular to the surface that says which way it faces. You add face normals and compute ambient, diffuse, and specular light per fragment.

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

The 24 vertices already split the cube at its edges for UVs. The same split gives each face its own outward normal; sharing normals across faces would round off the edges. Replace `VERTICES`, and keep chapter 14's `INDICES`:

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

In `create_pipeline()`, replace the UV attribute with these two. Normal takes location 1, and UV moves to location 2:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
```

## Extend the material uniform

The lighting coefficients are shared by every draw, so they belong in the material uniform from chapter 12. Add a second `vec4`-sized member:

```c
typedef struct
{
    float tint[4];
    float lighting[4];
} Material;

_Static_assert(
    sizeof(Material) == 32, "the aligned material block must match two shader vec4 values");
```

Replace the initializer in `create_material()`:

```c
    Material material = {
        .tint = {1.0f, 1.0f, 1.0f, 1.0f}, .lighting = {0.16f, 0.84f, 0.35f, 32.0f}};
```

The four `lighting` values are the ambient, diffuse, and specular strengths, then the specular exponent. Both members are 16 bytes, so `std140` and C still agree. The descriptor covers `sizeof(Material)`, so it grows with the struct.

## Fit the matrices into 128 bytes

Lighting needs positions and normals in view space, before projection, so the vertex shader needs the model-view matrix on its own. Send projection and model-view as two matrices: two `mat4`s are exactly the 128 bytes every device guarantees. Replace `Push`:

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

In `draw()`, keep the camera, arcball, and `clip_correction`, but replace the two intermediate matrices and three `multiply_mat4()` calls with:

```c
    multiply_mat4(mvp.view, mvp.model, push.model_view);
    multiply_mat4(clip_correction, mvp.proj, push.projection);
```

`sizeof(Push)` now covers 128 bytes. Only the vertex shader reads the push-constant block, so keep `VK_SHADER_STAGE_VERTEX_BIT`.

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

`model_view` moves positions into view space. Its upper-left 3 × 3 part rotates normals, which works because this program only rotates and translates. Under nonuniform scale, that matrix would tilt normals off the perpendicular; the correct normal matrix is then the inverse transpose of that 3 × 3 part. Normals never take translation: a direction has no position.

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

Every vector here is in view space, where the eye sits at the origin, so the view direction is `-view_position`. The light at `(2, 2, 0)` is in view space too: it moves with the camera, like a headlight. Interpolation shortens normals, so normalize again.

Ambient gives every surface a baseline brightness. Diffuse uses the positive part of the normal-light dot product. Blinn-Phong specular uses the halfway vector between the light and view directions, with the exponent setting the highlight's width, and is skipped on faces turned away from the light.

??? note "Linear light and sRGB"

    Lighting math assumes linear values. The texture is `VK_FORMAT_R8G8B8A8_SRGB`, so sampling decodes its RGB to linear before this arithmetic. On output, an sRGB attachment encodes the result for display, as you saw in chapter 2. A non-sRGB surface format stores the linear value as is, so the image looks darker. A production renderer requires an sRGB format or converts explicitly.

## Build and run

Build and run as before; the capture goes to `chapter15.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter15.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter15.png
    ```

The checkerboard remains, but each visible face has its own brightness. Drag the cube and watch the faces move through the light. The terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Set the ambient `0.16f` to `0.0f` and rebuild. Faces turned away from the light go completely black.
    2. Change the exponent `32.0f` to `4.0f` and rebuild. The highlight spreads into a broad sheen; `128.0f` shrinks it to a small spot.
    3. Move the light in `shader.frag` from `(2, 2, 0)` to `(-2, 2, 0)` and press **R**. The lit side switches from right to left, without a C rebuild.

??? info "Under the hood: values between shader stages"

    Raw Vulkan declares the same vertex attributes and push-constant range. The rasterizer interpolates vertex outputs before the fragment shader runs. Vulkan has no lighting model; the shading is all your shader code, and keeping its vectors in one space is your job.

!!! warning "When it goes wrong"

    A push-range validation error means one shader still declares the old block, or C and GLSL disagree on its size. A face that stays bright as it turns away suggests an untransformed normal. A highlight that slides oddly when you zoom suggests mixing world-space and view-space vectors.

## Checkpoint

1. Why does the lighting calculation need a position before projection?
2. Why does the push-constant block split into projection and model-view, and why does it still fit?
3. Why do cube faces need separate normals, and when is an inverse-transpose normal matrix necessary?

??? example "Your `main.c` after chapter 15"

    ```c
    --8<-- "examples/c/vulkan/step15.c"
    ```

??? example "Your `shader.vert` after chapter 15"

    ```glsl
    --8<-- "examples/c/vulkan/step15/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 15"

    ```glsl
    --8<-- "examples/c/vulkan/step15/shader.frag"
    ```

Next: [A real mesh](16-mesh.md).
