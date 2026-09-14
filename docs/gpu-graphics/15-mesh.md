# 15. A real mesh

**Your program at the end of this chapter: 689 C lines. The raw Vulkan equivalent: around 2100 lines, a rough estimate. The final viewer displays a rotatable, textured, lit sphere.**

![The final generated sphere viewer, textured and lit.](../assets/gpu-graphics/15-mesh.webp)

Continue from chapter 14. Replace the hand-authored cube with a generated sphere. Geometry supplies CPU positions, normals, UVs, and indices; the renderer converts and uploads those arrays. The shaders, image, descriptor, 128-byte push block, depth test, culling, input handling, and safe **R** reload all stay the same.

## Replace the cube arrays

Add this include:

```c
#include <datoviz/geom.h>
```

Delete the complete `VERTICES` and `INDICES` definitions. Keep `Vertex` and `Push`. Add this capacity beside `TEXTURE_SIZE`:

```c
#define MAX_VERTICES ((24 + 1) * (48 + 1))
```

A sphere with 24 rings and 48 sectors has `(24 + 1) * (48 + 1)`, or 1225 vertices. The extra column duplicates the UV seam. A fixed local conversion buffer keeps this small example's allocation visible and bounded; a larger or imported mesh should use checked dynamic storage.

Add this field beside the buffers in `Renderer`:

```c
    uint32_t index_count;
```

Add this complete helper between `create_texture()` and `draw()`:

```c
/**
 * Convert generated CPU geometry to the pipeline's float vertices and uint32 indices.
 * @param renderer Renderer that owns the uploaded GPU buffers.
 * @return Zero on success.
 */
static int create_mesh(Renderer* renderer)
{
    int result = -1;
    DvzGeometry* geometry = dvz_geometry_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc), .radius = 0.9, .rings = 24, .sectors = 48});
    if (geometry == NULL || geometry->positions == NULL || geometry->normals == NULL ||
        geometry->texcoords == NULL || geometry->indices == NULL || geometry->vertex_count == 0 ||
        geometry->vertex_count > MAX_VERTICES || geometry->index_count == 0)
        goto cleanup;

    // The sphere supplies radial normals, including matching normals across its UV seam.
    Vertex vertices[MAX_VERTICES] = {0};
    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            vertices[i].position[j] = (float)geometry->positions[i][j];
            vertices[i].normal[j] = (float)geometry->normals[i][j];
        }
        vertices[i].uv[0] = (float)geometry->texcoords[i][0];
        vertices[i].uv[1] = (float)geometry->texcoords[i][1];
    }
    DvzSize vertex_bytes = (DvzSize)geometry->vertex_count * sizeof(Vertex);
    DvzSize index_bytes = (DvzSize)geometry->index_count * sizeof(DvzIndex);
    renderer->index_count = geometry->index_count;
    renderer->vertex_buffer = dvz_buffer_create_wrapper();
    renderer->index_buffer = dvz_buffer_create_wrapper();
    if (renderer->vertex_buffer == NULL || renderer->index_buffer == NULL)
        goto cleanup;
    dvz_buffer(renderer->device, renderer->allocator, renderer->vertex_buffer);
    dvz_buffer_size(renderer->vertex_buffer, vertex_bytes);
    dvz_buffer_usage(renderer->vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer->vertex_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    dvz_buffer(renderer->device, renderer->allocator, renderer->index_buffer);
    dvz_buffer_size(renderer->index_buffer, index_bytes);
    dvz_buffer_usage(renderer->index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer->index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    if (dvz_buffer_create(renderer->vertex_buffer) != 0 ||
        dvz_buffer_create(renderer->index_buffer) != 0)
        goto cleanup;
    dvz_buffer_upload(renderer->vertex_buffer, 0, vertex_bytes, vertices);
    dvz_buffer_upload(renderer->index_buffer, 0, index_bytes, geometry->indices);
    result = 0;

cleanup:
    dvz_geometry_destroy(geometry);
    return result;
}
```

The generator stores positions, normals, and texture coordinates as doubles. `Vertex` uses floats, so the conversion copies each component explicitly. The indices remain `DvzIndex`, a 32-bit unsigned type. Casting the counts to `DvzSize` before multiplying keeps these byte calculations in the wider size type. The generator supplies valid triangle indices; the helper rejects absent attributes, empty data, and vertex counts beyond the conversion buffer's capacity.

`dvz_buffer_upload()` copies CPU bytes before returning. The local float array and `geometry` can therefore disappear after both uploads. The GPU buffers remain owned by `Renderer` and keep the existing device-wait cleanup.

## Change initialization and the draw count

In `main()`, replace the entire vertex/index allocation and upload block, from `renderer.vertex_buffer = ...` through the index upload, with:

```c
    int mesh_result = create_mesh(&renderer);
    COURSE_CHECK(mesh_result == 0, "mesh creation failed");
```

Keep texture creation, pipeline creation, and controller creation that follow. In `draw()`, replace the index binding and indexed draw with:

```c
    dvz_cmd_bind_index_buffer(renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT32);
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, renderer->index_count, 0, 1);
```

The index type changes from `VK_INDEX_TYPE_UINT16` to `VK_INDEX_TYPE_UINT32`; the draw count changes from 36 to the generator's actual count. The vertex layout and both shader files are unchanged from chapter 14.

## Add a deterministic preview angle

The documentation build turns several fixed starting angles into the animated course preview. Extend the existing argument loop with `--time`:

```c
    const char* png_path = NULL;
    float capture_time = 0.0f;
    for (int argument_index = 1; argument_index + 1 < argc; argument_index++)
    {
        if (strcmp(argv[argument_index], "--png") == 0)
            png_path = argv[++argument_index];
        else if (strcmp(argv[argument_index], "--time") == 0)
            capture_time = strtof(argv[++argument_index], NULL);
    }
```

Use that value only when choosing the arcball's initial orientation:

```c
    DvzResult initial_result = dvz_arcball_initial(
        renderer.arcball, (vec3){-0.35f, 0.65f + (float)DVZ_PI * capture_time, 0.0f});
```

The live viewer still responds to the mouse. This argument makes offscreen preview frames reproducible without synthesizing input events. The angle advances by π radians per second, so the two-second preview completes one full turn. Its 12 frames at six frames per second sample `--time` values from `0` through `11 / 6`; returning to the first frame advances by the same 30 degrees as every other frame transition.

## Keep the sphere's smooth normals

The sphere generator supplies radial normals: each points away from its center. Duplicated seam vertices share the same normal even though one uses U = 0 and its counterpart uses U = 1. Copy these normals as the helper does above. Sharing a position does not require sharing its UV, but a smooth seam should still share its lighting direction.

For an indexed mesh that has positions and storage for normals but needs their values computed, insert this optional block after validating `geometry` and before the conversion loop:

```text
DvzResult normal_result = dvz_geometry_compute_normals(geometry);
if (normal_result != DVZ_OK)
    goto cleanup;
```

This function accumulates triangle normals at each indexed vertex, then normalizes the result. It averages only triangles sharing that index. It will not automatically smooth duplicated UV-seam vertices together or preserve a crease if both faces share a vertex. Recomputing the sphere's analytic normals is an experiment, not a required step: it can introduce small seam or pole artifacts. For the canonical sphere, keep the normals the generator already supplies.

## Build and run

Keep `main.c`, `shader.vert`, and `shader.frag` in the project directory from chapter 1. Run these commands there, so the program can find the shader files.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse --png chapter15.png
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe --png chapter15.png
    .\build\Release\vkcourse.exe
    ```

You should see a smooth sphere with a curved checkerboard and a specular highlight. UVs compress near the poles because a rectangular image is mapped around a sphere. Drag to rotate, scroll to zoom, and press **R** to reload shader calculations. Two offscreen captures should match and each run should report `validation errors: 0`. The optional `--time` argument changes the deterministic starting angle used by generated course previews; it does not replace mouse control in the live viewer.

!!! tip "Try it"

    1. Lower the sphere's `rings` to `8` and `sectors` to `16`, rebuild, and inspect the silhouette. The existing conversion buffer is large enough for this smaller mesh.
    2. Compare the analytic normals with the optional recomputation, especially across the texture seam. Restore the analytic normals afterward.
    3. Replace the sphere initializer with the complete torus initializer below. Its explicit tessellation fits the same conversion buffer and the existing pipeline.

```text
DvzGeometry* geometry = dvz_geometry_torus(&(DvzGeometryTorusDesc){
    DVZ_STRUCT_INIT_FIELDS(DvzGeometryTorusDesc),
    .major_radius = 0.6,
    .minor_radius = 0.25,
    .rings = 24,
    .sectors = 24,
});
```

??? info "Under the hood: geometry is CPU data"

    Vulkan accepts buffers and draw commands; it has no sphere or mesh object. A generator or loader prepares arrays, the application chooses a GPU format, and the draw selects an index range. Changing the source geometry leaves the descriptor layout, shader stages, frame attachments, and synchronization unchanged.

!!! warning "When it goes wrong"

    A garbled mesh usually means the index binding still says `UINT16` while the buffer holds 32-bit values. Missing triangles can mean the draw count is still 36. If a denser generator fails the capacity check, increase `MAX_VERTICES` to a checked bound for its vertex count or use appropriately sized dynamic storage. Dark seams after recomputing normals can result from averaging separated vertices independently; they are not necessarily a shader error.

## Checkpoint

1. Which changes were needed to draw generated geometry through the existing pipeline?
2. Why can CPU geometry be freed after upload while GPU buffers must survive submitted work?
3. What does normal recomputation average, and why can that matter at a UV seam?

??? example "Full current listing"

    ```c
    --8<-- "examples/c/vulkan/step15.c"
    ```

??? example "Current shader.vert"

    ```glsl
    --8<-- "examples/c/vulkan/step15/shader.vert"
    ```

??? example "Current shader.frag"

    ```glsl
    --8<-- "examples/c/vulkan/step15/shader.frag"
    ```
