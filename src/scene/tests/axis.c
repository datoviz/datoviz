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
#include "../_scene.h"
#include "../_scene_emit.h"
#include "datoviz/drp2/stream.h"
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
    ANN(axis->visual);
    DvzVisualDataView positions_view = {0};
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->visual, "position", &positions_view);
    ASSERT(res == 0);
    res = dvz_visual_data(axis->visual, "color", &colors_view);
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
        if (min_y <= -1.0f + 1e-4f && max_y >= +1.0f - 1e-4f)
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
    ANN(axis->visual);
    DvzVisualDataView positions_view = {0};
    DvzVisualDataView colors_view = {0};
    int res = dvz_visual_data(axis->visual, "position", &positions_view);
    ASSERT(res == 0);
    res = dvz_visual_data(axis->visual, "color", &colors_view);
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
        if (min_x <= -1.0f + 1e-4f && max_x >= +1.0f - 1e-4f)
            count++;
    }
    return count;
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
    AT(axis->enabled);
    AT(axis->visual != NULL);
    AT(axis->visual->type == DVZ_VISUAL_TYPE_PRIMITIVE);
    AT(panel->visual_count == 1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);

    _scene_prepare_axis_visuals(figure);
    AT(axis->tick_count >= 5);
    AT(axis->visual->visible);
    AT(axis->visual->attrs[0].item_count > 0);
    AT(_axis_test_inward_tick_line_count(axis) >= 8);
    AT(_axis_test_inward_minor_tick_line_count(axis) > 0);

    AT(dvz_axis_set_grid(axis, true));
    _scene_prepare_axis_visuals(figure);
    AT(axis->visual->attrs[0].item_count > 0);

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
    AT(axis->text_visual->text.string_count == axis->tick_count + 1);
    AT(strcmp(axis->text_visual->text.strings[axis->tick_count], "Time") == 0);
    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(axis->text_visual, "position", &position_view) == 0);
    AT(position_view.item_count == axis->text_visual->text.string_count);

    _scene_prepare_text_visuals(figure);
    ANN(axis->text_visual->text.glyph_visual);
    AT(axis->text_visual->text.glyph_visual->visible);

    AT(dvz_axis_set_visible(axis, false));
    _scene_prepare_axis_visuals(figure);
    AT(!axis->text_visual->visible);
    _scene_prepare_text_visuals(figure);
    AT(!axis->text_visual->text.glyph_visual->visible);

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
    AT(axis->text_visual->text.string_count >= 2);
    char second_before[DVZ_SCENE_LABEL_SIZE] = {0};
    dvz_strlcpy(second_before, axis->text_visual->text.strings[1], sizeof(second_before));

    AT(dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) == 0);
    _scene_prepare_axis_visuals(figure);
    AT(axis->text_visual->text.string_count >= 2);
    AT(strcmp(axis->text_visual->text.strings[1], second_before) != 0);

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
    double x_tick = strtod(x_axis->text_visual->text.strings[0], &x_tick_end);
    AT(x_tick_end != x_axis->text_visual->text.strings[0]);
    char* y_tick_end = NULL;
    double y_tick = strtod(y_axis->text_visual->text.strings[0], &y_tick_end);
    AT(y_tick_end != y_axis->text_visual->text.strings[0]);

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
    ANN(x_axis->text_visual->text.glyph_visual);
    DvzVisualDataView glyph_position_view = {0};
    AT(dvz_visual_data(
           x_axis->text_visual->text.glyph_visual, "position", &glyph_position_view) == 0);
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

    AT(!dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 1.50f, .right = 0.60f}));
    AT(dvz_panel_set_layout_reserve(panel, NULL));
    AT(dvz_panel_get_layout_reserve(panel, &reserve));
    AT(reserve.left == 0.0f && reserve.right == 0.0f);

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

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    AT(dvz_axis_set_tick_policy(
        axis, &(DvzAxisTickPolicy){.target_count = 12, .min_pixel_spacing = 0.0f}));
    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream0);
    uint32_t draw0 = _axis_test_draw_vertex_count(stream0);
    AT(draw0 > 0);
    dvz_drp2_stream_destroy(stream0);

    dvz_diagnostic_report_init(&report);
    AT(dvz_axis_set_tick_policy(
        axis, &(DvzAxisTickPolicy){.target_count = 2, .min_pixel_spacing = 0.0f}));
    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream1);
    uint32_t draw1 = _axis_test_draw_vertex_count(stream1);
    AT(draw1 > 0);
    AT(draw1 < draw0);
    dvz_drp2_stream_destroy(stream1);

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
    TST_CASE(test_axis_text_updates_after_domain_change);
    TST_CASE(test_axis_text_layout_reserve);
    TST_CASE(test_axis_text_inset_panel_coordinates);
    TST_CASE(test_panel_data_to_visual_positions);
    TST_CASE(test_axis_plot_margins);
    TST_CASE(test_axis_layout_reserve);
    TST_CASE(test_panel_visible_domain);
    TST_CASE(test_axis_panzoom_visible_domain);
    TST_CASE(test_axis_zoom_out_in_grid_regression);
    TST_CASE(test_axis_panzoom_resize_visual_smoke);
    TST_CASE(test_axis_dynamic_segment_draw_count);
    return 0;
}
