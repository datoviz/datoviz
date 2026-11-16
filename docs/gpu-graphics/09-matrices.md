# 9. Matrices and perspective

*About 526 lines of C; raw Vulkan would need about 1550.*

![A spinning cube with deliberately incorrect face order.](../assets/gpu-graphics/09-matrices.webp)

Time for 3D. The square becomes a cube, seen through a perspective camera. Rotation and scaling move from the vertex shader to the CPU, which combines three matrices into one and pushes it each frame. The result looks wrong on purpose: faces overlap in the wrong order until chapter 10.

## Eight corners and twelve triangles

Include `<datoviz/math/types.h>` for `mat4`. Give positions three floats, and replace the two-float `Push` with a matrix:

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

Replace both geometry arrays. Each face is two triangles, and each triangle lists its corners counter-clockwise when seen from outside the cube:

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

The buffer sizes use `sizeof(VERTICES)` and `sizeof(INDICES)`, so they follow. Change the position attribute to three floats:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
```

Change the indexed draw count from 6 to 36:

```c
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 36, 0, 1);
```

## Homogeneous coordinates

One position passes through several coordinate spaces, each suited to a different job:

- **Object space**: the cube's stored coordinates.
- **World space**: where the model matrix places the object.
- **View space**: the world relative to the camera, after the view matrix.
- **Clip space**: the projection matrix's output. Vulkan clips here.
- **Normalized device coordinates** (NDC): clip coordinates divided by `w`. The viewport maps them to framebuffer pixels.

```mermaid
flowchart LR
    A[Object space] -->|model| B[World space]
    B -->|view| C[View space]
    C -->|projection| D[Clip space]
    D -->|divide by w| E[Normalized device coordinates]
    E -->|viewport| F[Framebuffer coordinates]
```

A position is a four-component vector `(x, y, z, 1)`, so a 4 × 4 matrix can translate it as well as rotate and scale it. A direction uses `w = 0`, so translation leaves it alone. Perspective comes from the division by `w`: this projection gives distant points a larger `w`, so they shrink after division.

The three matrices combine into one **model-view-projection (MVP) matrix**, `projection * view * model`. The matrix nearest the vector applies first: the model matrix moves the vertex into the world, then the view, then the projection.

Add these two helpers before `create_pipeline()`. `mat4` stores columns, so an element is `[column][row]`. The temporary product makes it safe to pass the same matrix as input and output:

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

In the initializers below, each inner brace of a `mat4` is one column, so the matrices read transposed on the page. The model matrix keeps chapter 8's pulse and rotates the cube about y, with a fixed tilt to show three faces. The view puts the camera three units along positive z, looking at the origin. The projection uses a 60-degree vertical field of view (`1.0471976` radians), the framebuffer aspect ratio, and near and far distances of 0.1 and 100:

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

Points in front of this camera have negative view-space z, so the `-1` in `projection[2][3]` makes clip `w` positive. The depth terms map the near plane to NDC z = 0 and the far plane to z = 1, as Vulkan requires. Vulkan's NDC y points down, which is why chapter 4's triangle needed a negative y for its top vertex; the negative y scale here flips it, so world-space up appears up.

## Send the matrix

In `draw()`, keep the elapsed-time calculation but replace the old `Push` initializer:

```c
    Push push = {0};
    float aspect = (float)frame->extent.width / (float)frame->extent.height;
    make_mvp(aspect, time, push.mvp);
```

Dividing by the current aspect ratio fixes chapter 8's shear and keeps the cube square after a resize. `sizeof(Push)` now declares and pushes 64 bytes. Replace `shader.vert`; `shader.frag` is unchanged:

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

## Why it looks wrong

The cube has no depth attachment yet. A triangle drawn later overwrites an earlier one wherever they overlap, even if it is farther from the camera. Chapter 10 adds visibility testing.

## Build and run

Build and run as before; the capture goes to `chapter09.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter09.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter09.png
    ```

The cube spins with some back faces drawn over front ones. The terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Change `fov_y` from `1.0471976f` to `0.5235988f` (60 to 30 degrees). The cube looks bigger, as through a narrower lens.
    2. Move `near_clip` to `3.0f`. The near plane slices off the front of the cube, and you see into it.
    3. Set `angle` to zero and keep the tilt. The cube stops rotating, and you can study which faces wrongly cover others.
    4. Swap the arguments in the last `multiply_mat4()` call. The cube vanishes: matrix multiplication does not commute.

??? info "Under the hood: matrix math belongs to the application"

    Vulkan copies 64 bytes through `vkCmdPushConstants` and runs the vertex shader. It has no model or camera matrix. The coordinate conventions and multiplication order are this program's choices.

!!! warning "When it goes wrong"

    An invisible cube often means clip `w` has the wrong sign or the multiplication order is reversed. Wrong face overlap is expected here. If the cube stretches on resize, check the aspect ratio.

## Checkpoint

1. What does the perspective divide do?
2. Starting from one stored cube position, name each coordinate space it passes through before reaching a framebuffer pixel.
3. Why is the order `projection * view * model`?
4. Which depth values do the near and far planes produce?

??? example "Your `main.c` after chapter 9"

    ```c
    --8<-- "examples/c/vulkan/step09.c"
    ```

??? example "Your `shader.vert` after chapter 9"

    ```glsl
    --8<-- "examples/c/vulkan/step09/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 9"

    ```glsl
    --8<-- "examples/c/vulkan/step09/shader.frag"
    ```

Next: [Depth and culling](10-depth-culling.md).
