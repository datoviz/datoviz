# 6. Vertex buffers

**Your program at the end of this chapter: 390 C lines. The raw Vulkan equivalent: around 1300 lines, a rough estimate.**

![A colored square made from two triangles.](../assets/gpu-graphics/06-vertex-buffers.webp)

The shader has contained the triangle's data until now. Real applications generate, load, or update geometry on the CPU, then copy its bytes into a buffer the GPU can read. You will define a vertex record, upload six records, and draw a square as two triangles.

The external shader files, **R** reload callback, and candidate-pipeline swap from chapter 5 stay in place. Shader modules may be destroyed after successful pipeline creation; this program keeps them with the pipeline so startup, reload, and cleanup share one ownership path. The pipeline now describes a vertex buffer, and the draw binds that buffer before requesting vertices.

## Describe one vertex

A **vertex record** collects every attribute for one input vertex. This first record has a two-component position and a three-component color. The field names help the C reader, but the GPU sees only bytes; the pipeline description below tells it how to decode them.

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

Replace your external `shader.vert` with the following shader. Keep `shader.frag` beside it, unchanged from chapter 5.

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

The pipeline needs the same layout in Vulkan terms. A vertex-buffer **binding** identifies one byte stream and its **stride**, the distance from one record to the next. A vertex **attribute** describes one value inside each record: the buffer binding that supplies it, the shader **location** that receives it, its numeric **format**, and its byte offset. A binding number chooses a byte stream; a location names a shader input. They happen to be small integers here, but they serve different purposes. `offsetof` asks C for the actual field offsets instead of assuming that a struct has no padding.

```c
    dvz_graphics_vertex_binding(
        renderer->pipeline, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
```

Locations 0 and 1 must agree with `in_position` and `in_color` in the vertex shader. `VK_FORMAT_R32G32_SFLOAT` reads two consecutive 32-bit floats, while `VK_FORMAT_R32G32B32_SFLOAT` reads three. A format mismatch does not convert arbitrary memory into the intended values; it makes vertex fetch interpret the same bytes differently before each vertex-shader invocation.

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

The `DvzBuffer` is the durable GPU resource referenced by draw commands. This course starts with a host-visible mapped allocation, which lets the CPU copy bytes directly into memory the device can access and keeps the data path easy to inspect. A device-local allocation can offer better access for frequently drawn static geometry on some hardware, but filling it generally requires a host-visible staging buffer and a transfer command. The texture-upload chapter uses that two-step pattern for an image in chapter 13.

`dvz_buffer_upload()` copies `VERTICES` before it returns, so the CPU array does not need to remain alive for the draw. The `DvzBuffer` and its Vulkan allocation do need to remain alive until submitted draws finish.

## Bind, then draw

```c
    DvzSize vertex_offset = 0;
    dvz_cmd_bind_vertex_buffers(
        renderer->commands, 0, 1, renderer->vertex_buffer, &vertex_offset);
    dvz_cmd_draw(renderer->commands, 0, 6, 0, 1);
```

The triangle-list topology consumes six vertices as two independent triples. The duplicated corner records make the square simple, though wasteful. Chapter 7 removes that duplication.

Before `destroy_pipeline(&renderer)` in cleanup, destroy and free the vertex buffer. Keep the device wait first: a submitted frame may still read the allocation.

```c
    if (renderer.vertex_buffer != NULL)
        dvz_buffer_destroy(renderer.vertex_buffer);
    dvz_buffer_free(renderer.vertex_buffer);
```

## Try it

- Change one duplicated corner's color without changing its twin. A diagonal color seam should appear because the two triangles no longer share the same value there.
- Draw only the first three vertices. You should see half of the square.
- Exchange `position` and `color` in `Vertex` and rebuild. `offsetof` and `sizeof` follow the new C layout, so the image stays correct. Replacing an offset with an incorrect literal demonstrates why those helpers matter.

!!! info "Under the hood"
    A buffer needs a Vulkan buffer object, memory requirements, a suitable memory type, an allocation, binding, and mapping or transfer commands. Datoviz's allocator handles that machinery, while usage and allocation flags still express the choices Vulkan needs.

!!! failure "When it goes wrong"
    Wild geometry or flat colors usually point to a wrong stride, format, or offset. A validation error at draw time often means the buffer lacks `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` or was destroyed while a submitted frame still used it.

## Build, run, and capture

Keep using the project from chapter 1. Save `main.c`, `shader.vert`, and `shader.frag` in the `vkcourse` directory, then build and launch the executable from that directory. Press **R** in the live window to rebuild the pipeline from the shader files.

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

The terminal should end with `validation errors: 0`, and `chapter06.png` should contain the square shown above.

??? example "Your `main.c` at the end of chapter 6"

    ```c
    --8<-- "examples/c/vulkan/step06.c"
    ```

??? example "Your `shader.vert` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step06/shader.vert"
    ```

??? example "Your `shader.frag` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step06/shader.frag"
    ```

## Checkpoint

1. What is the difference between a binding stride and an attribute offset?
2. Why use `offsetof` instead of writing `8` by hand?
3. Which object must outlive the submitted draw: `VERTICES`, the GPU buffer, or both?
4. What tradeoff does host-visible memory make?

Next, an index buffer will let both triangles reuse the same four vertices.
