# 3. How a frame works

**Your program at the end of this chapter: about 140 lines.**

Chapter 2 asked you to accept five lines on faith. This chapter takes them apart, because they are
the shape of every frame you will ever draw: the triangle in chapter 4 and the lit textured mesh in
chapter 15 both slot into exactly the same sequence, between the same two calls.

Then you will make the window pulse, which proves the loop really is running rather than showing you
one frame forever.

## Two processors, one to-do list

A GPU program is two programs, on two processors, handed to each other every frame.

The CPU decides *what* to draw and writes it down: use this pipeline, draw these three vertices,
clear to this color. It does not draw anything. That written-down list is a **command buffer**.

The GPU reads the command buffer afterwards and executes it. By then your callback has long since
returned — the CPU is already working on the next frame while the GPU chews on the last one. This
lag is the reason Vulkan needs so much synchronization, and the reason the canvas earns its keep.

Two consequences worth internalizing now, because both are common sources of confusion:

- **Recording is not drawing.** When `draw` returns, nothing has happened on the GPU yet. Nothing
  happens until `dvz_canvas_submit`.
- **Anything the recorded commands point at must still exist when the GPU gets there.** A buffer you
  free at the end of your callback is a buffer the GPU may read after it is gone. Ownership, in this
  course, is not bookkeeping pedantry; it is the difference between a picture and a crash.

## What the canvas hands you

Your callback receives a `DvzStreamFrame*`. Three of its fields matter for now:

| Field | What it is |
| --- | --- |
| `frame->command_buffer` | A command buffer the canvas has already opened for recording. You write into it. |
| `frame->image_view` | The image this frame renders into — one of the swapchain images in live mode. |
| `frame->extent` | Its current size in pixels. This changes when the window is resized. |

All three are **borrowed**. They are valid for the duration of this one call and no longer. Do not
store them in `Renderer`, do not free them, and do not submit the command buffer yourself — the
canvas owns all of that. Each frame it may hand you a different image, and after a resize it will
hand you differently sized ones.

The `extent` field is the reason a correct renderer never caches the window size. Read it from the
frame every time.

## The five calls, one at a time

```c
    dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, renderer->commands);
```

The rest of the `vklite` API records through a typed `DvzCommands` object, and what the canvas gives
you is a raw Vulkan handle. This wraps the handle in the `DvzCommands` you allocated once at startup.
It creates nothing and owns nothing — think of it as pointing your recording tool at the canvas's
buffer. `_borrowed_recording` names both facts: borrowed, and already recording.

```c
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height,
        clear, renderer->rendering);
```

This describes *where* the next commands will draw. Despite the `dvz_cmd_` prefix it records nothing
at all — it fills in the `DvzRendering` description with: the render area, one color **attachment**
pointing at `frame->image_view`, a load operation of "clear", a store operation of "store", and the
clear value.

Those two operations deserve a moment, because they are how a GPU thinks about images:

- The **load op** says what happens to the attachment's existing contents when the pass begins.
  `CLEAR` overwrites everything with the clear value; `LOAD` keeps what was already there. Clearing
  is not a command you issue — it is a property of *starting to render*, which is why there is no
  `dvz_cmd_clear` in the sequence.
- The **store op** says whether the results are written back to memory when the pass ends. `STORE`
  keeps them, which is what you want for an image you intend to display.

Being explicit about this pays off on mobile and tiled GPUs, where "discard the contents, I'm going
to overwrite them all anyway" is dramatically cheaper than reading them back in.

```c
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);
```

*Now* something is recorded. This opens the rendering pass described above: from here on, draw
commands target that attachment. The split between describing and beginning is not ceremony — in
chapter 10 you will add a depth attachment to the description before beginning the pass.

```c
    dvz_cmd_rendering_end(renderer->commands);
```

Closes the pass. Every draw call in this course goes between these two lines. Right now there are
none, so the frame consists of just the clear.

```c
    dvz_commands_unwrap(renderer->commands);
```

Detaches your typed wrapper from the borrowed buffer. It does not end, reset, submit, or destroy the
command buffer; the canvas does all four. Skipping this leaves your `DvzCommands` pointing at a
handle that is about to become someone else's.

Who owns what, for this chapter:

| Value | Owner | Valid until |
| --- | --- | --- |
| `frame->command_buffer` | canvas | end of this callback |
| `frame->image_view` | canvas | end of this callback |
| `frame->extent` | canvas | end of this callback (changes on resize) |
| `renderer->device` | GPU context | program teardown |
| `renderer->commands`, `renderer->rendering` | your renderer | program teardown |

## Make it move

A static color cannot tell you whether the loop is running at 60 frames a second or froze after the
first one. Drive the color from a clock instead.

Add `<math.h>` to the top of the file:

```c
#include <math.h>
```

`Renderer` no longer needs a stored clear color, but it does need a start time and a flag for
whether to animate at all:

```c
typedef struct
{
    DvzDevice* device;
    DvzCommands* commands;
    DvzRendering* rendering;
    uint64_t start_ns;
    bool animate;
} Renderer;
```

In the callback, compute the color before recording:

```c
    // Elapsed seconds since startup. Offscreen captures use a fixed time so the image never
    // depends on how fast the machine ran.
    float t = 0.5f;
    if (renderer->animate)
        t = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;

    VkClearValue clear = {.color.float32 = {
                              0.10f + 0.10f * sinf(t * 1.7f),
                              0.12f + 0.12f * sinf(t * 2.3f),
                              0.18f + 0.18f * sinf(t * 1.1f),
                              1.00f,
                          }};
```

and pass that local `clear` to `dvz_cmd_rendering_default` in place of `renderer->clear`.

Two details in those seven lines are worth more than they look.

**Animate from a clock, not a frame counter.** `dvz_time_monotonic_ns` is a monotonic nanosecond
timer. Deriving motion from elapsed seconds means the animation runs at the same speed on a 30 Hz
laptop and a 144 Hz monitor, whereas `frame_index * 0.01f` would run five times faster on the
latter. Everything that moves in this course — the spinning cube, the arcball's inertia — takes its
time from here.

**Offscreen renders use a fixed time.** A wall clock would make every `--png` capture different, so
nothing could ever be compared against a reference. Freezing `t` at `0.5` offscreen makes captures
byte-for-byte reproducible: run it twice, get the same file.

Finally, update the initializer to match the new struct:

```c
    Renderer renderer = {
        .device = dvz_gpu_ctx_device(gpu),
        .commands = dvz_commands_create_wrapper(),
        .rendering = dvz_rendering_create_wrapper(),
        .start_ns = dvz_time_monotonic_ns(),
        .animate = live,
    };
```

## Run it

```sh
cmake --build build
./build/vkcourse
```

The window now breathes slowly through blues and greys. Drag its edge while it does — the animation
keeps going, because resizing changes only the `extent` your callback reads each frame.

```sh
./build/vkcourse --png chapter03.png
./build/vkcourse --png chapter03-again.png
```

Both files are identical.

??? info "Under the hood: the frame you did not have to orchestrate"

    In raw Vulkan, the sequence around your recording looks roughly like this, every frame:

    1. Wait on the fence for this frame slot, then reset it.
    2. `vkAcquireNextImageKHR` with a semaphore, and handle `VK_ERROR_OUT_OF_DATE_KHR` by rebuilding
       the swapchain and starting over.
    3. Reset the command buffer and `vkBeginCommandBuffer`.
    4. A pipeline barrier transitioning the acquired image from `UNDEFINED` (or `PRESENT_SRC_KHR`) to
       `COLOR_ATTACHMENT_OPTIMAL`, with the right source and destination stage masks.
    5. Your recording — the only part this course asks you to write.
    6. A second barrier back to `PRESENT_SRC_KHR`.
    7. `vkEndCommandBuffer`, then `vkQueueSubmit` with the acquire semaphore as a wait, a render
       semaphore as a signal, and the fence.
    8. `vkQueuePresentKHR` waiting on the render semaphore, and handle out-of-date *again*.

    Steps 1, 2, 4, 6, 7, and 8 are what `dvz_canvas_frame` and `dvz_canvas_submit` do. Getting the
    two image-layout barriers or the wait stages subtly wrong is the classic Vulkan beginner
    experience: it usually still renders, just with tearing, flicker, or a validation error you have
    to decode.

!!! tip "Try it"

    1. **Keep the previous contents.** Override the load op after the default setup, just before
       beginning the pass:

        ```c
        DvzAttachment* color = dvz_rendering_color(renderer->rendering, 0);
        dvz_attachment_ops(color, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
        ```

        Nothing overwrites the image now, so what you see is whatever was already in it. A `--png`
        run shows the canvas's own initial clear: opaque black. Live, you see stale frames, because
        each image still holds what was last drawn into it — a useful reminder that "the screen" is
        several images taking turns, not one.

        Worth knowing what you are relying on here. Vulkan does not define the contents of a fresh
        image; loading one is only meaningful because the canvas clears every target it creates
        before handing it over. Do the same for any image you create yourself in later chapters:
        write it before you read it.
    2. **Slow it down** by dividing the three `sinf` frequencies by 10. The window drifts almost
       imperceptibly.
    3. **Print the frame rate.** Every 60 frames, print `frame_index` divided by elapsed seconds. On
       a live window you should see your monitor's refresh rate, because presentation waits for
       vertical sync.
    4. **Remove `dvz_commands_unwrap`.** With validation on you will get complaints on the next
       frame. Read them: they are what a Vulkan mistake looks like when the layers catch it for you.

## When it goes wrong

| Symptom | Cause and fix |
| --- | --- |
| Nothing animates, one static color | `animate` is false, or `start_ns` was never initialized. Note that `--png` mode is *meant* to be static. |
| The animation runs at wildly different speeds on two machines | The color is being derived from `frame_index` rather than elapsed time. |
| Two `--png` runs differ | `t` is coming from the clock in offscreen mode. That breaks reproducible captures. |
| Validation complains about a command buffer in the wrong state | A `dvz_cmd_*` call is outside the `begin`/`end` pair, or `unwrap` is missing. |
| Flicker or torn frames in live mode | Almost always a load op of `LOAD` where `CLEAR` was intended, as in the first experiment above. |

??? example "Your `main.c` at the end of chapter 3"

    ```c
    --8<-- "examples/c/vulkan/step03.c"
    ```

## Checkpoint

- At what exact point in your program does the GPU begin executing the commands you recorded?
- Why must you read `frame->extent` every frame instead of storing the window size once?
- What is the difference between a load op of `CLEAR` and one of `LOAD`, and why is clearing not a
  command?
- Why does offscreen mode use a fixed time value instead of the clock?

You now have a program that owns a window, a GPU, and a frame. Chapter 4 puts a triangle in it.
