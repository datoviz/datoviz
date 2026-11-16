# 6. Vertex buffers

*About 390 lines of C; raw Vulkan would need about 1300.*

![A colored square made from two triangles.](../assets/gpu-graphics/06-vertex-buffers.webp)

Real applications don't hard-code geometry in shaders. They generate, load, or update it on the CPU, then copy its bytes into a buffer the GPU can read. In this chapter you define a vertex record, upload six of them, and draw a square as two triangles.

## Describe one vertex

A **vertex record** collects every attribute of one vertex. This one has a two-component position and a three-component color. The field names help you, but the GPU sees only bytes; the pipeline description below tells it how to decode them.

```c
typedef struct
{
    float position[2];
    float color[3];
} Vertex;
```

Add `<stddef.h>` for `offsetof`, and place these six records below `Vertex`. The two triangles duplicate their shared corners:

```c
static const Vertex VERTICES[6] = {
    {{-0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f}, {0.2f, 1.0f, 0.2f}},
    {{ 0.65f,  0.65f}, {0.2f, 0.3f, 1.0f}},
    {{-0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f,  0.65f}, {0.2f, 0.3f, 1.0f}},
    {{-0.65f,  0.65f}, {1.0f, 0.8f, 0.2f}},
};
```

Replace `shader.vert` with this version; `shader.frag` is unchanged:

```glsl
#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 color;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    color = in_color;
}
```

The pipeline needs the same layout in Vulkan terms. A **vertex binding** is one byte stream with a stride, the distance from one record to the next.

A vertex **attribute** is one value inside each record: which vertex binding supplies it, which shader location receives it, its format, and its byte offset. A binding number picks a byte stream; a location names a shader input. `offsetof` asks C for the real field offsets instead of assuming the struct has no padding.

```c
    dvz_graphics_vertex_binding(
        renderer->pipeline, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
```

Locations 0 and 1 must match `in_position` and `in_color`. `VK_FORMAT_R32G32_SFLOAT` reads two 32-bit floats, `VK_FORMAT_R32G32B32_SFLOAT` three. Get the format wrong and the GPU reads the same bytes as something else.

## Allocate and upload

Add `DvzBuffer* vertex_buffer;` to `Renderer`. In `main()`, after creating the pipeline, allocate the wrapper, configure the buffer, create it, and upload the records:

```c
    renderer.vertex_buffer = dvz_buffer_create_wrapper();
    COURSE_CHECK(renderer.vertex_buffer != NULL, "vertex buffer allocation failed");
    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.vertex_buffer);
    dvz_buffer_size(renderer.vertex_buffer, sizeof(VERTICES));
    dvz_buffer_usage(renderer.vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.vertex_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int buffer_result = dvz_buffer_create(renderer.vertex_buffer);
    COURSE_CHECK(buffer_result == 0, "vertex buffer creation failed");
    dvz_buffer_upload(renderer.vertex_buffer, 0, sizeof(VERTICES), VERTICES);
```

GPUs have two broad kinds of memory. Device-local memory is fast GPU memory the CPU usually cannot write; host-visible memory is CPU-mappable. The two allocation flags make this buffer host-visible and mapped, so `dvz_buffer_upload()` can copy directly. Chapter 13 uses a staging copy to fill a device-local image.

`dvz_buffer_upload()` copies `VERTICES` before it returns, so the CPU array need not stay alive. The `DvzBuffer` must stay alive until every submitted draw that reads it has finished.

## Bind, then draw

```c
    DvzSize vertex_offset = 0;
    dvz_cmd_bind_vertex_buffers(
        renderer->commands, 0, 1, renderer->vertex_buffer, &vertex_offset);
    dvz_cmd_draw(renderer->commands, 0, 6, 0, 1);
```

The triangle-list topology reads six vertices as two independent triples. The duplicated corners are simple but wasteful; chapter 7 removes them.

A submitted frame may still read the buffer, so in cleanup destroy it after the device wait and before `destroy_pipeline(&renderer)`:

```c
    if (renderer.vertex_buffer != NULL)
        dvz_buffer_destroy(renderer.vertex_buffer);
    dvz_buffer_free(renderer.vertex_buffer);
```

## Build and run

Build and run as before; the capture goes to `chapter06.png`. Press **R** in the live window to reload the shaders.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter06.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter06.png
    ```

You see the square above, and the terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Change the color of vertex 3 (a copy of vertex 0) without changing vertex 0. A hard color edge appears along the diagonal, because the two triangles no longer agree at that corner.
    2. Draw only the first three vertices. Half the square remains.
    3. Swap the `position` and `color` fields in `Vertex`, and the two brace groups in every `VERTICES` entry. The image stays correct: `offsetof` and `sizeof` follow the new layout. Then replace `offsetof(Vertex, position)` with `0`. The square scrambles, because the GPU now reads color bytes as positions.

??? info "Under the hood: a buffer and its memory"

    Raw Vulkan creates a buffer object, queries its memory requirements, picks a suitable memory type, allocates and binds memory, then maps it or records a transfer. It also fills a `VkPipelineVertexInputStateCreateInfo` with the binding and attribute descriptions. Datoviz's allocator handles the memory; you still choose the usage and allocation flags.

!!! warning "When it goes wrong"

    Wild geometry or flat colors usually mean a wrong stride, format, or offset. A validation error at draw time often means the buffer lacks `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`, or was destroyed while a submitted frame still used it. Red `has not been destroyed` messages after `validation errors: 0`, or a VMA `Some allocations were not freed` assertion, mean a buffer is missing from cleanup: those checks run when the device is destroyed, after the count is printed.

## Checkpoint

1. What is the difference between a binding stride and an attribute offset?
2. Why use `offsetof` instead of writing `8` by hand?
3. Which must outlive the submitted draw: `VERTICES`, the GPU buffer, or both?
4. What does host-visible memory make easy, and what does chapter 13 do instead for an image?

??? example "Your `main.c` after chapter 6"

    ```c
    --8<-- "examples/c/vulkan/step06.c"
    ```

??? example "Your `shader.vert` after chapter 6"

    ```glsl
    --8<-- "examples/c/vulkan/step06/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 6"

    ```glsl
    --8<-- "examples/c/vulkan/step06/shader.frag"
    ```

Next: [Index buffers](07-index-buffers.md).
