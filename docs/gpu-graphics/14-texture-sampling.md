# 14. Sampling the texture

**Your program at the end of this chapter: 708 C lines. The raw Vulkan equivalent: around 2050 lines, a rough estimate. Each cube face now samples the checkerboard.**

![A checkerboard covers each face of the mouse-controlled cube.](../assets/gpu-graphics/14-texture-sampling.webp)

Continue from chapter 13. The image is uploaded, but drawing still needs UV coordinates, an image view, a sampler, and a descriptor entry. This chapter adds those pieces to the same cube. The material uniform remains at set 0 binding 0; the texture joins the same descriptor set at binding 1. Mouse rotation, zoom, depth, culling, and shader reload stay in place.

## Give each face its own UVs

A vertex index selects a whole vertex, including its position and UV. The eight shared cube corners cannot give all six faces separate rectangular UV maps. Split the corners into four vertices per face: 24 vertices still draw 12 triangles, but each face can now cover the complete image. Sharing positions is harmless; sharing a vertex with conflicting UVs is not.

Replace `Vertex` with this format. The texture replaces vertex colors, so remove the color field:

```c
typedef struct
{
    float position[3];
    float uv[2];
} Vertex;
```

Replace both `VERTICES` and `INDICES` with these complete arrays. Each face uses `(0, 0)`, `(1, 0)`, `(1, 1)`, `(0, 1)` and keeps the outward winding from chapter 10:

```c
static const Vertex VERTICES[24] = {
    // Back face.
    {{0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, -0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
    // Front face.
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, 0.65f}, {0.0f, 1.0f}},
    // Bottom face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 1.0f}},
    // Top face.
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
    // Right face.
    {{0.65f, -0.65f, 0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, -0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, 0.65f}, {0.0f, 1.0f}},
    // Left face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, 0.65f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
};

// Each face winds counter-clockwise when seen from outside the cube.
static const uint16_t INDICES[36] = {
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};
```

The existing `sizeof(VERTICES)`, `sizeof(INDICES)`, `sizeof(Vertex)`, and 36-index draw adapt automatically. In `create_pipeline()`, replace the color attribute declaration with the UV declaration:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
```

Replace `shader.vert` with:

```glsl
#version 450

layout(push_constant) uniform Push { mat4 mvp; } push_data;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 0) out vec2 uv;

void main()
{
    gl_Position = push_data.mvp * vec4(in_position, 1.0);
    uv = in_uv;
}
```

Replace `shader.frag` with:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Material { vec4 tint; } material;
layout(set = 0, binding = 1) uniform sampler2D tex;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(tex, uv) * material.tint;
}
```

The rasterizer interpolates UVs across each triangle. Perspective-correct interpolation accounts for clip `w`, so the texture follows a receding surface rather than sliding through screen space. `texture(tex, uv)` uses the interpolated coordinate and applies the sampler's filtering and address rules while reading the image.

## Create a view and sampler

Add the view and sampler as owned fields after `texture`. Keep the existing descriptor field from chapter 12:

```c
    DvzImageViews* texture_view;
    DvzSampler* sampler;
```

Inside `create_texture()`, after the checked `dvz_cmd_end_result()` / `dvz_cmd_submit_result()` block and before `dvz_commands_destroy(upload)`, insert:

```c
    renderer->texture_view = dvz_image_views_create_wrapper();
    renderer->sampler = dvz_sampler_create_wrapper();
    if (renderer->texture_view == NULL || renderer->sampler == NULL)
        goto error;
    dvz_image_views(renderer->texture, renderer->texture_view);
    if (dvz_image_views_create(renderer->texture_view) != 0)
        goto error;
    dvz_sampler(renderer->device, renderer->sampler);
    dvz_sampler_min_filter(renderer->sampler, VK_FILTER_NEAREST);
    dvz_sampler_mag_filter(renderer->sampler, VK_FILTER_NEAREST);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    if (dvz_sampler_create(renderer->sampler) != 0)
        goto error;
```

An **image view** selects which image subresources and format interpretation the shader can access; the descriptor refers to the view rather than to the image allocation directly. A **sampler** supplies the lookup rules independently of the stored pixels. Here nearest filtering selects one texel and preserves crisp checker boundaries, while repeat addressing wraps coordinates outside `[0, 1]`. This small example uses a single mip level.

## Declare and write the descriptor

In `create_pipeline()`, immediately after `dvz_slots(renderer->device, renderer->slots);` and before `dvz_slots_push()`, insert:

```c
    dvz_slots_binding(renderer->slots, 0, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
```

Set 0 binding 0 remains the material uniform from chapter 12. Set 0 binding 1 now agrees with the fragment shader's `sampler2D`. A combined image sampler descriptor refers to both the image view and sampler, though those remain separate Vulkan resources with separate jobs and lifetimes. Replace the final descriptor creation with this version:

```c
    if (dvz_graphics_create(renderer->pipeline) != 0)
        return -1;
    renderer->descriptors = dvz_descriptors_create_wrapper();
    if (renderer->descriptors == NULL)
        return -1;
    dvz_descriptors(renderer->slots, renderer->descriptors);
    dvz_descriptors_image(
        renderer->descriptors, 0, 1, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        dvz_image_views_handle(renderer->texture_view, 0), dvz_sampler_handle(renderer->sampler));
    dvz_descriptors_buffer(renderer->descriptors, 0, 0, 0, dvz_buffer_handle(renderer->material_buffer), 0, sizeof(Material));
    return dvz_descriptors_handle(renderer->descriptors, 0) != VK_NULL_HANDLE ? 0 : -1;
```

The descriptor belongs with this pipeline's `slots`: its wrapper retains a pointer to them. Keeping it in the pipeline creation path also lets a reload build a complete candidate before changing the current draw resources.

Keep pipeline creation after texture creation. The image view and sampler must exist before `create_pipeline()` can write their descriptor:

```c
    int texture_result = create_texture(&renderer);
    COURSE_CHECK(texture_result == 0, "texture upload failed");
    int pipeline_result = create_pipeline(&renderer);
    COURSE_CHECK(pipeline_result == 0, "graphics pipeline creation failed");
```

In `draw()`, after setting the viewport and scissor and before pushing constants, bind the descriptor:

```c
    dvz_cmd_bind_descriptors(
        renderer->commands, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->descriptors, 0, 1, 0, NULL);
```

## Keep reload and cleanup safe

`destroy_pipeline()` already releases the descriptor before its layout from chapter 12. In the reload branch, add the view and sampler to the candidate initializer:

```c
            Renderer candidate = {
                .device = renderer.device,
                .color_format = renderer.color_format,
                .material_buffer = renderer.material_buffer,
                .texture_view = renderer.texture_view,
                .sampler = renderer.sampler,
            };
```

The candidate borrows the existing view and sampler while owning its new pipeline, slots, shaders, and descriptor set. In the successful reload branch, immediately after `destroy_pipeline(&renderer);` and before assigning `renderer.slots`, transfer the descriptor too:

```c
                renderer.descriptors = candidate.descriptors;
```

Keep the existing device wait before destroying the old resources, and the failed-candidate cleanup when compilation fails. A failed shader edit leaves the old pipeline and descriptor usable. Reload shader calculations within the existing interface; changing attribute locations, bindings, or the push layout also requires matching C edits and a rebuild.

At `cleanup:`, move the existing `destroy_pipeline(&renderer);` from its old position to immediately after the device wait. After it, insert this sampler and view cleanup, before destroying `renderer.texture`:

```c
    if (renderer.sampler != NULL)
        dvz_sampler_destroy(renderer.sampler);
    dvz_sampler_free(renderer.sampler);
    if (renderer.texture_view != NULL)
        dvz_image_views_destroy(renderer.texture_view);
    dvz_image_views_free(renderer.texture_view);
```

The descriptor is freed before its slots, view, and sampler. The image is freed after its view. Keep the buffer cleanup, arcball disconnection, controller destruction, and canvas cleanup that follow.

## Build and run

Keep `main.c`, `shader.vert`, and `shader.frag` in the project directory from chapter 1. Run these commands there, so the program can find the shader files.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse --png chapter14.png
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe --png chapter14.png
    .\build\Release\vkcourse.exe
    ```

The checkerboard should form squares on every face, including the sides when you rotate the cube. Offscreen captures use the same fixed camera pose as chapter 13 and should end with `validation errors: 0`.

!!! tip "Try it"

    1. Replace both `VK_FILTER_NEAREST` values with `VK_FILTER_LINEAR`, rebuild, and zoom in. Linear filtering blends neighboring texels, so the checker boundaries should soften.
    2. Change one face's maximum U from `1.0` to `2.0`. Compare repeat addressing with `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` after rebuilding.
    3. Change the fragment output to a constant color, save, and press **R**. Then restore sampling and reload again. The cube and mouse controls should remain usable throughout.

??? info "Under the hood: descriptor layouts and sets"

    Raw Vulkan declares a descriptor set layout, allocates a descriptor set from a pool, and writes a `VkDescriptorImageInfo` with `vkUpdateDescriptorSets`. `DvzSlots` owns the layout; `DvzDescriptors` owns the allocated sets and borrows the slots wrapper. The device owns the pool. None of these objects owns the image view or sampler named by the descriptor.

!!! warning "When it goes wrong"

    A set/binding validation error means the GLSL declaration, slots declaration, or descriptor write disagrees. A stretched side face usually means you kept eight shared vertices; each face needs its own UV corners. If the first frame works but **R** fails, check that the candidate receives the view and sampler and that its new descriptor replaces the old one before the old slots are freed.

## Checkpoint

1. Why do eight positions become 24 vertices when each cube face needs separate UVs?
2. What do the image view, sampler, and descriptor each contribute?
3. Why must shader reload replace the descriptor wrapper along with its slots?

??? example "Full current listing"

    ```c
    --8<-- "examples/c/vulkan/step14.c"
    ```

??? example "Current shader.vert"

    ```glsl
    --8<-- "examples/c/vulkan/step14/shader.vert"
    ```

??? example "Current shader.frag"

    ```glsl
    --8<-- "examples/c/vulkan/step14/shader.frag"
    ```
