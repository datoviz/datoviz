# 9. Matrices and perspective

**Your program at the end of this chapter: 527 C lines. The raw Vulkan equivalent: around 1550 lines, a rough estimate. The rotating cube's faces overlap in the wrong order.**

![A spinning cube with deliberately incorrect face order.](../assets/gpu-graphics/09-matrices.webp)

The square from chapter 8 becomes a cube. The same vertex buffer, index buffer, shader-file compiler, and reload loop stay in place. Rotation and scaling move from the vertex shader into a model matrix on the CPU, then the shader applies a combined model-view-projection matrix.

## Eight corners and twelve triangles

Include `<datoviz/math/types.h>` for `mat4`. Change the position to three floats and replace the two-float `Push` with a matrix:

```c
typedef struct
{
    float position[3];
    float color[3];
} Vertex;

typedef struct
{
    mat4 mvp;
} Push;
```

Replace both geometry arrays. Each face uses two triangles, and each triangle lists its corners counter-clockwise when viewed from outside the cube:

```c
static const Vertex VERTICES[8] = {
    {{-0.65f, -0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f, -0.65f}, {1.0f, 0.7f, 0.2f}},
    {{ 0.65f,  0.65f, -0.65f}, {0.2f, 0.9f, 0.3f}},
    {{-0.65f,  0.65f, -0.65f}, {0.2f, 0.5f, 1.0f}},
    {{-0.65f, -0.65f,  0.65f}, {0.5f, 0.2f, 1.0f}},
    {{ 0.65f, -0.65f,  0.65f}, {1.0f, 0.3f, 0.7f}},
    {{ 0.65f,  0.65f,  0.65f}, {0.3f, 1.0f, 0.9f}},
    {{-0.65f,  0.65f,  0.65f}, {0.9f, 0.9f, 0.2f}},
};

// Each face winds counter-clockwise when seen from outside the cube.
static const uint16_t INDICES[36] = {
    0, 2, 1, 0, 3, 2,
    4, 5, 6, 4, 6, 7,
    0, 1, 5, 0, 5, 4,
    3, 7, 6, 3, 6, 2,
    1, 2, 6, 1, 6, 5,
    0, 4, 7, 0, 7, 3,
};
```

The buffer sizes already use `sizeof(VERTICES)` and `sizeof(INDICES)`, so they follow the new arrays. Change the position attribute to three floats:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
```

Change the indexed draw count from six to 36:

```c
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 36, 0, 1);
```

## Homogeneous coordinates

The same position passes through several coordinate spaces, each chosen for a different job. The cube's stored coordinates begin in **object space**. The model matrix places them in **world space**. The view matrix expresses the world relative to the camera in **view space**. The projection matrix produces homogeneous **clip space**. Vulkan clips there, divides x, y, and z by `w` to reach **normalized device coordinates**, then the viewport maps those values into framebuffer coordinates.

```mermaid
flowchart LR
    A[Object space] -->|model| B[World space]
    B -->|view| C[View space]
    C -->|projection| D[Clip space]
    D -->|divide by w| E[Normalized device coordinates]
    E -->|viewport| F[Framebuffer coordinates]
```

A position is represented during these transforms as a four-component vector `(x, y, z, 1)`, which lets a matrix express translation as well as rotation and scale. A direction would use `w = 0` because translation should not move it. Perspective comes from the division by clip `w`: in this projection, more distant points acquire a larger `w`, so their x and y values become smaller after division.

Add these two helpers before `create_pipeline()`. `mat4` stores columns, so an element is indexed as `[column][row]`. The temporary product makes multiplication safe when the output is also an input:

```c
static void multiply_mat4(mat4 left, mat4 right, mat4 out)
{
    mat4 product = {{0}};
    for (uint32_t column = 0; column < 4; column++)
        for (uint32_t row = 0; row < 4; row++)
            for (uint32_t inner = 0; inner < 4; inner++)
                product[column][row] += left[inner][row] * right[column][inner];
    for (uint32_t column = 0; column < 4; column++)
        for (uint32_t row = 0; row < 4; row++)
            out[column][row] = product[column][row];
}
```

The model matrix keeps chapter 8's size pulse and rotates the object about y, with a fixed tilt to show three faces. The view matrix represents a camera three units along positive z looking toward the origin. The projection uses a 60-degree vertical field of view (`1.0471976` radians), the framebuffer aspect ratio, and near/far distances of 0.1 and 100:

```c
static void make_mvp(float aspect, float time, mat4 out)
{
    float angle = time * 0.7f;
    float cosine = cosf(angle);
    float sine = sinf(angle);
    float pulse = 1.0f + 0.08f * sinf(time * 3.0f);
    mat4 model = {
        {pulse * cosine, 0, -pulse * sine, 0},
        {0, pulse, 0, 0},
        {pulse * sine, 0, pulse * cosine, 0},
        {0, 0, 0, 1},
    };
    // Tilt the cube so that three faces can be seen.
    float tilt = -0.35f;
    mat4 tilt_matrix = {
        {1, 0, 0, 0},
        {0, cosf(tilt), sinf(tilt), 0},
        {0, -sinf(tilt), cosf(tilt), 0},
        {0, 0, 0, 1},
    };
    multiply_mat4(tilt_matrix, model, model);
    mat4 view = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -3, 1}};
    float near_clip = 0.1f;
    float far_clip = 100.0f;
    float fov_y = 1.0471976f;
    float focal_length = 1.0f / tanf(0.5f * fov_y);
    mat4 projection = {
        {focal_length / aspect, 0, 0, 0},
        {0, -focal_length, 0, 0},
        {0, 0, far_clip / (near_clip - far_clip), -1},
        {0, 0, near_clip * far_clip / (near_clip - far_clip), 0},
    };
    mat4 view_model = {{0}};
    multiply_mat4(view, model, view_model);
    multiply_mat4(projection, view_model, out);
}
```

For a point in front of this camera, view-space z is negative. The `-1` in `projection[2][3]` therefore makes clip `w` positive. The depth coefficients map the near plane to normalized-device z = 0 and the far plane to z = 1, as Vulkan requires. The negative y scale compensates for the positive-height viewport, so world-space positive y appears upward on the image.

## Send the matrix

In `draw()`, keep the elapsed-time calculation but replace the old `Push` initializer with:

```c
    Push push = {0};
    float aspect = (float)frame->extent.width / (float)frame->extent.height;
    make_mvp(aspect, time, push.mvp);
```

`sizeof(Push)` now declares and uploads a 64-byte range. Replace `shader.vert` with this version; `shader.frag` stays unchanged:

```glsl
#version 450

layout(push_constant) uniform Push { mat4 mvp; } push_data;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 color;

void main()
{
    gl_Position = push_data.mvp * vec4(in_position, 1.0);
    color = in_color;
}
```

Recomputing aspect from the current frame prevents stretching after a resize. Live frames still use the monotonic clock; `--png` uses the fixed capture time. The old shader-side `time` and `pulse` members disappear because their effects are now included in `mvp`.

## Why it looks wrong

The cube has no depth attachment yet. A triangle drawn later overwrites an earlier one wherever they overlap, even if the later triangle is farther from the camera. This is the expected result for this chapter; chapter 10 adds visibility testing.

## Try it

1. Change `fov_y` from `1.0471976f` to `0.5235988f` (60 to 30 degrees). The cube becomes larger, like zooming in with a narrower lens.
2. Move `near_clip` to `3.0f`. The near plane cuts through the cube.
3. Set `angle` to zero. Keep the tilt and inspect which faces overwrite one another.

??? info "Under the hood: matrix math belongs to the application"

    Vulkan copies 64 bytes through `vkCmdPushConstants` and runs the vertex shader. It provides no model or camera matrix. The coordinate convention and multiplication order are choices made by this program.

!!! warning "When it goes wrong"

    An invisible cube often means clip w has the wrong sign or multiplication order is reversed. Incorrect face overlap is expected here. If the cube stretches on resize, check the framebuffer aspect ratio.

## Build and run

Keep using the project from chapter 1. Save `main.c`, `shader.vert`, and `shader.frag` in the project directory, then run these commands there. Press **R** in the live window to rebuild the pipeline from the shader files.

```sh
cmake --build build
./build/vkcourse
./build/vkcourse --png frame.png
```

??? example "Your `main.c` at the end of chapter 9"

    ```c
    --8<-- "examples/c/vulkan/step09.c"
    ```

??? example "Your `shader.vert` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step09/shader.vert"
    ```

??? example "Your `shader.frag` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step09/shader.frag"
    ```

## Checkpoint

- What does the perspective divide do?
- Starting with one stored cube position, name each coordinate space it passes through before reaching a framebuffer location.
- Why is the order `projection * view * model`?
- Which depth values do the near and far planes produce?

Next, add depth testing and cull faces that point away from the camera.
