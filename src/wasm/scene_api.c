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
#include <stdio.h>
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
#include "datoviz/scene/frame_packets.h"
#include "datoviz/vk/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WASM_API_MAX_WRAPPERS 256
#define DVZ_WASM_VISUAL_POINT 1
#define DVZ_WASM_VISUAL_PIXEL 2
#define DVZ_WASM_VISUAL_MARKER 3
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
    DvzInputRouter* router;
    DvzPointerGestureHandler* gestures;
    DvzCapabilitySnapshot caps;
    char* json;
    DvzDiagnosticReport report;
    void* packets[4];
    uint64_t packet_sizes[4];
    void* arenas[4];
    uint64_t arena_sizes[4];
    int packet_status;
    uint64_t resource_version;
    uint64_t frame_index;
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
    for (uint32_t i = DVZ_DRP2_PACKET_SETUP; i <= DVZ_DRP2_PACKET_FRAME; i++)
    {
        dvz_drp2_packet_destroy(scene->packets[i]);
        dvz_drp2_packet_destroy(scene->arenas[i]);
        scene->packets[i] = NULL;
        scene->packet_sizes[i] = 0;
        scene->arenas[i] = NULL;
        scene->arena_sizes[i] = 0;
    }
    scene->packet_status = 0;
    dvz_diagnostic_report_init(&scene->report);
}



static int _fail(DvzWasmApiScene* scene, const char* diagnostic)
{
    if (scene != NULL)
    {
        _clear_payload(scene);
        if (diagnostic != NULL)
            (void)dvz_diagnostic_report_add(&scene->report, diagnostic);
    }
    return -1;
}



static uint32_t _fail_handle(DvzWasmApiScene* scene, const char* diagnostic)
{
    (void)_fail(scene, diagnostic);
    return 0;
}



static int _fail_upload(
    DvzWasmApiScene* scene, const char* kind, const char* attr, uint32_t item_count)
{
    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
    int ret = snprintf(
        diagnostic, sizeof(diagnostic), "WASM %s visual upload failed: attr=%s item_count=%u",
        kind != NULL ? kind : "data", attr != NULL ? attr : "<null>", item_count);
    if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
        return _fail(scene, "WASM visual upload failed");
    return _fail(scene, diagnostic);
}



static bool _remember(DvzWasmApiScene* scene, void* wrapper)
{
    if (scene == NULL || wrapper == NULL || scene->wrapper_count >= DVZ_WASM_API_MAX_WRAPPERS)
        return false;
    scene->wrappers[scene->wrapper_count++] = wrapper;
    return true;
}



static void _emit_resize(
    DvzWasmApiScene* scene, uint32_t width, uint32_t height, float device_scale)
{
    if (scene == NULL || scene->router == NULL)
        return;

    DvzInputResizeEvent resize = {
        .framebuffer_width = width,
        .framebuffer_height = height,
        .window_width = device_scale > 0.0f ? (uint32_t)((float)width / device_scale) : width,
        .window_height = device_scale > 0.0f ? (uint32_t)((float)height / device_scale) : height,
        .content_scale_x = device_scale > 0.0f ? device_scale : 1.0f,
        .content_scale_y = device_scale > 0.0f ? device_scale : 1.0f,
    };
    dvz_input_emit_resize(scene->router, &resize);
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
    scene->caps = dvz_capability_snapshot();
    scene->caps.shader_format_wgsl = true;
    scene->caps.shader_format_glsl = false;
    scene->width = width;
    scene->height = height;
    scene->color_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    scene->scene = dvz_scene();
    scene->router = dvz_input_router();
    if (scene->scene == NULL || scene->router == NULL)
    {
        if (scene->router != NULL)
            dvz_input_router_destroy(scene->router);
        if (scene->scene != NULL)
            dvz_scene_destroy(scene->scene);
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
    if (scene->gestures != NULL)
    {
        dvz_pointer_gesture_handler_destroy(scene->gestures);
        scene->gestures = NULL;
    }
    if (scene->router != NULL)
    {
        dvz_input_router_destroy(scene->router);
        scene->router = NULL;
    }
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
        return _fail_handle(scene, "WASM figure wrapper allocation failed");
    figure->owner = scene;
    figure->figure = dvz_figure(scene->scene, width, height, 0);
    if (figure->figure == NULL || !_remember(scene, figure))
    {
        free(figure);
        return _fail_handle(scene, "WASM figure creation failed");
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
        return _fail_handle(figure->owner, "WASM panel wrapper allocation failed");
    panel->owner = figure->owner;
    panel->panel = dvz_panel_full(figure->figure);
    if (panel->panel == NULL || !_remember(figure->owner, panel))
    {
        free(panel);
        return _fail_handle(figure->owner, "WASM panel creation failed");
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
        return _fail_handle(scene, "WASM visual wrapper allocation failed");
    visual->owner = scene;
    switch (visual_type)
    {
    case DVZ_WASM_VISUAL_POINT:
        visual->visual = dvz_point(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_PIXEL:
        visual->visual = dvz_pixel(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_MARKER:
        visual->visual = dvz_marker(scene->scene, flags);
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
        free(visual);
        return _fail_handle(scene, "unsupported WASM visual type");
        break;
    }
    if (visual->visual == NULL || !_remember(scene, visual))
    {
        free(visual);
        return _fail_handle(scene, "WASM visual creation failed");
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
        return _fail_handle(scene, "WASM controller wrapper allocation failed");
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
        free(controller);
        return _fail_handle(scene, "unsupported WASM controller type");
        break;
    }
    if (controller->controller == NULL || !_remember(scene, controller))
    {
        free(controller);
        return _fail_handle(scene, "WASM controller creation failed");
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
        DvzWasmApiScene* owner = panel != NULL ? panel->owner : visual != NULL ? visual->owner : NULL;
        return _fail(owner, "invalid WASM panel/visual handle");
    }
    _clear_payload(panel->owner);
    if (dvz_panel_add_visual(panel->panel, visual->visual, NULL) != 0)
        return _fail(panel->owner, "WASM panel add visual failed");
    return 0;
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
        DvzWasmApiScene* owner =
            panel != NULL ? panel->owner : controller != NULL ? controller->owner : NULL;
        return _fail(owner, "invalid WASM panel/controller handle");
    }
    _clear_payload(panel->owner);
    if (dvz_panel_bind_controller(panel->panel, controller->controller, (DvzDimMask)dims) != 0)
        return _fail(panel->owner, "WASM panel bind controller failed");
    if (dvz_panel_connect_input(panel->panel, panel->owner->router) != 0)
        return _fail(panel->owner, "WASM panel input connection failed");
    if (panel->owner->gestures == NULL)
    {
        panel->owner->gestures = dvz_pointer_gesture_handler(panel->owner->router);
        if (panel->owner->gestures == NULL)
            return _fail(panel->owner, "WASM pointer gesture setup failed");
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_set_camera(
    uint32_t panel_handle, float eye_x, float eye_y, float eye_z, float target_x, float target_y,
    float target_z, float fov_y, float near, float far)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    if (panel == NULL || panel->owner == NULL || panel->panel == NULL)
        return _fail(panel != NULL ? panel->owner : NULL, "invalid WASM panel handle");
    _clear_payload(panel->owner);
    DvzCameraDesc desc = dvz_camera_desc();
    desc.eye[0] = eye_x;
    desc.eye[1] = eye_y;
    desc.eye[2] = eye_z;
    desc.target[0] = target_x;
    desc.target[1] = target_y;
    desc.target[2] = target_z;
    desc.fov_y = fov_y;
    desc.near = near;
    desc.far = far;
    if (dvz_panel_set_camera(panel->panel, &desc) == NULL)
        return _fail(panel->owner, "WASM panel camera setup failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_arcball_initial(
    uint32_t controller_handle, float angle_x, float angle_y, float angle_z)
{
    DvzWasmApiController* controller = _controller(controller_handle);
    if (controller == NULL || controller->owner == NULL || controller->controller == NULL)
        return _fail(
            controller != NULL ? controller->owner : NULL, "invalid WASM controller handle");
    DvzArcball* arcball = dvz_controller_arcball(controller->controller);
    if (arcball == NULL)
        return _fail(controller->owner, "WASM controller is not an arcball");
    _clear_payload(controller->owner);
    dvz_arcball_initial(arcball, (vec3){angle_x, angle_y, angle_z});
    return 0;
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
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM f32 visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_data(visual->visual, attr, data, item_count) != 0)
        return _fail_upload(visual->owner, "f32", attr, item_count);
    return 0;
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
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM rgba8 visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_data(visual->visual, attr, data, item_count) != 0)
        return _fail_upload(visual->owner, "rgba8", attr, item_count);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_u32(
    uint32_t visual_handle, const char* attr, const uint32_t* data, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        data == NULL || item_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM u32 visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_data(visual->visual, attr, data, item_count) != 0)
        return _fail_upload(visual->owner, "u32", attr, item_count);
    return 0;
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
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM RGBA8 texture upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_texture(visual->visual, rgba, width, height) != 0)
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic), "WASM RGBA8 texture upload failed: %ux%u", width,
            height);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(visual->owner, "WASM RGBA8 texture upload failed");
        return _fail(visual->owner, diagnostic);
    }
    return 0;
}



/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_resize(
    uint32_t scene_handle, uint32_t figure_handle, uint32_t width, uint32_t height,
    float device_scale)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (
        scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL ||
        width == 0 || height == 0)
    {
        return _fail(scene, "invalid WASM resize request");
    }
    _clear_payload(scene);
    scene->width = width;
    scene->height = height;
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
    dvz_pointer_emit_position(
        scene->router, (DvzPointerEventType)type, x, y, x, y, (DvzPointerButton)button, mods,
        content_scale, timestamp_ns, NULL);
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
    dvz_pointer_emit_wheel(
        scene->router, x, y, x, y, dir_x, dir_y, mods, content_scale, timestamp_ns, NULL);
    return 0;
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
        return _fail(scene, "unsupported WASM canvas format");
    _clear_payload(scene);
    scene->color_format = color_format;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_set_capabilities(
    uint32_t scene_handle, uint32_t max_texture_dimension_2d, uint32_t max_bind_groups,
    uint32_t max_vertex_buffers, uint32_t max_buffer_size,
    uint32_t min_texture_copy_bytes_per_row_alignment, uint32_t max_sample_count)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (
        max_texture_dimension_2d == 0 || max_bind_groups == 0 || max_vertex_buffers == 0 ||
        max_buffer_size == 0 || min_texture_copy_bytes_per_row_alignment == 0 ||
        max_sample_count == 0)
    {
        return _fail(scene, "invalid WASM capability snapshot");
    }
    _clear_payload(scene);
    scene->caps.max_texture_dimension_2d = max_texture_dimension_2d;
    scene->caps.max_bind_groups = max_bind_groups;
    scene->caps.max_vertex_buffers = max_vertex_buffers;
    scene->caps.max_buffer_size = max_buffer_size;
    scene->caps.max_color_sample_count = max_sample_count;
    scene->caps.max_depth_sample_count = max_sample_count;
    scene->caps.min_texture_copy_bytes_per_row_alignment =
        min_texture_copy_bytes_per_row_alignment;
    return 0;
}



static int _emit(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
        return _fail(scene, "invalid WASM emit request");
    _clear_payload(scene);

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 0;
    emit_cfg.color_target_format = scene->color_format;
    emit_cfg.target_width = scene->width;
    emit_cfg.target_height = scene->height;

    scene->stream = dvz_figure_emit_ex(figure->figure, &scene->caps, &scene->report, &emit_cfg);
    if (scene->stream == NULL)
    {
        if (dvz_diagnostic_report_count(&scene->report) == 0)
            (void)dvz_diagnostic_report_add(&scene->report, "WASM scene emission failed");
        return -1;
    }
    if (dvz_diagnostic_report_count(&scene->report) > 0)
        return -1;
    scene->json = dvz_drp2_stream_json(scene->stream, "wasm_api_scene");
    if (scene->json == NULL)
        return _fail(scene, "WASM DRP2 JSON serialization failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit(scene_handle, figure_handle);
}



static int _emit_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
    {
        int ret = _fail(scene, "invalid WASM packet emit request");
        if (scene != NULL)
            scene->packet_status = -1;
        return ret;
    }
    _clear_payload(scene);

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 0;
    emit_cfg.color_target_format = scene->color_format;
    emit_cfg.target_width = scene->width;
    emit_cfg.target_height = scene->height;

    scene->stream = dvz_figure_emit_ex(figure->figure, &scene->caps, &scene->report, &emit_cfg);
    if (scene->stream == NULL)
    {
        if (dvz_diagnostic_report_count(&scene->report) == 0)
            (void)dvz_diagnostic_report_add(&scene->report, "WASM scene packet emission failed");
        scene->packet_status = -1;
        return -1;
    }
    if (dvz_diagnostic_report_count(&scene->report) > 0)
    {
        scene->packet_status = -1;
        return -1;
    }

    scene->frame_index++;
    scene->resource_version++;
    const DvzDrp2PacketKind phases[3] = {
        DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_PACKET_UPDATE,
        DVZ_DRP2_PACKET_FRAME,
    };
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzDrp2PacketKind kind = phases[i];
        if (!dvz_drp2_packet_encode_stream_phase(
                scene->stream, kind, scene->resource_version, scene->frame_index,
                &scene->packets[kind], &scene->packet_sizes[kind], &scene->arenas[kind],
                &scene->arena_sizes[kind]))
        {
            (void)_fail(scene, "WASM DRP2 packet encoding failed");
            scene->packet_status = -2;
            return -1;
        }
    }
    scene->packet_status = 0;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit_packets(scene_handle, figure_handle);
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



static bool _valid_packet_kind(uint32_t kind)
{
    return kind >= DVZ_DRP2_PACKET_SETUP && kind <= DVZ_DRP2_PACKET_FRAME;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_packet_status(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL ? scene->packet_status : -1;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_ptr(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && _valid_packet_kind(kind) && scene->packets[kind] != NULL
               ? (uint32_t)(uintptr_t)scene->packets[kind]
               : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_size(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    uint64_t size = scene != NULL && _valid_packet_kind(kind) ? scene->packet_sizes[kind] : 0;
    return size <= UINT32_MAX ? (uint32_t)size : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_arena_ptr(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && _valid_packet_kind(kind) && scene->arenas[kind] != NULL
               ? (uint32_t)(uintptr_t)scene->arenas[kind]
               : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_arena_size(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    uint64_t size = scene != NULL && _valid_packet_kind(kind) ? scene->arena_sizes[kind] : 0;
    return size <= UINT32_MAX ? (uint32_t)size : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_resource_version(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->resource_version <= UINT32_MAX ? (uint32_t)scene->resource_version
                                                                  : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_frame_index(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->frame_index <= UINT32_MAX ? (uint32_t)scene->frame_index : 0;
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
