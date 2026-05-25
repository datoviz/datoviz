/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text atlas tests                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "../_scene.h"
#include "../_shader_registry.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif




/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_TEXT_ATLAS_GPU_CASE(test)                                                       \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_TEXT_ATLAS_REQUIRE_VKLITE(ctx)                                                  \
    do                                                                                            \
    {                                                                                             \
        if (!_scene_vklite_runtime_available())                                                   \
        {                                                                                         \
            tst_skip((ctx), "Vulkan instance creation failed");                                   \
            return 0;                                                                             \
        }                                                                                         \
    } while (0)




/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the first color target used by a command stream render pass.
 *
 * @param stream the DRP2 command stream
 * @return target texture id, or zero when no render pass exists
 */
static uint64_t _first_render_target_id(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            return command->u.begin_render_pass.texture_id;
    }
    return 0;
}



/**
 * Count green text pixels in a downloaded RGBA buffer.
 *
 * @param pixels downloaded RGBA pixels
 * @param width image width
 * @param height image height
 * @return number of pixels dominated by the green channel
 */
static uint32_t _green_text_pixel_count(const uint8_t* pixels, uint32_t width, uint32_t height)
{
    ANN(pixels);
    uint32_t count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &pixels[4 * i];
        if (pixel[1] > 60 && pixel[1] > pixel[0] + 20 && pixel[1] > pixel[2] + 20)
            count++;
    }
    return count;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
/**
 * Execute a readback copy from one rendered texture target.
 *
 * @param runtime the DRP2 runtime
 * @param target_id rendered texture id
 * @param width target width
 * @param height target height
 * @param out_pixels output RGBA buffer
 * @return true on success
 */
static bool _download_render_target(
    DvzDrp2Runtime* runtime, uint64_t target_id, uint32_t width, uint32_t height,
    uint8_t* out_pixels)
{
    ANN(runtime);
    ANN(out_pixels);
    ASSERT(target_id != 0);

    const uint64_t readback_buffer_id = 9201;
    const uint64_t encoder_id = 9202;
    const uint64_t command_buffer_id = 9203;
    const uint64_t submission_id = 9204;
    const uint64_t byte_size = (uint64_t)width * height * 4;

    DvzDrp2CommandStream* readback = dvz_drp2_stream();
    ANN(readback);
    bool ok = dvz_drp2_stream_create_buffer(
                  readback, readback_buffer_id, byte_size,
                  DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ) &&
              dvz_drp2_stream_begin_command_encoder(readback, encoder_id) &&
              dvz_drp2_stream_copy_texture_to_buffer(
                  readback, encoder_id, target_id, readback_buffer_id, 0, width, height,
                  width * 4, height) &&
              dvz_drp2_stream_finish_command_encoder(readback, encoder_id, command_buffer_id) &&
              dvz_drp2_stream_queue_submit(readback, command_buffer_id, submission_id);

    if (ok)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, readback);
        ok = result.ok && result.code == DVZ_DRP2_VALIDATION_OK;
    }
    if (ok)
        ok = _dvz_drp2_runtime_vklite_download_buffer(
            runtime, readback_buffer_id, 0, byte_size, out_pixels);

    dvz_drp2_stream_destroy(readback);
    return ok;
}
#endif




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Verify MSDF glyph shaders decode RGB distance with alpha used only as artifact guard.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_msdf_shader_uses_rgb_distance(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GLYPH, true);
    const char* wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_GLYPH, true);
    ANN(glsl);
    ANN(wgsl);

    AT(strstr(glsl, "float median3") != NULL);
    AT(strstr(glsl, "float msdf = median3(texel.r, texel.g, texel.b);") != NULL);
    AT(strstr(glsl, "float sdf = texel.a;") != NULL);
    AT(strstr(glsl, "opacity = distanceOpacity(sd);") != NULL);
    AT(strstr(glsl, "opacity = distanceOpacity(texel.a);") != NULL);

    AT(strstr(wgsl, "fn median3") != NULL);
    AT(strstr(wgsl, "let msdf = median3(texel.r, texel.g, texel.b);") != NULL);
    AT(strstr(wgsl, "let sdf = texel.a;") != NULL);
    AT(strstr(wgsl, "opacity = distanceOpacity(input.uv, sd);") != NULL);
    AT(strstr(wgsl, "opacity = distanceOpacity(input.uv, texel.a);") != NULL);

    return 0;
}



/**
 * Verify font-backed UTF-8 text renders through scene DRP2 runtime readback.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_utf8_runtime_readback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    TST_SCENE_TEXT_ATLAS_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_scene_text_atlas_utf8_runtime_readback skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    const uint32_t width = 192;
    const uint32_t height = 72;
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, width, height, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"Atlas UTF-8 caf" "\xC3" "\xA9"};
    vec3 positions[1] = {{8.0f, 16.0f, 0.0f}};
    vec2 text_anchors[1] = {{0.0f, 0.0f}};
    float sizes[1] = {24.0f};
    float angles[1] = {0.0f};
    DvzColor colors[1] = {{0, 255, 0, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    _scene_prepare_text_visuals(figure);
    ANN(text->text.glyph_visual);
    AT(scene->font_count == 1);
    DvzTextAtlasSpec spec = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, sizes[0]);
    DvzTextAtlas* atlas = _scene_text_atlas_get(&scene->fonts[0], &spec);
    ANN(atlas);
    ANN(atlas->field);
    DvzTextAtlasGlyph* utf8_glyph = _scene_text_atlas_glyph(atlas, 0x00E9u);
    ANN(utf8_glyph);
    AT(text->text.glyph_visual->field == atlas->field);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = width;
    emit_cfg.target_height = height;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    if (stream == NULL && dvz_diagnostic_report_count(&report) > 0)
    {
        log_error("%s", dvz_diagnostic_report_get(&report, 0));
    }
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    uint64_t target_id = _first_render_target_id(stream);
    AT(target_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t pixels[192 * 72 * 4] = {0};
    AT(_download_render_target(runtime, target_id, width, height, pixels));
    AT(_green_text_pixel_count(pixels, width, height) > 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}




/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Register scene text atlas tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_text_atlas(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("text-atlas");

    TST_CASE(test_scene_text_msdf_shader_uses_rgb_distance);
    TST_SCENE_TEXT_ATLAS_GPU_CASE(test_scene_text_atlas_utf8_runtime_readback);

    return 0;
}
