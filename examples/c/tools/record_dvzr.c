/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* record_dvzr — capture an offscreen app scene and replay its DVZR recording.
 *
 * Build:  just build
 * Run:    ./build/examples/c/tools/record_dvzr
 *
 * Outputs next to the executable:
 *   record_dvzr.dvzr/
 *   record_dvzr_original.png
 *   record_dvzr_replay.png
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/canvas/enums.h"
#include "datoviz/drp2.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  512u
#define HEIGHT 384u

// Current app recordings reserve this target id for the first borrowed frame target.
#define APP_RECORD_TARGET_ID UINT64_C(0xF000000000000000)
#define REPLAY_READBACK_ID   UINT64_C(0xE000000000000001)
#define REPLAY_ENCODER_ID    UINT64_C(0xE000000000000002)
#define REPLAY_COMMAND_ID    UINT64_C(0xE000000000000003)
#define REPLAY_SUBMIT_ID     UINT64_C(0xE000000000000004)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build an output path next to the executable.
 *
 * @param exe executable path
 * @param name output file name
 * @param out output buffer
 * @param out_size output buffer size
 */
static void _outpath(const char* exe, const char* name, char* out, size_t out_size)
{
    const char* slash = strrchr(exe, '/');
    if (slash != NULL)
        dvz_snprintf(out, out_size, "%.*s/%s", (int)(slash - exe), exe, name);
    else
        dvz_snprintf(out, out_size, "%s", name);
}



/**
 * Create a GPU context suitable for DRP2 replay.
 *
 * @return GPU context, or NULL on failure
 */
static DvzGpuCtx* _gpu_context(void)
{
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = VK_TRUE,
    };
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
        .synchronization2 = VK_TRUE,
    };
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    return dvz_gpu_ctx(&gpu_cfg);
}



/**
 * Save the replayed recording target as a PNG.
 *
 * @param recording_path input DVZR directory
 * @param png_path output PNG path
 * @return process status code
 */
static int _save_replay_png(const char* recording_path, const char* png_path)
{
    DvzDrp2Recording* recording = dvz_drp2_recording_open(recording_path);
    if (recording == NULL)
    {
        dvz_fprintf(stderr, "failed to open recording %s\n", recording_path);
        return 1;
    }

    DvzGpuCtx* ctx = _gpu_context();
    if (ctx == NULL)
    {
        dvz_fprintf(stderr, "GPU context creation failed for replay\n");
        dvz_drp2_recording_close(recording);
        return 1;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    if (runtime == NULL)
    {
        dvz_fprintf(stderr, "DRP2 runtime creation failed for replay\n");
        dvz_gpu_ctx_destroy(ctx);
        dvz_drp2_recording_close(recording);
        return 1;
    }

    DvzDrp2ValidationResult result = dvz_drp2_recording_execute_all(recording, runtime);
    if (!result.ok)
    {
        dvz_fprintf(
            stderr, "recording replay failed at command %u code %d\n", result.command_index,
            (int)result.code);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        dvz_drp2_recording_close(recording);
        return 1;
    }

    const uint64_t byte_count = (uint64_t)WIDTH * HEIGHT * 4;
    DvzDrp2CommandStream* readback = dvz_drp2_stream();
    if (readback == NULL ||
        !dvz_drp2_stream_create_buffer(
            readback, REPLAY_READBACK_ID, byte_count, DVZ_DRP2_BUFFER_USAGE_COPY_DST) ||
        !dvz_drp2_stream_begin_command_encoder(readback, REPLAY_ENCODER_ID) ||
        !dvz_drp2_stream_copy_texture_to_buffer(
            readback, REPLAY_ENCODER_ID, APP_RECORD_TARGET_ID, REPLAY_READBACK_ID, 0, WIDTH,
            HEIGHT, WIDTH * 4, HEIGHT) ||
        !dvz_drp2_stream_finish_command_encoder(
            readback, REPLAY_ENCODER_ID, REPLAY_COMMAND_ID) ||
        !dvz_drp2_stream_queue_submit(readback, REPLAY_COMMAND_ID, REPLAY_SUBMIT_ID))
    {
        dvz_fprintf(stderr, "failed to build replay readback stream\n");
        dvz_drp2_stream_destroy(readback);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        dvz_drp2_recording_close(recording);
        return 1;
    }

    result = dvz_drp2_runtime_execute(runtime, readback);
    if (!result.ok)
    {
        dvz_fprintf(
            stderr, "replay readback failed at command %u code %d\n", result.command_index,
            (int)result.code);
        dvz_drp2_stream_destroy(readback);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        dvz_drp2_recording_close(recording);
        return 1;
    }

    uint8_t* pixels = (uint8_t*)dvz_calloc((size_t)byte_count, 1);
    if (pixels == NULL ||
        !dvz_drp2_runtime_download_buffer(
            runtime, REPLAY_READBACK_ID, 0, byte_count, pixels) ||
        dvz_write_png(png_path, WIDTH, HEIGHT, pixels) != 0)
    {
        dvz_fprintf(stderr, "failed to write replay PNG %s\n", png_path);
        dvz_free(pixels);
        dvz_drp2_stream_destroy(readback);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        dvz_drp2_recording_close(recording);
        return 1;
    }

    dvz_free(pixels);
    dvz_drp2_stream_destroy(readback);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_drp2_recording_close(recording);
    return 0;
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    (void)argc;

    char recording_path[512] = {0};
    char original_png[512] = {0};
    char replay_png[512] = {0};
    _outpath(argv[0], "record_dvzr.dvzr", recording_path, sizeof(recording_path));
    _outpath(argv[0], "record_dvzr_original.png", original_png, sizeof(original_png));
    _outpath(argv[0], "record_dvzr_replay.png", replay_png, sizeof(replay_png));

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    DvzVisual* visual = dvz_point(scene, 0);
    if (figure == NULL || panel == NULL || visual == NULL)
    {
        dvz_fprintf(stderr, "failed to create scene objects\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[5][3] = {
        {-0.7f, -0.5f, 0.0f},
        {-0.2f, 0.45f, 0.0f},
        {0.25f, -0.2f, 0.0f},
        {0.65f, 0.45f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };
    uint8_t colors[5][4] = {
        {255, 64, 64, 255},
        {64, 255, 128, 255},
        {64, 128, 255, 255},
        {255, 220, 64, 255},
        {255, 255, 255, 255},
    };
    float sizes[5] = {36.0f, 42.0f, 48.0f, 40.0f, 28.0f};
    dvz_point_data(visual, positions, colors, sizes, 5);
    dvz_panel_add_visual(panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, WIDTH, HEIGHT);
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    if (dvz_app_window_record_start(win, recording_path) != 0)
    {
        dvz_fprintf(stderr, "failed to start DVZR recording\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_app_window_render_once(win) != DVZ_CANVAS_FRAME_READY ||
        dvz_app_window_record_stop(win) != 0 ||
        dvz_app_window_capture_png(win, original_png) != 0)
    {
        dvz_fprintf(stderr, "failed to render/capture original scene\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);

    if (_save_replay_png(recording_path, replay_png) != 0)
        return 1;

    dvz_fprintf(stdout, "record_dvzr: wrote %s\n", recording_path);
    dvz_fprintf(stdout, "record_dvzr: wrote %s\n", original_png);
    dvz_fprintf(stdout, "record_dvzr: wrote %s\n", replay_png);
    return 0;
}
