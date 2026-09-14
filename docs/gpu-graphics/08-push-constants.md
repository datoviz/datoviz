# 8. Push constants

**Your program at the end of this chapter: 444 C lines. The raw Vulkan equivalent: around 1450 lines, a rough estimate. The indexed square rotates and changes size.**

![A colorful quad rotating over a dark background.](../assets/gpu-graphics/08-push-constants.webp)

The quad from chapter 7 already has a vertex buffer and an index buffer. This chapter gives its vertex shader a small piece of changing data. The result moves every frame, and the CPU does not rebuild either buffer.

## Three routes to shader data

**Push constants** are a small block of values copied into the command stream and exposed directly to selected shader stages. Each draw can receive different values without referring to a buffer or descriptor set. Vulkan guarantees that every device supports at least 128 bytes, though a device may support more; this example uses eight bytes for two floats.

A **uniform buffer** stores read-only shader parameters in a buffer range reached through a descriptor, while a **storage buffer** permits broader access patterns, including large arrays and shader writes when declared that way. These choices depend on capacity, access, update frequency, sharing, and device limits. Push constants are often convenient for small per-draw values; uniform buffers commonly hold structured parameters shared across draws; storage buffers suit data that needs their larger or less restricted interface. Chapter 12 builds the complete uniform-buffer and descriptor path.

The shader interface and the **pipeline layout** must agree about the block's size, offset, and visible stages. `dvz_slots_push()` declares that contract while `dvz_cmd_push_constants()` copies the current value into the recorded commands. The local C `Push` variable can therefore be reused when that call returns; submitted work does not retain its address. A descriptor works differently: it refers to a resource that must remain valid while the GPU may use it.

## Declare the slot

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

Replace `shader.vert` with this version. The input records and fragment shader are unchanged. Rotation happens in the shader; `pulse` scales the same positions before rotating them.

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

Include `<math.h>` and `<datoviz/common/functions.h>`. Add these fields to `Renderer`, keeping the existing resources and reload flag:

```c
    uint64_t start_ns;
    float capture_time;
    bool animate;
    bool draw_failed;
```

At the beginning of `draw()`, after obtaining `renderer`, calculate the elapsed time and fill the two values:

```c
    float time = renderer->capture_time;
    if (renderer->animate)
        time = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;
    Push push = {.time = time, .pulse = 1.0f + 0.08f * sinf(time * 3.0f)};
```

After setting the viewport and before binding the vertex buffer, record the push-constant update:

```c
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;
```

Replace the old PNG argument parsing in `main()` with the following block. `--time` overrides the default `capture_time` of `0.75f` for a still image. Only the live path reads the clock.

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

Then add this initialization before installing the draw callback:

```c
    renderer.start_ns = dvz_time_monotonic_ns();
    renderer.capture_time = capture_time;
    renderer.animate = live;
```

After `dvz_canvas_frame()` reports a ready frame, check `renderer.draw_failed` before submitting. The callback cannot return a status, so this flag passes a failed update back to the frame loop.

```c
        COURSE_CHECK(!renderer.draw_failed, "push-constant update failed");
```

The shader-file compiler and **R** reload path keep working. A successful reload builds the same push-constant declaration into the replacement pipeline layout. If C declares eight bytes but GLSL expects another size or member order, the shader reads a different interface from the one the command supplies.

## Try it

1. Double the time used for `cos` and `sin` in the shader, save, and press **R**. Rotation speeds up without changing the breathing rate.
2. Set the C initializer's `.pulse` value to `1.0f` and rebuild. Rotation continues while the square stays the same size.
3. Change `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST` to `VK_PRIMITIVE_TOPOLOGY_LINE_LIST`; the six indices now describe three independent lines.

??? info "Under the hood: one small Vulkan update"

    Raw Vulkan needs a `VkPushConstantRange` in `VkPipelineLayoutCreateInfo`, then a `vkCmdPushConstants` call with the exact stage mask, offset, and byte count. Datoviz's slots wrapper keeps the declaration and validation together. The command still belongs to the borrowed Canvas command buffer, so the Canvas remains responsible for ending and submitting it.

!!! warning "When it goes wrong"

    A validation message about a push-constant range usually means the offset or size is not four-byte aligned, or the pipeline layout did not declare the range. A quad that stays still usually has `animate` disabled because it was rendered with `--png`, or the shader is reading a different member order than the C struct.


## Build and run

Keep using the project from chapter 1. Save `main.c`, `shader.vert`, and `shader.frag` in the project directory, then run these commands there. Press **R** in the live window to rebuild the pipeline from the shader files.

```sh
cmake --build build
./build/vkcourse
./build/vkcourse --png frame.png
```

??? example "Your `main.c` at the end of chapter 8"

    ```c
    --8<-- "examples/c/vulkan/step08.c"
    ```

??? example "Your `shader.vert` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step08/shader.vert"
    ```

??? example "Your `shader.frag` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step08/shader.frag"
    ```

## Checkpoint

- Why does a push constant need to be declared in the pipeline layout?
- Which values belong in a uniform buffer instead of a push constant?
- Why can the `Push` variable be reused immediately after `dvz_cmd_push_constants()` returns?

The next chapter replaces the quad's screen coordinates with a cube and pushes a complete model-view-projection matrix.
