# 10. Depth and culling

*About 538 lines of C; raw Vulkan would need about 1650.*

![A solid cube with hidden faces correctly occluded.](../assets/gpu-graphics/10-depth-culling.webp)

In chapter 9, draw order decided which face covered another, so the cube looked wrong. This chapter fixes that with a depth test, and skips faces that point away from the camera with back-face culling. A **depth attachment** stores a depth value per framebuffer sample, so Vulkan can reject a fragment that lies behind a closer one. It sits beside the color attachment: one records visibility, the other the color you see.

## Request and attach depth

In `main()`, before creating the canvas, request a 32-bit float depth image:

```c
    canvas_config.depth_format = VK_FORMAT_D32_SFLOAT;
```

Vulkan guarantees depth-attachment support for `D16_UNORM` and for at least one of `X8_D24_UNORM_PACK32` and `D32_SFLOAT`. The canvas allocates exactly the format you request and does not substitute another, so on a device without `D32_SFLOAT` depth images, pick one of the others. The canvas also recreates the image when the window is resized.

In `draw()`, right after `dvz_cmd_rendering_default()` and before beginning rendering, attach this frame's borrowed depth view and clear it:

```c
    DvzAttachment* depth = dvz_rendering_depth(renderer->rendering);
    dvz_attachment_image(depth, frame->depth_view, frame->depth_layout);
    dvz_attachment_ops(depth, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(depth, (VkClearValue){.depthStencil = {1.0f, 0}});
```

The canvas keeps the depth image in the right image layout and reports it in `frame->depth_layout`; chapter 13 explains layouts. Clearing to 1.0 starts every sample at the far end of the depth range.

## Enable testing and writes

In `create_pipeline()`, after the color attachment format and before the pipeline layout, add:

```c
    dvz_graphics_attachment_depth(renderer->pipeline, VK_FORMAT_D32_SFLOAT);
    dvz_graphics_depth(
        renderer->pipeline, false, true, VK_COMPARE_OP_LESS, DVZ_GRAPHICS_FLAGS_FIXED);
```

The attachment format must match the canvas. The first Boolean turns off depth clamping, which would otherwise pin depths outside the near and far planes to the range instead of clipping them. The second turns on depth writes.

A **depth test** compares an incoming fragment's depth with the stored value; `VK_COMPARE_OP_LESS` keeps it only if it is closer.

A **depth write** then stores the new depth, so later fragments compare against it. You can test without writing, which is why the two settings are separate.

## Winding and culling

Add these calls right after the depth state:

```c
    dvz_graphics_cull_mode(
        renderer->pipeline, VK_CULL_MODE_BACK_BIT, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_front_face(
        renderer->pipeline, VK_FRONT_FACE_COUNTER_CLOCKWISE, DVZ_GRAPHICS_FLAGS_FIXED);
```

**Back-face culling** looks at the order of a projected triangle's vertices on screen and drops triangles whose winding marks them as facing away. The cube's indices wind counter-clockwise from outside, and chapter 9's y flip keeps that winding counter-clockwise on screen. Culling never looks at normals, and it cannot tell which of two front-facing triangles is closer.

Depth and culling do different jobs. Depth resolves visibility fragment by fragment, so opaque results no longer depend on draw order. Culling skips whole triangles before rasterization. You still need depth whenever front-facing triangles overlap.

## Build and run

Build and run as before; the capture goes to `chapter10.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter10.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter10.png
    ```

The cube is solid from every angle. The terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Set the depth-write Boolean to `false` and use `VK_CULL_MODE_NONE`. With no writes, the stored depth stays at 1.0, every fragment passes, and the chapter 9 overlap returns.
    2. Restore both, then flip the front face to `VK_FRONT_FACE_CLOCKWISE`. You see the cube's inner faces.
    3. Restore counter-clockwise fronts and switch between `VK_CULL_MODE_NONE` and `VK_CULL_MODE_BACK_BIT`. The image does not change: depth alone already hides the back faces.
    4. See the triangles. Wireframe drawing is an optional device feature, so request it before creating the GPU context:

        ```c
        VkPhysicalDeviceFeatures features = {.fillModeNonSolid = VK_TRUE};
        dvz_gpu_ctx_config_features10(&gpu_config, &features);
        ```

        Then, in `create_pipeline()`, draw polygons as lines:

        ```c
        dvz_graphics_polygon_mode(renderer->pipeline, VK_POLYGON_MODE_LINE, DVZ_GRAPHICS_FLAGS_FIXED);
        ```

        Each face splits into its two triangles, and with back-face culling you see only the three faces turned toward you. Switch to `VK_CULL_MODE_NONE` to see all twelve triangles.

??? info "Under the hood: another image and two pipeline states"

    Raw Vulkan needs a depth image, its memory, a view, a layout transition, and recreation on resize. The pipeline also carries depth-stencil and rasterization state structures. The canvas manages the image; you choose the comparison, the clear, the winding, and the culling.

!!! warning "When it goes wrong"

    An attachment-format validation error means the pipeline and canvas disagree. An inside-out cube points to the winding or front-face setting. Flickering where two surfaces meet means they are nearly at the same depth: depth has finite precision, so keep a sensible near/far range.

## Checkpoint

1. Why is depth cleared to 1.0?
2. What distinguishes a depth test from a depth write?
3. What does back-face culling use to classify a triangle?

??? example "Your `main.c` after chapter 10"

    ```c
    --8<-- "examples/c/vulkan/step10.c"
    ```

??? example "Your `shader.vert` after chapter 10"

    ```glsl
    --8<-- "examples/c/vulkan/step10/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 10"

    ```glsl
    --8<-- "examples/c/vulkan/step10/shader.frag"
    ```

Next: [Mouse control](11-mouse-control.md).
