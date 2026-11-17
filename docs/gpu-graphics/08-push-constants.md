# 8. Push constants

*About 443 lines of C; raw Vulkan would need about 1450.*

![A colorful square rotating over a dark background.](../assets/gpu-graphics/08-push-constants.webp)

Chapters 6 and 7 put the square's geometry in GPU buffers: four corners and six indices, uploaded once at startup. The square is drawn every frame, but it never moves.

To animate it, something must change every frame. We could rewrite the corner positions on the CPU and upload them again each frame. That works for four vertices, but it scales with the mesh, and the buffer may still be in use by an earlier frame in flight (chapter 3). A better idea: keep the geometry fixed, send the few numbers that describe the motion, and let the vertex shader apply them to every vertex. Here that is two numbers: an angle and a scale.

This chapter sends those two floats with each draw as push constants, and the square rotates and pulses. Chapter 9 keeps the same mechanism but sends a 4×4 matrix, which can describe any rotation, scaling, camera or perspective.

In this chapter you will:

1. declare a push-constant range in the pipeline layout;
2. read the two values in the vertex shader to rotate and scale the square;
3. compute them from the clock and push them before every draw;
4. add a fixed capture time so that `--png` stays reproducible.

## Small values that travel with the draw

**Push constants** are a small block of values copied into the command buffer and visible to the shader stages you choose. Since chapter 3, `draw()` records the frame's commands every frame, so each frame can push new values. A push applies to the draws recorded after it, until the next push, so two draws in one frame can also receive different values. No buffer is involved.

The price of copying is size. Every Vulkan device supports at least 128 bytes; this chapter uses eight, for two floats, and chapter 9 uses 64, for one matrix.

The shader and the pipeline layout must agree on the block's size, offset, and visible stages. The pipeline layout is chapter 4's `DvzSlots`, empty until now; this is its first entry. `dvz_slots_push()` declares that range in the layout; `dvz_cmd_push_constants()` copies the current value into the recorded commands. Because the bytes are copied, the C variable can be reused as soon as the call returns. Chapter 12 introduces data that is referenced instead of copied.

## Declare the range in the pipeline layout

Start with the C side of the block. Add this struct after `Vertex`. Its two floats match the GLSL block in the same order:

```c
typedef struct
{
    float time;
    float pulse;
} Push;
```

The layout must know about the range before it is created, and the pipeline is built from the layout. In `create_pipeline()`, declare the range between `dvz_slots()` and `dvz_slots_create()`:

```c
    dvz_slots(renderer->device, renderer->slots);
    dvz_slots_push(renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push));
    if (dvz_slots_create(renderer->slots) != 0)
        return -1;
```

Read the new call as: the vertex stage may read a push-constant range starting at byte 0, `sizeof(Push)` bytes long, here 8.

## Read the values in the vertex shader

The shader side of the contract is a `layout(push_constant)` block with the same two floats in the same order. Replace `shader.vert` with this version; `shader.frag` is unchanged. `pulse` scales the positions, then `time` rotates them:

```glsl
#version 450

layout(push_constant) uniform Push { float time; float pulse; } push_data;
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 color;

void main()
{
    float cosine = cos(push_data.time);
    float sine = sin(push_data.time);
    vec2 position = in_position * push_data.pulse;
    gl_Position = vec4(
        cosine * position.x - sine * position.y,
        sine * position.x + cosine * position.y, 0.0, 1.0);
    color = in_color;
}
```

The last lines are the standard 2D rotation by an angle, here `time` in radians: x becomes `cos·x − sin·y` and y becomes `sin·x + cos·y`. Every vertex runs the same code with the same two values, so the whole square turns as one. The vertex buffer never changes.

## Push new values every frame

Include `<math.h>`, and keep `<stdlib.h>`: the restored `--time` option uses `strtof`. `dvz_time_monotonic_ns()` comes from `<datoviz.h>`, as in chapter 3. Add these fields to `Renderer`:

```c
    uint64_t start_ns;
    float capture_time;
    bool animate;
    bool draw_failed;
```

`start_ns` is the clock at startup, `capture_time` the fixed time for captures, `animate` picks between them, and `draw_failed` reports a failed push. At the beginning of `draw()`, compute the elapsed time and fill the block:

```c
    float time = renderer->capture_time;
    if (renderer->animate)
        time = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;
    Push push = {.time = time, .pulse = 1.0f + 0.08f * sinf(time * 3.0f)};
```

Live runs use the seconds elapsed since start. `pulse` swings between 0.92 and 1.08 about every two seconds, so the square grows and shrinks by 8%.

The values must be recorded before the draw that uses them, so push them after setting the viewport and before binding the vertex buffer:

```c
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;
```

Read the push as: into this command buffer, with the layout in `slots`, for the vertex stage, copy `sizeof(push)` bytes from `&push` to offset 0 of the range. It returns an error if that range is not declared in the layout, is not four-byte aligned, or is not visible to the requested stages.

## Keep captures reproducible

As in chapter 3, captures use a fixed time. Replace the argument parsing in `main()`. Offscreen runs use `capture_time`, `0.75f` unless `--time` overrides it; only live runs read the clock.

```c
    const char* png_path = NULL;
    float capture_time = 0.75f;
    for (int argument_index = 1; argument_index + 1 < argc; argument_index++)
    {
        if (strcmp(argv[argument_index], "--png") == 0)
            png_path = argv[++argument_index];
        else if (strcmp(argv[argument_index], "--time") == 0)
            capture_time = strtof(argv[++argument_index], NULL);
    }
    COURSE_CHECK(isfinite(capture_time), "capture time must be finite");
```

Then initialize the timing fields before installing the draw callback:

```c
    renderer.start_ns = dvz_time_monotonic_ns();
    renderer.capture_time = capture_time;
    renderer.animate = live;
```

The draw callback cannot return a status, so `draw_failed` carries a failed push back to the loop. `dvz_canvas_frame()` runs the callback, so check the flag after it reports a ready frame, before submitting:

```c
        COURSE_CHECK(!renderer.draw_failed, "push-constant update failed");
```

Reload keeps working: each candidate pipeline declares the same range, and after a swap `draw()` pushes through the new `renderer->slots`.

The square shears as it turns: x and y are still clip-space coordinates, from −1 to 1 across the whole 4:3 window, so one unit is wider than it is tall. Chapter 9 fixes this with the aspect ratio.

## Build and run

Build and run as before; the capture goes to `chapter08.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter08.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter08.png
    ```

The live square rotates and breathes; the capture is the still above. The terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. In the shader, use `push_data.time * 2.0` for `cos` and `sin`, save, and press **R**. Rotation doubles in speed; the breathing rate is unchanged.
    2. Set `.pulse` to `1.0f` in the C initializer and rebuild. The square keeps rotating at a constant size.
    3. Run `--png` with `--time 0` and `--time 0.75`. The first capture is unrotated; the second is rotated by 0.75 radians, about 43 degrees.

??? info "Under the hood: one small Vulkan update"

    Raw Vulkan puts a `VkPushConstantRange` in `VkPipelineLayoutCreateInfo`, then calls `vkCmdPushConstants` with the exact stage mask, offset, and size. The slots wrapper keeps the declaration and its checks together.

!!! warning "When it goes wrong"

    A validation message about a push-constant range usually means the offset or size is not a multiple of four, or the pipeline layout does not declare the range. A square that stays still was rendered with `--png`, or the shader reads the members in a different order than the C struct.

## What carries over

- Keep large data fixed on the GPU and send a few parameters that describe how it changes; the vertex shader applies them to every vertex. Chapter 9's matrices are this idea in general form.
- A command buffer is an ordered list: a value set before a draw applies to that draw and to later ones until it is set again. Pipelines and vertex buffers follow the same rule.

## Checkpoint

1. Why does a push constant need to be declared in the pipeline layout?
2. What limits how much data you can send as push constants?
3. Why can the `Push` variable be reused immediately after `dvz_cmd_push_constants()` returns?

??? example "Your `main.c` after chapter 8"

    ```c
    --8<-- "examples/c/vulkan/step08.c"
    ```

??? example "Your `shader.vert` after chapter 8"

    ```glsl
    --8<-- "examples/c/vulkan/step08/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 8"

    ```glsl
    --8<-- "examples/c/vulkan/step08/shader.frag"
    ```

Next: [Matrices and perspective](09-matrices.md).
