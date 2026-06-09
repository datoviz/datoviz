# Raw triangle examples — implementation brief (COMPLETED)

**Status: DONE as of 2026-05-05.** All items below have been implemented.

This document describes two C examples to be added under `examples/c/`, plus one small API
addition needed to support them.  The existing `examples/c/visuals/point.c` and
`examples/c/CMakeLists.txt` are already in place; these two files extend that set.

---

## Context

`visuals/point.c` shows the highest-level path: scene → DvzApp → offscreen canvas.  The two
new examples sit below that, showing progressively rawer APIs:

| Example              | Layer used for rendering  | Canvas / presentation |
|----------------------|---------------------------|-----------------------|
| `visuals/point.c`      | Scene + DvzApp            | handled by DvzApp     |
| `advanced/raw_triangle_vklite.c` | vklite draw commands | DvzCanvas (explicit) |
| `advanced/raw_triangle_drp2.c` | DRP2 stream commands | DvzGpuCtx only (no canvas) |

---

## Part 1 — `raw_triangle_vklite.c` (vklite draw commands into DvzCanvas)

### Goal

Show a power user who knows Vulkan how to write their own draw commands using vklite helpers,
while letting DvzCanvas handle all presentation plumbing (frame timing, submission,
semaphores, offscreen images, swapchain, video recording).

The same draw callback runs unchanged for three backends, selected via `argv[1]`:

```
./raw_triangle offscreen   → renders 1 frame, writes raw_triangle.png
./raw_triangle glfw        → interactive GLFW window, closes on Escape
./raw_triangle video       → renders N frames, writes raw_triangle.mp4
```

### New API needed first: `dvz_compile_glsl`

The pipeline needs GLSL shaders compiled to SPIR-V at runtime.  Shaderc is already used
internally in `src/drp2/runtime.c` (lazy-loaded via dlopen).  A small public wrapper must be
added so examples (and future user code) can compile GLSL strings without going through DRP2.

**Where to add it:**

- Declaration in `include/datoviz/vk/gpu_ctx.h` (or a new `include/datoviz/vk/shader_util.h`)
- Implementation in `src/vk/` (or `src/drp2/runtime.c` as a thin exported wrapper)

**Signature:**

```c
/**
 * Compile a GLSL source string to SPIR-V using shaderc.
 *
 * @param stage   VK_SHADER_STAGE_VERTEX_BIT or VK_SHADER_STAGE_FRAGMENT_BIT
 * @param glsl    null-terminated GLSL source
 * @param out_size receives the byte size of the returned buffer
 * @returns heap-allocated SPIR-V words (caller must dvz_free()), or NULL on error
 */
DVZ_EXPORT uint32_t* dvz_compile_glsl(VkShaderStageFlagBits stage,
                                       const char* glsl,
                                       DvzSize* out_size);
```

The implementation should follow the same lazy-load pattern already used in `runtime.c`
(`_load_shaderc_once()` + function-pointer table).  If shaderc is unavailable at runtime,
return NULL and log an error.

### Triangle geometry and shaders

Three hard-coded vertices with per-vertex colour, interleaved in one buffer:

```c
typedef struct { float x, y; float r, g, b; } Vertex;
static const Vertex TRIANGLE[] = {
    { 0.0f,  0.6f,  1.0f, 0.0f, 0.0f},   /* top,   red   */
    {-0.6f, -0.6f,  0.0f, 1.0f, 0.0f},   /* left,  green */
    { 0.6f, -0.6f,  0.0f, 0.0f, 1.0f},   /* right, blue  */
};
```

Vertex shader (GLSL 450):
```glsl
#version 450
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;
void main() {
    gl_Position = vec4(inPos, 0.0, 1.0);
    fragColor = inColor;
}
```

Fragment shader:
```glsl
#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;
void main() { outColor = vec4(fragColor, 1.0); }
```

### Setup code (once, before canvas creation)

```c
typedef struct {
    DvzDevice*   device;
    DvzAllocator* alloc;
    DvzGraphics* pipeline;
    DvzBuffer*   vbuf;
} TriState;

static void tri_state_create(TriState* s, DvzDevice* device, DvzAllocator* alloc)
{
    s->device = device;
    s->alloc  = alloc;

    /* Compile shaders */
    DvzSize vs_sz, fs_sz;
    uint32_t* vs_spv = dvz_compile_glsl(VK_SHADER_STAGE_VERTEX_BIT,   VERT_GLSL, &vs_sz);
    uint32_t* fs_spv = dvz_compile_glsl(VK_SHADER_STAGE_FRAGMENT_BIT, FRAG_GLSL, &fs_sz);

    DvzShader vs = {0}, fs = {0};
    dvz_shader(device, vs_sz, vs_spv, &vs);
    dvz_shader(device, fs_sz, fs_spv, &fs);
    dvz_free(vs_spv);
    dvz_free(fs_spv);

    /* Pipeline */
    s->pipeline = dvz_graphics_create_wrapper();
    dvz_graphics(device, s->pipeline);
    dvz_graphics_shader(s->pipeline, VK_SHADER_STAGE_VERTEX_BIT,   dvz_shader_handle(&vs));
    dvz_graphics_shader(s->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fs));
    /* vertex binding: stride = sizeof(Vertex), per-vertex */
    dvz_graphics_vertex_binding(s->pipeline, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    /* attributes: location 0 = vec2 pos, location 1 = vec3 color */
    dvz_graphics_vertex_attr(s->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex,x));
    dvz_graphics_vertex_attr(s->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,r));
    /* color attachment format must match the canvas image format (typically VK_FORMAT_B8G8R8A8_UNORM) */
    dvz_graphics_attachment_color(s->pipeline, 0, canvas_color_format);
    dvz_graphics_create(s->pipeline);

    dvz_shader_destroy(&vs);
    dvz_shader_destroy(&fs);

    /* Vertex buffer */
    s->vbuf = dvz_buffer_create_wrapper();
    dvz_buffer(device, alloc, s->vbuf);
    dvz_buffer_config(s->vbuf, sizeof(TRIANGLE),
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_create(s->vbuf);
    dvz_buffer_upload(s->vbuf, 0, sizeof(TRIANGLE), TRIANGLE);
}
```

**Key question to resolve during implementation:** what `VkFormat` does the canvas expose for
its color image?  Check `frame->color_format` (already a field of `DvzStreamFrame`) or look
at how `canvas_offscreen_prepare_frame` creates the image.  Use that format for
`dvz_graphics_attachment_color`.

### Draw callback (identical for all three backends)

```c
static void draw_triangle(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    TriState* s = user_data;
    VkCommandBuffer cmd = frame->command_buffer;
    if (cmd == VK_NULL_HANDLE) return;

    DvzCommands* cmds = dvz_commands_create_wrapper();
    dvz_commands_wrap(s->device, cmd, cmds);

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    dvz_cmd_rendering_default(cmds, frame->image_view,
                              frame->extent.width, frame->extent.height,
                              (VkClearValue){.color.float32={0.05f,0.05f,0.08f,1.0f}},
                              rendering);
    dvz_cmd_rendering_begin(cmds, rendering);

    dvz_cmd_bind_graphics(cmds, s->pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &dvz_buffer_handle(s->vbuf), &offset);

    /* Viewport and scissor must be set dynamically for dynamic rendering */
    VkViewport vp = {0, 0, (float)frame->extent.width, (float)frame->extent.height, 0, 1};
    VkRect2D   sc = {{0,0}, frame->extent};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    dvz_cmd_draw(cmds, 3, 1, 0, 0);

    dvz_cmd_rendering_end(cmds);
    dvz_rendering_free(rendering);
    dvz_commands_free(cmds);
}
```

### Backend-specific main() structure

```c
int main(int argc, char** argv)
{
    const char* backend_name = (argc > 1) ? argv[1] : "offscreen";

    /* GPU context — needs dynamicRendering + synchronization2 for canvas */
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    /* ... add VkPhysicalDeviceVulkan13Features as in dvz_app() ... */
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);

    DvzWindowHost* host = dvz_window_host();
    DvzWindowConfig wcfg = dvz_window_config();
    wcfg.width = 800; wcfg.height = 600;

    DvzCanvasConfig ccfg = dvz_canvas_config();
    ccfg.device = dvz_gpu_ctx_device(ctx);

    if (strcmp(backend_name, "glfw") == 0) {
        DvzWindow* win = dvz_window_create(host, DVZ_BACKEND_GLFW, &wcfg);
        ccfg.window      = win;
        ccfg.render_mode = DVZ_CANVAS_RENDER_MODE_ONSCREEN;
    } else {
        DvzWindow* win = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &wcfg);
        ccfg.window      = win;
        ccfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    }

    DvzCanvas* canvas = dvz_canvas_create(&ccfg);

    /* Setup pipeline + vertex buffer — need canvas color format */
    TriState state = {0};
    tri_state_create(&state, dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));

    if (strcmp(backend_name, "video") == 0) {
        DvzVideoSinkConfig vcfg = {0}; /* use defaults */
        vcfg.path = "raw_triangle.mp4";
        dvz_canvas_configure_video_sink(canvas, true, &vcfg);
    }

    dvz_canvas_set_draw_callback(canvas, draw_triangle, &state);

    /* Run */
    uint32_t n_frames = strcmp(backend_name, "glfw") == 0 ? 0 : 60;
    for (uint32_t i = 0; i < (n_frames == 0 ? UINT32_MAX : n_frames); i++) {
        int rc = dvz_canvas_frame(canvas);
        if (rc != DVZ_CANVAS_FRAME_READY) break;
        dvz_canvas_submit(canvas);
        if (strcmp(backend_name, "glfw") == 0 && user_pressed_escape) break;
    }

    if (strcmp(backend_name, "offscreen") == 0)
        dvz_canvas_capture_png(canvas, "raw_triangle.png");

    /* Cleanup: tri_state_destroy, dvz_canvas_destroy, dvz_window_destroy,
       dvz_window_host_destroy, dvz_gpu_ctx_destroy */
}
```

For the GLFW interactive loop, register a keyboard callback via `dvz_canvas_input` /
`dvz_input_router_*` as shown in `test_canvas_glfw.c` (`canvas_glfw_keyboard_callback`).

### `dvz_buffer_handle` helper

Check whether `dvz_buffer_handle(buf) → VkBuffer` is already public.  If not, expose it from
`include/datoviz/vklite/buffers.h` alongside the existing `dvz_buffer_create_wrapper`.

### CMakeLists addition

In `examples/c/CMakeLists.txt`, add:
```cmake
dvz_add_example(raw_triangle)
```

---

## Part 2 — `raw_triangle_drp2.c` (DRP2 stream commands, no canvas)

### Goal

Show how the DRP2 protocol works at the command-stream level.  Target audience: developers of
other scientific visualization libraries who want to understand or implement DRP2 consumers,
and power users who want to experiment with the protocol directly.

This example bypasses both DvzScene and DvzCanvas.  It constructs a `DvzDrp2CommandStream`
manually, executes it with `dvz_drp2_runtime_vklite`, then reads back pixels and saves a PNG.

### New API needed: `dvz_drp2_runtime_download_buffer`

The runtime already has an internal `_dvz_drp2_runtime_vklite_download_buffer` (used in
tests).  It must be promoted to a public export:

```c
/**
 * Download bytes from a DRP2 buffer into CPU memory.
 *
 * Must be called after dvz_drp2_runtime_execute() has completed.
 *
 * @param runtime    the vklite runtime
 * @param buffer_id  the DRP2 buffer id used in the stream (must have COPY_DST usage)
 * @param offset     byte offset within the buffer
 * @param size       number of bytes to read
 * @param dst        destination CPU buffer (caller-allocated, at least `size` bytes)
 * @returns true on success
 */
DVZ_EXPORT bool dvz_drp2_runtime_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id,
    DvzSize offset, DvzSize size, void* dst);
```

Add the declaration to `include/datoviz/drp2/runtime.h` and rename/export the existing
implementation in `src/drp2/runtime.c`.

### DRP2 stream sequence

ID constants (arbitrary u64, must be unique within one stream):

```c
#define ID_VBUF      1
#define ID_READBUF   2
#define ID_VS        3
#define ID_FS        4
#define ID_PIPELINE  5
#define ID_COLOR_TEX 6
#define ID_ENCODER   7
#define ID_RPASS     8
#define ID_SUBMIT    9

#define WIDTH  256
#define HEIGHT 256
```

Vertex data: same `Vertex` struct and `TRIANGLE` array as Part 1.

GLSL shaders: same strings as Part 1.

```c
DvzDrp2CommandStream* stream = dvz_drp2_stream();

/* Handshake — required by the protocol */
dvz_drp2_stream_hello_renderer(stream, "raw_triangle_drp2");
dvz_drp2_stream_renderer_hello_reply(stream, "vklite");

/* Resources */
dvz_drp2_stream_create_buffer(stream, ID_VBUF,
    sizeof(TRIANGLE), DVZ_DRP2_BUFFER_USAGE_VERTEX);
dvz_drp2_stream_write_buffer(stream, ID_VBUF, 0, sizeof(TRIANGLE),
    base64_encode(TRIANGLE, sizeof(TRIANGLE)));  /* see note below */

dvz_drp2_stream_create_buffer(stream, ID_READBUF,
    (DvzSize)WIDTH * HEIGHT * 4,
    DVZ_DRP2_BUFFER_USAGE_COPY_DST);

dvz_drp2_stream_create_shader_module_format(stream, ID_VS, "vertex",   "glsl", VERT_GLSL);
dvz_drp2_stream_create_shader_module_format(stream, ID_FS, "fragment", "glsl", FRAG_GLSL);

dvz_drp2_stream_create_render_pipeline(stream, ID_PIPELINE, ID_VS, ID_FS, /*n_color_att=*/1);

dvz_drp2_stream_create_texture_2d(stream, ID_COLOR_TEX, WIDTH, HEIGHT);

/* Command encoding */
dvz_drp2_stream_begin_command_encoder(stream, ID_ENCODER);

dvz_drp2_stream_begin_render_pass(stream, ID_RPASS, ID_ENCODER, ID_COLOR_TEX);
dvz_drp2_stream_set_pipeline(stream, ID_RPASS, ID_PIPELINE);
dvz_drp2_stream_set_vertex_buffer(stream, ID_RPASS, /*slot=*/0, ID_VBUF, /*offset=*/0);
dvz_drp2_stream_draw(stream, ID_RPASS, /*vertex_count=*/3, /*instance_count=*/1,
                     /*first_vertex=*/0, /*first_instance=*/0);
dvz_drp2_stream_end_render_pass(stream, ID_RPASS);

/* Copy rendered texture to CPU-readable buffer */
dvz_drp2_stream_copy_texture_to_buffer(stream,
    ID_COLOR_TEX, /*tex_offset_x=*/0, /*tex_offset_y=*/0,
    WIDTH, HEIGHT,
    ID_READBUF, /*buf_offset=*/0);

dvz_drp2_stream_finish_command_encoder(stream, ID_ENCODER, ID_SUBMIT);
dvz_drp2_stream_queue_submit(stream, ID_SUBMIT, /*fence_id=*/0);
```

**Note on `write_buffer` / base64:** `dvz_drp2_stream_write_buffer` takes a base64-encoded
string (the DRP2 protocol is text-oriented).  Use `dvz_base64_encode` from
`include/datoviz/common/` (or add a thin wrapper if it is not yet public).  Alternatively
check whether an overload accepting raw bytes exists; if not, add one as a convenience:

```c
DVZ_EXPORT bool dvz_drp2_stream_write_buffer_bytes(
    DvzDrp2CommandStream* stream, uint64_t buffer_id,
    DvzSize offset, DvzSize size, const void* data);
```

### Execution and PNG save

```c
DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
/* Add Vulkan 1.3 features: dynamicRendering + synchronization2 (same as dvz_app()) */
DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);

DvzDrp2RuntimeConfig rt_cfg =
    dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&rt_cfg);

DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
if (!result.ok) { /* log error, cleanup, return 1 */ }

/* Read pixels back */
uint8_t* pixels = dvz_calloc((size_t)WIDTH * HEIGHT * 4, 1);
dvz_drp2_runtime_download_buffer(runtime, ID_READBUF, 0, WIDTH * HEIGHT * 4, pixels);

dvz_write_png("raw_triangle_drp2.png", WIDTH, HEIGHT, pixels);
dvz_free(pixels);

dvz_drp2_runtime_destroy(runtime);
dvz_drp2_stream_destroy(stream);
dvz_gpu_ctx_destroy(ctx);
```

### Vertex attribute layout in DRP2

The render pipeline created by `dvz_drp2_stream_create_render_pipeline` does not take an
explicit vertex layout — it infers it from the shader.  **The GLSL vertex shader must declare
the attribute locations explicitly** (as shown in Part 1).  The DRP2 runtime/vklite backend
reads the SPIR-V reflection data to set up the `VkPipelineVertexInputStateCreateInfo`.

Verify this is the case by reading `src/drp2/runtime.c` around the pipeline creation path
before assuming it works.  If reflection-based layout inference is not yet implemented, either:
a) add it, or b) add a `dvz_drp2_stream_create_render_pipeline_with_vertex_layout` variant
(similar to the existing `_with_bind_group_layout` variant).

### CMakeLists addition

```cmake
dvz_add_example(raw_triangle_drp2)
```

---

## Summary of new public API needed

| Symbol | File | Notes |
|--------|------|-------|
| `dvz_compile_glsl` | `include/datoviz/vk/shader_util.h` (new) or `gpu_ctx.h` | lazy-loads shaderc, returns heap SPIR-V |
| `dvz_buffer_handle` | `include/datoviz/vklite/buffers.h` | may already exist privately |
| `dvz_drp2_runtime_download_buffer` | `include/datoviz/drp2/runtime.h` | promote existing internal `_dvz_drp2_runtime_vklite_download_buffer` |
| `dvz_drp2_stream_write_buffer_bytes` | `include/datoviz/drp2/stream.h` | optional convenience over base64 |

---

## Existing code to read before starting

- `src/canvas/tests/test_canvas_glfw.c` — `canvas_glfw_clear_draw` shows the minimal draw
  callback pattern with `dvz_commands_wrap` + `dvz_cmd_rendering_*`
- `src/drp2/runtime.c` — shaderc lazy-load code (around line 2183) and
  `_dvz_drp2_runtime_vklite_download_buffer`
- `src/scene/tests/test_scene.c` — `test_frame_plan_emit_drp2_static_render_glsl_executes`
  and `test_frame_plan_emit_drp2_readback_glsl_executes` show the full GPU setup + execute +
  download pattern
- `src/app/app.c` — `dvz_app()` for the exact Vulkan 1.2/1.3 feature request pattern needed
  by both examples
- `src/drp2/tests/test_drp2.c` — exhaustive DRP2 stream construction examples

## Just targets to add

```
# in justfile, alongside the existing `example-c` target:
[linux]
example-c name: build
    ./build/examples/c/{{name}}
```

Already present — no change needed.
