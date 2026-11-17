# 15. Lighting

*About 729 lines of C; raw Vulkan would need about 2150.*

![A checkerboard cube with ambient, diffuse, and specular light.](../assets/gpu-graphics/15-lighting.webp)

The cube is now textured: each face samples a checkerboard through a descriptor (chapter 14), and it turns under the mouse (chapter 11). Yet every face is equally bright, whichever way it points. The cube reads as 3D only through its outline and the perspective of the tiles.

In the real world, shape shows through light. A face turned toward a lamp is bright, a face turned away is dark, and a shiny surface shows a highlight where it reflects the lamp toward your eye. This chapter adds one light and computes that effect in the fragment shader. It reuses everything so far: a new vertex attribute (chapter 6), new values in the material uniform (chapter 12), and a reorganized push-constant block (chapter 8). At the end, each face of the textured cube has its own brightness, with a highlight that moves as you turn it.

In this chapter you will:

1. give each vertex a **normal**, the direction its surface faces;
2. add the lighting strengths to the material uniform;
3. send the model-view matrix on its own, so the shaders can work before projection;
4. compute ambient, diffuse, and specular light for each fragment.

## How light reveals a shape

The lighting model in this chapter adds up three simple effects. Each answers one question about a point on the surface.

**Diffuse light: does the surface face the light?** Hold a sheet of paper under a lamp. Facing the lamp, it is brightest. Tilted, the same light spreads over a larger area, so each point gets less. Turned edge-on or away, it gets none. A matte surface scatters that light equally in all directions, so its brightness does not depend on where you stand.

**Specular light: does the surface reflect the light toward the eye?** A shiny surface also acts a little like a mirror. Where it would bounce the light straight into your eye, you see a bright highlight. Unlike diffuse light, the highlight moves when you move.

**Ambient light: what about the rest?** In a real room, light bounces off walls and reaches surfaces that face away from the lamp. Computing that is expensive, so we add a small constant instead, and no face goes fully black.

To answer the first two questions, the shader needs the direction the surface faces at each point. That is the **normal**: a unit-length vector perpendicular to the surface, pointing outward. It also needs two more directions from the same point: toward the light, and toward the eye.

With those vectors, the questions become arithmetic:

- Diffuse is the cosine of the angle between the normal and the light direction. For unit vectors, that cosine is their dot product: 1 when the surface faces the light, 0 when edge-on, negative when turned away, so we clamp it at 0.
- Specular uses the **halfway vector**, midway between the light and eye directions. When the normal lines up with it, the surface reflects the light into the eye. Raising that dot product to a power, the **specular exponent**, makes the highlight tight (large exponent) or broad (small one). This is the **Blinn-Phong** model.
- Ambient is a constant.

The texture and tint give the surface color, its **albedo**. Ambient and diffuse light scale it. Specular is added on top in white, because a highlight shows the light's color, not the surface's.

One rule makes it all work: every vector must be in the same coordinate space. We use **view space**, where the camera sits at the origin. It comes after the model and view matrices but before projection, which would distort angles.

## Add face normals

Normals are one more vertex attribute, between position and UV. Replace `Vertex` with:

```c
typedef struct
{
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;
```

The 24 vertices already split the cube at its edges for UVs. The same split gives each face its own outward normal; sharing normals across faces would round off the edges. Replace `VERTICES`, and keep chapter 13's `INDICES`:

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

The normals are simple for a cube: each face points along one axis, so the front face's four vertices all carry `(0, 0, 1)` and the left face's carry `(-1, 0, 0)`.

In `create_pipeline()`, replace the UV attribute with these two. Normal takes location 1, and UV moves to location 2:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
```

Read them as: from vertex binding 0, location 1 is three floats at the `normal` offset, and location 2 is two floats at the `uv` offset. The vertex shader will use the same locations.

## Extend the material uniform

The model needs four numbers: how strong ambient, diffuse, and specular light are, and the specular exponent. They describe the surface and are shared by every draw, so they belong in the material uniform from chapter 12. Add a second `vec4`-sized member:

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

The four `lighting` values are the ambient, diffuse, and specular strengths, then the specular exponent. Ambient and diffuse add up to 1, so a face that looks straight at the light shows the texture at full brightness. Both members are 16 bytes, so `std140` and C still agree. The descriptor covers `sizeof(Material)`, so it grows with the struct.

## Fit the matrices into 128 bytes

So far the vertex shader received one combined matrix, model-view-projection, and only needed the final clip-space position (chapter 9). Lighting needs positions and normals in view space, before projection, so the vertex shader needs the model-view matrix on its own. Send projection and model-view as two matrices: two `mat4`s are exactly the 128 bytes every device guarantees. Replace `Push`:

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

The C side now computes two products instead of one chain. The model-view matrix combines model and view; the projection matrix absorbs chapter 9's `clip_correction`. In `draw()`, keep the camera, arcball, and `clip_correction`, but replace the two intermediate matrices and three `multiply_mat4()` calls with:

```c
    multiply_mat4(mvp.view, mvp.model, push.model_view);
    multiply_mat4(clip_correction, mvp.proj, push.projection);
```

`sizeof(Push)` now covers 128 bytes. Only the vertex shader reads the push-constant block, so keep `VK_SHADER_STAGE_VERTEX_BIT`.

## Shade each fragment

The vertex shader now does the last two multiplications itself: model-view first, to get the view-space position the lighting needs, then projection, for `gl_Position`. It passes the view-space position, normal, and UV on to the fragment shader. Replace `shader.vert` with:

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

The fragment shader turns the model from the start of the chapter into code, one line per vector or term. Replace `shader.frag` with:

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

Map each line back to the model. `n`, `l`, and `v` are the normal, the light direction, and the eye direction, all unit length; `h` is the halfway vector. Ambient gives every surface a baseline brightness. Diffuse uses the positive part of the normal-light dot product. Blinn-Phong specular uses the halfway vector between the light and view directions, with the exponent setting the highlight's width, and is skipped on faces turned away from the light. The last lines scale the albedo by ambient plus diffuse and add the white highlight.

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

## What carries over

- Lighting is your shader code, not a GPU feature. Ambient, diffuse, and specular terms are a starting point that physically based models refine, with the same inputs: a normal, a light direction, and an eye direction.
- Pick one coordinate space for all lighting vectors and transform everything into it. Positions take the full matrix; directions such as normals take only its 3 × 3 part, or its inverse transpose under nonuniform scale.
- Do lighting math on linear values, and let sRGB formats decode inputs and encode outputs.

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
