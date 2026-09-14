# 7. Index buffers

**Your program at the end of this chapter: 409 C lines. The raw Vulkan equivalent: around 1350 lines, a rough estimate.**

![The same colored square drawn with four vertices and six indices.](../assets/gpu-graphics/07-index-buffers.webp)

The square in chapter 6 stores six vertex records even though it has only four distinct corners. An **index buffer** is an integer array that tells primitive assembly which vertex records to fetch and in what order. It separates the records themselves from the sequence used to form triangles.

The two shader files and their reload path are unchanged. Only the geometry arrays, a second buffer, and the draw command change.

The image is deliberately identical to chapter 6. This chapter reduces duplicated vertex data and changes how the GPU addresses it; it does not change the square's pixels.

## Four vertices, six references

Replace the six-record `VERTICES` array with these four corners:

```c
static const Vertex VERTICES[4] = {
    {{-0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f}, {0.2f, 1.0f, 0.2f}},
    {{ 0.65f,  0.65f}, {0.2f, 0.3f, 1.0f}},
    {{-0.65f,  0.65f}, {1.0f, 0.8f, 0.2f}},
};
```

Then add the index array:

```c
static const uint16_t INDICES[6] = {0, 1, 2, 0, 2, 3};
```

The first three indices form one triangle; the last three form the other. Corners 0 and 2 are reused. An index selects the complete record, not just its position, so a reused vertex also reuses its color and, later, its texture coordinates and normal. Duplicate a vertex deliberately when two faces occupy the same position but need different attributes, as they will at texture seams and sharp lighting creases.

`uint16_t` supports indices through 65535 and halves index bandwidth compared with `uint32_t`. Large meshes need 32-bit indices. The C element type, bound `VkIndexType`, and uploaded byte count must all agree.

## Create the second buffer

Add `DvzBuffer* index_buffer;` to `Renderer`. The `<stdint.h>` include carried forward from chapter 4 supplies `uint16_t`. Allocate the index-buffer wrapper beside the vertex-buffer wrapper and check both pointers:

```c
    renderer.vertex_buffer = dvz_buffer_create_wrapper();
    renderer.index_buffer = dvz_buffer_create_wrapper();
    COURSE_CHECK(
        renderer.vertex_buffer != NULL && renderer.index_buffer != NULL,
        "buffer allocation failed");
```

After uploading the vertices, create and upload the index buffer:

```c
    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.index_buffer);
    dvz_buffer_size(renderer.index_buffer, sizeof(INDICES));
    dvz_buffer_usage(renderer.index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int index_buffer_result = dvz_buffer_create(renderer.index_buffer);
    COURSE_CHECK(index_buffer_result == 0, "index buffer creation failed");
    dvz_buffer_upload(renderer.index_buffer, 0, sizeof(INDICES), INDICES);
```

The allocation policy matches the vertex buffer, but the usage flag records a different role. Uploading copies all six index values into the mapped allocation.

## Indexed drawing

```c
    dvz_cmd_bind_index_buffer(
        renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 6, 0, 1);
```

The arguments to `dvz_cmd_draw_indexed()` are first index, vertex offset, index count, first instance, and instance count. The bound `VK_INDEX_TYPE_UINT16` tells Vulkan to read each entry as an unsigned 16-bit value; this declaration must match the array's storage. Each value selects a complete record from the already bound vertex buffer. The vertex offset is added to every fetched index; zero is right for this standalone mesh.

In cleanup, after the device wait and before releasing the vertex buffer, release the index buffer too:

```c
    if (renderer.index_buffer != NULL)
        dvz_buffer_destroy(renderer.index_buffer);
    dvz_buffer_free(renderer.index_buffer);
```

## Try it

- Reverse each triangle to `{2, 1, 0, 3, 2, 0}`. The image remains the same until face culling is enabled in chapter 10, but the winding reverses.
- Change the index count from 6 to 3. Only one triangle should remain.
- Make a backup, then change the `INDICES` declaration to `uint32_t` and the binding to `VK_INDEX_TYPE_UINT32` together. Rebuild before running; the upload size already follows the new element type through `sizeof(INDICES)`. The image stays the same while the index buffer doubles in size. Restore both lines together when you finish.

!!! info "Under the hood"
    Indexing does not make primitive assembly search for duplicate positions. You supply the reuse explicitly. The GPU follows the index stream, fetches referenced vertex records, and may reuse recent vertex-shader results through a hardware cache.

!!! failure "When it goes wrong"
    Spikes or missing triangles often mean an index exceeds the vertex count. A wholly scrambled mesh suggests that `VK_INDEX_TYPE_UINT16` or `VK_INDEX_TYPE_UINT32` disagrees with the element type used to fill the buffer.

## Build, run, and capture

Keep using the project from chapter 1. Save `main.c`, `shader.vert`, and `shader.frag` in the `vkcourse` directory, then build and launch the executable from that directory. Press **R** in the live window to rebuild the pipeline from the shader files.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter07.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter07.png
    ```

The terminal should end with `validation errors: 0`. `chapter07.png` should match chapter 6 because indexing changes reuse, not the rendered result.

??? example "Your `main.c` at the end of chapter 7"

    ```c
    --8<-- "examples/c/vulkan/step07.c"
    ```

??? example "Your `shader.vert` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step07/shader.vert"
    ```

??? example "Your `shader.frag` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step07/shader.frag"
    ```

## Checkpoint

1. How do six indices describe a square using four vertex records?
2. When does a mesh require 32-bit indices?
3. Why might two faces deliberately duplicate a vertex at the same position?

You now have the core data path for a mesh: shaders, a pipeline, vertex records, and indices. Chapter 8 starts changing shader data every frame.
