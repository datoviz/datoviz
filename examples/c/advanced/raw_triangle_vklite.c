/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* raw_triangle_vklite - vklite draw commands into DvzCanvas.
 *
 * Scenario: advanced_raw_triangle_vklite
 * Style: advanced, native-only, low-level vklite/canvas, 1920x1080 output target
 *
 * Shows how to write your own Vulkan draw commands using vklite helpers while
 * letting DvzCanvas manage all presentation plumbing (offscreen images, frame
 * timing, submission, video recording). The draw callback is identical for
 * every backend; only the canvas configuration differs.
 *
 * Usage:
 *   ./raw_triangle_vklite [--png|--video]
 *
 *   --png    render one frame, save raw_triangle_vklite.png (default)
 *   --video  render 120 frames, save raw_triangle_vklite.mp4
 *
 * NOTE: GLFW onscreen rendering requires Vulkan surface extensions to be
 * requested at instance creation time, which DvzGpuCtx does not expose today.
 * It will be added in a follow-up commit using the raw dvz_instance_create path.
 *
 * Build:  just example-c advanced/raw_triangle_vklite
 * Run:    ./build/examples/c/advanced/raw_triangle_vklite [--png|--video]
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/canvas.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/video.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite.h"
#include "datoviz/window.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_OUTPUT_WIDTH
#define HEIGHT EXAMPLE_OUTPUT_HEIGHT



/*************************************************************************************************/
/*  Vertex layout                                                                                */
/*************************************************************************************************/

typedef struct
{
    float x, y;
    float r, g, b;
} Vertex;

static const Vertex TRIANGLE[3] = {
    { 0.0f,  0.6f,  1.0f, 0.2f, 0.2f},
    {-0.6f, -0.6f,  0.2f, 1.0f, 0.2f},
    { 0.6f, -0.6f,  0.2f, 0.2f, 1.0f},
};



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* VERT_GLSL =
    "#version 450\n"
    "layout(location = 0) in vec2 inPos;\n"
    "layout(location = 1) in vec3 inColor;\n"
    "layout(location = 0) out vec3 fragColor;\n"
    "void main() {\n"
    "    gl_Position = vec4(inPos, 0.0, 1.0);\n"
    "    fragColor = inColor;\n"
    "}\n";

static const char* FRAG_GLSL =
    "#version 450\n"
    "layout(location = 0) in vec3 fragColor;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "void main() { outColor = vec4(fragColor, 1.0); }\n";



/*************************************************************************************************/
/*  Triangle state                                                                               */
/*************************************************************************************************/

typedef struct
{
    DvzDevice*   device;
    DvzVma*      alloc;
    DvzSlots*    slots;
    DvzGraphics* pipeline;
    DvzBuffer*   vbuf;
} TriState;


static int tri_state_create(TriState* s, DvzDevice* device, DvzVma* alloc)
{
    s->device = device;
    s->alloc  = alloc;

    /* Compile shaders */
    uint64_t vs_sz = 0, fs_sz = 0;
    uint32_t* vs_spv = dvz_compile_glsl("vertex",   VERT_GLSL, &vs_sz);
    uint32_t* fs_spv = dvz_compile_glsl("fragment", FRAG_GLSL, &fs_sz);
    if (!vs_spv || !fs_spv) {
        dvz_fprintf(stderr, "GLSL compilation failed\n");
        return -1;
    }

    DvzShader* vs = dvz_shader_create_wrapper();
    DvzShader* fs = dvz_shader_create_wrapper();
    dvz_shader(device, vs_sz, vs_spv, vs);
    dvz_shader(device, fs_sz, fs_spv, fs);
    free(vs_spv);
    free(fs_spv);

    /* Empty pipeline layout (no descriptors, no push constants) */
    s->slots = dvz_slots_create_wrapper();
    dvz_slots(device, s->slots);
    dvz_slots_create(s->slots);

    /* Graphics pipeline */
    s->pipeline = dvz_graphics_create_wrapper();
    dvz_graphics(device, s->pipeline);

    dvz_graphics_shader(s->pipeline, VK_SHADER_STAGE_VERTEX_BIT,   dvz_shader_handle(vs));
    dvz_graphics_shader(s->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(fs));

    dvz_graphics_vertex_binding(s->pipeline, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(s->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT,
                             offsetof(Vertex, x));
    dvz_graphics_vertex_attr(s->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT,
                             offsetof(Vertex, r));

    dvz_graphics_primitive(s->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    dvz_graphics_attachment_color(s->pipeline, 0, DVZ_DEFAULT_COLOR_FORMAT);
    dvz_graphics_layout(s->pipeline, dvz_slots_handle(s->slots));
    dvz_graphics_viewport(s->pipeline, 0, 0, 0, 0, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(s->pipeline, 0, 0, 0, 0, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    int rc = dvz_graphics_create(s->pipeline);
    if (rc != 0) {
        dvz_fprintf(stderr, "Pipeline creation failed\n");
        return -1;
    }

    dvz_shader_destroy(vs);
    dvz_shader_free(vs);
    dvz_shader_destroy(fs);
    dvz_shader_free(fs);

    /* Vertex buffer */
    s->vbuf = dvz_buffer_create_wrapper();
    dvz_buffer(device, alloc, s->vbuf);
    dvz_buffer_size(s->vbuf, sizeof(TRIANGLE));
    dvz_buffer_usage(s->vbuf, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(s->vbuf, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    if (dvz_buffer_create(s->vbuf) != 0) {
        dvz_fprintf(stderr, "Vertex buffer creation failed\n");
        return -1;
    }
    dvz_buffer_upload(s->vbuf, 0, sizeof(TRIANGLE), TRIANGLE);

    return 0;
}


static void tri_state_destroy(TriState* s)
{
    dvz_buffer_destroy(s->vbuf);
    dvz_buffer_free(s->vbuf);
    dvz_graphics_destroy(s->pipeline);
    dvz_graphics_free(s->pipeline);
    dvz_slots_destroy(s->slots);
    dvz_slots_free(s->slots);
}



/*************************************************************************************************/
/*  Draw callback: same for every backend                                                        */
/*************************************************************************************************/

static void draw_triangle(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    TriState* s = (TriState*)user_data;

    VkCommandBuffer cmd = frame->command_buffer;
    if (cmd == VK_NULL_HANDLE)
        return;

    DvzCommands* cmds = dvz_commands_create_wrapper();
    dvz_commands_wrap(s->device, cmd, cmds);

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    dvz_cmd_rendering_default(cmds, frame->image_view,
                              frame->extent.width, frame->extent.height,
                              (VkClearValue){.color.float32 = {0.05f, 0.05f, 0.08f, 1.0f}},
                              rendering);
    dvz_cmd_rendering_begin(cmds, rendering);

    dvz_cmd_bind_graphics(cmds, s->pipeline);

    /* Viewport and scissor must be set dynamically for dynamic rendering */
    VkViewport vp = {
        .x = 0, .y = 0,
        .width  = (float)frame->extent.width,
        .height = (float)frame->extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f,
    };
    VkRect2D sc = {{0, 0}, frame->extent};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    DvzSize offset = 0;
    dvz_cmd_bind_vertex_buffers(cmds, 0, 1, s->vbuf, &offset);
    dvz_cmd_draw(cmds, 0, 3, 0, 1);

    dvz_cmd_rendering_end(cmds);
    dvz_rendering_free(rendering);
    dvz_commands_free(cmds);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    const bool video_mode =
        example_arg_has(argc, argv, "--video") || example_arg_has(argc, argv, "video");
    bool video_enabled = false;

    /* GPU context: dynamicRendering + synchronization2 required by DvzCanvas. */
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features feat12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = VK_TRUE,
    };
    dvz_gpu_ctx_config_features12(&gpu_cfg, &feat12);
    VkPhysicalDeviceVulkan13Features feat13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
        .synchronization2 = VK_TRUE,
    };
    dvz_gpu_ctx_config_features13(&gpu_cfg, &feat13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (!ctx) { dvz_fprintf(stderr, "GPU context creation failed\n"); return 1; }

    /* Window + canvas (offscreen for both modes) */
    DvzWindowHost* host = dvz_window_host();
    DvzWindowConfig wcfg = dvz_window_config();
    wcfg.width  = WIDTH;
    wcfg.height = HEIGHT;
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &wcfg);
    if (!window) { dvz_fprintf(stderr, "Window creation failed\n"); dvz_gpu_ctx_destroy(ctx); return 1; }

    DvzCanvasConfig ccfg = dvz_canvas_config();
    ccfg.window      = window;
    ccfg.device      = dvz_gpu_ctx_device(ctx);
    ccfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (!canvas) { dvz_fprintf(stderr, "Canvas creation failed\n"); dvz_window_destroy(window); dvz_window_host_destroy(host); dvz_gpu_ctx_destroy(ctx); return 1; }

    /* Video sink */
    if (video_mode) {
        DvzVideoSinkConfig vsink = dvz_video_sink_config();
        vsink.encoder.backend  = "auto";
        vsink.encoder.width    = wcfg.width;
        vsink.encoder.height   = wcfg.height;
        vsink.encoder.fps      = 30;
        char mp4_out[512], h26x_out[512];
        example_outpath(argv[0], "raw_triangle_vklite.mp4", mp4_out, sizeof(mp4_out));
        example_outpath(argv[0], "raw_triangle_vklite.h26x", h26x_out, sizeof(h26x_out));
        vsink.encoder.mp4_path = mp4_out;
        vsink.encoder.raw_path = h26x_out;
        if (dvz_canvas_configure_video_sink(canvas, true, &vsink) != 0)
            dvz_fprintf(stderr, "Warning: video sink could not be enabled\n");
        else
            video_enabled = true;
    }

    /* Triangle state: compile shaders, build pipeline, upload vertices */
    TriState state = {0};
    if (tri_state_create(&state, dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx)) != 0) {
        dvz_canvas_destroy(canvas);
        dvz_window_destroy(window);
        dvz_window_host_destroy(host);
        dvz_gpu_ctx_destroy(ctx);
        return 1;
    }

    dvz_canvas_set_draw_callback(canvas, draw_triangle, &state);

    /* Frame loop */
    uint32_t n_frames = video_mode ? 120 : 1;
    for (uint32_t i = 0; i < n_frames; i++) {
        dvz_window_host_poll(host);
        int rc = dvz_canvas_frame(canvas);
        if (rc != DVZ_CANVAS_FRAME_READY) break;
        dvz_canvas_submit(canvas);
    }

    /* Save PNG for offscreen mode */
    if (!video_mode) {
        char png_out[512];
        example_outpath(argv[0], "raw_triangle_vklite.png", png_out, sizeof(png_out));
        dvz_canvas_capture_png(canvas, png_out);
        dvz_fprintf(stdout, "raw_triangle_vklite: saved %s\n", png_out);
    } else if (video_enabled) {
        char mp4_msg[512];
        example_outpath(argv[0], "raw_triangle_vklite.mp4", mp4_msg, sizeof(mp4_msg));
        dvz_fprintf(stdout, "raw_triangle_vklite: saved %s\n", mp4_msg);
    }

    /* Cleanup */
    tri_state_destroy(&state);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
