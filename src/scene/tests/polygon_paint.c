/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon paint rendering regressions                                                          */
/*************************************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "domain/field_internal.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"

#define PAINT_SIZE 64
#define PAINT_BYTES (PAINT_SIZE * PAINT_SIZE * 4)

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
typedef struct
{
    DvzGpuCtx* ctx;
    DvzDrp2Runtime* runtime;
    uint32_t serial;
} PaintCapture;

bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);

/**
 * Create a GPU context with the rendering features needed by scene streams.
 *
 * @param suite active test context
 * @return owned context, or NULL when unavailable
 */
static DvzGpuCtx* _paint_gpu(TstContext* suite)
{
    if (!_scene_vklite_runtime_available())
        return NULL;
    DvzGpuCtxConfig config = dvz_testing_gpu_ctx_config(suite);
    VkPhysicalDeviceVulkan13Features features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = true,
        .synchronization2 = true,
    };
    dvz_gpu_ctx_config_features13(&config, &features);
    return dvz_gpu_ctx(&config);
}



/**
 * Render a full scene stream and copy its color target into an RGBA buffer.
 *
 * @param capture capture state retaining runtime resources between frames
 * @param figure figure to render
 * @param pixels output buffer of PAINT_BYTES bytes
 * @return whether emission, execution, and readback succeeded
 */
static bool _paint_capture(PaintCapture* capture, DvzFigure* figure, uint8_t* pixels)
{
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig config = dvz_frame_plan_emit_config();
    config.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    config.color_target_format = DVZ_FORMAT_R8G8B8A8_SRGB;
    config.target_width = PAINT_SIZE;
    config.target_height = PAINT_SIZE;
    config.clear_color[0] = config.clear_color[1] = config.clear_color[2] = 0.0f;
    config.clear_color[3] = 1.0f;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &config);
    if (stream == NULL)
    {
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
            log_warn("paint capture emission: %s", dvz_diagnostic_report_get(&report, i));
        return false;
    }
    uint64_t target = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            /* The final pass targets presentation color after linear scene composition. */
            target = command->u.begin_render_pass.texture_id;
        }
    }
    if (capture->runtime == NULL)
    {
        DvzDrp2RuntimeConfig runtime_config = dvz_drp2_runtime_vklite_config(
            dvz_gpu_ctx_device(capture->ctx), dvz_gpu_ctx_alloc(capture->ctx));
        capture->runtime = dvz_drp2_runtime_vklite(&runtime_config);
    }
    DvzDrp2Runtime* runtime = capture->runtime;
    const uint64_t buffer_id = 9201 + 10u * capture->serial;
    const uint64_t encoder_id = buffer_id + 1;
    const uint64_t command_id = buffer_id + 2;
    const uint64_t submission_id = buffer_id + 3;
    capture->serial++;
    bool ok = runtime != NULL && target != 0 && dvz_diagnostic_report_count(&report) == 0;
    if (ok)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
        ok = result.ok;
        if (!ok)
            log_warn("paint capture execution: code=%u command=%u", result.code,
                     result.command_index);
    }
    DvzDrp2CommandStream* readback = dvz_drp2_stream();
    if (ok)
    {
        ok = dvz_drp2_stream_create_buffer(
                 readback, buffer_id, PAINT_BYTES,
                 DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ) &&
             dvz_drp2_stream_begin_command_encoder(readback, encoder_id) &&
             dvz_drp2_stream_copy_texture_to_buffer(
                 readback, encoder_id, target, buffer_id, 0, PAINT_SIZE, PAINT_SIZE, PAINT_SIZE * 4,
                 PAINT_SIZE) &&
             dvz_drp2_stream_finish_command_encoder(readback, encoder_id, command_id) &&
             dvz_drp2_stream_queue_submit(readback, command_id, submission_id);
    }
    if (ok)
        ok = dvz_drp2_runtime_execute(runtime, readback).ok;
    if (ok)
        ok = _dvz_drp2_runtime_vklite_download_buffer(runtime, buffer_id, 0, PAINT_BYTES, pixels);
    ok = ok && dvz_gpu_ctx_error_count(capture->ctx) == 0;
    dvz_drp2_stream_destroy(readback);
    _test_scene_stream_destroy(stream);
    return ok;
}



/**
 * Create an untransformed panel for deterministic paint coverage.
 *
 * @param scene owning scene
 * @param out_figure output borrowed figure
 * @return borrowed panel
 */
static DvzPanel* _paint_panel(DvzScene* scene, DvzFigure** out_figure)
{
    *out_figure = dvz_figure(scene, PAINT_SIZE, PAINT_SIZE, 0);
    if (*out_figure == NULL)
        return NULL;
    return dvz_panel(
        *out_figure, &(DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
}



/**
 * Add one square polygon using authored fill and stroke colors.
 *
 * @param scene owning scene
 * @param panel receiving panel
 * @param xy four borrowed vertices
 * @param fill fill RGBA
 * @param stroke stroke RGBA
 * @return source polygon or NULL on setup failure
 */
static DvzPolygon* _paint_square(
    DvzScene* scene, DvzPanel* panel, const dvec2* xy, const DvzColor fill,
    const DvzColor stroke)
{
    DvzPolygon* polygon = dvz_polygon(scene, 0);
    if (polygon == NULL || dvz_polygon_set_geometry(
            polygon, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                          .outer = {.xy = xy, .count = 4}}) != DVZ_OK ||
        dvz_polygon_set_fill_color(polygon, fill) != DVZ_OK ||
        dvz_polygon_set_stroke_color(polygon, stroke) != DVZ_OK ||
        dvz_polygon_set_stroke_width_px(polygon, 8.0f) != DVZ_OK)
        return NULL;
    DvzComposite* composite = dvz_polygon_composite(polygon, 0);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    attach.controller_mode = DVZ_CONTROLLER_FIXED;
    if (composite == NULL || dvz_panel_add_composite(panel, composite, &attach) != DVZ_OK)
        return NULL;
    return polygon;
}
#endif



/**
 * Verify palette RGB remains exact when scene lights change substantially.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
static int test_polygon_paint_palette_ignores_lights(TstContext* suite, const TstCase* item)
{
    (void)item;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx* ctx = _paint_gpu(suite);
    if (ctx == NULL)
    {
        tst_skip(suite, "Vulkan rendering unavailable");
        return 0;
    }
    PaintCapture capture = {.ctx = ctx};
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = NULL;
    DvzPanel* panel = _paint_panel(scene, &figure);
    AT(panel != NULL);
    const dvec2 square[4] = {{-.75, -.75}, {.75, -.75}, {.75, .75}, {-.75, .75}};
    DvzPolygon* polygon = _paint_square(
        scene, panel, square, (DvzColor){92, 189, 132, 255}, (DvzColor){0, 0, 0, 0});
    AT(polygon != NULL);
    uint8_t first[PAINT_BYTES] = {0};
    uint8_t second[PAINT_BYTES] = {0};
    AT(_paint_capture(&capture, figure, first));
    AT(dvz_light_set_intensity(dvz_scene_default_ambient(scene), 0.0f) == DVZ_OK);
    AT(dvz_light_set_intensity(dvz_scene_default_directional(scene), 8.0f) == DVZ_OK);
    AT(_paint_capture(&capture, figure, second));
    const uint8_t* pixel = _pixel_at(first, PAINT_SIZE, PAINT_SIZE, 32, 32);
    log_info("paint palette RGB=%u,%u,%u expected=92,189,132", pixel[0], pixel[1], pixel[2]);
    AT(abs((int)pixel[0] - 92) <= 1);
    AT(abs((int)pixel[1] - 189) <= 1);
    AT(abs((int)pixel[2] - 132) <= 1);
    AT(memcmp(first, second, PAINT_BYTES) == 0);
    dvz_drp2_runtime_destroy(capture.runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}



/**
 * Verify attached alpha mutations retain source-over composition at every endpoint.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
static int test_polygon_paint_alpha_mutation_linear(TstContext* suite, const TstCase* item)
{
    (void)item;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx* ctx = _paint_gpu(suite);
    if (ctx == NULL)
    {
        tst_skip(suite, "Vulkan rendering unavailable");
        return 0;
    }
    PaintCapture capture = {.ctx = ctx};
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = NULL;
    DvzPanel* panel = _paint_panel(scene, &figure);
    AT(panel != NULL);
    const dvec2 square[4] = {{-.75, -.75}, {.75, -.75}, {.75, .75}, {-.75, .75}};
    DvzPolygon* polygon = _paint_square(
        scene, panel, square, (DvzColor){255, 0, 0, 255}, (DvzColor){0, 0, 0, 0});
    AT(polygon != NULL);
    const uint8_t alphas[4] = {255, 128, 0, 255};
    const uint8_t expected_red[4] = {255, 188, 0, 255};
    uint8_t pixels[PAINT_BYTES] = {0};
    for (uint32_t i = 0; i < 4; i++)
    {
        AT(dvz_polygon_set_fill_color(polygon, (DvzColor){255, 0, 0, alphas[i]}) == DVZ_OK);
        AT(_paint_capture(&capture, figure, pixels));
        const uint8_t* pixel = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 32, 32);
        log_info("paint alpha=%u RGB=%u,%u,%u expected=%u,0,0", alphas[i], pixel[0],
                 pixel[1], pixel[2], expected_red[i]);
        AT(abs((int)pixel[0] - expected_red[i]) <= 1);
        AT(pixel[1] <= 1);
        AT(pixel[2] <= 1);
    }
    dvz_drp2_runtime_destroy(capture.runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}



/**
 * Verify collection item order matches singles while all-boundaries mode is explicit.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
static int test_polygon_paint_collection_matches_singles(TstContext* suite, const TstCase* item)
{
    (void)item;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx* ctx = _paint_gpu(suite);
    if (ctx == NULL)
    {
        tst_skip(suite, "Vulkan rendering unavailable");
        return 0;
    }
    const dvec2 squares[2][4] = {
        {{-.75, -.75}, {.50, -.75}, {.50, .75}, {-.75, .75}},
        {{0, -.50}, {.90, -.50}, {.90, .50}, {0, .50}},
    };
    const DvzColor fills[2] = {{0, 0, 255, 255}, {255, 0, 0, 128}};
    const DvzColor strokes[2] = {{0, 255, 0, 255}, {255, 255, 0, 255}};
    uint8_t singles[PAINT_BYTES] = {0};
    uint8_t items[PAINT_BYTES] = {0};
    uint8_t collection[PAINT_BYTES] = {0};
    PaintCapture capture = {.ctx = ctx};
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = NULL;
    DvzPanel* panel = _paint_panel(scene, &figure);
    AT(panel != NULL);
    for (uint32_t i = 0; i < 2; i++)
    {
        DvzPolygon* polygon = _paint_square(scene, panel, squares[i], fills[i], strokes[i]);
        AT(polygon != NULL);
    }
    AT(_paint_capture(&capture, figure, singles));
    dvz_drp2_runtime_destroy(capture.runtime);
    capture.runtime = NULL;
    dvz_scene_destroy(scene);

    scene = dvz_scene();
    panel = _paint_panel(scene, &figure);
    AT(panel != NULL);
    DvzPolygons* set = dvz_polygons(scene, 0);
    AT(set != NULL);
    for (uint32_t i = 0; i < 2; i++)
    {
        uint32_t index = dvz_polygons_add_region(
            set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                     .outer = {.xy = squares[i], .count = 4}});
        AT(index == i);
        AT(dvz_polygons_set_region_fill_color(set, i, fills[i]) == DVZ_OK);
        AT(dvz_polygons_set_region_stroke_color(set, i, strokes[i]) == DVZ_OK);
        AT(dvz_polygons_set_region_stroke_width_px(set, i, 8.0f) == DVZ_OK);
    }
    DvzComposite* composite = dvz_polygons_composite(set, 0);
    AT(composite != NULL);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    attach.controller_mode = DVZ_CONTROLLER_FIXED;
    AT(dvz_panel_add_composite(panel, composite, &attach) == DVZ_OK);
    AT(_paint_capture(&capture, figure, items));
    for (uint32_t i = 0; i < PAINT_BYTES; i++)
        AT(abs((int)singles[i] - items[i]) <= 1);
    AT(dvz_polygon_composite_set_order(composite, DVZ_POLYGON_ORDER_COLLECTION) == DVZ_OK);
    AT(_paint_capture(&capture, figure, collection));
    const uint8_t* item_pixel = _pixel_at(items, PAINT_SIZE, PAINT_SIZE, 48, 32);
    const uint8_t* collection_pixel = _pixel_at(collection, PAINT_SIZE, PAINT_SIZE, 48, 32);
    AT(item_pixel[0] > 160 && item_pixel[1] > 160);
    AT(collection_pixel[0] < 16 && collection_pixel[1] > 240);
    dvz_drp2_runtime_destroy(capture.runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}



/**
 * Verify holes and hidden contributions leave underlying paint visible.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
static int test_polygon_paint_holes_and_visibility(TstContext* suite, const TstCase* item)
{
    (void)item;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx* ctx = _paint_gpu(suite);
    if (ctx == NULL)
    {
        tst_skip(suite, "Vulkan rendering unavailable");
        return 0;
    }
    PaintCapture capture = {.ctx = ctx};
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = NULL;
    DvzPanel* panel = _paint_panel(scene, &figure);
    AT(panel != NULL);
    const dvec2 square[4] = {{-.75, -.75}, {.75, -.75}, {.75, .75}, {-.75, .75}};
    DvzPolygon* back = _paint_square(
        scene, panel, square, (DvzColor){0, 0, 255, 255}, (DvzColor){0, 0, 0, 0});
    AT(back != NULL);
    DvzPolygon* front = _paint_square(
        scene, panel, square, (DvzColor){255, 0, 0, 255}, (DvzColor){0, 0, 0, 0});
    AT(front != NULL);
    const dvec2 hole[4] = {{-.25, -.25}, {-.25, .25}, {.25, .25}, {.25, -.25}};
    AT(dvz_polygon_set_hole(front, 0, hole, 4) == DVZ_OK);
    uint8_t pixels[PAINT_BYTES] = {0};
    AT(_paint_capture(&capture, figure, pixels));
    const uint8_t* center = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 32, 32);
    const uint8_t* side = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 16, 32);
    log_info("paint hole center=%u,%u,%u side=%u,%u,%u", center[0], center[1], center[2],
             side[0], side[1], side[2]);
    AT(center[0] <= 1 && center[1] <= 1 && center[2] >= 254);
    AT(side[0] >= 254 && side[1] <= 1 && side[2] <= 1);
    AT(dvz_polygon_set_visible(front, false) == DVZ_OK);
    AT(_paint_capture(&capture, figure, pixels));
    side = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 16, 32);
    AT(side[0] <= 1 && side[1] <= 1 && side[2] >= 254);
    AT(dvz_polygon_set_visible(front, true) == DVZ_OK);
    AT(dvz_polygon_set_fill_color(front, (DvzColor){255, 0, 0, 0}) == DVZ_OK);
    AT(_paint_capture(&capture, figure, pixels));
    side = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 16, 32);
    AT(side[0] <= 1 && side[1] <= 1 && side[2] >= 254);
    dvz_drp2_runtime_destroy(capture.runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}



/**
 * Verify images explicitly admitted to paint interleave with semantic polygons.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
static int test_polygon_paint_image_interleaves(TstContext* suite, const TstCase* item)
{
    (void)item;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx* ctx = _paint_gpu(suite);
    if (ctx == NULL)
    {
        tst_skip(suite, "Vulkan rendering unavailable");
        return 0;
    }
    PaintCapture capture = {.ctx = ctx};
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = NULL;
    DvzPanel* panel = _paint_panel(scene, &figure);
    AT(panel != NULL);
    const dvec2 square[4] = {{-.9, -.9}, {.9, -.9}, {.9, .9}, {-.9, .9}};
    DvzPolygon* back = _paint_square(
        scene, panel, square, (DvzColor){0, 0, 255, 255}, (DvzColor){0, 0, 0, 0});
    AT(back != NULL);
    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    const vec3 positions[4] = {
        {-.9f, -.9f, 0}, {-.9f, .9f, 0}, {.9f, -.9f, 0}, {.9f, .9f, 0}};
    const vec2 texcoords[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    const DvzColor texture[4] = {
        {255, 0, 0, 128}, {255, 0, 0, 128}, {255, 0, 0, 128}, {255, 0, 0, 128}};
    AT(dvz_visual_set_data(image, "position", positions, 4) == DVZ_OK);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == DVZ_OK);
    AT(_scene_visual_set_texture_rgba8(image, (const uint8_t*)texture, 2, 2, sizeof(texture)) ==
       DVZ_OK);
    AT(dvz_visual_set_paint(image, true) == DVZ_OK);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    attach.controller_mode = DVZ_CONTROLLER_FIXED;
    AT(dvz_panel_add_visual(panel, image, &attach) == DVZ_OK);
    const dvec2 inset[4] = {{-.25, -.25}, {.25, -.25}, {.25, .25}, {-.25, .25}};
    DvzPolygon* front = _paint_square(
        scene, panel, inset, (DvzColor){0, 255, 0, 255}, (DvzColor){0, 0, 0, 0});
    AT(front != NULL);
    uint8_t pixels[PAINT_BYTES] = {0};
    AT(_paint_capture(&capture, figure, pixels));
    const uint8_t* outside = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 16, 32);
    log_info("paint image RGB=%u,%u,%u expected=188,0,187", outside[0], outside[1], outside[2]);
    AT(abs((int)outside[0] - 188) <= 1);
    AT(outside[1] <= 1);
    AT(abs((int)outside[2] - 187) <= 1);
    const uint8_t* center = _pixel_at(pixels, PAINT_SIZE, PAINT_SIZE, 32, 32);
    AT(center[0] <= 1 && center[1] >= 254 && center[2] <= 1);
    dvz_drp2_runtime_destroy(capture.runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}



/**
 * Verify large sets reuse two visuals while emitting more than 128 ordered draws.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
static int test_polygon_paint_large_set_shared_buffers(TstContext* suite, const TstCase* item)
{
    (void)item;
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, PAINT_SIZE, PAINT_SIZE, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, &(DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
    AT(panel != NULL);
    DvzPolygons* set = dvz_polygons(scene, 0);
    AT(set != NULL);
    const dvec2 square[4] = {{-.75, -.75}, {.75, -.75}, {.75, .75}, {-.75, .75}};
    for (uint32_t i = 0; i < 80; i++)
    {
        uint32_t index = dvz_polygons_add_region(
            set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                     .outer = {.xy = square, .count = 4}});
        AT(index == i);
        AT(dvz_polygons_set_region_fill_color(set, i, (DvzColor){255, 0, 0, 128}) == DVZ_OK);
        AT(dvz_polygons_set_region_stroke_color(set, i, (DvzColor){0, 255, 0, 255}) == DVZ_OK);
        AT(dvz_polygons_set_region_stroke_width_px(set, i, 2.0f) == DVZ_OK);
    }
    DvzComposite* composite = dvz_polygons_composite(set, 0);
    AT(composite != NULL);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    attach.controller_mode = DVZ_CONTROLLER_FIXED;
    AT(dvz_panel_add_composite(panel, composite, &attach) == DVZ_OK);
    AT(dvz_composite_visual_count(composite) == 2);
    AT(panel->visual_count == 2);
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig config = dvz_frame_plan_emit_config();
    config.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    config.target_width = PAINT_SIZE;
    config.target_height = PAINT_SIZE;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &config);
    if (stream == NULL)
    {
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
            log_warn("large paint emission: %s", dvz_diagnostic_report_get(&report, i));
    }
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_stream_draw_indexed_count(stream) == 160);
    _test_scene_stream_destroy(stream);

    const uint32_t expected_draws[4] = {2, 3, 5, 160};
    for (uint32_t phase = 0; phase < 4; phase++)
    {
        if (phase == 0)
        {
            AT(dvz_polygon_composite_set_order(composite, DVZ_POLYGON_ORDER_COLLECTION) == DVZ_OK);
        }
        else if (phase == 1)
        {
            /* The skipped fill retains geometry, so its neighbors cannot share a draw range. */
            AT(dvz_polygons_set_region_fill_color(set, 40, (DvzColor){255, 0, 0, 0}) == DVZ_OK);
        }
        else if (phase == 2)
        {
            /* A second skipped item splits both the fill and stroke runs. */
            AT(dvz_polygons_set_region_visible(set, 20, false) == DVZ_OK);
        }
        else
        {
            AT(dvz_polygons_set_region_fill_color(set, 40, (DvzColor){255, 0, 0, 128}) == DVZ_OK);
            AT(dvz_polygons_set_region_visible(set, 20, true) == DVZ_OK);
            AT(dvz_polygon_composite_set_order(composite, DVZ_POLYGON_ORDER_ITEMS) == DVZ_OK);
        }
        dvz_diagnostic_report_init(&report);
        stream = _test_scene_emit_stream_ex(figure, &caps, &report, &config);
        AT(stream != NULL);
        AT(dvz_diagnostic_report_count(&report) == 0);
        AT(_stream_draw_indexed_count(stream) == expected_draws[phase]);
        AT(dvz_composite_visual_count(composite) == 2);
        AT(panel->visual_count == 2);
        _test_scene_stream_destroy(stream);
    }
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Register process-isolated Vulkan paint regressions.
 *
 * @param suite suite receiving the tests
 * @return zero on success
 */
int test_scene_polygon_paint(TstSuite* suite)
{
    TST_MODULE(suite, "scene");
    TST_GROUP("polygon-paint");
    const char* tags = "scene";
    TST_CASE(test_polygon_paint_large_set_shared_buffers);
    TstFunction tests[] = {
        test_polygon_paint_palette_ignores_lights,
        test_polygon_paint_alpha_mutation_linear,
        test_polygon_paint_collection_matches_singles,
        test_polygon_paint_image_interleaves,
        test_polygon_paint_holes_and_visibility,
    };
    const char* names[] = {
        "test_polygon_paint_palette_ignores_lights",
        "test_polygon_paint_alpha_mutation_linear",
        "test_polygon_paint_collection_matches_singles",
        "test_polygon_paint_image_interleaves",
        "test_polygon_paint_holes_and_visibility",
    };
    for (uint32_t i = 0; i < 5; i++)
    {
        TstCaseDesc desc = tst_case_desc(names[i], names[i], tests[i]);
        desc.tags = "scene";
        desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;
        desc.isolation = TST_ISOLATION_PROCESS;
        desc.run_flags = TST_RUN_CASE_ADAPTER_SUPPORTED;
        tst_suite_add_case(suite, desc);
    }
    return 0;
}
