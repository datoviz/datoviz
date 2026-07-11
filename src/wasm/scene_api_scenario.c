/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge scenario registry                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_api_internal.h"

/*************************************************************************************************/
/*  Scenario factories                                                                           */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_annotation_readout_scenario(void);
DvzScenarioSpec dvz_example_alpha_blending_scenario(void);
DvzScenarioSpec dvz_example_animation_tracks_scenario(void);
DvzScenarioSpec dvz_example_axes_2d_scenario(void);
DvzScenarioSpec dvz_example_axis_labels_scenario(void);
DvzScenarioSpec dvz_example_bezier_curve_path_scenario(void);
DvzScenarioSpec dvz_example_basic_scene_scenario(void);
DvzScenarioSpec dvz_example_builtin_shapes_2d_scenario(void);
DvzScenarioSpec dvz_example_builtin_shapes_3d_scenario(void);
DvzScenarioSpec dvz_example_bars_bands_scenario(void);
DvzScenarioSpec dvz_example_colorbar_scenario(void);
DvzScenarioSpec dvz_example_colormap_scale_scenario(void);
DvzScenarioSpec dvz_example_compute_buffer_animation_scenario(void);
DvzScenarioSpec dvz_composite_polygon_scenario(void);
DvzScenarioSpec dvz_example_controller_fly_scenario(void);
DvzScenarioSpec dvz_example_controller_turntable_scenario(void);
DvzScenarioSpec dvz_example_guide_lines_scenario(void);
DvzScenarioSpec dvz_example_guide_spans_scenario(void);
DvzScenarioSpec dvz_example_timer_animation_scenario(void);
DvzScenarioSpec dvz_example_update_partial_scenario(void);
DvzScenarioSpec dvz_example_update_visual_data_scenario(void);
DvzScenarioSpec dvz_example_visibility_scenario(void);
DvzScenarioSpec dvz_example_picking_scenario(void);
DvzScenarioSpec dvz_example_image_probe_scenario(void);
DvzScenarioSpec dvz_example_isolines_scenario(void);
DvzScenarioSpec dvz_example_legend_categorical_scenario(void);
DvzScenarioSpec dvz_example_lighting_scenario(void);
DvzScenarioSpec dvz_example_material_mesh_scenario(void);
DvzScenarioSpec dvz_example_obj_loading_scenario(void);
DvzScenarioSpec dvz_example_overlay_card_scenario(void);
DvzScenarioSpec dvz_example_panel_background_scenario(void);
DvzScenarioSpec dvz_example_panel_grid_scenario(void);
DvzScenarioSpec dvz_example_panel_linked_scenario(void);
DvzScenarioSpec dvz_example_panel_multi_scenario(void);
DvzScenarioSpec dvz_example_panel_single_scenario(void);
DvzScenarioSpec dvz_example_panzoom_scenario(void);
DvzScenarioSpec dvz_example_path_join_scenario(void);
DvzScenarioSpec dvz_example_scalebar_scenario(void);
DvzScenarioSpec dvz_example_scalebar_units_scenario(void);
DvzScenarioSpec dvz_example_selection_mesh_instances_scenario(void);
DvzScenarioSpec dvz_example_selection_pixel_scenario(void);
DvzScenarioSpec dvz_example_selection_sphere_scenario(void);
DvzScenarioSpec dvz_example_sampled_field_update_scenario(void);
DvzScenarioSpec dvz_example_text_block_scenario(void);
DvzScenarioSpec dvz_example_depth_test_scenario(void);
DvzScenarioSpec dvz_showcase_linked_probe_colorbar_scenario(void);
DvzScenarioSpec dvz_showcase_gpu_particle_smoke_scenario(void);
DvzScenarioSpec dvz_showcase_linked_panel_axes_scenario(void);
DvzScenarioSpec dvz_showcase_protein_scenario(void);
DvzScenarioSpec dvz_showcase_scalebar_measurement_scenario(void);
DvzScenarioSpec dvz_showcase_scientific_plotting_scenario(void);
DvzScenarioSpec dvz_showcase_surface_grid_scenario(void);
DvzScenarioSpec dvz_showcase_textured_planet_scenario(void);
DvzScenarioSpec dvz_showcase_us_state_choropleth_scenario(void);
DvzScenarioSpec dvz_showcase_wind_field_scenario(void);
DvzScenarioSpec dvz_visual_glyph_scenario(void);
DvzScenarioSpec dvz_visual_image_scenario(void);
DvzScenarioSpec dvz_visual_image_rgba_scenario(void);
DvzScenarioSpec dvz_visual_labels_scenario(void);
DvzScenarioSpec dvz_visual_marker_scenario(void);
DvzScenarioSpec dvz_visual_mesh_scenario(void);
DvzScenarioSpec dvz_visual_path_scenario(void);
DvzScenarioSpec dvz_visual_pixel_scenario(void);
DvzScenarioSpec dvz_visual_point_scenario(void);
DvzScenarioSpec dvz_visual_primitive_scenario(void);
DvzScenarioSpec dvz_visual_segment_scenario(void);
DvzScenarioSpec dvz_visual_sphere_scenario(void);
DvzScenarioSpec dvz_visual_text_scenario(void);
DvzScenarioSpec dvz_visual_vector_scenario(void);



static DvzScenarioSpec _scenario_spec(uint32_t index)
{
    switch (index)
    {
    case 0:
        return dvz_example_basic_scene_scenario();
    case 1:
        return dvz_example_timer_animation_scenario();
    case 2:
        return dvz_example_builtin_shapes_2d_scenario();
    case 3:
        return dvz_example_builtin_shapes_3d_scenario();
    case 4:
        return dvz_example_isolines_scenario();
    case 5:
        return dvz_example_animation_tracks_scenario();
    case 6:
        return dvz_example_obj_loading_scenario();
    case 7:
        return dvz_example_picking_scenario();
    case 8:
        return dvz_example_selection_pixel_scenario();
    case 9:
        return dvz_example_selection_sphere_scenario();
    case 10:
        return dvz_example_selection_mesh_instances_scenario();
    case 11:
        return dvz_example_compute_buffer_animation_scenario();
    case 12:
        return dvz_example_image_probe_scenario();
    case 13:
        return dvz_example_colorbar_scenario();
    case 14:
        return dvz_example_scalebar_scenario();
    case 15:
        return dvz_example_scalebar_units_scenario();
    case 16:
        return dvz_example_legend_categorical_scenario();
    case 17:
        return dvz_example_annotation_readout_scenario();
    case 18:
        return dvz_showcase_linked_probe_colorbar_scenario();
    case 19:
        return dvz_showcase_scientific_plotting_scenario();
    case 20:
        return dvz_visual_vector_scenario();
    case 21:
        return dvz_showcase_wind_field_scenario();
    case 22:
        return dvz_showcase_gpu_particle_smoke_scenario();
    case 23:
        return dvz_example_panel_single_scenario();
    case 24:
        return dvz_example_panel_grid_scenario();
    case 25:
        return dvz_example_panzoom_scenario();
    case 26:
        return dvz_example_axes_2d_scenario();
    case 27:
        return dvz_example_axis_labels_scenario();
    case 28:
        return dvz_visual_point_scenario();
    case 29:
        return dvz_visual_pixel_scenario();
    case 30:
        return dvz_visual_marker_scenario();
    case 31:
        return dvz_visual_primitive_scenario();
    case 32:
        return dvz_visual_segment_scenario();
    case 33:
        return dvz_visual_path_scenario();
    case 34:
        return dvz_visual_image_scenario();
    case 35:
        return dvz_visual_image_rgba_scenario();
    case 36:
        return dvz_visual_mesh_scenario();
    case 37:
        return dvz_visual_sphere_scenario();
    case 38:
        return dvz_visual_text_scenario();
    case 39:
        return dvz_visual_glyph_scenario();
    case 40:
        return dvz_visual_labels_scenario();
    case 41:
        return dvz_example_panel_multi_scenario();
    case 42:
        return dvz_example_panel_linked_scenario();
    case 43:
        return dvz_example_text_block_scenario();
    case 44:
        return dvz_example_overlay_card_scenario();
    case 45:
        return dvz_example_guide_lines_scenario();
    case 46:
        return dvz_example_guide_spans_scenario();
    case 47:
        return dvz_example_bars_bands_scenario();
    case 48:
        return dvz_example_controller_fly_scenario();
    case 49:
        return dvz_example_controller_turntable_scenario();
    case 50:
        return dvz_example_sampled_field_update_scenario();
    case 51:
        return dvz_example_colormap_scale_scenario();
    case 52:
        return dvz_example_panel_background_scenario();
    case 53:
        return dvz_composite_polygon_scenario();
    case 54:
        return dvz_showcase_linked_panel_axes_scenario();
    case 55:
        return dvz_showcase_scalebar_measurement_scenario();
    case 56:
        return dvz_showcase_surface_grid_scenario();
    case 57:
        return dvz_showcase_us_state_choropleth_scenario();
    case 58:
        return dvz_example_update_partial_scenario();
    case 59:
        return dvz_example_update_visual_data_scenario();
    case 60:
        return dvz_example_visibility_scenario();
    case 61:
        return dvz_example_depth_test_scenario();
    case 62:
        return dvz_example_alpha_blending_scenario();
    case 63:
        return dvz_example_material_mesh_scenario();
    case 64:
        return dvz_example_lighting_scenario();
    case 65:
        return dvz_showcase_textured_planet_scenario();
    case 66:
        return dvz_showcase_protein_scenario();
    case 67:
        return dvz_example_bezier_curve_path_scenario();
    case 68:
        return dvz_example_path_join_scenario();
    default:
        return (DvzScenarioSpec){0};
    }
}



static const char* _requirement_name(uint64_t bit)
{
    switch (bit)
    {
    case DVZ_SCENARIO_REQ_POINT_VISUAL:
        return "point";
    case DVZ_SCENARIO_REQ_PIXEL_VISUAL:
        return "pixel";
    case DVZ_SCENARIO_REQ_MARKER_VISUAL:
        return "marker";
    case DVZ_SCENARIO_REQ_MESH_VISUAL:
        return "mesh";
    case DVZ_SCENARIO_REQ_IMAGE_VISUAL:
        return "image";
    case DVZ_SCENARIO_REQ_TEXT_VISUAL:
        return "text";
    case DVZ_SCENARIO_REQ_SCENE_BUFFERS:
        return "scene-buffers";
    case DVZ_SCENARIO_REQ_STORAGE_BUFFERS:
        return "storage-buffers";
    case DVZ_SCENARIO_REQ_SCENE_COMPUTE:
        return "scene-compute";
    case DVZ_SCENARIO_REQ_QUERY_READBACK:
        return "query-readback";
    case DVZ_SCENARIO_REQ_FRAME_CALLBACKS:
        return "frame-callbacks";
    case DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES:
        return "continuous-frames";
    case DVZ_SCENARIO_REQ_NATIVE_CAPTURE:
        return "native-capture";
    case DVZ_SCENARIO_REQ_NATIVE_VIEW:
        return "native-view";
    case DVZ_SCENARIO_REQ_CONTROLLER:
        return "controller";
    case DVZ_SCENARIO_REQ_PANZOOM:
        return "panzoom";
    case DVZ_SCENARIO_REQ_ARCBALL:
        return "arcball";
    default:
        return "unknown";
    }
}



static uint64_t _scenario_effective_requirements(const DvzScenarioSpec* spec)
{
    uint64_t requirements = spec != NULL ? spec->requirements : 0;
    if (spec != NULL && spec->continuous_frames)
        requirements |= DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES;
    if (spec != NULL && spec->frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->post_frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->native_view != NULL)
        requirements |= DVZ_SCENARIO_REQ_NATIVE_VIEW;
    return requirements;
}



static int _fail_unsupported_requirements(
    DvzWasmApiScene* scene, const DvzScenarioSpec* spec, uint64_t unsupported)
{
    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
    const char* scenario_id = spec != NULL && spec->id != NULL ? spec->id : "<unknown>";
    int written = snprintf(
        diagnostic, sizeof(diagnostic),
        "WASM scenario '%s' has unsupported requirements: ", scenario_id);
    if (written < 0 || (size_t)written >= sizeof(diagnostic))
        return _fail(scene, "WASM scenario has unsupported requirements");

    size_t offset = (size_t)written;
    bool first = true;
    for (uint32_t bit_index = 0; bit_index < 64; bit_index++)
    {
        const uint64_t bit = 1ull << bit_index;
        if ((unsupported & bit) == 0)
            continue;
        const char* name = _requirement_name(bit);
        written = snprintf(
            diagnostic + offset, sizeof(diagnostic) - offset, "%s%s", first ? "" : ",", name);
        if (written < 0 || (size_t)written >= sizeof(diagnostic) - offset)
            return _fail(scene, "WASM scenario has unsupported requirements");
        offset += (size_t)written;
        first = false;
    }
    return _fail(scene, diagnostic);
}
static DvzScenarioPointerType _scenario_pointer_type_from_wasm(DvzPointerEventType type)
{
    switch (type)
    {
    case DVZ_POINTER_EVENT_RELEASE:
        return DVZ_SCENARIO_POINTER_RELEASE;
    case DVZ_POINTER_EVENT_PRESS:
        return DVZ_SCENARIO_POINTER_PRESS;
    case DVZ_POINTER_EVENT_MOVE:
        return DVZ_SCENARIO_POINTER_MOVE;
    case DVZ_POINTER_EVENT_CLICK:
        return DVZ_SCENARIO_POINTER_CLICK;
    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        return DVZ_SCENARIO_POINTER_DOUBLE_CLICK;
    case DVZ_POINTER_EVENT_DRAG_START:
        return DVZ_SCENARIO_POINTER_DRAG_START;
    case DVZ_POINTER_EVENT_DRAG:
        return DVZ_SCENARIO_POINTER_DRAG;
    case DVZ_POINTER_EVENT_DRAG_STOP:
        return DVZ_SCENARIO_POINTER_DRAG_STOP;
    case DVZ_POINTER_EVENT_WHEEL:
        return DVZ_SCENARIO_POINTER_WHEEL;
    default:
        return DVZ_SCENARIO_POINTER_NONE;
    }
}



static int _connect_scenario_controller_bindings(DvzWasmApiScene* scene)
{
    if (scene == NULL || scene->router == NULL)
        return _fail(scene, "invalid WASM scenario input router");
    if (scene->scenario_ctx.controller_binding_count == 0)
        return 0;

    for (uint32_t i = 0; i < scene->scenario_ctx.controller_binding_count; i++)
    {
        DvzPanel* panel = scene->scenario_ctx.controller_bindings[i].panel;
        if (panel == NULL)
            return _fail(scene, "invalid WASM scenario controller binding");
        if (dvz_panel_connect_input(panel, scene->router) != 0)
            return _fail(scene, "WASM scenario panel input connection failed");
    }

    if (scene->gestures == NULL)
    {
        scene->gestures = dvz_pointer_gesture_handler(scene->router);
        if (scene->gestures == NULL)
            return _fail(scene, "WASM scenario pointer gesture setup failed");
    }
    return 0;
}
EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_count(void) { return DVZ_WASM_API_SCENARIO_COUNT; }



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_id(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return _handle((void*)spec.id);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_title(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return _handle((void*)spec.title);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_width(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return spec.width;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_height(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return spec.height;
}



EMSCRIPTEN_KEEPALIVE
double dvz_wasm_api_scenario_fps(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return spec.fps;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_requirements(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    const uint64_t requirements = _scenario_effective_requirements(&spec);
    return requirements <= UINT32_MAX ? (uint32_t)requirements : 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_create(uint32_t scene_handle, uint32_t index)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return _fail(scene, "invalid WASM scenario scene handle");
    if (scene->scenario_active || scene->wrapper_count != 0)
        return _fail(scene, "WASM scenario must be the first object created in a scene");

    DvzScenarioSpec spec = _scenario_spec(index);
    if (spec.id == NULL || spec.init == NULL)
        return _fail(scene, "invalid WASM scenario index");

    const uint64_t requirements = _scenario_effective_requirements(&spec);
    const uint64_t unsupported = requirements & ~DVZ_WASM_BROWSER_SUPPORTED_REQUIREMENTS;
    if (unsupported != 0)
        return _fail_unsupported_requirements(scene, &spec, unsupported);

    _clear_payload(scene);
    scene->scenario_spec = spec;
    if (scene->width == 0)
        scene->width = spec.width != 0 ? spec.width : 640;
    if (scene->height == 0)
        scene->height = spec.height != 0 ? spec.height : 640;
    if (spec.fps > 0)
    {
        dvz_scene_set_clock_mode(scene->scene, DVZ_SCENE_CLOCK_EXTERNAL);
        dvz_scene_set_fps(scene->scene, spec.fps);
    }
    scene->scenario_ctx = (DvzScenarioContext){
        .scene = scene->scene,
        .logical_width = scene->width,
        .logical_height = scene->height,
        .framebuffer_width = scene->width,
        .framebuffer_height = scene->height,
        .device_scale = 1.0f,
        .user_scale = 1.0f,
        .render_scale = 1.0f,
        .width = scene->width,
        .height = scene->height,
    };

    if (!spec.init(&scene->scenario_ctx, &scene->scenario_user) ||
        scene->scenario_ctx.figure == NULL)
    {
        scene->scenario_user = NULL;
        return _fail(scene, "WASM scenario init failed");
    }
    if (_connect_scenario_controller_bindings(scene) != 0)
    {
        if (spec.destroy != NULL)
            spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        return -1;
    }

    DvzWasmApiFigure* figure = (DvzWasmApiFigure*)calloc(1, sizeof(DvzWasmApiFigure));
    if (figure == NULL)
    {
        if (spec.destroy != NULL)
            spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        return _fail(scene, "WASM scenario figure wrapper allocation failed");
    }
    figure->owner = scene;
    figure->figure = scene->scenario_ctx.figure;
    if (!_remember(scene, figure))
    {
        free(figure);
        if (spec.destroy != NULL)
            spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        return _fail(scene, "WASM scenario figure wrapper registration failed");
    }
    scene->scenario_figure = figure;
    scene->scenario_active = true;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_figure(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active || scene->scenario_figure == NULL)
        return _fail_handle(scene, "WASM scenario has no active figure");
    return _handle(scene->scenario_figure);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_frame(uint32_t scene_handle, double t, double dt)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario frame requested without an active scenario");
    _clear_payload(scene);
    scene->scenario_ctx.time = t;
    scene->scenario_ctx.dt = dt;
    dvz_scene_step_external(scene->scene, t, dt);
    if (scene->scenario_spec.frame != NULL)
        scene->scenario_spec.frame(&scene->scenario_ctx, scene->scenario_user);
    scene->scenario_ctx.frame_index++;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_post_frame(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario post-frame requested without an active scenario");
    if (scene->scenario_spec.post_frame == NULL)
        return 0;
    scene->scenario_spec.post_frame(&scene->scenario_ctx, scene->scenario_user);
    return 0;
}


/**
 * Convert one WASM scenario pointer position to figure layout coordinates.
 *
 * @param scene WASM scene wrapper
 * @param x input host/canvas logical x coordinate
 * @param y input host/canvas logical y coordinate
 * @param content_scale input content scale
 * @param out_x output figure-layout x coordinate
 * @param out_y output figure-layout y coordinate
 */
static void _wasm_scenario_pointer_to_figure(
    DvzWasmApiScene* scene, float x, float y, float content_scale, float* out_x, float* out_y)
{
    ANN(out_x);
    ANN(out_y);
    *out_x = x;
    *out_y = y;
    if (scene == NULL || scene->scenario_ctx.figure == NULL)
        return;

    DvzInputResizeEvent resize = {0};
    const bool has_resize =
        scene->router != NULL && dvz_input_router_last_resize(scene->router, &resize);
    const uint32_t logical_width = scene->scenario_ctx.logical_width != 0
                                       ? scene->scenario_ctx.logical_width
                                       : scene->scenario_ctx.width;
    const uint32_t logical_height = scene->scenario_ctx.logical_height != 0
                                        ? scene->scenario_ctx.logical_height
                                        : scene->scenario_ctx.height;
    const float window_width =
        has_resize && resize.window_width > 0 ? (float)resize.window_width : (float)logical_width;
    const float window_height = has_resize && resize.window_height > 0
                                    ? (float)resize.window_height
                                    : (float)logical_height;
    const float content_scale_x =
        has_resize && resize.content_scale_x > 0.0f ? resize.content_scale_x : content_scale;
    const float content_scale_y =
        has_resize && resize.content_scale_y > 0.0f ? resize.content_scale_y : content_scale;
    (void)dvz_figure_window_to_layout(
        scene->scenario_ctx.figure, x, y, window_width, window_height, content_scale_x,
        content_scale_y, out_x, out_y);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_pointer(
    uint32_t scene_handle, int type, float x, float y, int button, int mods, float content_scale,
    double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario pointer requested without an active scenario");
    if (scene->scenario_spec.event == NULL)
        return 0;

    _clear_payload(scene);
    DvzScenarioEvent event = {0};
    event.kind = DVZ_SCENARIO_EVENT_POINTER;
    event.content.pointer.type = _scenario_pointer_type_from_wasm((DvzPointerEventType)type);
    _wasm_scenario_pointer_to_figure(
        scene, x, y, content_scale > 0.0f ? content_scale : 1.0f, &event.content.pointer.x,
        &event.content.pointer.y);
    event.content.pointer.content_scale = content_scale > 0.0f ? content_scale : 1.0f;
    event.content.pointer.button = button >= 0 ? (uint32_t)button : 0;
    event.content.pointer.modifiers = mods >= 0 ? (uint32_t)mods : 0;
    event.content.pointer.timestamp_ns =
        timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    scene->scenario_spec.event(&scene->scenario_ctx, &event, scene->scenario_user);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_wheel(
    uint32_t scene_handle, float x, float y, float dir_x, float dir_y, int mods,
    float content_scale, double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario wheel requested without an active scenario");
    if (scene->scenario_spec.event == NULL)
        return 0;

    _clear_payload(scene);
    DvzScenarioEvent event = {0};
    event.kind = DVZ_SCENARIO_EVENT_POINTER;
    event.content.pointer.type = DVZ_SCENARIO_POINTER_WHEEL;
    _wasm_scenario_pointer_to_figure(
        scene, x, y, content_scale > 0.0f ? content_scale : 1.0f, &event.content.pointer.x,
        &event.content.pointer.y);
    event.content.pointer.dx = dir_x;
    event.content.pointer.dy = dir_y;
    event.content.pointer.content_scale = content_scale > 0.0f ? content_scale : 1.0f;
    event.content.pointer.modifiers = mods >= 0 ? (uint32_t)mods : 0;
    event.content.pointer.timestamp_ns =
        timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    scene->scenario_spec.event(&scene->scenario_ctx, &event, scene->scenario_user);
    return 0;
}
