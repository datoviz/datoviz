# 3. How a frame works

**Your program at the end of this chapter: about 140 lines.**

<picture>
  <source media="(prefers-reduced-motion: no-preference)" srcset="/assets/gpu-graphics/03-frame.webp">
  <img src="/assets/gpu-graphics/03-frame-still.webp" alt="The chapter 3 clear color pulsing through blues and greys.">
</picture>

Chapter 2 asked you to accept five lines on faith. This chapter takes them apart because they establish the shape of every frame in the course. The triangle in chapter 4 and the lit, textured mesh in chapter 15 both fit between the same two calls.

You will then make the window pulse, proving that the loop is running rather than displaying one frame forever.

## Two processors, one to-do list

A GPU program consists of work for two processors, handed from one to the other every frame.

The CPU decides *what* to draw and writes down instructions: use this pipeline, draw these three vertices, clear to this color. The CPU does not draw the image itself. That recorded list is a **command buffer**.

The GPU reads the command buffer after submission and executes it asynchronously. Your callback has returned by then, and the CPU may be preparing later work while the GPU chews on the submitted frame. Reusing a frame slot can still make the CPU wait. Managing that overlap safely is why Vulkan needs so much synchronization and why the canvas earns its keep.

This separation has two consequences that often confuse new Vulkan programmers:

- **Recording happens on the CPU; drawing happens later, on the GPU.** When `draw` returns, the recorded work has not reached the GPU. Submission begins with `dvz_canvas_submit`.
- **Anything referenced by the recorded commands must still exist when the GPU uses it.** If you free a buffer at the end of the callback, the GPU may try to read it afterwards. Ownership is not bookkeeping pedantry here; it is the difference between a picture and a crash.

## What the canvas hands you

Your callback receives a `DvzStreamFrame*`. Three of its fields matter for now:

| Field | What it is |
| --- | --- |
| `frame->command_buffer` | A command buffer the canvas has already opened for recording. You write into it. |
| `frame->image_view` | A Canvas-owned frame target. In live mode, the Canvas later transfers it to the acquired swapchain image. |
| `frame->extent` | Its current size in pixels. This changes when the window is resized. |

The command buffer and image view are **borrowed** handles. They are valid only for this callback. Do not store or free them, and do not submit the command buffer yourself; the canvas owns those operations. A later frame may use a different image view, especially after a resize.

The `extent` is a copied value rather than a borrowed handle, so storing the number itself is safe. It may become stale after a resize, however. Read the current extent from each frame before recording size-dependent commands.

## The frame calls, one at a time

```c
    dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, renderer->commands);
```

The `vklite` API records through a typed `DvzCommands` object, while the canvas gives you a raw Vulkan handle. This function wraps the handle in the `DvzCommands` object allocated at startup. It creates and owns nothing. Think of it as pointing your recording tool at the canvas's buffer. `_borrowed_recording` records both facts in the name: the handle is borrowed and already recording.

```c
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height,
        clear, renderer->rendering);
```

This describes *where* the next commands will draw. Despite the `dvz_cmd_` prefix, it records nothing. It fills the `DvzRendering` description with the render area, one color **attachment** that points to `frame->image_view`, a load operation of "clear," a store operation of "store," and the clear value.

The load and store operations define what happens to the attachment at the boundaries of the rendering pass:

- The **load op** says what happens to the attachment's existing contents when the pass begins. `CLEAR` overwrites everything with the clear value, while `LOAD` keeps what was already there. Clearing is a property of *starting to render*, not a separate command, which is why the sequence has no `dvz_cmd_clear`.
- The **store op** says whether the results are preserved when the pass ends. `STORE` keeps them, which is what you need for an image that will be displayed or read back.

This distinction can matter on mobile and tiled GPUs, where discarding contents that will be overwritten may avoid unnecessary memory work.

```c
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);
```

*Now* something is recorded. This opens the rendering pass described above, and subsequent draw commands target its attachments. Description and recording are separate because you may need to finish configuring the attachments first. In chapter 10, for example, you will add a depth attachment before beginning the pass.

```c
    dvz_cmd_set_viewport_scissor(renderer->commands, frame->extent);
```

The viewport maps normalized device coordinates into pixels, and the scissor limits drawing to a rectangular pixel region. This helper sets both to the full current frame. They are dynamic pipeline state, so you record them for the current extent before issuing draw calls. The clear does not need them, but the triangle in chapter 4 will.

```c
    dvz_cmd_rendering_end(renderer->commands);
```

This closes the pass. Every draw call in the course goes between `dvz_cmd_rendering_begin` and `dvz_cmd_rendering_end`. There are no draw calls yet, so the frame contains only the clear.

```c
    dvz_commands_unwrap(renderer->commands);
```

This detaches your typed wrapper from the borrowed buffer. It does not end, reset, submit, or destroy the command buffer; the canvas owns that lifecycle. Skipping the detach leaves your `DvzCommands` pointing at a callback-scoped handle after the callback returns.

Who owns what, for this chapter:

| Value | Owner | Valid until |
| --- | --- | --- |
| `frame->command_buffer` | canvas | end of this callback |
| `frame->image_view` | canvas | end of this callback |
| `frame->extent` | copied frame value | the value may be stored, but a later frame may have a different extent |
| `renderer->device` | GPU context | program teardown |
| `renderer->commands`, `renderer->rendering` | your renderer | program teardown |

## Make it move

A static color cannot tell you whether the loop is running at 60 frames per second or froze after the first frame. Drive the color from a clock instead.

Add `<math.h>` to the top of the file:

```c
#include <math.h>
```

`Renderer` no longer needs to store a clear color. It now needs a start time and a flag that enables animation:

```c
typedef struct
{
    DvzDevice* device;
    DvzCommands* commands;
    DvzRendering* rendering;
    uint64_t start_ns;
    float capture_time;
    bool animate;
} Renderer;
```

In the callback, compute the color before recording:

```c
    // Elapsed seconds since startup. Offscreen captures use a fixed time so the image never
    // depends on how fast the machine ran.
    float t = renderer->capture_time;
    if (renderer->animate)
        t = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;

    VkClearValue clear = {.color.float32 = {
                              0.10f + 0.10f * sinf(t * 1.7f),
                              0.12f + 0.12f * sinf(t * 2.3f),
                              0.18f + 0.18f * sinf(t * 1.1f),
                              1.00f,
                          }};
```

Pass this local `clear` value to `dvz_cmd_rendering_default` in place of `renderer->clear`.

The timing code follows two rules that will matter throughout the course.

**Animate from a clock, not a frame counter.** `dvz_time_monotonic_ns` is a monotonic nanosecond timer. Deriving motion from elapsed time keeps the animation speed independent of the display rate. In contrast, `frame_index * 0.01f` would run nearly five times faster at 144 Hz than at 30 Hz. Everything that moves later in the course, including the spinning cube and the arcball's inertia, follows this clock-based approach.

**Offscreen renders use a fixed time.** Reading the clock would make every `--png` capture different and prevent comparison with a reference image. Add `float capture_time = 0.5f;` beside `png_path`, accept an optional fixed time in the existing argument loop, and store it in `Renderer`:

```c
        else if (strcmp(argv[i], "--time") == 0)
            capture_time = strtof(argv[i + 1], NULL);
```

This requires `#include <stdlib.h>` for `strtof`. Ordinary offscreen runs freeze `t` at `0.5`. The `--time` option selects another reproducible instant and is also how the chapter's animated preview is generated.

Update the initializer to match the new struct:

```c
    Renderer renderer = {
        .device = dvz_gpu_ctx_device(gpu),
        .commands = dvz_commands_create_wrapper(),
        .rendering = dvz_rendering_create_wrapper(),
        .start_ns = dvz_time_monotonic_ns(),
        .capture_time = capture_time,
        .animate = live,
    };
```

## Run it

```sh
cmake --build build
./build/vkcourse
```

The window now breathes slowly through blues and greys. Drag an edge while it runs. The animation continues because each callback reads the new `extent` after a resize.

```sh
./build/vkcourse --png chapter03.png
./build/vkcourse --png chapter03-again.png
./build/vkcourse --png later.png --time 1.5
```

The first two files are identical. The third captures another reproducible point in the same animation.

??? info "Under the hood: the frame you did not have to orchestrate"

    In raw Vulkan, the sequence around this Canvas-style recording looks roughly like this for every live frame:

    1. Wait on the fence for this frame slot, then reset it.
    2. Call `vkAcquireNextImageKHR` with a semaphore, and handle `VK_ERROR_OUT_OF_DATE_KHR` by rebuilding the swapchain and starting over.
    3. Reset the command buffer and `vkBeginCommandBuffer`.
    4. Transition the Canvas-owned frame target to `COLOR_ATTACHMENT_OPTIMAL` with the right source and destination stage masks.
    5. Record your rendering, the only part the course asks you to write.
    6. Transition the completed frame target for transfer, transfer it into the acquired swapchain image, and transition that image to `PRESENT_SRC_KHR`.
    7. Call `vkEndCommandBuffer`, then `vkQueueSubmit` with the acquire semaphore as a wait, a render semaphore as a signal, and the fence.
    8. Call `vkQueuePresentKHR` while waiting on the render semaphore, and handle an out-of-date swapchain *again*.

    `dvz_canvas_frame` and `dvz_canvas_submit` coordinate this sequence around your callback. A wrong image transition or wait stage may still produce an image, but it can also cause flicker, stale contents, synchronization failures, or validation errors that are difficult to decode.

!!! tip "Try it"

    1. **Keep the previous contents.** Override the load op after the default setup, just before
       beginning the pass:

        ```c
        DvzAttachment* color = dvz_rendering_color(renderer->rendering, 0);
        dvz_attachment_ops(color, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
        ```

        Nothing overwrites the image now, so you see whatever it already contained. A `--png` run shows the canvas's initial opaque-black clear. In a live run, frame targets may show stale contents from their previous use. This is a useful reminder that presentation involves several images taking turns, not one permanent "screen" image.

        Vulkan does not define the contents of a fresh image. Loading this one is meaningful only because the canvas clears every target before first use. Follow the same rule for images you create later: write them before you read them.
    2. **Slow it down** by dividing the three `sinf` frequencies by 10. The window drifts almost imperceptibly.
    3. **Print the frame rate.** Every 60 frames, print `frame_index` divided by elapsed seconds. On a live window, the result should be close to the display's refresh rate because the default presentation path waits for vertical sync.
    4. **Inspect frame targets.** Print `frame->image_view` and `frame->extent` in the callback, then resize the window. The extent changes, and the canvas may supply different target views. This is why size-dependent state comes from the current frame and why the image view must not be retained.

## When it goes wrong

| Symptom | Cause and fix |
| --- | --- |
| Nothing animates, one static color | `animate` is false, or `start_ns` was never initialized. Note that `--png` mode is *meant* to be static. |
| The animation runs at wildly different speeds on two machines | The color is being derived from `frame_index` rather than elapsed time. |
| Two `--png` runs differ | `t` is coming from the clock in offscreen mode. That breaks reproducible captures. |
| Validation complains about a command buffer in the wrong state | A `dvz_cmd_*` call may be outside the rendering `begin`/`end` pair, or code may have begun, ended, reset, or submitted the borrowed command buffer. |
| Alternating stale colors or flicker in live mode | A load op of `LOAD` may be preserving the previous contents of each reused frame target. Restore `CLEAR` unless preserving those contents is intentional. |

??? example "Your `main.c` at the end of chapter 3"

    ```c
    --8<-- "examples/c/vulkan/step03.c"
    ```

## Checkpoint

- At what exact point in your program does the GPU begin executing the commands you recorded?
- Why must you read `frame->extent` every frame instead of storing the window size once?
- What is the difference between a load op of `CLEAR` and one of `LOAD`, and why is clearing not a command?
- Why does offscreen mode use a fixed time value instead of the clock?

You now have a program that owns a window and GPU context and can record and submit a frame through the canvas. Chapter 4 puts a triangle in that frame.
