# 1. First Live Triangle

Your first result is a colored triangle in a live, resizable window. Nothing about its shape or
color comes from CPU-side geometry — both are computed on the GPU, inside the shader itself. That
makes this chapter the smallest possible GPU program: one triangle, two tiny shaders, and the draw
call that connects them.

![A pastel triangle rendered from positions and colors generated in the vertex shader](../../assets/tutorials/vulkan/first-triangle.webp)

## Run it

From the standalone tutorial build:

```console
./build/gpu-tutorial/first_triangle --live
```

For a bounded, deterministic check instead of a live window:

```console
./build/gpu-tutorial/first_triangle --offscreen --frames 3 --validate --png first-triangle.png
```

`--offscreen` renders into memory instead of a window and writes a PNG once it's done — useful for
scripting and for this tutorial's checkpoints. `--frames 3` stops after 3 frames instead of running
until you close the window. `--validate` turns on the Vulkan validation layers, which catch
API-misuse bugs and turn them into a failing exit status instead of undefined behavior.

The program should report three submitted and three drawn frames, zero format mismatches, zero
invalid frame contracts, and no creation failure. Keep this running (or re-run it after each
change below) — the rest of the chapter explains what you just saw.

## Before you go further: how a GPU draws one frame

A modern GPU program is really two programs running on two different processors, handed off to
each other every frame:

- The **CPU** decides *what* to draw and writes down a to-do list — "use this shader, draw these
  3 vertices" — into a **command buffer**. It does not draw anything itself.
- The **GPU** later reads that command buffer and actually executes it, in four steps:
  1. run your **vertex shader** once per vertex;
  2. group those vertices into shapes — **primitive assembly** — using a fixed rule called
     **primitive topology**: "every 3 vertices form one independent triangle," "every 2 form a
     line," "every vertex is its own point," and so on;
  3. fill in every pixel inside each assembled shape (**rasterization**) — a pixel counts as
     "covered" by the shape whenever its center point falls inside the shape's silhouette;
  4. run your **fragment shader** once for every pixel it filled in.

So for this chapter's triangle: the vertex shader runs 3 times (once per corner); primitive
assembly, using the **triangle-list** topology, groups those 3 vertices into exactly 1 triangle
because the draw call below asks for 3 vertices and nothing more; and the fragment shader runs
once per covered pixel — likely thousands of times. Critically, a GPU is built to run huge numbers
of same-stage invocations *at the same time* across many small cores, not one after another —
that's the entire reason GPUs exist instead of just using the CPU. The vertex shader's job is to
say *where* each corner goes; the fragment shader's job is to say *what color* each pixel is.

Both shaders, plus the topology that governs primitive assembly and the image format to draw into,
are compiled together into one GPU-ready object called a **pipeline**. For this chapter, the
pipeline you'll use has exactly those three facts baked in: the two shaders below, triangle-list
topology, and the Canvas color format. (The "format mismatches" counter in the run output above is
exactly this fact being checked at runtime: that the color format baked into the pipeline still
matches what Canvas hands it each frame.) Chapter 2 opens up the code that actually builds a
pipeline; for now, treat it as already assembled by the time your triangle appears, and focus on
the shaders and the draw call.

## Vulkan, vklite, and Canvas: which layer are you actually looking at?

This tutorial's title says Vulkan, but you will not write a single raw Vulkan call. That is
deliberate, and worth being explicit about, because "Vulkan" refers to several different things
depending on which layer of a program you're reading:

- **Raw Vulkan** is a C API where you are responsible for *everything*: picking a physical device,
  creating a logical device and queues, building a window surface and swapchain, synchronizing
  frames with semaphores and fences, and only then getting to shaders and draw calls. A minimal
  raw-Vulkan triangle commonly runs 700–1500 lines before anything appears on screen — almost none
  of it is about drawing a triangle, it's about standing up the machinery to be allowed to.
- **`vklite`** is a thin C wrapper, built inside Datoviz, over that same Vulkan machinery. It does
  not hide Vulkan's concepts — you still create shader modules, pipelines, and command buffers by
  name, and you still think in terms of binding and drawing — it just gives each concept a typed
  object (`DvzShader`, `DvzGraphics`, `DvzCommands`, ...) and a manageable function call instead of
  a raw handle and a 10-field struct literal. Every function you'll call in this tutorial is
  `vklite`.
- **Canvas** sits next to `vklite` and absorbs the part of raw Vulkan that has nothing to do with
  *your* rendering: the window, the swapchain or offscreen images, acquiring a frame, submitting
  it, presenting it, and recovering from a resize. Canvas hands your draw callback a ready
  command buffer and image each frame and takes it back afterward.
- Datoviz also has a higher-level, declarative **Scene** API (figures, panels, visuals — see
  [First Scene](../first-scene.md)) that hides shaders, pipelines, and command recording entirely.
  Most Datoviz users work at that level. This tutorial deliberately stays one level *below* it, at
  `vklite` + Canvas, because seeing the shader/pipeline/draw-call machinery explicitly is the whole
  point of a Vulkan concepts course — the Scene API would hide exactly what you're here to learn.

In short: Canvas removes the boilerplate that has nothing to teach you about GPU rendering;
`vklite` keeps every remaining Vulkan concept visible, just typed and named.

> **A note on shading languages.** The shaders below are written in **GLSL** (the OpenGL Shading
> Language), which Vulkan also accepts. GLSL source is compiled to **SPIR-V**, a binary
> instruction format, before the GPU can run it — Datoviz does that compilation for you at
> startup. GLSL is not the only shading language you'll encounter in GPU programming: **WebGPU**,
> the browser-oriented GPU API, uses a different language called **WGSL**, and cross-compilers
> such as Google's **Tint** or Mozilla's **Naga** (used by `wgpu`, and by Google's **Dawn** WebGPU
> implementation) translate between SPIR-V, WGSL, HLSL, and MSL so the same shader logic can target
> multiple backends. None of that matters for this tutorial — it's here so the term "GLSL" doesn't
> read as *the* shading language rather than *a* shading language.

## The shaders: where the triangle actually comes from

Open [`vklite_triangle.vert`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/shaders/vklite_triangle.vert), the vertex shader. It is short enough to read in full:

```glsl
#version 450

layout(location = 0) out vec3 color;

const vec2 positions[3] = vec2[3](
    vec2( 0.0, -0.65),
    vec2( 0.65, 0.65),
    vec2(-0.65, 0.65));

const vec3 colors[3] = vec3[3](
    vec3(1.0, 0.2, 0.2),
    vec3(0.2, 1.0, 0.2),
    vec3(0.2, 0.4, 1.0));

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    color = colors[gl_VertexIndex];
}
```

A few new pieces:

- The draw call you'll see below asks for 3 vertices, so the GPU launches 3 invocations of this
  exact shader code — in parallel, not in sequence. There is no "first" or "second" invocation;
  think of it as 3 simultaneous copies of the same tiny program, each one given a different value
  of `gl_VertexIndex` — `0`, `1`, and `2` — so it knows which vertex it is responsible for. The
  same is true of the fragment shader below, one invocation per pixel, all in parallel.
- `positions` and `colors` are plain constant arrays baked into the shader, indexed by
  `gl_VertexIndex`. There is no CPU-side geometry yet — the shader itself *is* the geometry.
  **This is not how real applications supply geometry** — it only works here because there are
  exactly 3 hardcoded vertices to name. Chapter 3 replaces these arrays with data uploaded from a
  C array into a GPU buffer, which is the normal approach for anything beyond a fixed demo shape;
  treat this version as a teaching shortcut that removes one moving part, not a pattern to reuse.
- `gl_Position` is the shader's required output: the vertex's location in **clip space**, where
  `x` and `y` each range from `-1` to `+1` regardless of window size or aspect ratio. The GPU maps
  clip space onto your actual window pixels. One Vulkan-specific quirk worth knowing up front:
  unlike OpenGL, Vulkan's clip space has **`y` increasing downward** — negative `y` is toward the
  *top* of the screen, not the bottom. That's exactly why the first vertex, at `y = -0.65`, is the
  top corner of the triangle you saw when you ran it, while the other two, at `y = +0.65`, form the
  bottom edge.
- `layout(location = 0) out vec3 color` declares an output that travels to the next stage.
  "Location 0" is just a numbered wire; the fragment shader will read the same wire by declaring
  a matching `location = 0` input.

Now the fragment shader, [`vklite_triangle.frag`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/shaders/vklite_triangle.frag):

```glsl
#version 450

layout(location = 0) in vec3 color;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(color, 1.0);
}
```

Each invocation of this shader handles one pixel, and its `color` input is *not* one of the three
vertex colors verbatim. Between the two shader stages, rasterization automatically blends the 3
vertex colors based on each pixel's position inside the triangle — a pixel near the red corner
receives a color close to red, a pixel near the middle receives a blend of all three. This
automatic blending is called **interpolation**, and it's the reason the triangle shows a smooth
gradient instead of 3 flat-color wedges. The fragment shader here does nothing clever with that
interpolated value — it just writes it out as the pixel's final color.

`out_color` is a `vec4` — 4 components, RGB plus **alpha**, the 4th channel that controls opacity
(`0.0` fully transparent, `1.0` fully opaque) when a pixel is blended with whatever is already
behind it. This shader hardcodes alpha to `1.0`: every pixel it writes is fully opaque, which is
why `color` only ever needs to carry 3 components (RGB) between the two shader stages.

## One frame: the draw sequence

Canvas calls your draw callback once per frame, handing it a `frame` struct that describes exactly
what to draw into this time: `frame->command_buffer` (already recording), `frame->image_view` (the
image to render into), and `frame->extent` (its current size, which changes as the window is
resized). The code below also uses `renderer->device`, Datoviz's wrapper around the Vulkan
**logical device** — your program's handle to the GPU, used to create or allocate almost every
other GPU object you'll meet in this tutorial (shaders, pipelines, buffers, command buffers).
Canvas creates and owns the device; the renderer only borrows a pointer to it.

Here is the complete draw sequence for this chapter:

```c
dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, commands);
dvz_cmd_rendering_default(commands, frame->image_view, frame->extent.width, frame->extent.height, clear, rendering);
dvz_cmd_rendering_begin(commands, rendering);
dvz_cmd_bind_graphics(commands, renderer->pipeline);
dvz_cmd_set_viewport_scissor(commands, frame->extent);
dvz_cmd_draw(commands, 0, 3, 0, 1);
dvz_cmd_rendering_end(commands);
dvz_commands_unwrap(commands);
```

Read it as a to-do list, one line at a time:

1. **`dvz_commands_wrap_borrowed_recording`** — Canvas already opened a command buffer for this
   frame and is letting you write into it. This line wraps that raw Vulkan handle in a small
   `DvzCommands` object so the rest of the calls below have something typed to write through. It
   does not create or own the underlying command buffer.
2. **`dvz_cmd_rendering_default`** — declares what you are about to draw into: which image
   (`frame->image_view`), at what size, and what color to clear it to before drawing (a dark blue
   here). This is where "the GPU should render into *this* image" gets recorded.
3. **`dvz_cmd_rendering_begin`** — actually starts the rendering pass described above. Nothing is
   drawn between declaring a target and beginning it; the split exists because a few chapters from
   now you'll insert a depth attachment between these two calls.
4. **`dvz_cmd_bind_graphics`** — tells the GPU which pipeline to use for the draw calls that
   follow. "Binding" means: from this point on, this shader pair and this fixed-function state are
   active, the way selecting a tool in an image editor makes it the active tool for your next
   strokes.
5. **`dvz_cmd_set_viewport_scissor`** — tells the GPU which rectangle of the target image
   corresponds to clip space `(-1, -1)`–`(1, 1)`. This is set dynamically, per frame, from
   `frame->extent` instead of being baked into the pipeline, precisely so the triangle keeps
   filling the window correctly when you resize it live.
6. **`dvz_cmd_draw(commands, 0, 3, 0, 1)`** — the actual draw call, with four arguments:
   first vertex index (`0`), how many vertices to draw (`3`, which is what drives `gl_VertexIndex`
   through `0, 1, 2`), first instance (`0`), and instance count (`1`, meaning "draw this triangle
   once"). The vertex count is also what primitive assembly consumes: with the triangle-list
   topology baked into the pipeline, exactly 3 vertices become exactly 1 triangle — ask for 6
   instead and you'd get 2 independent triangles, not one 6-cornered shape. Instancing — drawing
   the same geometry many times cheaply — is not used yet, but the argument is always there.
7. **`dvz_cmd_rendering_end`** — closes the rendering pass opened in step 3.
8. **`dvz_commands_unwrap`** — detaches the typed wrapper from the borrowed command buffer. It
   does not end, reset, submit, or destroy the buffer — Canvas still owns all of that.

Everything above only *records* the to-do list; the CPU returns from this callback immediately,
and the GPU executes the recorded commands afterward, once Canvas submits the frame.

The table below summarizes who owns what, since mixing that up is the single most common source
of Vulkan crashes:

| Value | Ownership | Valid until |
| --- | --- | --- |
| `frame->command_buffer` | borrowed from Canvas | end of this draw callback |
| `frame->image_view` | borrowed from Canvas | end of this draw callback |
| `renderer->device` | borrowed from the GPU context Canvas configured | renderer lifetime |
| `commands`, `rendering` wrappers | owned by your renderer | renderer lifetime |
| `renderer->pipeline` | owned by your renderer | until you explicitly destroy it |

## Experiment: change the shader, not the C code

Clip-space `x` and `y` normally span `-1` to `+1`. In `vklite_triangle.vert`, change the first
position from `(0.0, -0.65)` to `(0.0, -0.25)`, save the file, and restart the executable. The
triangle changes shape without recompiling any C, because the program reads and compiles the GLSL
file at startup, every time it runs.

## Deliberate failure

Copy the shader directory, introduce an invalid token in the copied fragment shader, and run:

```console
./build/gpu-tutorial/first_triangle --offscreen --frames 1 --shader-dir /path/to/copied/shaders
```

Compilation should fail before drawing and name the copied file and source line. A missing
provider, incompatible provider, missing file, invalid request, and GLSL compilation error each
have a distinct status in the [runtime shader API](../../reference/c-api/runtime-shader.md).

## Checkpoint

You should now be able to explain, in your own words: why the vertex and fragment shaders each run
as many parallel invocations rather than a sequence of runs; what `gl_VertexIndex` and
`gl_Position` are for; which way `y` points in Vulkan clip space and why that matters; what
primitive assembly and primitive topology do between the vertex shader and rasterization, and why
3 vertices with triangle-list topology produce exactly 1 triangle; what makes rasterization decide
a pixel is "covered"; what interpolation does to the 3 vertex colors between the shaders; what the
alpha channel controls; why embedding geometry directly in a shader is a teaching shortcut rather
than normal practice; what a Vulkan device is and why the renderer only borrows one; what "binding"
a pipeline means; why viewport/scissor are set dynamically instead of being fixed at pipeline
creation; and what Canvas takes care of versus what `vklite` leaves in your hands.

## Exercise

Turn the triangle into a narrow arrowhead by changing only the three shader positions. Predict the
resulting image before restarting, then explain why no CPU-side geometry changed even though the
rendered shape did.

Next: [Shaders and the graphics pipeline](shaders-and-pipeline.md).
