# 2. Your first window

**Your program at the end of this chapter: about 120 lines. The raw Vulkan equivalent: around 900.**

By the end of this chapter your program opens a resizable window, fills it with a color you chose,
and keeps drawing until you close it. Given one command-line flag, it renders the same picture to a
PNG file instead. Keep that flag from the start: it is how you will check every later chapter
without needing to eyeball a window.

Six Datoviz objects appear here. Take them one at a time; the next chapter dissects what happens
inside a frame, and this one is about getting the machinery standing up.

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

`vklite.h` is the low-level layer this course lives in: typed wrappers over Vulkan objects. `canvas.h`
is the piece that owns the window's images and the frame loop's synchronization. Both bring the
Vulkan headers with them, so `VkClearValue` and friends are available without any extra include.

Add the window size while you are here:

```c
#define WIDTH  800
#define HEIGHT 600
```

## A window, or a file

The same program can render to a window or straight to an image. That choice is made once, up front,
and everything downstream follows from it. At the top of `main`:

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

`main` now takes arguments, so change its signature to `int main(int argc, char** argv)`.

The **backend** decides who provides the window: GLFW talks to your desktop, and the offscreen
backend provides a windowless surface of the same size. The **render mode** decides where finished
frames go: presented to the screen, or kept in memory for you to read back.

## The window host

```c
    // The window host talks to the operating system's windowing layer.
    DvzWindowHost* host = dvz_window_host();
```

One host per program. It owns the connection to the platform's windowing system and the event queue
you will pump in the render loop.

## The GPU context

```c
    // The GPU context picks a physical device and creates the logical device and its queues.
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, true);
    dvz_canvas_configure_gpu_ctx(host, backend, mode, &gpu_config);
    DvzGpuCtx* gpu = dvz_gpu_ctx(&gpu_config);
    if (gpu == NULL)
    {
        fprintf(stderr, "no usable GPU found\n");
        return 1;
    }
```

This is where your program acquires a GPU. Two Vulkan terms deserve names now, because the whole
course refers back to them:

- A **physical device** is an actual GPU in the machine. Vulkan can enumerate several; the context
  picks one.
- A **logical device** is your program's private connection to that GPU. Every object you create
  later (shader, pipeline, buffer, image) is created *from* a device and is only valid with it.
  Whenever you see `DvzDevice*` in this course, that is what it is.

`dvz_canvas_configure_gpu_ctx` fills in the extensions and Vulkan features the canvas requires. The
canvas needs modern Vulkan (dynamic rendering, synchronization2, timeline semaphores), and this
call is what requests them, so a device that cannot support them fails here rather than mysteriously
later.

`dvz_gpu_ctx_config_validation(&gpu_config, true)` turns on the Vulkan **validation layers**. They
sit between your calls and the driver and complain, in words, when you misuse the API. Without them
a mistake usually shows up as a blank window or a crash with no explanation. Leave them on for the
whole course.

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
    DvzWindow* window = dvz_window_create(host, backend, &window_config);
```

Datoviz's config structs all follow this shape: a `dvz_*_config()` call returns a struct filled with
defaults, you overwrite the fields you care about, and you pass it to the create function. New
fields can be added to those structs without breaking your code.

## The canvas

```c
    // The canvas owns the swapchain, the per-frame images, and all frame synchronization.
    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;
    DvzCanvas* canvas = dvz_canvas_create(&canvas_config);
    if (canvas == NULL)
    {
        fprintf(stderr, "canvas creation failed\n");
        return 1;
    }
```

The canvas is the object this course leans on most, so this section is precise about what it
absorbs.

A window cannot be drawn to directly. Vulkan renders into images, and getting images that a window
will actually display means creating a **surface** (the Vulkan handle for "this operating system
window") and then a **swapchain**: a small set of images (typically two or three) that you and the
display hardware pass back and forth. Each frame you must ask the swapchain for the next free image,
render into it, and hand it back for presentation. Because the GPU runs behind the CPU, you also
need synchronization primitives per frame in flight so you never overwrite an image the display is
still reading. And when the window is resized, the whole swapchain becomes invalid and has to be
rebuilt.

The canvas owns all of that. What you get in return is one function that says "a frame is ready to
record into", which you will meet in a moment.

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

`Renderer` grows in nearly every chapter from here: pipeline, buffers, texture, and matrices all end
up in it. For now it holds the device, two small wrapper objects, and a clear color.

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

Read it, for now, as five lines that mean "clear this frame's image to a color". Chapter 3 takes
each one apart. The one thing to notice is that the callback draws nothing between
`dvz_cmd_rendering_begin` and `dvz_cmd_rendering_end`. The color you see is the *clear*, and every
triangle in this course will be drawn between those two calls.

Create the renderer in `main`, after the canvas, and hand it to the canvas:

```c
    Renderer renderer = {
        .device = dvz_gpu_ctx_device(gpu),
        .commands = dvz_commands_create_wrapper(),
        .rendering = dvz_rendering_create_wrapper(),
        .clear = {.color.float32 = {0.10f, 0.12f, 0.18f, 1.00f}},
    };
    dvz_canvas_set_draw_callback(canvas, draw, &renderer);
```

Clear-color components are floats from `0.0` to `1.0` in the order red, green, blue, alpha, not
bytes from 0 to 255.

## The render loop

```c
    // The render loop: one iteration draws and presents exactly one frame.
    uint64_t frame_index = 0;
    while (live ? !dvz_window_should_close(window) : frame_index < 1)
    {
        dvz_window_host_poll(host);
        int status = dvz_canvas_frame(canvas);
        if (status == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            continue;
        if (status != DVZ_CANVAS_FRAME_READY)
            break;
        dvz_canvas_submit(canvas);
        frame_index++;
    }
    printf("rendered %llu frames\n", (unsigned long long)frame_index);
```

A live run loops until the window is closed; a `--png` run needs a single frame. Each iteration does
three things:

1. `dvz_window_host_poll` drains operating-system events: keyboard, mouse, resize, close. Skip it
   and your window stops responding, even though it keeps rendering.
2. `dvz_canvas_frame` acquires the next image, opens a command buffer, and calls your `draw`
   callback with both. `DVZ_CANVAS_FRAME_READY` means that happened. `DVZ_CANVAS_FRAME_WAIT_SURFACE`
   means the surface is temporarily unusable (the window is minimized, or mid-resize), so there is
   nothing to draw this iteration and you simply try again.
3. `dvz_canvas_submit` sends the recorded commands to the GPU and, in live mode, presents the result.

Note the division of labour: your callback records *what* to draw, `dvz_canvas_submit` decides
*when* it runs. Nothing has reached the GPU until that call.

## Saving the image

```c
    if (png_path != NULL)
        dvz_canvas_capture_png(canvas, png_path);
```

Reading back an offscreen frame means waiting for the GPU to finish, copying the image into
host-visible memory, and encoding it. This one call does all of it.

## Cleanup

```c
    // Destroy in reverse order of creation.
    dvz_rendering_free(renderer.rendering);
    dvz_commands_free(renderer.commands);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    printf("validation errors: %u\n", dvz_gpu_ctx_error_count(gpu));
    dvz_gpu_ctx_destroy(gpu);
    return 0;
}
```

Destruction order matters: a GPU object cannot outlive the device that created it, and the device is
owned by the GPU context, so the context is destroyed last. Reverse order of creation is the rule
that keeps this correct as `Renderer` grows.

`dvz_gpu_ctx_error_count` reports how many validation errors the layers recorded. It is queried
*before* the context is destroyed, and it should print `0` for the rest of the course. When it does
not, read the messages on your terminal: they name the call that was wrong.

## Run it

```sh
cmake --build build
./build/vkcourse
```

A window appears, filled with dark blue, and stays until you close it. Resize it. The color still
fills it, because the canvas rebuilt its swapchain and your callback simply drew into the new,
larger image.

Then the same program, without a window:

```sh
./build/vkcourse --png chapter02.png
```

```
GPU: Apple M3
rendered 1 frames
validation errors: 0
```

??? info "Under the hood: what you just skipped"

    A raw Vulkan program reaching this same dark blue window writes, roughly:

    - `vkCreateInstance` with the right extensions for your platform, plus the debug-messenger
      plumbing that makes validation errors readable — about 80 lines.
    - `vkEnumeratePhysicalDevices`, then scoring each candidate for the extensions, features, and
      queue families you need — about 150 lines.
    - `vkCreateDevice`, requesting queues and chaining several feature structs — about 80 lines.
    - A platform surface, then swapchain creation: querying supported formats and present modes,
      choosing one, picking an image count, creating the swapchain, retrieving its images, and
      creating a view for each — about 250 lines.
    - A command pool, command buffers, and per-frame semaphores and fences — about 100 lines.
    - An acquire/record/submit/present sequence with correct wait stages, plus detecting
      `VK_ERROR_OUT_OF_DATE_KHR` and rebuilding the entire swapchain on resize — about 200 lines.

    That is the roughly 900 lines the canvas replaced. Not one of them is about graphics, which is
    why this course does not spend three chapters on them. But you now know what they are, and
    where to look when you eventually want to write them yourself.

!!! tip "Try it"

    1. **Change the clear color** to `{1.0f, 1.0f, 1.0f, 1.0f}`. The window turns white. Now try
       `{255.0f, 0.0f, 0.0f, 1.0f}`: still just red, because values are clamped to 1.0.
    2. **Comment out the two `dvz_cmd_rendering_*` calls** and run again. The window turns black.
       That black is worth understanding: Vulkan itself promises *nothing* about the contents of
       an image you have not written to. What you would get is undefined: zeros, stale pixels, or a
       driver's debug fill. You see black because the canvas clears each target it creates once,
       before your callback ever sees it, precisely so that this experiment has a defined outcome on
       every platform. From the second frame onward, what the target holds is entirely up to the
       rendering you record.
    3. **Delete `dvz_window_host_poll`** from the loop. The window still fills with color but stops
       responding: no resizing, and the close button does nothing.
    4. **Compare colors.** The `0.10f` red in your clear value comes out as 89 in the PNG, not 26.
       The canvas image uses an sRGB format, so the linear values you write are gamma-encoded on the
       way out. Chapter 14 returns to this when it starts to matter for lighting.

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

- What is the difference between a physical device and a logical device, and which one do the
  objects you create belong to?
- Name three things the canvas does for you that a raw Vulkan program would have to do by hand.
- Why does `dvz_canvas_frame` sometimes return `WAIT_SURFACE`, and why is that not an error?
- Why is the GPU context destroyed last?

Next: [How a frame works](03-frame.md).
