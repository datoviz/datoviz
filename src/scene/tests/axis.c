/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene axis tests                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../../drp2/_stream.h"
#include "_scene.h"
#include "annotation/prepare_internal.h"
#include "core/figure_emit_internal.h"
#include "scene_emit/scene_emit.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create and bind a scene-owned panzoom for axis tests.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the panzoom payload, or NULL
 */
static DvzPanzoom* _axis_test_bind_panzoom(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);
    DvzController* controller = dvz_panzoom(scene, NULL);
    if (controller == NULL)
        return NULL;
    if (dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) != 0)
        return NULL;
    return dvz_controller_panzoom(controller);
}


/**
 * Return the first draw vertex count in a command stream.
 *
 * @param stream the command stream
 * @return the draw vertex count, or 0
 */
static uint32_t _axis_test_draw_vertex_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            return cmd->u.draw.vertex_count;
    }
    return 0;
}


/**
 * Return one retained visual attribute dirty item count.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return dirty item count, or UINT64_MAX when the attribute is absent
 */
static uint64_t _axis_test_attr_dirty_item_count(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, attr_name) == 0)
            return visual->attrs[i].dirty_item_count;
    }
    return UINT64_MAX;
}


/**
 * Return the number of axis decoration rectangles with one color.
 *
 * @param axis the axis
 * @param color the rectangle color
 * @return rectangle count
 */
static uint32_t _axis_test_rect_color_count(DvzAxis* axis, const uint8_t color[4])
{
    ANN(axis);
    ANN(axis->visual);
    ANN(color);
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->visual, "color", &colors_view);
    ASSERT(res == 0);
    const uint8_t* colors = (const uint8_t*)colors_view.data;
    uint32_t count = 0;
    for (uint32_t i = 0; i + 5 < colors_view.item_count; i += 6)
    {
        if (memcmp(&colors[4 * i], color, 4) == 0)
            count++;
    }
    return count;
}


/**
 * Return the number of inward major tick rectangles emitted by one axis visual.
 *
 * @param axis the axis
 * @return inward tick line count
 */
static uint32_t _axis_test_inward_tick_line_count(DvzAxis* axis)
{
    ANN(axis);
    return _axis_test_rect_color_count(axis, axis->style.major_tick_color);
}


/**
 * Return the number of inward minor tick lines emitted by one axis visual.
 *
 * @param axis the axis
 * @return inward minor tick line count
 */
static uint32_t _axis_test_inward_minor_tick_line_count(DvzAxis* axis)
{
    ANN(axis);
    return _axis_test_rect_color_count(axis, axis->style.minor_tick_color);
}


/**
 * Return the number of vertical grid lines emitted by one X axis visual.
 *
 * @param axis the X axis
 * @return vertical grid line count
 */
static uint32_t _axis_test_vertical_grid_line_count(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->grid_visual);
    DvzVisualDataView positions_view = {0};
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->grid_visual, "position", &positions_view);
    ASSERT(res == 0);
    res = dvz_visual_data(axis->grid_visual, "color", &colors_view);
    ASSERT(res == 0);
    const float* positions = (const float*)positions_view.data;
    const uint8_t* colors = (const uint8_t*)colors_view.data;
    uint32_t count = 0;
    for (uint32_t i = 0; i + 5 < positions_view.item_count; i += 6)
    {
        if (memcmp(&colors[4 * i], axis->style.grid_color, 4) != 0)
            continue;
        float min_y = positions[3 * i + 1];
        float max_y = positions[3 * i + 1];
        for (uint32_t j = 1; j < 6; j++)
        {
            min_y = fminf(min_y, positions[3 * (i + j) + 1]);
            max_y = fmaxf(max_y, positions[3 * (i + j) + 1]);
        }
        float min_x = positions[3 * i + 0];
        float max_x = positions[3 * i + 0];
        for (uint32_t j = 1; j < 6; j++)
        {
            min_x = fminf(min_x, positions[3 * (i + j) + 0]);
            max_x = fmaxf(max_x, positions[3 * (i + j) + 0]);
        }
        float span_x = max_x - min_x;
        float span_y = max_y - min_y;
        if (span_x <= span_y && span_y > 1e-6f)
            count++;
    }
    return count;
}


/**
 * Return the number of horizontal grid lines emitted by one Y axis visual.
 *
 * @param axis the Y axis
 * @return horizontal grid line count
 */
static uint32_t _axis_test_horizontal_grid_line_count(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->grid_visual);
    DvzVisualDataView positions_view = {0};
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->grid_visual, "position", &positions_view);
    ASSERT(res == 0);
    res = dvz_visual_data(axis->grid_visual, "color", &colors_view);
    ASSERT(res == 0);
    const float* positions = (const float*)positions_view.data;
    const uint8_t* colors = (const uint8_t*)colors_view.data;
    uint32_t count = 0;
    for (uint32_t i = 0; i + 5 < positions_view.item_count; i += 6)
    {
        if (memcmp(&colors[4 * i], axis->style.grid_color, 4) != 0)
            continue;
        float min_x = positions[3 * i + 0];
        float max_x = positions[3 * i + 0];
        for (uint32_t j = 1; j < 6; j++)
        {
            min_x = fminf(min_x, positions[3 * (i + j) + 0]);
            max_x = fmaxf(max_x, positions[3 * (i + j) + 0]);
        }
        float min_y = positions[3 * i + 1];
        float max_y = positions[3 * i + 1];
        for (uint32_t j = 1; j < 6; j++)
        {
            min_y = fminf(min_y, positions[3 * (i + j) + 1]);
            max_y = fmaxf(max_y, positions[3 * (i + j) + 1]);
        }
        float span_x = max_x - min_x;
        float span_y = max_y - min_y;
        if (span_y <= span_x && span_x > 1e-6f)
            count++;
    }
    return count;
}


/**
 * Return whether a vertical grid line exists near one expected center.
 *
 * @param axis the X axis
 * @param expected_x expected source visual x coordinate
 * @param tolerance accepted absolute visual-coordinate distance
 * @param out_x closest matching grid-line center, or NULL
 * @return whether a matching vertical grid line was found
 */
static bool _axis_test_find_vertical_grid_center(
    DvzAxis* axis, float expected_x, float tolerance, float* out_x)
{
    ANN(axis);
    ANN(axis->grid_visual);
    DvzVisualDataView positions_view = {0};
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->grid_visual, "position", &positions_view);
    ASSERT(res == 0);
    res = dvz_visual_data(axis->grid_visual, "color", &colors_view);
    ASSERT(res == 0);
    const float* positions = (const float*)positions_view.data;
    const uint8_t* colors = (const uint8_t*)colors_view.data;
    float best_x = 0.0f;
    float best_distance = 1e9f;
    bool found = false;
    for (uint32_t i = 0; i + 5 < positions_view.item_count; i += 6)
    {
        if (memcmp(&colors[4 * i], axis->style.grid_color, 4) != 0)
            continue;
        float min_x = positions[3 * i + 0];
        float max_x = positions[3 * i + 0];
        float min_y = positions[3 * i + 1];
        float max_y = positions[3 * i + 1];
        for (uint32_t j = 1; j < 6; j++)
        {
            min_x = fminf(min_x, positions[3 * (i + j) + 0]);
            max_x = fmaxf(max_x, positions[3 * (i + j) + 0]);
            min_y = fminf(min_y, positions[3 * (i + j) + 1]);
            max_y = fmaxf(max_y, positions[3 * (i + j) + 1]);
        }
        float span_x = max_x - min_x;
        float span_y = max_y - min_y;
        if (span_x > span_y || span_y <= 1e-6f)
            continue;
        float center_x = 0.5f * (min_x + max_x);
        float distance = fabsf(center_x - expected_x);
        if (distance < best_distance)
        {
            best_distance = distance;
            best_x = center_x;
            found = true;
        }
    }
    if (out_x != NULL)
        *out_x = best_x;
    return found && best_distance <= tolerance;
}


/**
 * Return whether a horizontal grid line exists near one expected center.
 *
 * @param axis the Y axis
 * @param expected_y expected source visual y coordinate
 * @param tolerance accepted absolute visual-coordinate distance
 * @param out_y closest matching grid-line center, or NULL
 * @return whether a matching horizontal grid line was found
 */
static bool _axis_test_find_horizontal_grid_center(
    DvzAxis* axis, float expected_y, float tolerance, float* out_y)
{
    ANN(axis);
    ANN(axis->grid_visual);
    DvzVisualDataView positions_view = {0};
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->grid_visual, "position", &positions_view);
    ASSERT(res == 0);
    res = dvz_visual_data(axis->grid_visual, "color", &colors_view);
    ASSERT(res == 0);
    const float* positions = (const float*)positions_view.data;
    const uint8_t* colors = (const uint8_t*)colors_view.data;
    float best_y = 0.0f;
    float best_distance = 1e9f;
    bool found = false;
    for (uint32_t i = 0; i + 5 < positions_view.item_count; i += 6)
    {
        if (memcmp(&colors[4 * i], axis->style.grid_color, 4) != 0)
            continue;
        float min_x = positions[3 * i + 0];
        float max_x = positions[3 * i + 0];
        float min_y = positions[3 * i + 1];
        float max_y = positions[3 * i + 1];
        for (uint32_t j = 1; j < 6; j++)
        {
            min_x = fminf(min_x, positions[3 * (i + j) + 0]);
            max_x = fmaxf(max_x, positions[3 * (i + j) + 0]);
            min_y = fminf(min_y, positions[3 * (i + j) + 1]);
            max_y = fmaxf(max_y, positions[3 * (i + j) + 1]);
        }
        float span_x = max_x - min_x;
        float span_y = max_y - min_y;
        if (span_y > span_x || span_x <= 1e-6f)
            continue;
        float center_y = 0.5f * (min_y + max_y);
        float distance = fabsf(center_y - expected_y);
        if (distance < best_distance)
        {
            best_distance = distance;
            best_y = center_y;
            found = true;
        }
    }
    if (out_y != NULL)
        *out_y = best_y;
    return found && best_distance <= tolerance;
}


/**
 * Project one visual-space position with the same MVP uploaded for APPLY visuals.
 *
 * @param mvp panel MVP
 * @param position visual-space position
 * @param dim output coordinate dimension
 * @param out_coord projected coordinate before Vulkan Y/depth correction
 * @return whether the coordinate was written
 */
static bool _axis_test_apply_mvp_coord(
    DvzMVP* mvp, const vec3 position, DvzDim dim, float* out_coord)
{
    ANN(mvp);
    ANN(out_coord);
    uint32_t coord = (uint32_t)dim;
    if (coord > (uint32_t)DVZ_DIM_Z)
        return false;

    vec4 p = {position[0], position[1], position[2], 1.0f};
    vec4 tmp0 = {0};
    vec4 tmp1 = {0};
    vec4 clip = {0};
    glm_mat4_mulv(mvp->model, p, tmp0);
    glm_mat4_mulv(mvp->view, tmp0, tmp1);
    glm_mat4_mulv(mvp->proj, tmp1, clip);
    if (!isfinite(clip[coord]) || !isfinite(clip[3]) || fabsf(clip[3]) <= 1e-12f)
        return false;
    *out_coord = clip[coord] / clip[3];
    return isfinite(*out_coord);
}


/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_axis_domain_and_ticks(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(dvz_axis_set_grid(axis, true));
    AT(axis->enabled);
    AT(axis->visual != NULL);
    AT(axis->visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(axis->grid_visual != NULL);
    AT(axis->grid_visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(panel->visual_count == 2);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);

    _scene_prepare_axis_visuals(figure);
    AT(axis->tick_count >= 5);
    AT(axis->visual->visible);
    AT(axis->visual->attrs[0].item_count > 0);
    AT(_axis_test_inward_tick_line_count(axis) >= 8);
    AT(_axis_test_inward_minor_tick_line_count(axis) > 0);

    AT(dvz_axis_set_grid(axis, true));
    _scene_prepare_axis_visuals(figure);
    AT(axis->grid_visual->visible);
    AT(axis->grid_visual->attrs[0].item_count > 0);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_minor_ticks(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);

    _scene_prepare_axis_visuals(figure);
    uint32_t minor_count = _axis_test_inward_minor_tick_line_count(axis);
    AT(minor_count > 0);

    DvzAxisStyle style = axis->style;
    style.show_minor_ticks = false;
    AT(dvz_axis_set_style(axis, &style));
    _scene_prepare_axis_visuals(figure);
    AT(_axis_test_inward_minor_tick_line_count(axis) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_tick_density_tracks_panel_size(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* narrow_scene = dvz_scene();
    ANN(narrow_scene);
    DvzFigure* narrow_figure = dvz_figure(narrow_scene, 300, 400, 0);
    ANN(narrow_figure);
    DvzPanel* narrow_panel = dvz_panel(narrow_figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(narrow_panel);
    AT(dvz_panel_set_domain(narrow_panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* narrow_axis = dvz_panel_axis(narrow_panel, DVZ_DIM_X);
    ANN(narrow_axis);
    _scene_prepare_axis_visuals(narrow_figure);
    uint32_t narrow_count = _axis_test_inward_tick_line_count(narrow_axis);

    DvzScene* wide_scene = dvz_scene();
    ANN(wide_scene);
    DvzFigure* wide_figure = dvz_figure(wide_scene, 1300, 400, 0);
    ANN(wide_figure);
    DvzPanel* wide_panel = dvz_panel(wide_figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(wide_panel);
    AT(dvz_panel_set_domain(wide_panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* wide_axis = dvz_panel_axis(wide_panel, DVZ_DIM_X);
    ANN(wide_axis);
    _scene_prepare_axis_visuals(wide_figure);
    uint32_t wide_count = _axis_test_inward_tick_line_count(wide_axis);

    AT(wide_count > narrow_count);
    AT(wide_count >= 8);

    dvz_figure_resize(wide_figure, 300, 400);
    _scene_prepare_axis_visuals(wide_figure);
    uint32_t resized_count = _axis_test_inward_tick_line_count(wide_axis);
    AT(resized_count < wide_count);
    AT(resized_count == narrow_count);

    dvz_scene_destroy(wide_scene);
    dvz_scene_destroy(narrow_scene);
    return 0;
}


static int test_axis_text_labels(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(dvz_axis_set_label(axis, "Time"));

    _scene_prepare_axis_visuals(figure);
    ANN(axis->text_visual);
    AT(axis->text_visual->type == DVZ_VISUAL_TYPE_TEXT);
    AT(axis->text_visual->visible);
    AT(_visual_family_state(axis->text_visual)->text.string_count == axis->tick_count + 1);
    AT(strcmp(_visual_family_state(axis->text_visual)->text.strings[axis->tick_count], "Time") == 0);
    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(axis->text_visual, "position", &position_view) == 0);
    AT(position_view.item_count == _visual_family_state(axis->text_visual)->text.string_count);

    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(axis->text_visual)->text.glyph_visual != NULL);
    AT(_visual_family_state(axis->text_visual)->text.glyph_visual->visible);

    AT(dvz_axis_set_visible(axis, false));
    _scene_prepare_axis_visuals(figure);
    AT(!axis->text_visual->visible);
    _scene_prepare_text_visuals(figure);
    AT(!_visual_family_state(axis->text_visual)->text.glyph_visual->visible);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_numeric_unit_labels(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 5000.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    DvzUnits* length = dvz_units_builtin(scene, DVZ_UNIT_LADDER_METRIC_LENGTH, 1e-6);
    ANN(length);
    AT(dvz_axis_set_units(axis, length));

    _scene_prepare_axis_visuals(figure);
    ANN(axis->text_visual);
    uint32_t string_count = _visual_family_state(axis->text_visual)->text.string_count;
    bool saw_mm = false;
    bool saw_um = false;
    for (uint32_t i = 0; i < string_count; i++)
    {
        const char* string = _visual_family_state(axis->text_visual)->text.strings[i];
        if (strcmp(string, "1 mm") == 0)
            saw_mm = true;
        if (strstr(string, "um") != NULL)
            saw_um = true;
    }
    AT(saw_mm);
    AT(!saw_um);

    AT(dvz_axis_set_units(axis, NULL));
    _scene_prepare_axis_visuals(figure);
    string_count = _visual_family_state(axis->text_visual)->text.string_count;
    bool saw_plain = false;
    for (uint32_t i = 0; i < string_count; i++)
    {
        const char* string = _visual_family_state(axis->text_visual)->text.strings[i];
        if (strcmp(string, "1000") == 0)
            saw_plain = true;
    }
    AT(saw_plain);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_text_hidpi_scales_glyph_bounds(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    DvzAxisStyle style = axis->style;
    style.tick_size_px = 18.0f;
    style.label_size_px = 22.0f;
    AT(dvz_axis_set_style(axis, &style));
    AT(dvz_axis_set_label(axis, "Time"));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.device_scale_x = 2.0f;
    cfg.device_scale_y = 2.0f;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    ANN(axis->text_visual);
    AT(_visual_family_state(axis->text_visual)->text.glyph_visual != NULL);

    DvzVisualDataView bounds_view = {0};
    AT(dvz_visual_data(_visual_family_state(axis->text_visual)->text.glyph_visual, "bounds", &bounds_view) == 0);
    const float* bounds = (const float*)bounds_view.data;
    ANN(bounds);
    float max_extent = 0.0f;
    for (uint64_t i = 0; i < bounds_view.item_count; i++)
    {
        max_extent = fmaxf(max_extent, fabsf(bounds[4 * i + 0]));
        max_extent = fmaxf(max_extent, fabsf(bounds[4 * i + 1]));
        max_extent = fmaxf(max_extent, fabsf(bounds[4 * i + 2]));
        max_extent = fmaxf(max_extent, fabsf(bounds[4 * i + 3]));
    }
    AT(max_extent > 30.0f);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_text_renderer_style(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);

    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(axis->style.text_renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);
    _scene_prepare_axis_visuals(figure);
    ANN(axis->text_visual);
    AT(_visual_family_state(axis->text_visual)->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);

    DvzAxisStyle style = axis->style;
    style.text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    AT(dvz_axis_set_style(axis, &style));
    _scene_prepare_axis_visuals(figure);
    AT(_visual_family_state(axis->text_visual)->text.renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS);

    style.text_renderer = DVZ_TEXT_RENDERER_VECTOR_GPU;
    AT(dvz_axis_set_style(axis, &style));
    _scene_prepare_axis_visuals(figure);
    AT(_visual_family_state(axis->text_visual)->text.renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_text_updates_after_domain_change(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 100.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    _scene_prepare_axis_visuals(figure);
    ANN(axis->text_visual);
    AT(_visual_family_state(axis->text_visual)->text.string_count >= 2);
    char second_before[DVZ_SCENE_LABEL_SIZE] = {0};
    dvz_strlcpy(second_before, _visual_family_state(axis->text_visual)->text.strings[1], sizeof(second_before));

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    _scene_prepare_axis_visuals(figure);
    AT(_visual_family_state(axis->text_visual)->text.string_count >= 2);
    AT(strcmp(_visual_family_state(axis->text_visual)->text.strings[1], second_before) != 0);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_text_layout_reserve(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    _scene_prepare_axis_visuals(figure);
    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(axis->text_visual, "position", &position_view) == 0);
    const float* positions = (const float*)position_view.data;
    float y_before = positions[1];

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.0f, .right = 0.0f, .bottom = 0.20f,
                                        .top = 0.0f}));
    _scene_prepare_axis_visuals(figure);
    AT(dvz_visual_data(axis->text_visual, "position", &position_view) == 0);
    positions = (const float*)position_view.data;
    float y_after = positions[1];
    AT(y_after < y_before);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure axis text for inset panels uses panel-local pixels and viewport-local clip anchors.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
static int test_axis_text_inset_panel_coordinates(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1000, 700, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.08f, .y = 0.06f, .width = 0.86f, .height = 0.86f});
    ANN(panel);

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.04f, .bottom = 0.18f,
                                        .top = 0.04f}));
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, -5.0, +5.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -5.0, +5.0) == 0);
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(x_axis);
    ANN(y_axis);

    _scene_prepare_axis_visuals(figure);
    ANN(x_axis->text_visual);
    ANN(y_axis->text_visual);
    char* x_tick_end = NULL;
    double x_tick = strtod(_visual_family_state(x_axis->text_visual)->text.strings[0], &x_tick_end);
    AT(x_tick_end != _visual_family_state(x_axis->text_visual)->text.strings[0]);
    char* y_tick_end = NULL;
    double y_tick = strtod(_visual_family_state(y_axis->text_visual)->text.strings[0], &y_tick_end);
    AT(y_tick_end != _visual_family_state(y_axis->text_visual)->text.strings[0]);

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;

    DvzVisualDataView x_position_view = {0};
    DvzVisualDataView y_position_view = {0};
    AT(dvz_visual_data(x_axis->text_visual, "position", &x_position_view) == 0);
    AT(dvz_visual_data(y_axis->text_visual, "position", &y_position_view) == 0);
    const float* x_positions = (const float*)x_position_view.data;
    const float* y_positions = (const float*)y_position_view.data;
    const float tick_gap = 6.0f;
    double x_visible_min = 0.0;
    double x_visible_max = 0.0;
    double y_visible_min = 0.0;
    double y_visible_max = 0.0;
    AT(dvz_panel_visible_domain(panel, DVZ_DIM_X, &x_visible_min, &x_visible_max));
    AT(dvz_panel_visible_domain(panel, DVZ_DIM_Y, &y_visible_min, &y_visible_max));
    const float x_plot_min = -1.0f + 0.14f;
    const float x_plot_max = +1.0f - 0.04f;
    const float x_visual =
        x_plot_min + (float)((x_tick - x_visible_min) / (x_visible_max - x_visible_min)) *
                         (x_plot_max - x_plot_min);
    const float expected_x0 = 0.5f * (x_visual + 1.0f) * panel_width;
    const float expected_left_x = 0.5f * (x_plot_min + 1.0f) * panel_width;
    const float y_plot_min = -1.0f + 0.18f;
    const float y_plot_max = +1.0f - 0.04f;
    const float y_visual =
        y_plot_min + (float)((y_tick - y_visible_min) / (y_visible_max - y_visible_min)) *
                         (y_plot_max - y_plot_min);
    const float expected_y0 = 0.5f * (1.0f - y_visual) * panel_height;
    const float expected_x_tick_y = 0.5f * (1.0f - y_plot_min) * panel_height;
    AC(x_positions[0], expected_x0, 1e-3f);
    AC(x_positions[1], expected_x_tick_y + tick_gap, 1e-3f);
    AC(y_positions[0], expected_left_x - tick_gap, 1e-3f);
    AC(y_positions[1], expected_y0, 1e-3f);

    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(x_axis->text_visual)->text.glyph_visual != NULL);
    DvzVisualDataView glyph_position_view = {0};
    AT(dvz_visual_data(
           _visual_family_state(x_axis->text_visual)->text.glyph_visual, "position", &glyph_position_view) == 0);
    const float* glyph_positions = (const float*)glyph_position_view.data;
    const float expected_clip_x = 2.0f * expected_x0 / panel_width - 1.0f;
    const float expected_clip_y =
        1.0f - 2.0f * (expected_x_tick_y + tick_gap) / panel_height;
    AC(glyph_positions[0], expected_clip_x, 1e-3f);
    AC(glyph_positions[1], expected_clip_y, 1e-3f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_panel_data_to_visual_positions(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    float data[] = {0.0f, -5.0f, 2.0f, 5.0f, 5.0f, 3.0f, 10.0f, 0.0f, 4.0f};
    float visual[9] = {0};
    AT(dvz_panel_data_to_visual_positions(panel, data, visual, 3) == 0);
    AT(fabsf(visual[0] - 0.0f) < 1e-6f);
    AT(fabsf(visual[1] + 5.0f) < 1e-6f);
    AT(fabsf(visual[2] - 2.0f) < 1e-6f);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -5.0, 5.0) == 0);
    AT(dvz_panel_data_to_visual_positions(panel, data, visual, 3) == 0);
    AT(fabsf(visual[0] + 1.0f) < 1e-6f);
    AT(fabsf(visual[1] + 1.0f) < 1e-6f);
    AT(fabsf(visual[3] - 0.0f) < 1e-6f);
    AT(fabsf(visual[4] - 1.0f) < 1e-6f);
    AT(fabsf(visual[6] - 1.0f) < 1e-6f);
    AT(fabsf(visual[7] - 0.0f) < 1e-6f);
    AT(fabsf(visual[8] - 4.0f) < 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_plot_margins(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -5.0, 5.0) == 0);
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(x_axis);
    ANN(y_axis);
    AT(dvz_axis_set_plot_margins(x_axis, 0.10f, 0.04f, 0.10f, 0.04f));
    AT(dvz_axis_set_plot_margins(y_axis, 0.10f, 0.04f, 0.10f, 0.04f));

    float data[] = {0.0f, -5.0f, 2.0f, 5.0f, 5.0f, 3.0f, 10.0f, 0.0f, 4.0f};
    float visual[9] = {0};
    AT(dvz_panel_data_to_visual_positions(panel, data, visual, 3) == 0);
    AT(fabsf(visual[0] + 0.90f) < 1e-6f);
    AT(fabsf(visual[1] + 0.90f) < 1e-6f);
    AT(fabsf(visual[3] - 0.03f) < 1e-6f);
    AT(fabsf(visual[4] - 0.96f) < 1e-6f);
    AT(fabsf(visual[6] - 0.96f) < 1e-6f);
    AT(fabsf(visual[7] - 0.03f) < 1e-6f);

    AT(!dvz_axis_set_plot_margins(x_axis, -0.10f, 0.0f, 0.0f, 0.0f));
    AT(!dvz_axis_set_plot_margins(x_axis, 1.20f, 0.90f, 0.0f, 0.0f));

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_layout_reserve(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzPanelLayoutReserve reserve = dvz_panel_layout_reserve();
    AT(reserve.left == 0.0f);
    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.20f, .right = 0.10f, .bottom = 0.05f,
                                        .top = 0.15f}));
    AT(dvz_panel_get_layout_reserve(panel, &reserve));
    AT(fabsf(reserve.left - 0.20f) < 1e-6f);
    AT(fabsf(reserve.right - 0.10f) < 1e-6f);
    AT(fabsf(reserve.bottom - 0.05f) < 1e-6f);
    AT(fabsf(reserve.top - 0.15f) < 1e-6f);

    float plot_visual[4] = {0};
    _scene_panel_plot_visual_rect(panel, plot_visual);
    AT(fabsf(plot_visual[0] + 0.80f) < 1e-6f);
    AT(fabsf(plot_visual[1] - 0.90f) < 1e-6f);
    AT(fabsf(plot_visual[2] + 0.95f) < 1e-6f);
    AT(fabsf(plot_visual[3] - 0.85f) < 1e-6f);

    float plot_x = 0.0f;
    float plot_y = 0.0f;
    float plot_width = 0.0f;
    float plot_height = 0.0f;
    _scene_panel_plot_pixel_rect(panel, &plot_x, &plot_y, &plot_width, &plot_height);
    AT(fabsf(plot_x - 80.0f) < 1e-4f);
    AT(fabsf(plot_y - 45.0f) < 1e-4f);
    AT(fabsf(plot_width - 680.0f) < 1e-6f);
    AT(fabsf(plot_height - 540.0f) < 1e-6f);

    DvzPanelDesc plot_desc = _scene_panel_plot_desc(panel);
    AT(fabsf(plot_desc.x - 0.10f) < 1e-6f);
    AT(fabsf(plot_desc.y - 0.075f) < 1e-6f);
    AT(fabsf(plot_desc.width - 0.85f) < 1e-6f);
    AT(fabsf(plot_desc.height - 0.90f) < 1e-6f);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -5.0, 5.0) == 0);
    float data[] = {0.0f, -5.0f, 0.0f, 10.0f, 5.0f, 0.0f};
    float visual[6] = {0};
    AT(dvz_panel_data_to_visual_positions(panel, data, visual, 2) == 0);
    AT(fabsf(visual[0] + 0.80f) < 1e-6f);
    AT(fabsf(visual[1] + 0.95f) < 1e-6f);
    AT(fabsf(visual[3] - 0.90f) < 1e-6f);
    AT(fabsf(visual[4] - 0.85f) < 1e-6f);

    dvz_figure_resize(figure, 1200, 900);
    AT(dvz_panel_get_layout_reserve(panel, &reserve));
    AT(fabsf(reserve.left - 0.20f) < 1e-6f);
    AT(fabsf(reserve.bottom - 0.05f) < 1e-6f);
    _scene_panel_plot_visual_rect(panel, plot_visual);
    AT(fabsf(plot_visual[0] + 0.80f) < 1e-6f);
    AT(fabsf(plot_visual[1] - 0.90f) < 1e-6f);
    AT(fabsf(plot_visual[2] + 0.95f) < 1e-6f);
    AT(fabsf(plot_visual[3] - 0.85f) < 1e-6f);
    dvz_figure_resize(figure, 800, 600);

    AT(!dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 1.50f, .right = 0.60f}));
    AT(dvz_panel_set_layout_reserve(panel, NULL));
    AT(dvz_panel_get_layout_reserve(panel, &reserve));
    AT(reserve.left == 0.0f && reserve.right == 0.0f);

    DvzPanelReserve padding = {
        .left_px = 32.0f,
        .right_px = 16.0f,
        .top_px = 24.0f,
        .bottom_px = 8.0f,
    };
    AT(dvz_panel_set_padding(panel, &padding));
    DvzPanelReserve padding_out = {0};
    AT(dvz_panel_get_padding(panel, &padding_out));
    AT(fabsf(padding_out.left_px - 32.0f) < 1e-6f);
    AT(fabsf(padding_out.right_px - 16.0f) < 1e-6f);
    DvzRect inner_rect = {0};
    AT(dvz_panel_inner_rect_px(panel, &inner_rect));
    AT(fabsf(inner_rect.x - 32.0f) < 1e-4f);
    AT(fabsf(inner_rect.y - 24.0f) < 1e-4f);
    AT(fabsf(inner_rect.width - 752.0f) < 1e-4f);
    AT(fabsf(inner_rect.height - 568.0f) < 1e-4f);

    DvzPanelReserve pixel_reserve = {
        .left_px = 80.0f,
        .right_px = 120.0f,
        .top_px = 30.0f,
        .bottom_px = 50.0f,
    };
    AT(dvz_panel_set_reserve(panel, &pixel_reserve));
    DvzPanelReserve pixel_out = {0};
    AT(dvz_panel_get_reserve(panel, &pixel_out));
    AT(fabsf(pixel_out.left_px - 80.0f) < 1e-6f);
    DvzRect plot_rect = {0};
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.x - 112.0f) < 1e-4f);
    AT(fabsf(plot_rect.y - 54.0f) < 1e-4f);
    AT(fabsf(plot_rect.width - 552.0f) < 1e-4f);
    AT(fabsf(plot_rect.height - 488.0f) < 1e-4f);

    DvzPanelDesc padded_plot_desc = _scene_panel_plot_desc(panel);
    AT(fabsf(padded_plot_desc.x - 0.14f) < 1e-6f);
    AT(fabsf(padded_plot_desc.y - 0.09f) < 1e-6f);
    AT(fabsf(padded_plot_desc.width - 0.69f) < 1e-6f);
    AT(fabsf(padded_plot_desc.height - (488.0f / 600.0f)) < 1e-6f);

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.20f, .right = 0.10f, .bottom = 0.05f,
                                        .top = 0.15f}));
    dvz_figure_resize(figure, 1200, 900);
    AT(dvz_panel_get_layout_reserve(panel, &reserve));
    AT(fabsf(reserve.left - 0.20f) < 1e-6f);
    AT(fabsf(reserve.bottom - 0.05f) < 1e-6f);

    AT(dvz_panel_set_reserve(panel, &pixel_reserve));
    AT(dvz_panel_inner_rect_px(panel, &inner_rect));
    AT(fabsf(inner_rect.x - 32.0f) < 1e-4f);
    AT(fabsf(inner_rect.y - 24.0f) < 1e-4f);
    AT(fabsf(inner_rect.width - 1152.0f) < 1e-4f);
    AT(fabsf(inner_rect.height - 868.0f) < 1e-4f);
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.x - 112.0f) < 1e-4f);
    AT(fabsf(plot_rect.y - 54.0f) < 1e-4f);
    AT(fabsf(plot_rect.width - 952.0f) < 1e-4f);
    AT(fabsf(plot_rect.height - 788.0f) < 1e-4f);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(x_axis);
    DvzAxisStyle x_style = x_axis->style;
    x_style.reserve_px = 40.0f;
    AT(dvz_axis_set_style(x_axis, &x_style));
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(y_axis);
    DvzAxisStyle y_style = y_axis->style;
    y_style.reserve_px = 55.0f;
    AT(dvz_axis_set_style(y_axis, &y_style));

    AT(dvz_panel_get_layout_reserve(panel, &reserve));
    AT(fabsf(reserve.left - (2.0f * 80.0f / 1200.0f)) < 1e-6f);
    AT(fabsf(reserve.bottom - (2.0f * 50.0f / 900.0f)) < 1e-6f);
    AT(dvz_panel_get_reserve(panel, &pixel_out));
    AT(fabsf(pixel_out.left_px - 135.0f) < 1e-6f);
    AT(fabsf(pixel_out.bottom_px - 90.0f) < 1e-6f);
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.x - 167.0f) < 1e-4f);
    AT(fabsf(plot_rect.y - 54.0f) < 1e-4f);
    AT(fabsf(plot_rect.width - 897.0f) < 1e-4f);
    AT(fabsf(plot_rect.height - 748.0f) < 1e-4f);

    AT(!dvz_panel_set_padding(
        panel, &(DvzPanelReserve){
                   .left_px = 1000.0f,
                   .right_px = 100.0f,
               }));
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.x - 167.0f) < 1e-4f);
    AT(fabsf(plot_rect.width - 897.0f) < 1e-4f);

    AT(dvz_panel_set_padding(panel, NULL));
    AT(dvz_panel_plot_rect_px(panel, &plot_rect));
    AT(fabsf(plot_rect.x - 135.0f) < 1e-4f);
    AT(fabsf(plot_rect.y - 30.0f) < 1e-4f);
    AT(fabsf(plot_rect.width - 945.0f) < 1e-4f);
    AT(fabsf(plot_rect.height - 780.0f) < 1e-4f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_panel_visible_domain(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    double min = 0.0;
    double max = 0.0;
    AT(dvz_panel_visible_domain(panel, DVZ_DIM_X, &min, &max));
    AT(fabs(min + 1.0) < 1e-9);
    AT(fabs(max - 1.0) < 1e-9);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 100.0) == 0);
    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);
    dvz_panzoom_zoom(pz, (vec2){2.0f, 2.0f});
    dvz_panzoom_pan(pz, (vec2){0.5f, 0.0f});

    AT(dvz_panel_visible_domain(panel, DVZ_DIM_X, &min, &max));
    AT(fabs(min) < 1e-9);
    AT(fabs(max - 50.0) < 1e-9);

    dvz_scene_destroy(scene);
    return 0;
}


int test_axis_panzoom_visible_domain(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 100.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);

    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);
    dvz_panzoom_zoom(pz, (vec2){2.0f, 2.0f});
    dvz_panzoom_pan(pz, (vec2){0.5f, 0.0f});

    float extent[4] = {0};
    AT(dvz_panzoom_extent(pz, extent));
    AT(fabsf(extent[0] + 1.0f) < 1e-5f);
    AT(fabsf(extent[1] - 0.0f) < 1e-5f);

    _scene_prepare_axis_visuals(figure);
    AT(axis->tick_count >= 3);
    AT(axis->ticks[0] <= 0.0);
    AT(axis->ticks[axis->tick_count - 1] >= 50.0);
    double step = axis->tick_lstep;

    AT(dvz_axis_set_grid(axis, true));
    _scene_prepare_axis_visuals(figure);
    double lmin = axis->tick_lmin;
    double covered_min = axis->tick_covered_min;
    double covered_max = axis->tick_covered_max;
    AT(_axis_test_vertical_grid_line_count(axis) > 0);

    dvz_panzoom_pan(pz, (vec2){0.45f, 0.0f});
    _scene_prepare_axis_visuals(figure);
    AT(fabs(axis->tick_lstep - step) < 1e-9);
    AT(fabs(axis->tick_lmin - lmin) < 1e-9);
    AT(fabs(axis->tick_covered_min - covered_min) < 1e-9);
    AT(fabs(axis->tick_covered_max - covered_max) < 1e-9);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_panzoom_layout_aligns_grid_to_plot(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1280, 960, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.16f, .right = 0.05f, .bottom = 0.15f,
                                        .top = 0.05f}));
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -2.0, 2.0) == 0);
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(x_axis);
    ANN(y_axis);
    AT(dvz_axis_set_grid(x_axis, true));
    AT(dvz_axis_set_grid(y_axis, true));

    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);
    dvz_panzoom_zoom(pz, (vec2){1.20f, 1.0f});
    dvz_panzoom_pan(pz, (vec2){0.30f, 0.0f});

    float data[] = {0.0f, 0.0f, 0.0f};
    float visual[3] = {0};
    AT(dvz_panel_data_to_visual_positions(panel, data, visual, 1) == 0);
    DvzMVP apply_mvp = {0};
    _scene_panel_apply_mvp(panel, &apply_mvp);
    float expected_x = 0.0f;
    AT(_axis_test_apply_mvp_coord(&apply_mvp, visual, DVZ_DIM_X, &expected_x));

    float plot[4] = {0};
    _scene_panel_plot_visual_rect(panel, plot);
    double visible_min = 0.0;
    double visible_max = 0.0;
    AT(dvz_panel_visible_domain(panel, DVZ_DIM_X, &visible_min, &visible_max));
    double expected_min =
        10.0 * (((double)plot[0] / (double)pz->zoom[0] - (double)pz->pan[0]) -
                (double)plot[0]) /
        ((double)plot[1] - (double)plot[0]);
    double expected_max =
        10.0 * (((double)plot[1] / (double)pz->zoom[0] - (double)pz->pan[0]) -
                (double)plot[0]) /
        ((double)plot[1] - (double)plot[0]);
    AT(fabs(visible_min - expected_min) < 1e-6);
    AT(fabs(visible_max - expected_max) < 1e-6);

    _scene_prepare_axis_visuals(figure);
    float grid_x = 0.0f;
    AT(_axis_test_find_vertical_grid_center(x_axis, visual[0], 1e-5f, &grid_x));
    AT(fabsf(grid_x - visual[0]) < 1e-5f);
    vec3 grid_pos = {grid_x, visual[1], visual[2]};
    float grid_projected_x = 0.0f;
    AT(_axis_test_apply_mvp_coord(&apply_mvp, grid_pos, DVZ_DIM_X, &grid_projected_x));
    AT(fabsf(grid_projected_x - expected_x) < 1e-5f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Check that raw visual-space grid lines and points share the same APPLY transform.
 *
 * @param suite the test suite
 * @param item the test case
 * @return zero on success
 */
static int test_axis_raw_visual_panzoom_alignment(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1280, 960, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.04f, .bottom = 0.12f,
                                        .top = 0.04f}));
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(x_axis);
    ANN(y_axis);
    AT(dvz_axis_set_grid(x_axis, true));
    AT(dvz_axis_set_grid(y_axis, true));

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 11;
    ticks.min_pixel_spacing = 75.0f;
    ticks.minor_per_interval = 0;
    AT(dvz_axis_set_tick_policy(x_axis, &ticks));
    AT(dvz_axis_set_tick_policy(y_axis, &ticks));

    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);
    dvz_panzoom_zoom(pz, (vec2){2.0f, 2.0f});
    dvz_panzoom_pan(pz, (vec2){0.20f, -0.10f});

    float plot[4] = {0};
    _scene_panel_plot_visual_rect(panel, plot);
    double visible_min = 0.0;
    double visible_max = 0.0;
    AT(dvz_panel_visible_domain(panel, DVZ_DIM_Y, &visible_min, &visible_max));
    double expected_min = -(double)pz->pan[1] + (double)plot[2] / (double)pz->zoom[1];
    double expected_max = -(double)pz->pan[1] + (double)plot[3] / (double)pz->zoom[1];
    AT(fabs(visible_min - expected_min) < 1e-6);
    AT(fabs(visible_max - expected_max) < 1e-6);

    vec3 visual = {0.0f, 0.0f, 0.0f};
    DvzMVP apply_mvp = {0};
    _scene_panel_apply_mvp(panel, &apply_mvp);
    float expected_x = 0.0f;
    float expected_y = 0.0f;
    AT(_axis_test_apply_mvp_coord(&apply_mvp, visual, DVZ_DIM_X, &expected_x));
    AT(_axis_test_apply_mvp_coord(&apply_mvp, visual, DVZ_DIM_Y, &expected_y));

    _scene_prepare_axis_visuals(figure);
    float grid_x = 0.0f;
    float grid_y = 0.0f;
    AT(_axis_test_find_vertical_grid_center(x_axis, visual[0], 1e-5f, &grid_x));
    AT(_axis_test_find_horizontal_grid_center(y_axis, visual[1], 1e-5f, &grid_y));
    AT(fabsf(grid_x - visual[0]) < 1e-5f);
    AT(fabsf(grid_y - visual[1]) < 1e-5f);
    vec3 grid_x_pos = {grid_x, visual[1], visual[2]};
    vec3 grid_y_pos = {visual[0], grid_y, visual[2]};
    float grid_projected_x = 0.0f;
    float grid_projected_y = 0.0f;
    AT(_axis_test_apply_mvp_coord(&apply_mvp, grid_x_pos, DVZ_DIM_X, &grid_projected_x));
    AT(_axis_test_apply_mvp_coord(&apply_mvp, grid_y_pos, DVZ_DIM_Y, &grid_projected_y));
    AT(fabsf(grid_projected_x - expected_x) < 1e-5f);
    AT(fabsf(grid_projected_y - expected_y) < 1e-5f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Check integer lattice points against generated grid lines after panzoom.
 *
 * @param suite the test suite
 * @param item the test case
 * @return zero on success
 */
static int test_axis_integer_lattice_panzoom_alignment(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1280, 960, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    const uint32_t lattice_max = 20;
    const uint32_t side = lattice_max + 1;
    const uint32_t count = side * side;
    vec3* data = (vec3*)dvz_calloc(count, sizeof(vec3));
    vec3* visual = (vec3*)dvz_calloc(count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(count, sizeof(DvzColor));
    float* diameters = (float*)dvz_calloc(count, sizeof(float));
    ANN(data);
    ANN(visual);
    ANN(colors);
    ANN(diameters);

    for (uint32_t j = 0; j < side; j++)
    {
        for (uint32_t i = 0; i < side; i++)
        {
            uint32_t idx = j * side + i;
            data[idx][0] = (float)i;
            data[idx][1] = (float)j;
            data[idx][2] = 0.0f;
            colors[idx] = dvz_color_rgba(90, 210, 230, 230);
            diameters[idx] = 7.0f;
        }
    }

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.04f, .bottom = 0.12f,
                                        .top = 0.04f}));
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, (double)lattice_max) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, (double)lattice_max) == 0);
    AT(dvz_panel_data_to_visual_positions(panel, (const float*)data, (float*)visual, count) == 0);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter", .data = diameters, .item_count = count},
    };
    AT(dvz_visual_set_data_many(point, updates, 3) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(x_axis);
    ANN(y_axis);
    AT(dvz_axis_set_grid(x_axis, true));
    AT(dvz_axis_set_grid(y_axis, true));
    DvzAxisTickPolicy policy = dvz_axis_tick_policy();
    policy.target_count = 11;
    policy.min_pixel_spacing = 75.0f;
    policy.minor_per_interval = 0;
    AT(dvz_axis_set_tick_policy(x_axis, &policy));
    AT(dvz_axis_set_tick_policy(y_axis, &policy));

    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);
    dvz_panzoom_zoom(pz, (vec2){1.65f, 1.45f});
    dvz_panzoom_pan(pz, (vec2){-0.22f, 0.18f});

    _scene_prepare_axis_visuals(figure);
    DvzMVP apply_mvp = {0};
    _scene_panel_apply_mvp(panel, &apply_mvp);

    DvzFramePlan* plan = dvz_frame_plan("axis.integer_lattice", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_node_count(plan) == 1);
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == 5);
    uint32_t apply_count = 0;
    uint32_t fixed_count = 0;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
            fixed_count++;
        else
            apply_count++;
    }
    AT(apply_count == 3);
    AT(fixed_count == 2);
    AT(render->u.render.has_mvp);
    AT(fabsf(render->u.render.apply_mvp.proj[0][0] - apply_mvp.proj[0][0]) < 1e-6f);
    AT(fabsf(render->u.render.apply_mvp.proj[1][1] - apply_mvp.proj[1][1]) < 1e-6f);
    dvz_frame_plan_destroy(plan);

    uint32_t x_checks = 0;
    for (uint32_t ti = 0; ti < x_axis->tick_count; ti++)
    {
        double value = x_axis->ticks[ti];
        double rounded = round(value);
        if (fabs(value - rounded) > 1e-9 || rounded < 0.0 || rounded > (double)lattice_max)
            continue;
        uint32_t idx = (lattice_max / 2u) * side + (uint32_t)rounded;
        float expected_x = 0.0f;
        AT(_axis_test_apply_mvp_coord(&apply_mvp, visual[idx], DVZ_DIM_X, &expected_x));
        float plot[4] = {0};
        _scene_panel_plot_visual_rect(panel, plot);
        if (expected_x < plot[0] || expected_x > plot[1])
            continue;
        float grid_x = 0.0f;
        AT(_axis_test_find_vertical_grid_center(x_axis, visual[idx][0], 1e-5f, &grid_x));
        AT(fabsf(grid_x - visual[idx][0]) < 1e-5f);
        vec3 grid_pos = {grid_x, visual[idx][1], visual[idx][2]};
        float grid_projected_x = 0.0f;
        AT(_axis_test_apply_mvp_coord(&apply_mvp, grid_pos, DVZ_DIM_X, &grid_projected_x));
        AT(fabsf(grid_projected_x - expected_x) < 1e-5f);
        x_checks++;
    }

    uint32_t y_checks = 0;
    for (uint32_t ti = 0; ti < y_axis->tick_count; ti++)
    {
        double value = y_axis->ticks[ti];
        double rounded = round(value);
        if (fabs(value - rounded) > 1e-9 || rounded < 0.0 || rounded > (double)lattice_max)
            continue;
        uint32_t idx = (uint32_t)rounded * side + (lattice_max / 2u);
        float expected_y = 0.0f;
        AT(_axis_test_apply_mvp_coord(&apply_mvp, visual[idx], DVZ_DIM_Y, &expected_y));
        float plot[4] = {0};
        _scene_panel_plot_visual_rect(panel, plot);
        if (expected_y < plot[2] || expected_y > plot[3])
            continue;
        float grid_y = 0.0f;
        AT(_axis_test_find_horizontal_grid_center(y_axis, visual[idx][1], 1e-5f, &grid_y));
        AT(fabsf(grid_y - visual[idx][1]) < 1e-5f);
        vec3 grid_pos = {visual[idx][0], grid_y, visual[idx][2]};
        float grid_projected_y = 0.0f;
        AT(_axis_test_apply_mvp_coord(&apply_mvp, grid_pos, DVZ_DIM_Y, &grid_projected_y));
        AT(fabsf(grid_projected_y - expected_y) < 1e-5f);
        y_checks++;
    }

    AT(x_checks >= 3);
    AT(y_checks >= 3);

    dvz_free(diameters);
    dvz_free(colors);
    dvz_free(visual);
    dvz_free(data);
    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_zoom_out_in_grid_regression(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 900, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 100.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(dvz_axis_set_grid(axis, true));

    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);

    dvz_panzoom_zoom(pz, (vec2){0.10f, 1.0f});
    _scene_prepare_axis_visuals(figure);
    AT(axis->tick_lstep >= 50.0);
    AT(_axis_test_vertical_grid_line_count(axis) > 0);

    dvz_panzoom_zoom(pz, (vec2){20.0f, 1.0f});
    _scene_prepare_axis_visuals(figure);
    AT(axis->tick_lstep <= 1.0);
    AT(_axis_test_vertical_grid_line_count(axis) >= 4);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_panzoom_resize_visual_smoke(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1000, 700, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, -10.0, 10.0) == 0);
    AT(dvz_panel_set_domain(panel, DVZ_DIM_Y, -10.0, 10.0) == 0);
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    ANN(x_axis);
    ANN(y_axis);
    AT(dvz_axis_set_grid(x_axis, true));
    AT(dvz_axis_set_grid(y_axis, true));

    DvzPanzoom* pz = _axis_test_bind_panzoom(scene, panel);
    ANN(pz);

    const uint32_t widths[] = {1000, 320, 900};
    const float zooms[] = {1.0f, 0.25f, 12.0f, 1.5f};
    for (uint32_t wi = 0; wi < 3; wi++)
    {
        dvz_figure_resize(figure, widths[wi], 700);
        for (uint32_t zi = 0; zi < 4; zi++)
        {
            dvz_panzoom_zoom(pz, (vec2){zooms[zi], zooms[zi]});
            dvz_panzoom_pan(pz, (vec2){0.15f * (float)zi, -0.05f * (float)wi});
            _scene_prepare_axis_visuals(figure);
            AT(x_axis->tick_count >= 2);
            AT(y_axis->tick_count >= 2);
            AT(_axis_test_vertical_grid_line_count(x_axis) > 0);
            AT(_axis_test_horizontal_grid_line_count(y_axis) > 0);
        }
    }

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Check that preparing a static axis twice leaves retained axis data clean.
 *
 * @param suite the test suite
 * @param item the test case
 * @return zero on success
 */
static int test_axis_static_prepare_idempotent(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1000, 700, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.08f, .y = 0.06f, .width = 0.86f, .height = 0.86f});
    ANN(panel);

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.0f, .bottom = 0.18f,
                                        .top = 0.0f}));
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, -5.0, +5.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(dvz_axis_set_grid(axis, true));
    AT(dvz_axis_set_label(axis, "x"));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    AT(!_scene_figure_has_pending_render_work(figure));

    _scene_prepare_axis_visuals(figure);
    _scene_prepare_text_visuals(figure);
    AT(_axis_test_attr_dirty_item_count(axis->visual, "position") == 0);
    AT(_axis_test_attr_dirty_item_count(axis->visual, "color") == 0);
    AT(_axis_test_attr_dirty_item_count(axis->grid_visual, "position") == 0);
    AT(_axis_test_attr_dirty_item_count(axis->grid_visual, "color") == 0);
    AT(!_scene_figure_has_pending_render_work(figure));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure axis geometry uses the panel clip so outward ticks survive plot scissoring.
 *
 * @param suite the test suite
 * @param item the test item
 * @return zero on success
 */
static int test_axis_visual_clip_rect_panel(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 240, 180, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.25f, .right = 0.05f, .bottom = 0.25f,
                                        .top = 0.05f}));
    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(dvz_axis_set_grid(axis, true));

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    vec3 pos = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 255, 255};
    float size = 8.0f;
    AT(dvz_visual_set_data(point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(point, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    _scene_prepare_axis_visuals(figure);
    AT(axis->visual != NULL);
    AT(axis->visual->visible);

    DvzFramePlan* plan = dvz_frame_plan("axis.clip_rect_panel", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_node_count(plan) == 1);
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == 3);
    uint32_t panel_clip_count = 0;
    uint32_t plot_clip_count = 0;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.visual_metadata[i].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PANEL)
            panel_clip_count++;
        if (render->u.render.visual_metadata[i].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PLOT)
            plot_clip_count++;
    }
    AT(panel_clip_count == 1);
    AT(plot_clip_count == 2);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_axis_dynamic_segment_draw_count(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 100.0) == 0);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);
    AT(dvz_axis_set_grid(axis, true));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    AT(dvz_axis_set_tick_policy(
        axis, &(DvzAxisTickPolicy){DVZ_STRUCT_INIT_FIELDS(DvzAxisTickPolicy), .target_count = 12, .min_pixel_spacing = 0.0f}));
    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream0);
    uint32_t draw0 = _axis_test_draw_vertex_count(stream0);
    AT(draw0 > 0);
    dvz_drp2_stream_destroy(stream0);

    dvz_diagnostic_report_init(&report);
    AT(dvz_axis_set_tick_policy(
        axis, &(DvzAxisTickPolicy){DVZ_STRUCT_INIT_FIELDS(DvzAxisTickPolicy), .target_count = 2, .min_pixel_spacing = 0.0f}));
    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream1);
    uint32_t draw1 = _axis_test_draw_vertex_count(stream1);
    AT(draw1 > 0);
    AT(draw1 < draw0);
    dvz_drp2_stream_destroy(stream1);

    dvz_scene_destroy(scene);
    return 0;
}


int test_axis_descriptor_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzAxis* axis = dvz_panel_axis(panel, DVZ_DIM_X);
    ANN(axis);

    DvzAxisTickPolicy policy = dvz_axis_tick_policy();
    policy.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_axis_set_tick_policy(axis, &policy));

    policy = dvz_axis_tick_policy();
    policy.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_axis_set_tick_policy(axis, &policy));

    DvzAxisStyle style = dvz_axis_style();
    style.struct_size = DVZ_STRUCT_SIZE(DvzAxisStyle) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_axis_set_style(axis, &style));

    style = dvz_axis_style();
    style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_axis_set_style(axis, &style));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_axis(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene,axis";

    TST_MODULE(suite, "scene");
    TST_GROUP("axis");

    TST_CASE(test_axis_domain_and_ticks);
    TST_CASE(test_axis_minor_ticks);
    TST_CASE(test_axis_tick_density_tracks_panel_size);
    TST_CASE(test_axis_text_labels);
    TST_CASE(test_axis_numeric_unit_labels);
    TST_CASE(test_axis_text_hidpi_scales_glyph_bounds);
    TST_CASE(test_axis_text_renderer_style);
    TST_CASE(test_axis_text_updates_after_domain_change);
    TST_CASE(test_axis_text_layout_reserve);
    TST_CASE(test_axis_text_inset_panel_coordinates);
    TST_CASE(test_panel_data_to_visual_positions);
    TST_CASE(test_axis_plot_margins);
    TST_CASE(test_axis_layout_reserve);
    TST_CASE(test_panel_visible_domain);
    TST_CASE(test_axis_panzoom_visible_domain);
    TST_CASE(test_axis_panzoom_layout_aligns_grid_to_plot);
    TST_CASE(test_axis_raw_visual_panzoom_alignment);
    TST_CASE(test_axis_integer_lattice_panzoom_alignment);
    TST_CASE(test_axis_zoom_out_in_grid_regression);
    TST_CASE(test_axis_panzoom_resize_visual_smoke);
    TST_CASE(test_axis_static_prepare_idempotent);
    TST_CASE(test_axis_visual_clip_rect_panel);
    TST_CASE(test_axis_dynamic_segment_draw_count);
    TST_CASE(test_axis_descriptor_abi_rejects_invalid_structs);
    return 0;
}
