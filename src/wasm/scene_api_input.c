/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge input routing                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_api_internal.h"



static void _wasm_input_window_size(DvzWasmApiScene* scene, float* out_width, float* out_height)
{
    ANN(out_width);
    ANN(out_height);
    *out_width = scene != NULL ? (float)scene->width : 0.0f;
    *out_height = scene != NULL ? (float)scene->height : 0.0f;
    if (scene == NULL)
        return;

    DvzInputResizeEvent resize = {0};
    if (scene->router != NULL && dvz_input_router_last_resize(scene->router, &resize))
    {
        if (resize.window_width > 0)
            *out_width = (float)resize.window_width;
        if (resize.window_height > 0)
            *out_height = (float)resize.window_height;
        return;
    }

    if (scene->scenario_ctx.logical_width != 0)
        *out_width = (float)scene->scenario_ctx.logical_width;
    if (scene->scenario_ctx.logical_height != 0)
        *out_height = (float)scene->scenario_ctx.logical_height;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_resize(
    uint32_t scene_handle, uint32_t figure_handle, uint32_t width, uint32_t height,
    float device_scale)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL ||
        width == 0 || height == 0)
    {
        return _fail(scene, "invalid WASM resize request");
    }
    _clear_payload(scene);
    scene->width = width;
    scene->height = height;
    if (scene->scenario_active)
    {
        scene->scenario_ctx.logical_width = width;
        scene->scenario_ctx.logical_height = height;
        scene->scenario_ctx.framebuffer_width = width;
        scene->scenario_ctx.framebuffer_height = height;
        scene->scenario_ctx.device_scale = device_scale > 0.0f ? device_scale : 1.0f;
        scene->scenario_ctx.width = width;
        scene->scenario_ctx.height = height;
    }
    dvz_figure_resize(figure->figure, width, height);
    _emit_resize(scene, width, height, device_scale);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_pointer(
    uint32_t scene_handle, int type, float x, float y, int button, int mods, float content_scale,
    double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->router == NULL)
        return _fail(scene, "invalid WASM pointer request");
    _clear_payload(scene);
    uint64_t timestamp_ns = timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    float window_width = 0.0f;
    float window_height = 0.0f;
    _wasm_input_window_size(scene, &window_width, &window_height);
    dvz_pointer_emit_position(
        scene->router, (DvzPointerEventType)type, x, y, window_width, window_height,
        (DvzPointerButton)button, mods, content_scale, timestamp_ns, NULL);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_wheel(
    uint32_t scene_handle, float x, float y, float dir_x, float dir_y, int mods,
    float content_scale, double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->router == NULL)
        return _fail(scene, "invalid WASM wheel request");
    _clear_payload(scene);
    uint64_t timestamp_ns = timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    float window_width = 0.0f;
    float window_height = 0.0f;
    _wasm_input_window_size(scene, &window_width, &window_height);
    dvz_pointer_emit_wheel(
        scene->router, x, y, window_width, window_height, dir_x, dir_y, mods, content_scale,
        timestamp_ns, NULL);
    return 0;
}
