# 2. Shaders and the Graphics Pipeline

Chapter 1 asked you to treat the pipeline as something already assembled by the time your triangle
appeared. This chapter opens that up: same renderer, same draw sequence, but you'll read the
shaders that produce a new effect and the actual code that builds the pipeline they run in.

![A pastel triangle with a luminous cyan edge produced by the fragment shader](../../assets/tutorials/vulkan/shaders-and-pipeline.webp)

## Run it

```console
./build/gpu-tutorial/shaders_and_pipeline --live
```

Or validate it offscreen:

```console
./build/gpu-tutorial/shaders_and_pipeline --offscreen --frames 3 --validate --png shader-pipeline.png
```

(See [chapter 1](first-triangle.md) if you need a reminder of what `--offscreen`, `--frames`, and
`--validate` do.) This executable is compiled from the exact same C renderer as chapter 1 — only
the external shader directory differs, pointing at `shaders/pipeline/` instead of `shaders/`.

## The shaders: an edge glow from barycentric coordinates

Here is the vertex shader, [`shaders/pipeline/vklite_triangle.vert`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/shaders/pipeline/vklite_triangle.vert):

```glsl
#version 450

layout(location = 0) out vec3 barycentric;

const vec2 positions[3] = vec2[3](
    vec2( 0.0, -0.65),
    vec2( 0.65, 0.65),
    vec2(-0.65, 0.65));

const vec3 corners[3] = vec3[3](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0));

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    barycentric = corners[gl_VertexIndex];
}
```

The positions are exactly the same triangle from chapter 1. What's new is `corners`: instead of an
RGB color, each vertex now outputs a **barycentric coordinate** — a 3-component vector that is `1`
in the slot matching its own corner and `0` in the other two. Vertex 0 outputs `(1,0,0)`, vertex 1
outputs `(0,1,0)`, vertex 2 outputs `(0,0,1)`.

Rasterization interpolates this exactly like it interpolated color in chapter 1 — it's the same
mechanism, just carrying different numbers. The useful property: at any point inside the triangle,
each component of the interpolated vector tells you how close that point is to the *opposite* edge.
A component reaches `0` exactly on the edge facing its own corner, and `1` only at its own corner.
So `min(barycentric.x, barycentric.y, barycentric.z)` — the smallest of the three — is a single
number that is `0` on any edge and rises toward the middle: a ready-made "distance from the nearest
edge" signal, built entirely by the interpolator, with no per-pixel geometry math.

The fragment shader, [`shaders/pipeline/vklite_triangle.frag`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/shaders/pipeline/vklite_triangle.frag), uses exactly that:

```glsl
#version 450

layout(location = 0) in vec3 barycentric;
layout(location = 0) out vec4 out_color;

void main()
{
    float nearest_edge = min(barycentric.x, min(barycentric.y, barycentric.z));
    float glow = smoothstep(0.0, 0.08, nearest_edge);
    vec3 interior = 0.25 + 0.75 * pow(barycentric, vec3(0.45));
    vec3 edge = vec3(0.03, 0.90, 1.00);
    out_color = vec4(mix(edge, interior, glow), 1.0);
}
```

- `nearest_edge` is the "distance from the nearest edge" signal described above.
- `smoothstep(0.0, 0.08, nearest_edge)` returns `0.0` when `nearest_edge` is `0.0` (right on an
  edge), `1.0` once `nearest_edge` reaches `0.08` or beyond, and a smooth S-curve in between — it's
  GLSL's built-in way to fade a value in or out over a chosen range instead of a hard cutoff.
- `interior` reuses `barycentric` a second time, this time as a pastel color rather than a
  distance — `pow(..., 0.45)` just brightens it — so the triangle's interior is tinted by the same
  values that drove the edge glow.
- `mix(edge, interior, glow)` blends between the fixed cyan `edge` color and the computed
  `interior` color, using `glow` as the blend weight: cyan right at the boundary, fading to the
  pastel interior a small distance in.

This is a general GPU pattern worth naming: the vertex stage supplies a few numbers per vertex,
rasterization interpolates them for free across every covered pixel, and the fragment stage turns
that interpolated value into a visual effect. Chapter 1 used it for color; here the same mechanism
drives a distance-based glow.

## From GLSL source to a shader module

Both shaders above are plain text files. Two steps turn each one into something the GPU can
execute: **compiling** GLSL to **SPIR-V**, then wrapping that SPIR-V in a **shader module**.

> **Build-time vs. runtime compilation.** Most Vulkan applications compile GLSL to SPIR-V once,
> at build time, using an offline tool such as `glslc`, and ship only the resulting `.spv` binary —
> the GPU never sees GLSL text at all. This tutorial does something less common: it bundles a
> shader compiler and calls it every time the program starts, so editing a `.vert` or `.frag` file
> and restarting shows the change immediately, with no separate build step. That's convenient for
> learning and for the "deliberate failure" exercises in this course, but it is not the typical
> production setup — a shipped application usually precompiles shaders to avoid paying compilation
> cost, and the size and dependency of a compiler, on every launch.

The renderer reads a shader file and asks Datoviz's runtime compiler to turn it into SPIR-V:

```c
char* source = dvz_read_text(path, &source_size);
DvzShaderCompileRequest request = {
    .stage = stage,
    .profile = DVZ_SHADER_PROFILE_GRAPHICS,
    .source = source,
    .source_size = source_size,
    .source_name = path,
    .entry_point = "main",
};
DvzShaderCompileStatus status = dvz_shader_compile(&request, result);
```

`stage` says which shader stage this text is for (vertex or fragment — the two need different
compilation contexts because they see different built-in variables, such as `gl_VertexIndex` versus
`gl_FragCoord`). `DVZ_SHADER_PROFILE_GRAPHICS` targets Vulkan 1.0 and SPIR-V 1.0, the baseline this
tutorial is written against. On success, `result` owns the compiled SPIR-V bytes and any compiler
diagnostics, until `dvz_shader_compile_result_destroy()` releases them — the same
allocate/read/destroy discipline you've already seen for buffers.

SPIR-V bytes on their own aren't yet something the GPU can bind. They're wrapped in a **shader
module**, a GPU-side object representing "this compiled code, loaded and ready":

```c
DvzShader* vertex_shader = dvz_shader_create_wrapper();
dvz_shader(device, vertex.spirv_size, vertex.spirv, vertex_shader);
```

`vertex_shader` is what gets attached to the pipeline next.

## Building the pipeline

Here is the actual pipeline-construction code from this chapter's renderer (trimmed of the
depth/texture/arcball branches used by later, optional chapters — irrelevant here since none of
those features are enabled):

```c
renderer->slots = dvz_slots_create_wrapper();
renderer->pipeline = dvz_graphics_create_wrapper();

dvz_slots(device, renderer->slots);
dvz_slots_create(renderer->slots);

dvz_graphics(device, renderer->pipeline);
dvz_graphics_shader(renderer->pipeline, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(vertex_shader));
dvz_graphics_shader(renderer->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(fragment_shader));
dvz_graphics_primitive(renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
dvz_graphics_attachment_color(renderer->pipeline, 0, color_format);
dvz_graphics_layout(renderer->pipeline, dvz_slots_handle(renderer->slots));
dvz_graphics_viewport(renderer->pipeline, 0, 0, 0, 0, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
dvz_graphics_scissor(renderer->pipeline, 0, 0, 0, 0, DVZ_GRAPHICS_FLAGS_DYNAMIC);

dvz_graphics_create(renderer->pipeline);
```

One call at a time:

1. **`dvz_slots` / `dvz_slots_create`** — builds this pipeline's **pipeline layout**: a declaration
   of what GPU resources it can access — descriptor sets (textures, uniform buffers) and push
   constants. This chapter's shaders read no resources beyond their vertex inputs, so the layout
   built here is empty. `vklite` calls the object "slots" because a pipeline layout is, mechanically,
   a list of resource-binding slots; you'll fill some in once a later chapter adds a texture. Note
   that the pipeline layout is a *separate* object from the graphics pipeline itself — it's built
   first, then attached, in step 6 below.
2. **`dvz_graphics`** — creates an empty pipeline object tied to the device, ready to have state
   attached to it.
3. **`dvz_graphics_shader`** — attaches a compiled shader module to one stage of the pipeline. Two
   calls here, one per stage, each naming which `VK_SHADER_STAGE_*` it fills.
4. **`dvz_graphics_primitive`** — sets the primitive topology (chapter 1): triangle-list, so every
   3 vertices in a draw call become one independent triangle.
5. **`dvz_graphics_attachment_color`** — declares that this pipeline renders into one color
   attachment, in `color_format`. This format must match the format Canvas actually hands the
   renderer each frame — it's the exact fact the "format mismatches" counter from chapter 1 checks
   at runtime.
6. **`dvz_graphics_layout`** — attaches the pipeline layout built in step 1 to this pipeline.
7. **`dvz_graphics_viewport` / `dvz_graphics_scissor`** — instead of baking in fixed pixel
   rectangles, both are declared **dynamic** (`DVZ_GRAPHICS_FLAGS_DYNAMIC`), meaning their real
   values are supplied later, per frame, by `dvz_cmd_set_viewport_scissor()` in the draw callback
   you read in chapter 1. That's why resizing the live window doesn't require rebuilding the
   pipeline.
8. **`dvz_graphics_create`** — the step that actually asks the driver to build the GPU-native
   pipeline object from everything declared above. This is typically the slow, one-time call in
   this whole sequence: the driver validates and compiles the shader modules together with all the
   fixed-function state into a form its specific GPU can execute directly.

A graphics pipeline is best thought of as one **immutable** object: once `dvz_graphics_create`
returns, none of the state above can be changed in place. That's exactly why editing a shader file
and restarting is required instead of somehow patching the running pipeline — the compiled shader
module is baked into the pipeline object at creation time, so a new shader means a new pipeline.

## Experiment

In the fragment shader, change `0.08` in the `smoothstep()` call:

- `0.02` produces a thin edge;
- `0.20` produces a broad glow;
- reversing the two threshold arguments deliberately produces a very different transition.

Restart after each edit and connect the visible edge width to the interpolated `nearest_edge` value.

## Deliberate interface failure

Change the vertex shader output location from `0` to `1` without changing the fragment input. Each
shader may still compile individually — runtime compilation treats stages independently, and
`location = 1` is valid GLSL on its own. The mismatch only becomes visible when the two stages meet
inside `dvz_graphics_create`, where the pipeline layout requires every output location the vertex
shader declares to have a matching input location in the fragment shader. With validation enabled,
this incompatibility is reported there. Restore matching locations before continuing.

## Checkpoint

You should now be able to distinguish: build-time `glslc` compilation from this tutorial's runtime
`shaderc`-based compilation; GLSL source from compiled SPIR-V from a loaded shader module; a
pipeline layout from the graphics pipeline it's attached to; what makes a pipeline object
immutable once created, and why that forces a restart on shader changes; what dynamic viewport/
scissor state buys you; and how barycentric-coordinate interpolation produces the edge-glow effect
from nothing but per-vertex constants and the rasterizer.

## Exercise

Use the three barycentric components to give each edge a different color. Keep the vertex shader
unchanged and implement the complete effect in the fragment shader.

Next: [Vertex data and GPU buffers](vertex-buffers.md).
