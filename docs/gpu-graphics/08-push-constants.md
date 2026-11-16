# 8. Push constants

*About 443 lines of C; raw Vulkan would need about 1450.*

![A colorful square rotating over a dark background.](../assets/gpu-graphics/08-push-constants.webp)

To animate the square, you need to change a value every frame without rebuilding the pipeline or re-uploading the buffers. In this chapter the indexed square rotates and pulses, driven by two floats the CPU sends with each draw.

## Push constants

**Push constants** are a small block of values copied into the command buffer and visible to the shader stages you choose. Each draw can receive different values without any buffer. Every Vulkan device supports at least 128 bytes; this chapter uses eight, for two floats.

The shader and the pipeline layout must agree on the block's size, offset, and visible stages. `dvz_slots_push()` declares that range in the layout; `dvz_cmd_push_constants()` copies the current value into the recorded commands. Because the bytes are copied, the C variable can be reused as soon as the call returns. Chapter 12 introduces data that is referenced instead of copied.

## Declare the range

Add this struct after `Vertex`. Its two floats match the GLSL block in the same order:

```c
typedef struct
{
    float time;
    float pulse;
} Push;
```

Declare the range before creating the pipeline layout:

```c
    dvz_slots(renderer->device, renderer->slots);
    dvz_slots_push(renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push));
    if (dvz_slots_create(renderer->slots) != 0)
        return -1;
```

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

## Update every frame

Include `<math.h>`, and keep `<stdlib.h>`: the restored `--time` option uses `strtof`. `dvz_time_monotonic_ns()` comes from `<datoviz.h>`, as in chapter 3. Add these fields to `Renderer`:

```c
    uint64_t start_ns;
    float capture_time;
    bool animate;
    bool draw_failed;
```

At the beginning of `draw()`, compute the elapsed time and fill the block:

```c
    float time = renderer->capture_time;
    if (renderer->animate)
        time = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;
    Push push = {.time = time, .pulse = 1.0f + 0.08f * sinf(time * 3.0f)};
```

The values must be recorded before the draw that uses them, so push them after setting the viewport and before binding the vertex buffer:

```c
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;
```

Replace the argument parsing in `main()`. Offscreen runs use `capture_time`, `0.75f` unless `--time` overrides it; only live runs read the clock.

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

The callback cannot return a status, so `draw_failed` carries a failed push back to the loop. Check it after `dvz_canvas_frame()` reports a ready frame, before submitting:

```c
        COURSE_CHECK(!renderer.draw_failed, "push-constant update failed");
```

Reload keeps working: each candidate pipeline declares the same range.

The square shears as it turns: x and y are still normalized device coordinates, which span the whole 4:3 window, so one unit is wider than it is tall. Chapter 9 fixes this with the aspect ratio.

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
