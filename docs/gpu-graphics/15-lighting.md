# 15. Lighting

*About 820 lines of C; raw Vulkan would need about 2150.*

![A checkerboard cube with ambient, diffuse, and specular light.](../assets/gpu-graphics/15-lighting.webp)

The cube is now textured: each face samples a checkerboard through a descriptor (chapter 14), and it turns under the mouse (chapter 11). But every face is equally bright, whichever way it points. Only its outline and the perspective of the tiles make the cube look 3D.

In the real world, light shows the shape of things. A face turned toward a lamp is bright, a face turned away is dark, and a shiny surface shows a highlight where it reflects the lamp toward your eye. This chapter adds one light and computes that effect in the fragment shader. It reuses what you already know: a new vertex attribute (chapter 6), new values in the material uniform (chapter 12), and a reorganized push-constant block (chapter 8). At the end, each face of the textured cube has its own brightness, with a highlight that moves as you turn it.

In this chapter you will:

1. compute ambient, diffuse and specular light for each fragment;
2. move positions and normals into view space, before projection, in the vertex shader;
3. give each vertex a **normal**, the direction its surface faces;
4. add the lighting strengths to the material uniform, and send the model-view matrix on its own.

## How light shows a shape

The lighting model in this chapter adds up three simple effects. Each answers one question about a point on the surface.

**Diffuse light: does the surface face the light?** Hold a sheet of paper under a lamp. It is brightest when it faces the lamp. Tilted, the same light spreads over a larger area, so each point gets less. Turned edge-on or away, it gets none. A matte surface scatters that light equally in all directions, so its brightness does not depend on where you stand.

**Specular light: does the surface reflect the light toward the eye?** A shiny surface also acts a little like a mirror. Where it would bounce the light straight into your eye, you see a bright highlight. Unlike diffuse light, the highlight moves when you move.

**Ambient light: what about the rest?** In a real room, light bounces off walls and reaches surfaces that face away from the lamp. Computing that is expensive, so we add a small constant instead, and no face goes fully black.

### The vectors the shader needs

To answer the first two questions, the shader needs the direction the surface faces at each point. That direction is the **normal**: a unit-length vector perpendicular to the surface, pointing outward. The shader also needs two more directions from the same point: toward the light, and toward the eye.

With those vectors, each question becomes arithmetic:

- Diffuse is the cosine of the angle between the normal and the light direction. For unit vectors, that cosine is their dot product: 1 when the surface faces the light, 0 when it is edge-on, and negative when it is turned away, so we clamp it at 0.
- Specular uses the **halfway vector**, midway between the light and eye directions. When the normal lines up with it, the surface reflects the light into the eye. Raising that dot product to a power, the **specular exponent**, makes the highlight tight (large exponent) or broad (small exponent). This is the **Blinn-Phong** model.
- Ambient is a constant.

Figure 1 places the directions at one illustrative surface point. The light and eye use this chapter's view-space positions. Here the surface normal happens to point toward the eye, so **n** and **v** overlap. The halfway vector bisects the angle between the light and eye directions; it does not point to the midpoint of their positions.

<figure class="dvz-course-figure" id="fig-lighting-vectors" markdown="1">

<div class="dvz-figure-panels" markdown="0">

--8<-- "docs/images/gpu-graphics/light-positions.svg"

--8<-- "docs/images/gpu-graphics/light-directions.svg"

</div>

<figcaption>Figure 1. Lighting compares unit directions at one surface point, all in view space. The halfway direction lies between the light and eye directions.</figcaption>

</figure>

The texture and tint give the surface color, its **albedo**. Ambient and diffuse light scale the albedo. Specular is added on top in white, because a highlight shows the light's color rather than the surface's, and our light is white.

These are this example's choices, made to keep the shader short: the Blinn-Phong model, one white light, and a small constant for ambient light. Many renderers use other lighting models, colored or several lights, and better estimates of the light that bounces around a scene.

### One coordinate space for every vector

The arithmetic only works if every vector is in the same coordinate space. World space or view space both work, as long as all vectors share it. We use view space (chapter 9), where the camera sits at the origin, so the direction toward the eye is easy to compute. View space comes after the model and view matrices but before projection. The spaces after projection distort angles, so lighting does not happen there.

## Light each fragment

The fragment shader turns the model above into code, in four groups: the vectors, the lighting terms, the surface color, and the final composition. Replace `shader.frag` now, block by block. It reads values that the rest of the chapter provides:

- `normal` and `view_position` come from the vertex shader, which you update in the next section;
- `material.lighting` holds the four lighting numbers, which you add to the C material uniform later in the chapter.

You can save each shader as you go, but don't run the program or press **R** in chapter 14's program until [Build and run](#build-and-run): both shaders need the C changes later in the chapter. The file starts with its inputs:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Material { vec4 tint; vec4 lighting; } material;
layout(set = 0, binding = 1) uniform sampler2D tex;
layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 view_position;
layout(location = 0) out vec4 out_color;
```

The material block gains a second `vec4`, `lighting`, after chapter 12's `tint`. The three inputs must use the same locations as the vertex shader's outputs.

### Compute the four vectors

```glsl
void main()
{
    // Unit vectors in view space: normal, toward the light, toward the eye, and halfway.
    vec3 n = normalize(normal);
    vec3 l = normalize(vec3(2.0, 2.0, 0.0) - view_position);
    vec3 v = normalize(-view_position);
    vec3 h = normalize(l + v);
```

`n`, `l`, `v` and `h` are the normal, light, eye and halfway vectors. Every vector here is in view space, where the eye sits at the origin, so the direction toward the eye is `-view_position`. The light at `(2, 2, 0)` is in view space too, so it moves with the camera, like a headlight. Interpolating between normals that differ can make them shorter than unit length, so the shader normalizes `normal` again.

### Compute the lighting terms

```glsl
    // Lighting terms: diffuse from the light angle, specular only where the light reaches.
    float diffuse = max(dot(n, l), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), material.lighting.w) : 0.0;
```

`diffuse` is the clamped cosine from the model. `specular` raises the halfway term to the exponent in `material.lighting.w`, and is skipped on faces turned away from the light.

<div class="dvz-course-equation" markdown="0">

--8<-- "docs/images/gpu-graphics/equation-diffuse.svg"

</div>

Here, *d* is the diffuse term. For unit directions, a dot product of 1 means they point the same way, and 0 means they are perpendicular. Clamping removes negative contributions from light behind the surface.

### Read the surface color

```glsl
    // Surface color from the sRGB texture, decoded to linear values by the sampler.
    vec3 albedo = texture(tex, uv).rgb;
```

### Combine the terms

```glsl
    // Ambient and diffuse scale the tinted albedo; white specular is added on top.
    vec3 color = albedo * material.tint.rgb * (material.lighting.x + material.lighting.y * diffuse)
        + vec3(specular * material.lighting.z);
    out_color = vec4(color, 1.0);
}
```

`lighting.x` is the ambient strength and `lighting.y` the diffuse strength; together they scale the tinted albedo. `lighting.z` scales the white specular highlight added on top.

Figure 2 separates the terms on an illustrative sphere, using the same shader formula, light position and lighting strengths as this program. The sphere has a constant linear surface color and a white tint so the terms are easier to compare; it is not a capture of the checkerboard cube. The shader adds the terms in linear light, then the display encoding is applied. Adding the displayed RGB values yourself would give a different result.

<figure class="dvz-course-figure" id="fig-lighting-components" markdown="1">

<div class="dvz-figure-panels dvz-figure-panels--pairs" markdown="0">

--8<-- "docs/images/gpu-graphics/lighting-ambient.svg"

--8<-- "docs/images/gpu-graphics/lighting-diffuse.svg"

--8<-- "docs/images/gpu-graphics/lighting-specular.svg"

--8<-- "docs/images/gpu-graphics/lighting-combined.svg"

</div>

<figcaption>Figure 2. Ambient and diffuse scale the surface color; white specular is added on top. All four panels use the same scale.</figcaption>

</figure>

### Linear light and sRGB

Lighting math assumes linear values, where doubling a number doubles the light. The texture is `VK_FORMAT_R8G8B8A8_SRGB`, so sampling decodes its RGB to linear values before this arithmetic. On output, an sRGB attachment encodes the result for display, as you saw in chapter 2. A non-sRGB surface format stores the linear value as is, so the image looks darker. A production renderer requires an sRGB format or converts explicitly.

## Move to view space in the vertex shader

The fragment shader needs a position and a normal in view space. Until now, the vertex shader received one combined model-view-projection matrix and only produced the final clip-space position (chapter 9). Now it receives the model-view matrix on its own and does the last two multiplications itself: model-view first, for the view-space position, then projection, for `gl_Position`. It passes the view-space position, normal and UV on to the fragment shader. Replace `shader.vert` with:

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
    // View space: the camera sits at the origin, and the lighting happens here.
    vec4 position = push_data.model_view * vec4(in_position, 1.0);
    view_position = position.xyz;

    // Rotate the normal by the upper-left 3 x 3 part; no translation for a direction.
    normal = mat3(push_data.model_view) * in_normal;
    uv = in_uv;

    // Clip space, for the rasterizer.
    gl_Position = push_data.projection * position;
}
```

`model_view` moves positions into view space. Its upper-left 3 × 3 part rotates normals, which works because this program only rotates and translates. Under nonuniform scale, that matrix would tilt normals away from the perpendicular, and the correct **normal matrix** would be the inverse transpose of that 3 × 3 part. Normals never take translation, because a direction has no position.

The shaders are now complete. The C program still has to supply three of their new inputs: the normal at location 1, the lighting values in the material uniform, and the two-matrix push block. The next sections add them in that order.

## Add face normals

### Extend the vertex record

The vertex shader reads the normal as one more vertex attribute, between position and UV. Replace `Vertex` with:

```c
// One vertex record: a position, the outward surface normal, and the UV coordinates.
typedef struct
{
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;
```

### Give each face its normal

The 24 vertices already split the cube at its edges for UVs. The same split lets each face have its own outward normal; sharing normals across faces would make the edges look rounded. Replace `VERTICES`, and keep chapter 13's `INDICES`:

```c
// Four vertices per face, each with the face's normal and its own UVs.
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

For a cube the normals are simple: each face points along one axis. The front face's four vertices all carry `(0, 0, 1)`, and the left face's carry `(-1, 0, 0)`.

### Read the normal in the pipeline

In `create_pipeline()`, replace the UV attribute with these two. The normal takes location 1, and the UV moves to location 2:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
```

Read them as: from vertex binding 0, location 1 is three floats at the `normal` offset, and location 2 is two floats at the `uv` offset. These match the input locations in `shader.vert`.

## Extend the material uniform

The fragment shader reads four numbers from `material.lighting`: the strengths of ambient, diffuse and specular light, and the specular exponent. They describe the surface and are shared by every draw, so they belong in the material uniform from chapter 12. Add a second `vec4`-sized member:

```c
// Uniform block read through set 0, binding 0: tint, then ambient, diffuse, specular, exponent.
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

The four `lighting` values are the ambient, diffuse and specular strengths, then the specular exponent. Ambient and diffuse add up to 1, so a face that looks straight at the light shows the texture at full brightness. Both members are 16 bytes, so `std140` and C still agree. The descriptor covers `sizeof(Material)`, so it grows with the struct.

## Fit the matrices into 128 bytes

The vertex shader's push block now holds two matrices, `projection` and `model_view`, and the C struct must match it. Two `mat4` values take exactly 128 bytes, the push-constant size every Vulkan device supports (chapter 8). Many devices allow more, but staying within this minimum means the program never needs a larger push-constant range. Replace `Push`:

```c
// Push-constant block: two matrices, copied into each recorded draw.
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

The C side now computes two products instead of one chain. The model-view matrix combines model and view, and the projection matrix absorbs chapter 11's `clip_correction`. In `draw()`, keep the camera, arcball and `clip_correction` lines, but replace the two intermediate matrices and three `multiply_mat4()` calls. The block becomes:

```c
    // Two matrices: model-view for view-space lighting, then projection for the position.
    dvz_camera_mvp(renderer->camera, &mvp);
    dvz_arcball_mvp(renderer->arcball, &mvp);
    // Camera matrices use OpenGL clip coordinates; convert y and z for this Vulkan viewport.
    mat4 clip_correction = {{1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, 0.5f, 0}, {0, 0, 0.5f, 1}};
    multiply_mat4(mvp.view, mvp.model, push.model_view);
    multiply_mat4(clip_correction, mvp.proj, push.projection);
```

The push range in `create_pipeline()` and the push call in `draw()` both take their size from the struct, which now covers 128 bytes. Only the vertex shader reads the push-constant block, so keep `VK_SHADER_STAGE_VERTEX_BIT`.

## Build and run

Build and run as before. The capture goes to `chapter15.png`.

=== "Linux"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter15.png
    ```

=== "macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter15.png
    ```

=== "Windows"

    In a Developer PowerShell for Visual Studio, with the `PATH` setup from chapter 1:

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter15.png
    ```

The checkerboard remains, but each visible face has its own brightness. Drag the cube and watch the faces move through the light. The terminal ends with `validation errors: 0`.

??? tip "Try it"

    1. Set the ambient `0.16f` to `0.0f` and rebuild. Faces turned away from the light go completely black.
    2. Change the exponent `32.0f` to `4.0f` and rebuild. The highlight spreads into a broad sheen; `128.0f` shrinks it to a small spot.
    3. Move the light in `shader.frag` from `(2, 2, 0)` to `(-2, 2, 0)` and press **R**. The lit side switches from right to left, without a C rebuild.

??? info "Under the hood: values between shader stages"

    Raw Vulkan declares the same vertex attributes and push-constant range. The rasterizer interpolates vertex outputs before the fragment shader runs. Vulkan has no lighting model: the shading is all your shader code, and keeping its vectors in one space is your job.

??? warning "When it goes wrong"

    A push-range validation error means one shader still declares the old block, or C and GLSL disagree on its size. A face that stays bright as it turns away suggests an untransformed normal. A highlight that slides oddly when you zoom suggests mixing world-space and view-space vectors.

## What carries over

- Lighting is your shader code, not a GPU feature. Ambient, diffuse and specular terms are a starting point that physically based models refine, with the same inputs: a normal, a light direction and an eye direction.
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
