/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "datoviz/drp2.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "datoviz/vk/enums.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzWasmScene DvzWasmScene;

struct DvzWasmScene
{
    DvzScene* scene;
    DvzFigure* figure;
    DvzPanel* panel;
    DvzVisual* visual;
    DvzVisual* primitive;
    DvzVisual* image;
    DvzVisual* mesh;
    DvzController* controller;
    DvzInputRouter* router;
    DvzPointerGestureHandler* gestures;
    DvzDrp2CommandStream* stream;
    char* json;
    DvzDiagnosticReport report;
    uint32_t width;
    uint32_t height;
    uint32_t color_format;
    const char* stream_name;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzWasmScene* _ctx(uint32_t handle) { return (DvzWasmScene*)(uintptr_t)handle; }



static uint32_t _handle(DvzWasmScene* ctx) { return (uint32_t)(uintptr_t)ctx; }



static void _clear_payload(DvzWasmScene* ctx)
{
    if (ctx == NULL)
        return;
    if (ctx->json != NULL)
    {
        dvz_drp2_stream_json_destroy(ctx->json);
        ctx->json = NULL;
    }
    if (ctx->stream != NULL)
    {
        dvz_drp2_stream_destroy(ctx->stream);
        ctx->stream = NULL;
    }
    dvz_diagnostic_report_init(&ctx->report);
}



static void _emit_resize(DvzWasmScene* ctx, uint32_t width, uint32_t height, float device_scale)
{
    if (ctx == NULL || ctx->router == NULL)
        return;

    DvzInputResizeEvent resize = {
        .framebuffer_width = width,
        .framebuffer_height = height,
        .window_width = device_scale > 0.0f ? (uint32_t)((float)width / device_scale) : width,
        .window_height = device_scale > 0.0f ? (uint32_t)((float)height / device_scale) : height,
        .content_scale_x = device_scale > 0.0f ? device_scale : 1.0f,
        .content_scale_y = device_scale > 0.0f ? device_scale : 1.0f,
    };
    dvz_input_emit_resize(ctx->router, &resize);
}



static void _destroy_ctx(DvzWasmScene* ctx)
{
    if (ctx == NULL)
        return;
    _clear_payload(ctx);
    if (ctx->gestures != NULL)
    {
        dvz_pointer_gesture_handler_destroy(ctx->gestures);
        ctx->gestures = NULL;
    }
    if (ctx->router != NULL)
    {
        dvz_input_router_destroy(ctx->router);
        ctx->router = NULL;
    }
    if (ctx->scene != NULL)
    {
        dvz_scene_destroy(ctx->scene);
        ctx->scene = NULL;
    }
    free(ctx);
}



/*************************************************************************************************/
/*  Exported functions                                                                           */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scene_create(uint32_t width, uint32_t height)
{
    if (width == 0)
        width = 640;
    if (height == 0)
        height = 640;

    DvzWasmScene* ctx = (DvzWasmScene*)calloc(1, sizeof(DvzWasmScene));
    if (ctx == NULL)
        return 0;
    dvz_diagnostic_report_init(&ctx->report);
    ctx->width = width;
    ctx->height = height;
    ctx->color_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    ctx->stream_name = "wasm_scene_point_primitive_image_mesh_panzoom";

    ctx->scene = dvz_scene();
    if (ctx->scene == NULL)
        goto fail;

    ctx->figure = dvz_figure(ctx->scene, width, height, 0);
    ctx->panel = dvz_panel_full(ctx->figure);
    ctx->visual = dvz_point(ctx->scene, 0);
    ctx->controller = dvz_panzoom(ctx->scene, NULL);
    ctx->router = dvz_input_router();
    if (
        ctx->figure == NULL || ctx->panel == NULL || ctx->visual == NULL ||
        ctx->controller == NULL || ctx->router == NULL)
    {
        goto fail;
    }

    if (dvz_panel_bind_controller(ctx->panel, ctx->controller, DVZ_DIM_MASK_XY) != 0)
        goto fail;
    if (dvz_panel_connect_input(ctx->panel, ctx->router) != 0)
        goto fail;
    ctx->gestures = dvz_pointer_gesture_handler(ctx->router);
    if (ctx->gestures == NULL)
        goto fail;

    if (dvz_panel_add_visual(ctx->panel, ctx->visual, NULL) != 0)
        goto fail;
    _emit_resize(ctx, width, height, 1.0f);
    return _handle(ctx);

fail:
    _destroy_ctx(ctx);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scene_create_3d(uint32_t width, uint32_t height)
{
    if (width == 0)
        width = 640;
    if (height == 0)
        height = 640;

    DvzWasmScene* ctx = (DvzWasmScene*)calloc(1, sizeof(DvzWasmScene));
    if (ctx == NULL)
        return 0;
    dvz_diagnostic_report_init(&ctx->report);
    ctx->width = width;
    ctx->height = height;
    ctx->color_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    ctx->stream_name = "wasm_scene_mesh3d_arcball";

    ctx->scene = dvz_scene();
    if (ctx->scene == NULL)
        goto fail;

    ctx->figure = dvz_figure(ctx->scene, width, height, 0);
    ctx->panel = dvz_panel_full(ctx->figure);
    ctx->controller = dvz_arcball(ctx->scene, NULL);
    ctx->router = dvz_input_router();
    if (
        ctx->figure == NULL || ctx->panel == NULL || ctx->controller == NULL ||
        ctx->router == NULL)
    {
        goto fail;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.target[0] = 0.0f;
    camera_desc.target[1] = 0.0f;
    camera_desc.target[2] = 0.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (dvz_panel_set_camera(ctx->panel, &camera_desc) == NULL)
        goto fail;

    if (dvz_panel_bind_controller(ctx->panel, ctx->controller, DVZ_DIM_MASK_XYZ) != 0)
        goto fail;
    DvzArcball* arcball = dvz_controller_arcball(ctx->controller);
    if (arcball == NULL)
        goto fail;
    dvz_arcball_initial(arcball, (vec3){0.45f, -0.65f, 0.20f});

    if (dvz_panel_connect_input(ctx->panel, ctx->router) != 0)
        goto fail;
    ctx->gestures = dvz_pointer_gesture_handler(ctx->router);
    if (ctx->gestures == NULL)
        goto fail;

    _emit_resize(ctx, width, height, 1.0f);
    return _handle(ctx);

fail:
    _destroy_ctx(ctx);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_set_canvas_format(uint32_t handle, uint32_t color_format)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL)
        return -1;
    if (color_format != DVZ_FORMAT_R8G8B8A8_UNORM && color_format != DVZ_FORMAT_B8G8R8A8_UNORM)
        return -1;
    _clear_payload(ctx);
    ctx->color_format = color_format;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
void dvz_wasm_scene_destroy(uint32_t handle) { _destroy_ctx(_ctx(handle)); }



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_resize(
    uint32_t handle, uint32_t width, uint32_t height, float device_scale)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->figure == NULL)
        return -1;
    if (width == 0 || height == 0)
        return -1;
    _clear_payload(ctx);
    ctx->width = width;
    ctx->height = height;
    dvz_figure_resize(ctx->figure, width, height);
    _emit_resize(ctx, width, height, device_scale);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_set_points(
    uint32_t handle, const float* positions, const uint8_t* colors, const float* sizes,
    uint32_t count)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->visual == NULL || positions == NULL || colors == NULL ||
        sizes == NULL || count == 0)
    {
        return -1;
    }
    _clear_payload(ctx);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter", .data = sizes, .item_count = count},
    };
    return dvz_visual_set_data_many(ctx->visual, updates, 3);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_set_primitive(
    uint32_t handle, const float* positions, const uint8_t* colors, uint32_t count)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->scene == NULL || ctx->panel == NULL || positions == NULL ||
        colors == NULL || count == 0 || count % 3 != 0)
    {
        return -1;
    }
    _clear_payload(ctx);
    if (ctx->primitive == NULL)
    {
        ctx->primitive = dvz_primitive(ctx->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        if (ctx->primitive == NULL || dvz_panel_add_visual(ctx->panel, ctx->primitive, NULL) != 0)
            return -1;
    }
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
    };
    return dvz_visual_set_data_many(ctx->primitive, updates, 2);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_set_image(
    uint32_t handle, const uint8_t* rgba, uint32_t width, uint32_t height)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->scene == NULL || ctx->panel == NULL || rgba == NULL || width == 0 ||
        height == 0)
    {
        return -1;
    }
    _clear_payload(ctx);
    if (ctx->image == NULL)
    {
        ctx->image = dvz_image(ctx->scene, 0);
        if (ctx->image == NULL || dvz_panel_add_visual(ctx->panel, ctx->image, NULL) != 0)
            return -1;
    }

    vec3 positions[4] = {
        {0.18f, -0.78f, 0.05f},
        {0.18f, -0.12f, 0.05f},
        {0.86f, -0.78f, 0.05f},
        {0.86f, -0.12f, 0.05f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    if (dvz_visual_set_data(ctx->image, "position", positions, 4) != 0)
        return -1;
    if (dvz_visual_set_data(ctx->image, "texcoords", texcoords, 4) != 0)
        return -1;
    return dvz_visual_set_texture(ctx->image, rgba, width, height);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_set_mesh(
    uint32_t handle, const float* positions, const uint8_t* colors, const float* normals,
    uint32_t count)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->scene == NULL || ctx->panel == NULL || positions == NULL ||
        colors == NULL || normals == NULL || count == 0 || count % 3 != 0)
    {
        return -1;
    }
    _clear_payload(ctx);
    if (ctx->mesh == NULL)
    {
        ctx->mesh = dvz_mesh(ctx->scene, 0);
        if (ctx->mesh == NULL || dvz_panel_add_visual(ctx->panel, ctx->mesh, NULL) != 0)
            return -1;
    }
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "normal", .data = normals, .item_count = count},
    };
    return dvz_visual_set_data_many(ctx->mesh, updates, 3);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_emit(uint32_t handle)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->figure == NULL)
        return -1;
    _clear_payload(ctx);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256u * 1024u * 1024u;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 0;
    emit_cfg.color_target_format = ctx->color_format;
    emit_cfg.target_width = ctx->width;
    emit_cfg.target_height = ctx->height;

    ctx->stream = dvz_figure_emit_ex(ctx->figure, &caps, &ctx->report, &emit_cfg);
    if (ctx->stream == NULL || dvz_diagnostic_report_count(&ctx->report) > 0)
        return -1;
    ctx->json = dvz_drp2_stream_json(ctx->stream, ctx->stream_name);
    return ctx->json != NULL ? 0 : -1;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scene_payload_ptr(uint32_t handle)
{
    DvzWasmScene* ctx = _ctx(handle);
    return ctx != NULL && ctx->json != NULL ? (uint32_t)(uintptr_t)ctx->json : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scene_payload_size(uint32_t handle)
{
    DvzWasmScene* ctx = _ctx(handle);
    return ctx != NULL && ctx->json != NULL ? (uint32_t)strlen(ctx->json) : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scene_diagnostic_count(uint32_t handle)
{
    DvzWasmScene* ctx = _ctx(handle);
    return ctx != NULL ? dvz_diagnostic_report_count(&ctx->report) : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scene_diagnostic(uint32_t handle, uint32_t index)
{
    DvzWasmScene* ctx = _ctx(handle);
    const char* diagnostic = ctx != NULL ? dvz_diagnostic_report_get(&ctx->report, index) : NULL;
    return diagnostic != NULL ? (uint32_t)(uintptr_t)diagnostic : 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_pointer(
    uint32_t handle, int type, float x, float y, int button, int mods, float content_scale,
    double timestamp_ms)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->router == NULL)
        return -1;
    _clear_payload(ctx);
    uint64_t timestamp_ns = timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    dvz_pointer_emit_position(
        ctx->router, (DvzPointerEventType)type, x, y, x, y, (DvzPointerButton)button, mods,
        content_scale, timestamp_ns, NULL);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_scene_wheel(
    uint32_t handle, float x, float y, float dir_x, float dir_y, int mods, float content_scale,
    double timestamp_ms)
{
    DvzWasmScene* ctx = _ctx(handle);
    if (ctx == NULL || ctx->router == NULL)
        return -1;
    _clear_payload(ctx);
    uint64_t timestamp_ns = timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    dvz_pointer_emit_wheel(
        ctx->router, x, y, x, y, dir_x, dir_y, mods, content_scale, timestamp_ns, NULL);
    return 0;
}
