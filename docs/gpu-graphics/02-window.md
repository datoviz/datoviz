# 2. Your first window

**Your program at the end of this chapter: about 150 lines. The raw Vulkan equivalent: around 900.**

![The chapter 2 window filled with its deterministic blue-grey clear color.](../assets/gpu-graphics/02-window.webp)

By the end of this chapter, your program will open a resizable window, fill it with a color you choose, and keep drawing until you close it. With one command-line flag, the same program will render to a PNG file instead. Keep that option throughout the course so you can check each chapter without relying on a window.

This chapter introduces the objects that connect your program to a GPU and a presentation target. Take them one at a time. For each one, keep track of what it represents, who owns it, and how long it remains valid. Chapter 3 explains what happens inside a frame; for now, the goal is to get the program running.

## The headers

Replace the includes in `main.c` with these:

```c
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <datoviz.h>
#include <datoviz/canvas.h>
#include <datoviz/stream/frame_stream.h>
#include <datoviz/vk/gpu_ctx.h>
#include <datoviz/vklite.h>
#include <datoviz/window.h>
```

`vklite.h` provides the typed wrappers over Vulkan objects used throughout this course. `canvas.h` provides the canvas, which owns the window's images and the frame loop's synchronization. Both include the Vulkan headers, so types such as `VkClearValue` are available without another include.

Add the window size while you are here:

```c
#define WIDTH  800
#define HEIGHT 600

#define COURSE_CHECK(condition, message)                                                         \
    do                                                                                           \
    {                                                                                            \
        if (!(condition))                                                                        \
        {                                                                                        \
            fprintf(stderr, "%s\n", message);                                                   \
            goto cleanup;                                                                        \
        }                                                                                        \
    } while (0)
```

`COURSE_CHECK` keeps routine failure handling on one line and sends every failure to the cleanup block at the end of `main`. The course uses it at boundaries where continuing would produce a misleading result. It does not wrap ordinary configuration calls whose return values do not change the lesson.

## A window, or a file

The same program can render to a window or directly to an image. You make that choice once at startup, and the rest of the setup follows from it. At the top of `main`:

```c
    // With --png PATH the program renders offscreen and saves an image instead of opening a window.
    const char* png_path = NULL;
    for (int i = 1; i < argc - 1; i++)
        if (strcmp(argv[i], "--png") == 0)
            png_path = argv[i + 1];
    bool live = png_path == NULL;

    DvzBackend backend = live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN;
    DvzCanvasRenderMode mode =
        live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
```

`main` now takes arguments. Change its signature and initialize the objects that the cleanup block will release:

```c
int main(int argc, char** argv)
{
    int exit_code = 1;
    DvzWindowHost* host = NULL;
    DvzGpuCtx* gpu = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    DvzCommands* commands = NULL;
    DvzRendering* rendering = NULL;
```

The course checks creation, frame submission, and file output because continuing after one of those failures would hide the real problem. Every failure jumps to one cleanup block. Routine configuration calls stay on the main path unless their result determines whether setup can continue.

The **backend** decides who provides the window abstraction. GLFW communicates with the desktop, while the offscreen backend provides a windowless render target of the same size. The **render mode** decides where finished frames go: either to the screen for presentation or into memory for readback. Both routes produce an image that rendering commands can write; only the live route hands the finished image to a display.

## The window host

```c
    // The window host talks to the operating system's windowing layer.
    host = dvz_window_host();
    COURSE_CHECK(host != NULL, "window host creation failed");
```

Your program needs one window host. It owns the connection to the platform's windowing system and the event queue that you will process in the render loop.

## The GPU context

```c
    // The GPU context picks a physical device and creates the logical device and its queues.
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, true);
    DvzResult configure_result =
        dvz_canvas_configure_gpu_ctx(host, backend, mode, &gpu_config);
    COURSE_CHECK(configure_result == DVZ_OK, "GPU configuration failed");
    gpu = dvz_gpu_ctx(&gpu_config);
    COURSE_CHECK(gpu != NULL, "no usable GPU found");
```

This is where your program acquires access to a GPU. The rest of the course relies on two Vulkan terms:

- A **physical device** describes one Vulkan-capable implementation and its hardware limits, memory types, queues, and supported features. It is usually a GPU in the machine, though a software implementation can also appear as a physical device. Vulkan may find several, and the context chooses one.
- A **logical device** is the live Vulkan connection your program creates from that physical device. It enables selected features and gives the program queues and device-level operations. Every resource created later, including shader modules, pipelines, buffers, and images, belongs to this logical device and must be destroyed before it. This is what `DvzDevice*` refers to throughout the course.

A **resource** is storage or executable state that later GPU work refers to, such as a buffer, image, shader module, or pipeline. Creating a C wrapper does not by itself create the underlying Vulkan resource; the configure-and-create calls later in the course make that distinction visible.

`dvz_canvas_configure_gpu_ctx` adds the extensions and Vulkan features required by the canvas: dynamic rendering, synchronization2, and timeline semaphores. Requesting them here means that an incompatible device fails during setup instead of causing an obscure error later.

`dvz_gpu_ctx_config_validation(&gpu_config, true)` requests the Vulkan **validation layers**. When the layers are installed, they sit between your code and the driver and complain, in words, when you misuse the API. Without them, a Vulkan mistake may look like nothing more than a blank window or an unexplained crash. Keep them requested throughout the course, and check the startup log if you need to confirm that they were available.

Print which GPU you got:

```c
    DvzGpuInfo info = {0};
    if (dvz_gpu_ctx_gpu_info(gpu, &info))
        printf("GPU: %s\n", info.name);
```

## The window

```c
    DvzWindowConfig window_config = dvz_window_config();
    window_config.width = WIDTH;
    window_config.height = HEIGHT;
    window_config.title = "Modern GPU Graphics in Vulkan";
    window = dvz_window_create(host, backend, &window_config);
    COURSE_CHECK(window != NULL, "window creation failed");
```

Datoviz config structs follow a common pattern: a `dvz_*_config()` function returns a struct filled with defaults, you replace the fields you care about, and then pass it to the create function. This pattern allows new fields to be added without breaking your code.

## The canvas

```c
    // The canvas owns the swapchain, the per-frame images, and all frame synchronization.
    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;
    canvas = dvz_canvas_create(&canvas_config);
    COURSE_CHECK(canvas != NULL, "canvas creation failed");
```

The **canvas** owns the render targets and frame machinery needed to produce a window image or an offscreen image. It decides when a target is available, lends that target and a recording command buffer to your callback, submits the finished work, and handles presentation or readback. The course relies heavily on this object, so it is worth being precise about what it owns.

A window cannot be drawn to directly. Vulkan presentation requires a **surface**, the Vulkan connection to an operating-system window, followed by a **swapchain**. A swapchain is a rotating collection of presentable images. The program acquires one that the display is not using, rendering eventually supplies its pixels, and presentation returns it to the display system. The canvas gives your callback a separate frame target, then transfers the finished result into the acquired swapchain image. Because CPU recording, GPU execution, and display presentation overlap in time, synchronization prevents either side from reusing an image too early. Resizing can make the old swapchain incompatible with the surface, so the canvas rebuilds it.

The canvas owns all of that. In return, your callback runs when a frame is ready for recording.

## Something to draw with

The draw callback needs somewhere to keep its state. Add this struct above `main`:

```c
// Everything our program owns. It will grow in every chapter.
typedef struct
{
    DvzDevice* device;
    DvzCommands* commands;
    DvzRendering* rendering;
    VkClearValue clear;
} Renderer;
```

`Renderer` will grow in nearly every chapter as you add a pipeline, buffers, a texture, and matrices. For now, it holds the device, two small wrapper objects, and a clear color.

Then the callback itself, also above `main`:

```c
// Called once per frame by the canvas, with a command buffer we may record into.
static void draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    Renderer* renderer = (Renderer*)user_data;

    dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, renderer->commands);
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height,
        renderer->clear, renderer->rendering);
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);
    dvz_cmd_rendering_end(renderer->commands);
    dvz_commands_unwrap(renderer->commands);
}
```

For now, read these five lines as "clear this frame's image to a color." Chapter 3 will take them apart. Notice that the callback draws nothing between `dvz_cmd_rendering_begin` and `dvz_cmd_rendering_end`. The color you see comes from the *clear*, and every triangle in this course will be drawn between those two calls.

Create the renderer in `main`, after the canvas, and hand it to the canvas:

```c
    commands = dvz_commands_create_wrapper();
    rendering = dvz_rendering_create_wrapper();
    Renderer renderer = {
        .device = dvz_gpu_ctx_device(gpu),
        .commands = commands,
        .rendering = rendering,
        .clear = {.color.float32 = {0.10f, 0.12f, 0.18f, 1.00f}},
    };
    COURSE_CHECK(commands != NULL && rendering != NULL, "renderer allocation failed");
    dvz_canvas_set_draw_callback(canvas, draw, &renderer);
```

Clear-color components are floating-point values from `0.0` to `1.0`, in red, green, blue, alpha order. They are not bytes from 0 to 255.

## The render loop

```c
    // Process one frame attempt; READY attempts are submitted below.
    uint64_t frame_index = 0;
    while (live ? !dvz_window_should_close(window) : frame_index < 1)
    {
        dvz_window_host_poll(host);
        int status = dvz_canvas_frame(canvas);
        if (status == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            continue;
        COURSE_CHECK(status == DVZ_CANVAS_FRAME_READY, "frame preparation failed");
        int submit_result = dvz_canvas_submit(canvas);
        COURSE_CHECK(submit_result == 0, "frame submission failed");
        frame_index++;
    }
    printf("rendered %llu frames\n", (unsigned long long)frame_index);
```

A live run loops until the window closes, while a `--png` run needs only one frame. Each iteration does three things:

1. `dvz_window_host_poll` processes operating-system events such as keyboard, mouse, resize, and close. If you skip it, the window keeps rendering but stops responding.
2. `dvz_canvas_frame` acquires the next image, opens a command buffer, and calls your `draw` callback with both. `DVZ_CANVAS_FRAME_READY` means the frame is ready. `DVZ_CANVAS_FRAME_WAIT_SURFACE` means the surface is temporarily unusable because the window is minimized or being resized. There is nothing to draw in that iteration, so the loop tries again.
3. `dvz_canvas_submit` sends the recorded commands to the GPU and, in live mode, presents the result.

Notice the division of labor: your callback records *what* to draw, while `dvz_canvas_submit` queues that recorded work for GPU execution. Submission makes the work eligible to run; it does not mean the GPU has already finished, or even that it begins at the instant the function is called.

## Saving the image

```c
    if (png_path != NULL)
    {
        int capture_result = dvz_canvas_capture_png(canvas, png_path);
        COURSE_CHECK(capture_result == 0, "PNG capture failed");
    }

    exit_code = 0;
```

Reading back an offscreen frame requires waiting for the GPU, copying the image into host-visible memory, and encoding it. This function performs all three steps.

## Cleanup

```c
cleanup:
    // Destroy resources in dependency-safe order.
    dvz_rendering_free(rendering);
    dvz_commands_free(commands);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    uint32_t validation_errors = gpu != NULL ? dvz_gpu_ctx_error_count(gpu) : 0;
    if (gpu != NULL)
        printf("validation errors: %u\n", validation_errors);
    dvz_gpu_ctx_destroy(gpu);
    return exit_code == 0 && validation_errors == 0 ? 0 : 1;
}
```

Destruction order follows dependencies. A GPU object cannot outlive the device that created it, and the GPU context owns the device, so the context must be destroyed last. As `Renderer` grows, destroy each resource before the object that owns or backs it.

`dvz_gpu_ctx_error_count` reports how many validation errors the layers recorded when validation was available. Query it *before* destroying the context. It should print `0` throughout the course. Also check that startup did not warn `validation layer is not supported`; without the layer, a zero count proves nothing. If the count is nonzero, read the terminal messages because they identify the incorrect call.

## Run it

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    ```

A dark blue window appears and remains open until you close it. Resize the window. The color still fills it because the canvas rebuilds its swapchain and your callback draws into the new, larger image.

Then the same program, without a window:

=== "Linux and macOS"

    ```sh
    ./build/vkcourse --png chapter02.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    .\build\Release\vkcourse.exe --png chapter02.png
    ```

```
GPU: Apple M3
rendered 1 frames
validation errors: 0
```

??? info "Under the hood: what you just skipped"

    A raw Vulkan program needs roughly the following code to reach the same dark blue window:

    - `vkCreateInstance` with the right platform extensions, plus the debug-messenger plumbing that makes validation errors readable (about 80 lines).
    - `vkEnumeratePhysicalDevices`, followed by scoring each candidate for the extensions, features, and queue families you need (about 150 lines).
    - `vkCreateDevice`, requesting queues and chaining several feature structs (about 80 lines).
    - A platform surface, followed by swapchain creation: querying supported formats and present modes, choosing one, selecting an image count, creating the swapchain, retrieving its images, and creating a view for each one (about 250 lines).
    - A command pool, command buffers, and per-frame semaphores and fences (about 100 lines).
    - An acquire, record, submit, and present sequence with the correct wait stages, plus detection of `VK_ERROR_OUT_OF_DATE_KHR` and complete swapchain rebuilding after a resize (about 200 lines).

    The canvas replaces roughly 900 lines of this machinery. None of those lines describes the graphics you want to draw, so the course does not spend its first three chapters implementing them. You now know what the machinery contains and where to look if you later decide to write it yourself.

!!! tip "Try it"

    1. **Change the clear color** to `{1.0f, 1.0f, 1.0f, 1.0f}`. The window turns white. Now try `{255.0f, 0.0f, 0.0f, 1.0f}`. The window is still red because values are clamped to 1.0.
    2. **Comment out the two `dvz_cmd_rendering_*` calls** and run again. The window turns black. That black is worth understanding: Vulkan promises *nothing* about the contents of an image you have not written to. The result could be zeros, stale pixels, or a driver's debug fill. You see black because the canvas clears each target once before your callback first sees it, giving this experiment a defined result on every platform. From the second frame onward, the target contains whatever your recorded rendering leaves there.
    3. **Delete `dvz_window_host_poll`** from the loop. The window still fills with color but stops responding: it no longer resizes, and the close button does nothing.
    4. **Compare colors.** The `0.10f` red component in your clear value becomes 89 in the PNG, not 26. The canvas image uses an sRGB format, so the linear values you write are gamma-encoded on output. Chapter 15 returns to this when it matters for lighting.

## When it goes wrong

| Symptom | Cause and fix |
| --- | --- |
| `no usable GPU found` | No Vulkan driver, or the driver is too old for the features the canvas requires. Verify your Vulkan installation first; `vulkaninfo` is the usual tool. |
| `canvas creation failed` in live mode but `--png` works | The window surface could not be created. Typical on a headless machine or over plain SSH without a display. |
| The window appears but is white or garbage | The `draw` callback is not being called. Check that `dvz_canvas_set_draw_callback` runs *before* the loop, and that the renderer you pass outlives it. |
| `validation errors: 3` and messages on the terminal | Read them; they name the offending call. Do not proceed to the next chapter with a nonzero count. |
| The window closes immediately | `dvz_canvas_frame` returned something other than `READY` or `WAIT_SURFACE`, so the loop broke out. Print `status` to see it. |

??? example "Your `main.c` at the end of chapter 2"

    ```c
    --8<-- "examples/c/vulkan/step02.c"
    ```

## Checkpoint

- What is the difference between a physical device and a logical device, and which one do the objects you create belong to?
- Name three things the canvas does for you that a raw Vulkan program would have to do by hand.
- Why does `dvz_canvas_frame` sometimes return `WAIT_SURFACE`, and why is that not an error?
- Why is the GPU context destroyed last?

Next: [How a frame works](03-frame.md).
