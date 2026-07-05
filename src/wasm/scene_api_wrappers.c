/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge object wrappers                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_api_internal.h"
#include "domain/field_internal.h"



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
    scene->caps = _wasm_capability_snapshot();
    scene->width = width;
    scene->height = height;
    scene->color_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    scene->scene = dvz_scene();
    scene->router = dvz_input_router();
    if (scene->scene == NULL || scene->router == NULL || !_ensure_query_emitter(scene->scene))
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
    _clear_query(scene);
    _clear_payload(scene);
    if (scene->scenario_active && scene->scenario_spec.destroy != NULL)
    {
        scene->scenario_spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        scene->scenario_active = false;
    }
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
    case DVZ_WASM_VISUAL_SEGMENT:
        visual->visual = dvz_segment(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_PATH:
        visual->visual = dvz_path(scene->scene, flags);
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
    case DVZ_WASM_VISUAL_GLYPH:
        visual->visual = dvz_glyph(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_SPHERE:
        visual->visual = dvz_sphere(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_TEXT:
        visual->visual = _scene_text_visual(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_LABELS:
        visual->visual = dvz_labels(scene->scene, flags);
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
uint32_t
dvz_wasm_api_buffer(uint32_t scene_handle, uint32_t usage, uint32_t stride, uint32_t byte_size)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    if (usage == 0 || stride == 0)
        return _fail_handle(scene, "invalid WASM scene buffer descriptor");
    _clear_payload(scene);

    DvzWasmApiBuffer* buffer = (DvzWasmApiBuffer*)calloc(1, sizeof(DvzWasmApiBuffer));
    if (buffer == NULL)
        return _fail_handle(scene, "WASM scene buffer wrapper allocation failed");
    buffer->owner = scene;
    DvzSceneBufferDesc desc = dvz_scene_buffer_desc();
    desc.usage = usage;
    desc.stride = stride;
    desc.byte_size = byte_size;
    buffer->buffer = dvz_scene_buffer(scene->scene, &desc);
    if (buffer->buffer == NULL || !_remember(scene, buffer))
    {
        free(buffer);
        return _fail_handle(scene, "WASM scene buffer creation failed");
    }
    return _handle(buffer);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_buffer_set_data(uint32_t buffer_handle, const void* data, uint32_t byte_size)
{
    DvzWasmApiBuffer* buffer = _buffer(buffer_handle);
    if (buffer == NULL || buffer->owner == NULL || buffer->buffer == NULL || data == NULL ||
        byte_size == 0)
    {
        return _fail(buffer != NULL ? buffer->owner : NULL, "invalid WASM scene buffer upload");
    }
    _clear_payload(buffer->owner);
    if (!dvz_scene_buffer_set_data(buffer->buffer, data, byte_size))
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic), "WASM scene buffer upload failed: byte_size=%u",
            byte_size);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(buffer->owner, "WASM scene buffer upload failed");
        return _fail(buffer->owner, diagnostic);
    }
    return 0;
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
    if (panel == NULL || visual == NULL || panel->owner == NULL || visual->owner != panel->owner ||
        panel->panel == NULL || visual->visual == NULL)
    {
        DvzWasmApiScene* owner = panel != NULL    ? panel->owner
                                 : visual != NULL ? visual->owner
                                                  : NULL;
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
    if (panel == NULL || controller == NULL || panel->owner == NULL ||
        controller->owner != panel->owner || panel->panel == NULL ||
        controller->controller == NULL)
    {
        DvzWasmApiScene* owner = panel != NULL        ? panel->owner
                                 : controller != NULL ? controller->owner
                                                      : NULL;
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
    desc.view.eye[0] = eye_x;
    desc.view.eye[1] = eye_y;
    desc.view.eye[2] = eye_z;
    desc.view.target[0] = target_x;
    desc.view.target[1] = target_y;
    desc.view.target[2] = target_z;
    desc.projection.fov_y = fov_y;
    desc.projection.near_clip = near;
    desc.projection.far_clip = far;
    if (dvz_panel_set_camera_desc(panel->panel, &desc) != 0)
        return _fail(panel->owner, "WASM panel camera setup failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_set_domain(uint32_t panel_handle, uint32_t dim, double min, double max)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    if (panel == NULL || panel->owner == NULL || panel->panel == NULL)
        return _fail(panel != NULL ? panel->owner : NULL, "invalid WASM panel handle");
    if (dim != DVZ_DIM_X && dim != DVZ_DIM_Y)
        return _fail(panel->owner, "unsupported WASM panel domain dimension");
    _clear_payload(panel->owner);
    if (dvz_panel_set_domain(panel->panel, (DvzDim)dim, min, max) != 0)
        return _fail(panel->owner, "WASM panel domain setup failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_panel_axis(uint32_t panel_handle, uint32_t dim)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    if (panel == NULL || panel->owner == NULL || panel->panel == NULL)
        return _fail_handle(panel != NULL ? panel->owner : NULL, "invalid WASM panel handle");
    if (dim != DVZ_DIM_X && dim != DVZ_DIM_Y)
        return _fail_handle(panel->owner, "unsupported WASM panel axis dimension");
    _clear_payload(panel->owner);
    DvzWasmApiAxis* axis = (DvzWasmApiAxis*)calloc(1, sizeof(DvzWasmApiAxis));
    if (axis == NULL)
        return _fail_handle(panel->owner, "WASM axis wrapper allocation failed");
    axis->owner = panel->owner;
    axis->axis = dvz_panel_axis(panel->panel, (DvzDim)dim);
    if (axis->axis != NULL)
    {
        DvzAxisStyle style = dvz_axis_style();
        style.text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
        (void)dvz_axis_set_style(axis->axis, &style);
    }
    if (axis->axis == NULL || !_remember(panel->owner, axis))
    {
        free(axis);
        return _fail_handle(panel->owner, "WASM panel axis creation failed");
    }
    return _handle(axis);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_visible(uint32_t axis_handle, uint32_t visible)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis handle");
    _clear_payload(axis->owner);
    if (dvz_axis_set_visible(axis->axis, visible != 0) != DVZ_OK)
        return _fail(axis->owner, "WASM axis visibility update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_grid(uint32_t axis_handle, uint32_t visible)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis handle");
    _clear_payload(axis->owner);
    if (dvz_axis_set_grid(axis->axis, visible != 0) != DVZ_OK)
        return _fail(axis->owner, "WASM axis grid update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_label(uint32_t axis_handle, const char* label)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL || label == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis label");
    _clear_payload(axis->owner);
    if (dvz_axis_set_label(axis->axis, label) != DVZ_OK)
        return _fail(axis->owner, "WASM axis label update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_plot_margins(
    uint32_t axis_handle, float left, float right, float bottom, float top)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis handle");
    _clear_payload(axis->owner);
    if (dvz_axis_set_plot_margins(axis->axis, left, right, bottom, top) != DVZ_OK)
        return _fail(axis->owner, "WASM axis plot margins update failed");
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
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
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
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
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
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
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
int dvz_wasm_api_visual_set_strings(
    uint32_t visual_handle, const char* attr, const char* const* strings, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        strings == NULL || item_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM string visual upload");
    }
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (strings[i] == NULL)
            return _fail(visual->owner, "invalid WASM string visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_strings(visual->visual, attr, strings, item_count) != 0)
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic),
            "WASM string visual upload failed: attr=%s item_count=%u",
            attr != NULL ? attr : "<null>", item_count);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(visual->owner, "WASM string visual upload failed");
        return _fail(visual->owner, diagnostic);
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_attr_buffer(
    uint32_t visual_handle, const char* attr, uint32_t buffer_handle, uint32_t byte_offset,
    uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    DvzWasmApiBuffer* buffer = _buffer(buffer_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        buffer == NULL || buffer->owner == NULL || buffer->buffer == NULL ||
        buffer->owner != visual->owner || item_count == 0)
    {
        return _fail(
            visual != NULL ? visual->owner : NULL, "invalid WASM visual attribute buffer bind");
    }
    _clear_payload(visual->owner);
    if (!dvz_visual_set_attr_buffer(visual->visual, attr, buffer->buffer, byte_offset, item_count))
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic),
            "WASM visual attribute buffer bind failed: attr=%s item_count=%u",
            attr != NULL ? attr : "<null>", item_count);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(visual->owner, "WASM visual attribute buffer bind failed");
        return _fail(visual->owner, diagnostic);
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_texture_rgba8(
    uint32_t visual_handle, const uint8_t* rgba, uint32_t width, uint32_t height)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL || rgba == NULL ||
        width == 0 || height == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM RGBA8 texture upload");
    }
    _clear_payload(visual->owner);
    if (_scene_visual_set_texture_rgba8(
            visual->visual, (const uint8_t*)rgba, width, height,
            (DvzSize)width * height * 4u) != 0)
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



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_labels_s32(
    uint32_t visual_handle, const int32_t* values, uint32_t width, uint32_t height,
    const int32_t* category_ids, const uint8_t* colors_rgba, uint32_t category_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->owner->scene == NULL ||
        visual->visual == NULL || values == NULL || category_ids == NULL || colors_rgba == NULL ||
        width == 0 || height == 0 || category_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM S32 labels upload");
    }
    if (dvz_labels_state(visual->visual) == NULL)
        return _fail(visual->owner, "WASM S32 labels upload requires a labels visual");
    _clear_payload(visual->owner);

    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.dim = DVZ_FIELD_DIM_2D;
    field_desc.format = DVZ_FIELD_FORMAT_R32_SINT;
    field_desc.semantic = DVZ_FIELD_SEMANTIC_LABEL;
    field_desc.color_role = DVZ_COLOR_ROLE_DATA;
    field_desc.width = width;
    field_desc.height = height;
    field_desc.depth = 1;

    DvzSampledField* field = dvz_sampled_field(visual->owner->scene, &field_desc);
    if (field == NULL)
        return _fail(visual->owner, "WASM S32 labels field creation failed");

    DvzFieldDataView view = {DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView)};
    view.data = values;
    view.bytes_per_row = (uint64_t)width * sizeof(int32_t);
    view.rows_per_image = height;
    if (dvz_sampled_field_set_data(field, &view) != DVZ_OK)
        return _fail(visual->owner, "WASM S32 labels field upload failed");
    if (dvz_visual_set_field(visual->visual, "field", field) != DVZ_OK)
        return _fail(visual->owner, "WASM S32 labels field bind failed");

    DvzScale* scale = dvz_scale(
        visual->owner->scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    if (scale == NULL)
        return _fail(visual->owner, "WASM S32 labels scale creation failed");

    DvzScaleCategory* categories =
        (DvzScaleCategory*)calloc(category_count, sizeof(DvzScaleCategory));
    if (categories == NULL)
        return _fail(visual->owner, "WASM S32 labels category allocation failed");
    for (uint32_t i = 0; i < category_count; i++)
    {
        categories[i].category_id = category_ids[i];
        categories[i].order = i;
        categories[i].color = dvz_color_rgba(
            colors_rgba[4 * i + 0], colors_rgba[4 * i + 1], colors_rgba[4 * i + 2],
            colors_rgba[4 * i + 3]);
    }
    bool ok = dvz_scale_set_categories(scale, categories, category_count) == DVZ_OK;
    free(categories);
    if (!ok)
        return _fail(visual->owner, "WASM S32 labels categories failed");
    if (dvz_visual_set_scale(visual->visual, "labels", scale) != 0)
        return _fail(visual->owner, "WASM S32 labels scale bind failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_material(
    uint32_t visual_handle, uint32_t model, float opacity, float base_r, float base_g,
    float base_b, float base_a, float light_x, float light_y, float light_z, float ambient,
    float diffuse, float specular, float shininess, float roughness, float standard_specular,
    float metallic, float emissive_r, float emissive_g, float emissive_b, float rim_strength)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM material visual handle");

    _clear_payload(visual->owner);
    DvzMaterialDesc material = model == DVZ_MATERIAL_MODEL_STANDARD ? dvz_standard_material_desc()
                               : model == DVZ_MATERIAL_MODEL_PHONG  ? dvz_phong_material_desc()
                                                                    : dvz_material_desc();
    material.model = (DvzMaterialModel)model;
    material.alpha_mode = DVZ_ALPHA_OPAQUE;
    material.opacity = opacity;
    material.base_color_factor[0] = base_r;
    material.base_color_factor[1] = base_g;
    material.base_color_factor[2] = base_b;
    material.base_color_factor[3] = base_a;
    material.light_direction[0] = light_x;
    material.light_direction[1] = light_y;
    material.light_direction[2] = light_z;
    material.phong.ambient = ambient;
    material.phong.diffuse = diffuse;
    material.phong.specular = specular;
    material.phong.shininess = shininess;
    material.standard.roughness = roughness;
    material.standard.specular = standard_specular;
    material.standard.metallic = metallic;
    material.standard.emissive[0] = emissive_r;
    material.standard.emissive[1] = emissive_g;
    material.standard.emissive[2] = emissive_b;
    material.standard.rim_strength = rim_strength;
    if (dvz_visual_set_material(visual->visual, &material) != 0)
        return _fail(visual->owner, "WASM visual material update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_segment_caps(
    uint32_t visual_handle, uint32_t start_cap, uint32_t end_cap)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(
            visual != NULL ? visual->owner : NULL, "invalid WASM segment cap visual handle");

    _clear_payload(visual->owner);
    if (dvz_segment_set_caps(visual->visual, (DvzSegmentCap)start_cap, (DvzSegmentCap)end_cap) !=
        0)
    {
        return _fail(visual->owner, "WASM segment cap update failed");
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_path_caps(uint32_t visual_handle, uint32_t start_cap, uint32_t end_cap)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM path cap visual handle");

    _clear_payload(visual->owner);
    if (dvz_path_set_caps(visual->visual, (DvzSegmentCap)start_cap, (DvzSegmentCap)end_cap) != 0)
        return _fail(visual->owner, "WASM path cap update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_path_join(uint32_t visual_handle, uint32_t join, float miter_limit)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(
            visual != NULL ? visual->owner : NULL, "invalid WASM path join visual handle");

    _clear_payload(visual->owner);
    if (dvz_path_set_join(visual->visual, (DvzPathJoin)join, miter_limit) != 0)
        return _fail(visual->owner, "WASM path join update failed");
    return 0;
}
