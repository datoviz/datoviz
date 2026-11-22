# 8. Push constants

*About 550 lines of C; raw Vulkan would need about 1450.*

![A colorful square rotating over a dark background.](../assets/gpu-graphics/08-push-constants.webp)

Chapters 6 and 7 put the square's geometry in GPU buffers: four corners and six indices, uploaded once at startup. The square is drawn every frame, but it never moves.

To animate it, something must change every frame. We could rewrite the corner positions on the CPU and upload them again each frame. That works for four vertices, but the cost grows with the mesh, and an earlier frame in flight may still be reading the buffer (chapter 3). Instead, we keep the geometry fixed, send the few numbers that describe the motion, and let the vertex shader apply them to every vertex. Here the motion needs two numbers: an angle and a scale.

This chapter sends those two floats with each draw as push constants, so the square rotates and pulses. Chapter 9 uses the same mechanism to send a 4×4 matrix, which can describe any rotation, scaling, camera or perspective.

In this chapter you will:

1. declare a push-constant range in the pipeline layout;
2. read the two values in the vertex shader to rotate and scale the square;
3. compute them from the clock and push them before every draw;
4. add a fixed capture time so that `--png` stays reproducible;
5. stop the program if a push fails.

## Small values recorded with the draw

**Push constants** are a small block of values copied into the command buffer and visible to the shader stages you choose. Since chapter 3, `draw()` records the frame's commands every frame, so each frame can push new values. A push applies to the draws recorded after it, until the next push, so two draws in one frame can also receive different values. You do not create or bind a buffer for them.

The space for push constants is limited. Vulkan guarantees at least 128 bytes on every device, and some devices allow more. This course treats 128 bytes as its portable baseline, so it never needs a larger push-constant range. Data that is larger or shared by many draws usually lives in a buffer instead; chapter 12 introduces that route. This chapter uses 8 bytes for two floats, and chapter 9 uses 64 bytes for one matrix.

The copy also explains why push constants suit animation. Each frame's command buffer carries its own copy of the values, so recording frame N+1 with a new angle does not disturb frame N, which the GPU may still be executing. The program can push the next value without waiting. Chapter 12 introduces data that the GPU reads by reference instead, which needs more care.

The shader and the pipeline layout must agree on the block's size, offset and visible stages. The pipeline layout is chapter 4's `DvzSlots`, empty until now; the push-constant range is its first entry. Two calls handle the block:

- `dvz_slots_push()` declares the range in the layout, once, when the pipeline is built;
- `dvz_cmd_push_constants()` copies the current values into the recorded commands, every frame.

## Declare the range in the pipeline layout

Start with the C side of the block. Add this struct after `Vertex`. Its two floats match the GLSL block in the same order:

```c
// Push-constant block: the C side of the shader's Push block, same members in the same order.
typedef struct
{
    float time;
    float pulse;
} Push;
```

The layout must know about the range before it is created, and the pipeline is built from the layout. In `create_pipeline()`, declare the range between `dvz_slots()` and `dvz_slots_create()`:

```c
    // Pipeline layout: one push-constant range of sizeof(Push) bytes, for the vertex stage.
    dvz_slots(renderer->device, renderer->slots);
    dvz_slots_push(renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push));
    if (dvz_slots_create(renderer->slots) != 0)
        return -1;
```

Read the new call as: the vertex stage may read a push-constant range starting at byte 0, `sizeof(Push)` bytes long, here 8.

Reload keeps working without other changes. Each candidate pipeline from chapter 5 runs the same `create_pipeline()`, so it declares the same range in its own layout.

## Read the values in the vertex shader

The shader side of the agreement is a `layout(push_constant)` block with the same two floats in the same order. In GLSL, `uniform` means the values are the same for every vertex of a draw, unlike the `in` variables, which change from vertex to vertex. `layout(push_constant)` says where those values come from: the copy pushed into the command buffer, not a buffer read through a descriptor, which chapter 12 introduces.

Replace `shader.vert` with this version; `shader.frag` is unchanged. `pulse` scales the positions, then `time` rotates them:

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

## Compute the animation time

The C side now needs a time value for each frame. Live runs read the clock; `--png` captures use a fixed time, as in chapter 3, so the same command always produces the same image.

Add `#include <math.h>` to the standard headers, for `sinf()` and `isfinite()`. `<stdlib.h>` is already included and provides `strtof()`, which the `--time` option uses below. `dvz_time_monotonic_ns()` comes from `<datoviz.h>`, as in chapter 3.

Add these fields to `Renderer`, after `reload_requested`:

```c
    uint64_t start_ns;
    float capture_time;
    bool animate;
    bool draw_failed;
```

- `start_ns` is the clock reading at startup, in nanoseconds.
- `capture_time` is the fixed time, in seconds, for `--png` captures.
- `animate` chooses between the two: true for live runs.
- `draw_failed` lets `draw()` report a failed push; a later section uses it.

At the beginning of `draw()`, compute the time for this frame:

```c
    // Elapsed seconds since startup in live runs; the fixed capture time with --png.
    float time = renderer->capture_time;
    if (renderer->animate)
        time = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;
```

The subtraction happens in integer nanoseconds, so the value stays small before it becomes a `float`. Multiplying by `1e-9f` converts it to seconds.

Then fill the push-constant block from that time:

```c
    // This frame's push-constant values: the rotation angle and a scale that swings by 8%.
    Push push = {.time = time, .pulse = 1.0f + 0.08f * sinf(time * 3.0f)};
```

The angle is the elapsed time itself, so the square turns by one radian per second, about one full turn every 6.3 seconds. `pulse` swings between 0.92 and 1.08 about every two seconds, so the square grows and shrinks by 8%.

## Push the values before the draw

A push applies only to draws recorded after it, so record it after setting the viewport and before binding the vertex buffer:

```c
    // Copy the values into the command buffer. draw() cannot return an error, so flag it.
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;
```

Read the push as: into this command buffer, with the layout in `slots`, for the vertex stage, copy `sizeof(push)` bytes from `&push` to offset 0 of the range. It returns an error if that range is not declared in the layout, is not four-byte aligned, or is not visible to the requested stages.

`push` is a local variable that disappears when `draw()` returns. That is safe because the call copies its 8 bytes into the command buffer before returning. The layout is passed by pointer: `renderer->slots` must exist while `draw()` records with it. The program destroys it only after `dvz_device_wait()`, on reload or at cleanup, together with the pipeline built from it. After a reload swaps pipelines, `draw()` pushes through the new `renderer->slots`.

## Keep captures reproducible

`main()` decides which time `draw()` uses. Replace the argument parsing with this version, which brings back chapter 3's `--time` option:

```c
    // With --png PATH, render offscreen and save an image; --time SECONDS sets its fixed time.
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

Offscreen runs use `capture_time`, which is `0.75f` unless `--time` sets another value. The `isfinite()` check rejects values such as `inf` or `nan`, which would give the shader no usable angle.

Then, after uploading the index buffer and before installing the draw callback, initialize the timing fields:

```c
    // Live runs measure time from this moment; --png runs use the fixed capture time.
    renderer.start_ns = dvz_time_monotonic_ns();
    renderer.capture_time = capture_time;
    renderer.animate = live;
```

They must be set before `dvz_canvas_set_draw_callback()`, because the first `draw()` reads them.

## Stop when a push fails

`draw()` returns `void`: the canvas calls it from inside `dvz_canvas_frame()` and has no way to pass a status back. The `draw_failed` flag carries the failure to the render loop instead. Check it after `dvz_canvas_frame()` reports a ready frame and before submitting:

```c
        // draw() has run; stop before submitting a frame whose push was rejected.
        COURSE_CHECK(!renderer.draw_failed, "push-constant update failed");
```

If the flag is set, `COURSE_CHECK` jumps to cleanup and the frame is never submitted. Without the check, the program would submit a draw whose push was rejected, so the shader would read push-constant values that were never set.

## Build and run

Build and run as before; the capture goes to `chapter08.png`.

=== "Linux"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter08.png
    ```

=== "macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter08.png
    ```

=== "Windows"

    In a Developer PowerShell for Visual Studio, with the `PATH` setup from chapter 1:

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter08.png
    ```

The live square rotates and pulses; the capture is the still image above. The terminal ends with `validation errors: 0`.

The square looks stretched as it turns. Its x and y are still clip-space coordinates, from −1 to 1 across the whole 4:3 window, so one unit is wider than it is tall. Chapter 9 corrects this with the aspect ratio.

??? tip "Try it"

    1. In the shader, use `push_data.time * 2.0` for `cos` and `sin`, save, and press **R**. The rotation speed doubles; the pulse rate does not change.
    2. Set `.pulse` to `1.0f` in the C initializer and rebuild. The square keeps rotating at a constant size.
    3. Run `--png` with `--time 0` and `--time 0.75`. The first capture is unrotated; the second is rotated by 0.75 radians, about 43 degrees.

??? info "Under the hood: one small Vulkan update"

    Raw Vulkan puts a `VkPushConstantRange` in `VkPipelineLayoutCreateInfo`, then calls `vkCmdPushConstants` with the exact stage mask, offset and size. The slots wrapper keeps the declaration and its checks together.

??? warning "When it goes wrong"

    If the program stops with `push-constant update failed`, `dvz_cmd_push_constants()` rejected the update. Check that the offset and size are multiples of four, and that the pipeline layout declares the range for the shader stage that uses it. The validation layer may also report that the shader uses push constants that were never set, because the rejected update recorded no push command.

    If the square stays still, you probably rendered with `--png`, where time is fixed. If it barely turns but keeps growing, the members are in a different order in the shader than in the C struct.

## What carries over

- Keep large data fixed on the GPU and send a few parameters that describe how it changes; the vertex shader applies them to every vertex. Chapter 9's matrices generalize this idea.
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
