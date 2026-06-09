/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* raw_triangle_drp2 — triangle rendered via a hand-written DRP2 command stream.
 *
 * Scenario: advanced_raw_triangle_drp2
 * Style: advanced, native-only, low-level DRP2/vklite
 *
 * DRP2 (Datoviz Rendering Protocol 2) is the backend-agnostic IR that sits
 * between the scene layer and the GPU.  This example bypasses both DvzScene
 * and DvzCanvas, constructs a DRP2 command stream from scratch, executes it
 * with the vklite runtime, downloads the pixels, and saves a PNG.
 *
 * Vertex layout is declared explicitly via dvz_drp2_stream_create_render_pipeline_ex().
 *
 * It is intentionally verbose — the goal is to show every step of the
 * protocol so that developers of other scientific-visualization libraries can
 * understand how DRP2 works and experiment with it directly.
 *
 * Build:  just example-c advanced/raw_triangle_drp2
 * Run:    ./build/examples/c/advanced/raw_triangle_drp2
 *         → raw_triangle_drp2.png
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/drp2.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/vk/gpu_ctx.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants — stable DRP2 object IDs (arbitrary u64, unique within the stream)                */
/*************************************************************************************************/

#define ID_VBUF     1   /* vertex buffer                         */
#define ID_RBUF     2   /* readback buffer (COPY_DST)            */
#define ID_VS       3   /* vertex shader module                  */
#define ID_FS       4   /* fragment shader module                */
#define ID_PIPE     5   /* render pipeline                       */
#define ID_TEX      6   /* color target texture (RENDER_ATTACHMENT | COPY_SRC) */
#define ID_ENC      7   /* command encoder                       */
#define ID_RPASS    8   /* render pass inside the encoder        */
#define ID_CMDBUF   9   /* command buffer produced by encoder    */
#define ID_SUBMIT   10  /* submission handle                     */

#define WIDTH  512u
#define HEIGHT 512u



/*************************************************************************************************/
/*  Vertex data                                                                                  */
/*************************************************************************************************/

typedef struct { float x, y, r, g, b; } Vertex;

static const Vertex TRIANGLE[3] = {
    { 0.0f,  0.6f,  1.0f, 0.2f, 0.2f},
    {-0.6f, -0.6f,  0.2f, 1.0f, 0.2f},
    { 0.6f, -0.6f,  0.2f, 0.2f, 1.0f},
};



/*************************************************************************************************/
/*  Shaders — same GLSL as raw_triangle_vklite.c                                                */
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
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    (void)argc;
    /* ------------------------------------------------------------------------------------------
     * 1. GPU context (dynamicRendering + synchronization2 required by the vklite runtime).
     * ------------------------------------------------------------------------------------------ */
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
    if (!ctx) { fprintf(stderr, "GPU context creation failed\n"); return 1; }


    /* ------------------------------------------------------------------------------------------
     * 2. DRP2 runtime (vklite backend).
     * ------------------------------------------------------------------------------------------ */
    DvzDrp2RuntimeConfig rt_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&rt_cfg);
    if (!runtime)
    {
        fprintf(stderr, "DRP2 runtime creation failed\n");
        dvz_gpu_ctx_destroy(ctx);
        return 1;
    }


    /* ------------------------------------------------------------------------------------------
     * 3. Build the DRP2 command stream.
     *
     *    Each dvz_drp2_stream_*() call appends one protocol command.  The stream is fully
     *    declarative — nothing hits the GPU until dvz_drp2_runtime_execute().
     * ------------------------------------------------------------------------------------------ */
    DvzDrp2CommandStream* stream = dvz_drp2_stream();

    /* Handshake — required by the protocol before any resource command. */
    dvz_drp2_stream_hello_renderer(stream, "raw_triangle_drp2");
    dvz_drp2_stream_renderer_hello_reply(stream, "vklite");

    /* --- Resources -------------------------------------------------------- */

    /* Vertex buffer: size = 3 × sizeof(Vertex).
       COPY_DST is required to receive the write_buffer upload. */
    dvz_drp2_stream_create_buffer(
        stream, ID_VBUF, sizeof(TRIANGLE),
        DVZ_DRP2_BUFFER_USAGE_VERTEX | DVZ_DRP2_BUFFER_USAGE_COPY_DST);

    /* Upload vertex data (base64-encoded internally by the _bytes variant). */
    dvz_drp2_stream_write_buffer_bytes(
        stream, ID_VBUF, 0, sizeof(TRIANGLE), TRIANGLE);

    /* Readback buffer: WIDTH × HEIGHT × 4 bytes RGBA, usage = COPY_DST.
       The rendered texture will be copied here before CPU download. */
    dvz_drp2_stream_create_buffer(
        stream, ID_RBUF,
        (uint64_t)WIDTH * HEIGHT * 4,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST);

    /* Compile GLSL shaders to SPIR-V at stream-execution time (format = "glsl"). */
    dvz_drp2_stream_create_shader_module_format(stream, ID_VS, "vertex",   "glsl", VERT_GLSL);
    dvz_drp2_stream_create_shader_module_format(stream, ID_FS, "fragment", "glsl", FRAG_GLSL);

    /* Render pipeline: explicit vertex layout — binding 0 stride=20 (vec2+vec3), TRIANGLE_LIST. */
    {
        uint32_t strides[1]   = {5 * sizeof(float)};           /* binding 0: Vertex = 5 floats */
        uint32_t bindings[2]  = {0, 0};                        /* both attrs from binding 0    */
        uint32_t locations[2] = {0, 1};
        uint32_t formats[2]   = {VK_FORMAT_R32G32_SFLOAT,      /* inPos:   vec2                */
                                  VK_FORMAT_R32G32B32_SFLOAT};  /* inColor: vec3                */
        uint32_t offsets[2]   = {0, 2 * sizeof(float)};
        dvz_drp2_stream_create_render_pipeline_ex(
            stream, ID_PIPE, ID_VS, ID_FS, /*slots=*/1,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            /*bindings=*/1, strides,
            /*attrs=*/2, bindings, locations, formats, offsets);
    }

    /* Color target texture: RENDER_ATTACHMENT so it can be drawn into, COPY_SRC so it can
       be copied to the readback buffer afterwards. */
    dvz_drp2_stream_create_texture_2d_usage(
        stream, ID_TEX, WIDTH, HEIGHT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC);

    /* --- Command encoding ------------------------------------------------- */

    /* Open an encoder.  Everything between begin and finish is recorded into
       the command buffer (ID_CMDBUF) produced by finish_command_encoder(). */
    dvz_drp2_stream_begin_command_encoder(stream, ID_ENC);

    /* Render pass: draw into ID_TEX (clear to dark blue-grey). */
    dvz_drp2_stream_begin_render_pass(stream, ID_RPASS, ID_ENC, ID_TEX);
    dvz_drp2_stream_set_pipeline(stream, ID_RPASS, ID_PIPE);
    dvz_drp2_stream_set_vertex_buffer(stream, ID_RPASS, /*slot=*/0, ID_VBUF, /*offset=*/0);
    dvz_drp2_stream_draw(stream, ID_RPASS,
                         /*vertex_count=*/3, /*instance_count=*/1,
                         /*first_vertex=*/0, /*first_instance=*/0);
    dvz_drp2_stream_end_render_pass(stream, ID_RPASS);

    /* Copy the rendered texture into the readback buffer. */
    dvz_drp2_stream_copy_texture_to_buffer(
        stream, ID_ENC, ID_TEX, ID_RBUF,
        /*dst_offset=*/0, WIDTH, HEIGHT,
        /*bytes_per_row=*/WIDTH * 4, /*rows_per_image=*/HEIGHT);

    /* Close the encoder → produces command buffer ID_CMDBUF. */
    dvz_drp2_stream_finish_command_encoder(stream, ID_ENC, ID_CMDBUF);

    /* Submit the command buffer to the GPU. */
    dvz_drp2_stream_queue_submit(stream, ID_CMDBUF, ID_SUBMIT);


    /* ------------------------------------------------------------------------------------------
     * 4. Execute the stream against the vklite runtime.
     * ------------------------------------------------------------------------------------------ */
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    if (!result.ok)
    {
        fprintf(stderr, "DRP2 execution failed (command %u, code %d)\n",
                result.command_index, result.code);
        dvz_drp2_stream_destroy(stream);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        return 1;
    }


    /* ------------------------------------------------------------------------------------------
     * 5. Download pixels from the GPU readback buffer and save a PNG.
     * ------------------------------------------------------------------------------------------ */
    uint64_t pixel_count = (uint64_t)WIDTH * HEIGHT * 4;
    uint8_t* pixels = (uint8_t*)calloc((size_t)pixel_count, 1);
    if (!pixels)
    {
        fprintf(stderr, "Out of memory\n");
        dvz_drp2_stream_destroy(stream);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        return 1;
    }

    dvz_drp2_runtime_download_buffer(runtime, ID_RBUF, 0, pixel_count, pixels);
    char out[512];
    example_outpath(argv[0], "raw_triangle_drp2.png", out, sizeof(out));
    dvz_write_png(out, WIDTH, HEIGHT, pixels);
    free(pixels);

    printf("raw_triangle_drp2: saved %s\n", out);


    /* ------------------------------------------------------------------------------------------
     * 6. Cleanup.
     * ------------------------------------------------------------------------------------------ */
    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
