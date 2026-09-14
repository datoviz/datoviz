# 10. Depth and culling

**Your program at the end of this chapter: 539 C lines. The raw Vulkan equivalent: around 1650 lines, a rough estimate. The cube is now correctly occluded and culled.**

![A solid cube with hidden faces correctly occluded.](../assets/gpu-graphics/10-depth-culling.webp)

Keep chapter 9's cube, matrix helpers, shaders, and animation. The draw order currently decides which face covers another. A depth attachment stores a second value for each pixel so Vulkan can reject a fragment behind one already drawn.

## Request and attach depth

In `main()`, before creating the canvas, request a D32 depth image:

```c
    canvas_config.depth_format = VK_FORMAT_D32_SFLOAT;
```

The canvas allocates this image and recreates it when the window changes size. In `draw()`, immediately after `dvz_cmd_rendering_default()` and before beginning rendering, attach the current frame's borrowed depth view and clear it:

```c
    DvzAttachment* depth = dvz_rendering_depth(renderer->rendering);
    dvz_attachment_image(depth, frame->depth_view, frame->depth_layout);
    dvz_attachment_ops(depth, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(depth, (VkClearValue){.depthStencil = {1.0f, 0}});
```

Use both the view and layout supplied by this frame. The canvas owns their lifetime and transitions. Clearing depth to 1.0 starts each pixel at the far plane.

## Enable testing and writes

In `create_pipeline()`, after the color attachment format and before the pipeline layout, add:

```c
    dvz_graphics_attachment_depth(renderer->pipeline, VK_FORMAT_D32_SFLOAT);
    dvz_graphics_depth(
        renderer->pipeline, false, true, VK_COMPARE_OP_LESS, DVZ_GRAPHICS_FLAGS_FIXED);
```

The attachment format must match the canvas. `DVZ_GRAPHICS_FLAGS_FIXED` enables depth testing with this fixed state; the first Boolean disables depth clamping, and the second enables depth writes. `VK_COMPARE_OP_LESS` keeps a fragment only when its depth is smaller than the stored value. Passing fragments write their depth for later triangles to compare against.

## Winding and culling

Add these calls directly after the depth state:

```c
    dvz_graphics_cull_mode(
        renderer->pipeline, VK_CULL_MODE_BACK_BIT, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_front_face(
        renderer->pipeline, VK_FRONT_FACE_COUNTER_CLOCKWISE, DVZ_GRAPHICS_FLAGS_FIXED);
```

The cube's indices wind counter-clockwise from outside. Together with chapter 9's y correction and positive-height viewport, that matches the counter-clockwise front-face setting. Back-face culling discards the faces that point away from the camera.

Depth and culling do different jobs. Depth resolves overlapping fragments; culling skips triangles according to projected winding. Depth remains necessary when front-facing triangles from different objects overlap.

## Try it

1. Set the depth-write Boolean to `false` and temporarily use `VK_CULL_MODE_NONE`. With no writes, the clear value stays in the depth image and draw order matters again.
2. Restore depth writes and back-face culling, then flip the front face to `VK_FRONT_FACE_CLOCKWISE`. You see the inner faces of the cube.
3. Restore counter-clockwise fronts and switch between `VK_CULL_MODE_NONE` and `VK_CULL_MODE_BACK_BIT`. Correct depth testing should keep the solid cube's visible surfaces the same.

??? info "Under the hood: another image and two pipeline states"

    Raw Vulkan needs a depth image, view, allocation, and resize handling. The graphics pipeline also carries depth/stencil and rasterization state. The canvas manages the image; this program chooses the comparison, attachment clear, winding, and culling.

!!! warning "When it goes wrong"

    An attachment-format error means the pipeline and canvas disagree. An inside-out cube points to winding or front-face settings. Flickering surfaces may be nearly coplanar: depth has finite precision, so use a sensible near/far range and avoid overlapping surfaces at the same depth.

## Build and run

Keep using the project from chapter 1. Save `main.c`, `shader.vert`, and `shader.frag` in the project directory, then run these commands there. Press **R** in the live window to rebuild the pipeline from the shader files.

```sh
cmake --build build
./build/vkcourse
./build/vkcourse --png frame.png
```

??? example "Your `main.c` at the end of chapter 10"

    ```c
    --8<-- "examples/c/vulkan/step10.c"
    ```

??? example "Your `shader.vert` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step10/shader.vert"
    ```

??? example "Your `shader.frag` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step10/shader.frag"
    ```

## Checkpoint

- Why is depth cleared to 1.0?
- What distinguishes a depth test from a depth write?
- What does back-face culling use to classify a triangle?

Next, replace the clock-driven rotation with mouse input.
