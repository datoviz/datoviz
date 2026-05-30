/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic WASM scene ABI                                                                       */
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
#include "datoviz/scene.h"
#include "datoviz/vk/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WASM_API_MAX_WRAPPERS 256
#define DVZ_WASM_VISUAL_POINT 1
#define DVZ_WASM_VISUAL_IMAGE 6
#define DVZ_WASM_VISUAL_MESH 7
#define DVZ_WASM_VISUAL_PRIMITIVE 9



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzWasmApiScene DvzWasmApiScene;

typedef struct
{
    DvzWasmApiScene* owner;
    DvzFigure* figure;
} DvzWasmApiFigure;

typedef struct
{
    DvzWasmApiScene* owner;
    DvzPanel* panel;
} DvzWasmApiPanel;

typedef struct
{
    DvzWasmApiScene* owner;
    DvzVisual* visual;
} DvzWasmApiVisual;

typedef struct
{
    DvzWasmApiScene* owner;
    DvzController* controller;
} DvzWasmApiController;

struct DvzWasmApiScene
{
    DvzScene* scene;
    DvzDrp2CommandStream* stream;
    char* json;
    DvzDiagnosticReport report;
    void* wrappers[DVZ_WASM_API_MAX_WRAPPERS];
    uint32_t wrapper_count;
    uint32_t width;
    uint32_t height;
    uint32_t color_format;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzWasmApiScene* _scene(uint32_t handle) { return (DvzWasmApiScene*)(uintptr_t)handle; }



static DvzWasmApiFigure* _figure(uint32_t handle) { return (DvzWasmApiFigure*)(uintptr_t)handle; }



static DvzWasmApiPanel* _panel(uint32_t handle) { return (DvzWasmApiPanel*)(uintptr_t)handle; }



static DvzWasmApiVisual* _visual(uint32_t handle) { return (DvzWasmApiVisual*)(uintptr_t)handle; }



static DvzWasmApiController* _controller(uint32_t handle)
{
    return (DvzWasmApiController*)(uintptr_t)handle;
}



static uint32_t _handle(void* ptr) { return (uint32_t)(uintptr_t)ptr; }



static void _clear_payload(DvzWasmApiScene* scene)
{
    if (scene == NULL)
        return;
    if (scene->json != NULL)
    {
        dvz_drp2_stream_json_destroy(scene->json);
        scene->json = NULL;
    }
    if (scene->stream != NULL)
    {
        dvz_drp2_stream_destroy(scene->stream);
        scene->stream = NULL;
    }
    dvz_diagnostic_report_init(&scene->report);
}



static bool _remember(DvzWasmApiScene* scene, void* wrapper)
{
    if (scene == NULL || wrapper == NULL || scene->wrapper_count >= DVZ_WASM_API_MAX_WRAPPERS)
        return false;
    scene->wrappers[scene->wrapper_count++] = wrapper;
    return true;
}



/*************************************************************************************************/
/*  Scene lifecycle                                                                              */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scene(uint32_t width, uint32_t height)
{
    if (width == 0)
        width = 640;
    if (height == 0)
        height = 640;

    DvzWasmApiScene* scene = (DvzWasmApiScene*)calloc(1, sizeof(DvzWasmApiScene));
    if (scene == NULL)
        return 0;
    dvz_diagnostic_report_init(&scene->report);
    scene->width = width;
    scene->height = height;
    scene->color_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    scene->scene = dvz_scene();
    if (scene->scene == NULL)
    {
        free(scene);
        return 0;
    }
    return _handle(scene);
}



EMSCRIPTEN_KEEPALIVE
void dvz_wasm_api_scene_destroy(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return;
    _clear_payload(scene);
    if (scene->scene != NULL)
    {
        dvz_scene_destroy(scene->scene);
        scene->scene = NULL;
    }
    for (uint32_t i = 0; i < scene->wrapper_count; i++)
        free(scene->wrappers[i]);
    free(scene);
}



/*************************************************************************************************/
/*  Objects                                                                                      */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_figure(uint32_t scene_handle, uint32_t width, uint32_t height)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    if (width == 0)
        width = scene->width;
    if (height == 0)
        height = scene->height;
    _clear_payload(scene);
    DvzWasmApiFigure* figure = (DvzWasmApiFigure*)calloc(1, sizeof(DvzWasmApiFigure));
    if (figure == NULL)
        return 0;
    figure->owner = scene;
    figure->figure = dvz_figure(scene->scene, width, height, 0);
    if (figure->figure == NULL || !_remember(scene, figure))
    {
        free(figure);
        return 0;
    }
    scene->width = width;
    scene->height = height;
    return _handle(figure);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_panel_full(uint32_t figure_handle)
{
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (figure == NULL || figure->owner == NULL || figure->figure == NULL)
        return 0;
    _clear_payload(figure->owner);
    DvzWasmApiPanel* panel = (DvzWasmApiPanel*)calloc(1, sizeof(DvzWasmApiPanel));
    if (panel == NULL)
        return 0;
    panel->owner = figure->owner;
    panel->panel = dvz_panel_full(figure->figure);
    if (panel->panel == NULL || !_remember(figure->owner, panel))
    {
        free(panel);
        return 0;
    }
    return _handle(panel);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_visual(uint32_t scene_handle, uint32_t visual_type, uint32_t flags)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    _clear_payload(scene);
    DvzWasmApiVisual* visual = (DvzWasmApiVisual*)calloc(1, sizeof(DvzWasmApiVisual));
    if (visual == NULL)
        return 0;
    visual->owner = scene;
    switch (visual_type)
    {
    case DVZ_WASM_VISUAL_POINT:
        visual->visual = dvz_point(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_PRIMITIVE:
        visual->visual = dvz_primitive(scene->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
        break;
    case DVZ_WASM_VISUAL_IMAGE:
        visual->visual = dvz_image(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_MESH:
        visual->visual = dvz_mesh(scene->scene, flags);
        break;
    default:
        break;
    }
    if (visual->visual == NULL || !_remember(scene, visual))
    {
        free(visual);
        return 0;
    }
    return _handle(visual);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_controller(uint32_t scene_handle, uint32_t controller_type)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    _clear_payload(scene);
    DvzWasmApiController* controller =
        (DvzWasmApiController*)calloc(1, sizeof(DvzWasmApiController));
    if (controller == NULL)
        return 0;
    controller->owner = scene;
    switch ((DvzControllerType)controller_type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        controller->controller = dvz_panzoom(scene->scene, NULL);
        break;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        controller->controller = dvz_arcball(scene->scene, NULL);
        break;
    default:
        break;
    }
    if (controller->controller == NULL || !_remember(scene, controller))
    {
        free(controller);
        return 0;
    }
    return _handle(controller);
}



/*************************************************************************************************/
/*  Mutators                                                                                     */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_add_visual(uint32_t panel_handle, uint32_t visual_handle)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        panel == NULL || visual == NULL || panel->owner == NULL || visual->owner != panel->owner ||
        panel->panel == NULL || visual->visual == NULL)
    {
        return -1;
    }
    _clear_payload(panel->owner);
    return dvz_panel_add_visual(panel->panel, visual->visual, NULL);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_bind_controller(
    uint32_t panel_handle, uint32_t controller_handle, uint32_t dims)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    DvzWasmApiController* controller = _controller(controller_handle);
    if (
        panel == NULL || controller == NULL || panel->owner == NULL ||
        controller->owner != panel->owner || panel->panel == NULL || controller->controller == NULL)
    {
        return -1;
    }
    _clear_payload(panel->owner);
    return dvz_panel_bind_controller(panel->panel, controller->controller, (DvzDimMask)dims);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_f32(
    uint32_t visual_handle, const char* attr, const float* data, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        data == NULL || item_count == 0)
    {
        return -1;
    }
    _clear_payload(visual->owner);
    return dvz_visual_set_data(visual->visual, attr, data, item_count);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_rgba8(
    uint32_t visual_handle, const char* attr, const uint8_t* data, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        data == NULL || item_count == 0)
    {
        return -1;
    }
    _clear_payload(visual->owner);
    return dvz_visual_set_data(visual->visual, attr, data, item_count);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_texture_rgba8(
    uint32_t visual_handle, const uint8_t* rgba, uint32_t width, uint32_t height)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || rgba == NULL ||
        width == 0 || height == 0)
    {
        return -1;
    }
    _clear_payload(visual->owner);
    return dvz_visual_set_texture(visual->visual, rgba, width, height);
}



/*************************************************************************************************/
/*  Emission and diagnostics                                                                     */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_set_canvas_format(uint32_t scene_handle, uint32_t color_format)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (color_format != DVZ_FORMAT_R8G8B8A8_UNORM && color_format != DVZ_FORMAT_B8G8R8A8_UNORM)
        return -1;
    _clear_payload(scene);
    scene->color_format = color_format;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
        return -1;
    _clear_payload(scene);

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
    emit_cfg.color_target_format = scene->color_format;
    emit_cfg.target_width = scene->width;
    emit_cfg.target_height = scene->height;

    scene->stream = dvz_figure_emit_ex(figure->figure, &caps, &scene->report, &emit_cfg);
    if (scene->stream == NULL || dvz_diagnostic_report_count(&scene->report) > 0)
        return -1;
    scene->json = dvz_drp2_stream_json(scene->stream, "wasm_api_scene");
    return scene->json != NULL ? 0 : -1;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_payload_ptr(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->json != NULL ? (uint32_t)(uintptr_t)scene->json : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_payload_size(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->json != NULL ? (uint32_t)strlen(scene->json) : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_diagnostic_count(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL ? dvz_diagnostic_report_count(&scene->report) : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_diagnostic(uint32_t scene_handle, uint32_t index)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    const char* diagnostic = scene != NULL ? dvz_diagnostic_report_get(&scene->report, index) : NULL;
    return diagnostic != NULL ? (uint32_t)(uintptr_t)diagnostic : 0;
}
