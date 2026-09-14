# 11. Mouse control

**Your program at the end of this chapter: 519 C lines. The raw Vulkan equivalent: around 1750 lines, a rough estimate. You can rotate and zoom the cube with the mouse.**

![A perspective cube ready for mouse rotation.](../assets/gpu-graphics/11-mouse-control.webp)

Keep the cube, buffers, shader files, depth attachment, culling, and matrix push from chapter 10. Interaction changes CPU state, and the draw callback turns that state into the same 64-byte matrix. The shaders need no changes.

## Create a camera and arcball

Include `<datoviz/controller.h>`. In `Renderer`, replace `start_ns`, `capture_time`, and `animate` with these two owned objects:

```c
    DvzArcball* arcball;
    DvzCamera* camera;
```

Remove the `make_mvp()` helper; keep `multiply_mat4()`. The camera now supplies the view and projection, while the arcball supplies model rotation and modifies the view for pan and dolly. This also replaces the automatic pulse with pointer-controlled motion.

In `main()`, replace the clock initialization before `dvz_canvas_set_draw_callback()` with:

```c
    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.projection.fov_y = 1.0471976f;
    renderer.camera = dvz_camera_create(&camera_desc);
    renderer.arcball = dvz_arcball_create(NULL);
    COURSE_CHECK(renderer.camera != NULL && renderer.arcball != NULL, "controller creation failed");
    DvzResult initial_result = dvz_arcball_initial(renderer.arcball, (vec3){-0.35f, 0.65f, 0.0f});
    COURSE_CHECK(initial_result == DVZ_OK, "initial rotation failed");
    DvzResult connect_result = dvz_arcball_connect(renderer.arcball, dvz_canvas_input(canvas));
    COURSE_CHECK(connect_result == DVZ_OK, "arcball connection failed");
```

The camera uses the same 60-degree field of view and default eye at `(0, 0, 3)`. The initial arcball angle makes the cube readable before any input. Connecting to `dvz_canvas_input(canvas)` subscribes the controller to pointer events; the existing `dvz_window_host_poll()` delivers those events. Keep the keyboard subscriber too, so **R** still reloads shaders.

## Compose the matrices

Replace the elapsed-time and `make_mvp()` code at the beginning of `draw()` with:

```c
    Push push = {0};
    DvzMVP mvp = {0};
    DvzResult camera_resize = dvz_camera_resize(
        renderer->camera, (float)frame->extent.width, (float)frame->extent.height);
    // Pointer positions use window coordinates, which may differ from framebuffer pixels.
    DvzInputResizeEvent input_size = {0};
    float pointer_width = (float)frame->extent.width;
    float pointer_height = (float)frame->extent.height;
    if (dvz_input_router_last_resize(dvz_canvas_input(canvas), &input_size) &&
        input_size.window_width > 0 && input_size.window_height > 0)
    {
        pointer_width = (float)input_size.window_width;
        pointer_height = (float)input_size.window_height;
    }
    DvzResult arcball_resize =
        dvz_arcball_resize(renderer->arcball, pointer_width, pointer_height);
    if (camera_resize != DVZ_OK || arcball_resize != DVZ_OK)
        renderer->draw_failed = true;
    dvz_camera_mvp(renderer->camera, &mvp);
    dvz_arcball_mvp(renderer->arcball, &mvp);
    // Camera matrices use OpenGL clip coordinates; convert y and z for this Vulkan viewport.
    mat4 clip_correction = {{1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, 0.5f, 0}, {0, 0, 0.5f, 1}};
    mat4 view_model = {{0}};
    mat4 projection_view_model = {{0}};
    multiply_mat4(mvp.view, mvp.model, view_model);
    multiply_mat4(mvp.proj, view_model, projection_view_model);
    multiply_mat4(clip_correction, projection_view_model, push.mvp);
```

The camera fills `view` and `proj`; the arcball fills `model` and applies pan/dolly to `view`. Calling them in that order matters. The camera uses framebuffer pixels for the projection aspect. The arcball uses the latest window size reported by the input router, because pointer positions are measured in window coordinates. These sizes may differ on a display with a high pixel density. Before any resize event arrives, the framebuffer extent is a usable fallback.

The camera helper currently produces OpenGL-style clip coordinates: z spans -w through w. Our Vulkan draw uses z from zero through w and a positive-height viewport. `clip_correction` flips y and changes z to `(z + w) / 2`, matching chapter 9's explicit projection. This correction belongs before the perspective divide.

The remaining rendering code, including the depth attachment and push-constant call, stays the same. The complete program reports a failed resize or push update through `draw_failed` before submitting.

## Disconnect before cleanup

After the device wait and buffer cleanup, but before destroying the canvas or input router, release the controller objects:

```c
    if (renderer.arcball != NULL && canvas != NULL)
        dvz_arcball_disconnect(renderer.arcball, dvz_canvas_input(canvas));
    if (renderer.arcball != NULL)
        dvz_arcball_destroy(renderer.arcball);
    dvz_camera_destroy(renderer.camera);
```

Remove the old `--time` parsing and fixed-time initialization. Offscreen mode draws one frame at the fixed initial camera pose, so repeated captures remain deterministic without simulated input.

## Try it

1. Drag with the left mouse button to rotate the cube around its center.
2. Scroll to move toward or away from the cube. The arcball adjusts the view; the mesh data stays fixed.
3. Resize the window while the cube is rotated. It should keep its proportions.

??? info "Under the hood: input is CPU work"

    Vulkan has no input API or camera type. A window toolkit delivers events, a controller tracks pointer state, and matrix math turns it into a view and model transform. The GPU still receives a matrix and an indexed draw.

!!! warning "When it goes wrong"

    A cube that never responds may be connected to the wrong router or missing event polling. If scrolling has no effect, check that `dvz_camera_mvp()` runs before `dvz_arcball_mvp()`, so the camera does not overwrite the arcball's view adjustment. Disconnect the controller before destroying its router.

## Build and run

Keep using the project from chapter 1. Save `main.c`, `shader.vert`, and `shader.frag` in the project directory, then run these commands there. Press **R** in the live window to rebuild the pipeline from the shader files.

```sh
cmake --build build
./build/vkcourse
./build/vkcourse --png frame.png
```

??? example "Your `main.c` at the end of chapter 11"

    ```c
    --8<-- "examples/c/vulkan/step11.c"
    ```

??? example "Your `shader.vert` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step11/shader.vert"
    ```

??? example "Your `shader.frag` at the end of this chapter"

    ```glsl
    --8<-- "examples/c/vulkan/step11/shader.frag"
    ```

## Checkpoint

- Which part of the program handles mouse events?
- Why does the callback refresh the camera and arcball sizes?
- What does the clip correction change before the perspective divide?

You now have an interactive 3D cube. Next, upload a texture that later chapters will sample on its faces.
