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
#include <string.h>

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
 * Return one visual attribute by name.
 *
 * @param visual the visual
 * @param name the attribute name
 * @return the visual attribute, or NULL
 */
static DvzVisualAttr* _axis_test_attr(DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, name) == 0)
            return &visual->attrs[i];
    }
    return NULL;
}


/**
 * Return the first indexed draw count in a command stream.
 *
 * @param stream the command stream
 * @return the indexed draw count, or 0
 */
static uint32_t _axis_test_draw_index_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            return cmd->u.draw_indexed.index_count;
    }
    return 0;
}


/**
 * Return the number of inward major tick lines emitted by one axis visual.
 *
 * @param axis the axis
 * @return inward tick line count
 */
static uint32_t _axis_test_inward_tick_line_count(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->visual);
    DvzVisualAttr* starts_attr = _axis_test_attr(axis->visual, "position_start");
    DvzVisualAttr* ends_attr = _axis_test_attr(axis->visual, "position_end");
    DvzVisualAttr* colors_attr = _axis_test_attr(axis->visual, "color");
    ANN(starts_attr);
    ANN(ends_attr);
    ANN(colors_attr);
    const float* starts = (const float*)starts_attr->data;
    const float* ends = (const float*)ends_attr->data;
    const uint8_t* colors = (const uint8_t*)colors_attr->data;
    uint32_t count = 0;
    for (uint32_t i = 0; i < starts_attr->item_count; i++)
    {
        float sx = starts[3 * i + 0];
        float sy = starts[3 * i + 1];
        float ex = ends[3 * i + 0];
        float ey = ends[3 * i + 1];
        if (memcmp(&colors[4 * i], axis->style.major_tick_color, 4) != 0)
            continue;
        if (axis->dim == DVZ_DIM_X && fabsf(sx - ex) < 1e-5f && fabsf(sy + 1.0f) < 1e-5f &&
            ey > sy + 1e-5f && ey < sy + 0.10f)
            count++;
        if (axis->dim == DVZ_DIM_Y && fabsf(sy - ey) < 1e-5f && fabsf(sx + 1.0f) < 1e-5f &&
            ex > sx + 1e-5f && ex < sx + 0.10f)
            count++;
    }
    return count;
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
    ANN(axis->visual);
    DvzVisualAttr* colors_attr = _axis_test_attr(axis->visual, "color");
    ANN(colors_attr);
    const uint8_t* colors = (const uint8_t*)colors_attr->data;
    uint32_t count = 0;
    for (uint32_t i = 0; i < colors_attr->item_count; i++)
    {
        if (memcmp(&colors[4 * i], axis->style.minor_tick_color, 4) == 0)
            count++;
    }
    return count;
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
    DvzVisualAttr* starts_attr = _axis_test_attr(axis->visual, "position_start");
    DvzVisualAttr* ends_attr = _axis_test_attr(axis->visual, "position_end");
    ANN(starts_attr);
    ANN(ends_attr);
    const float* starts = (const float*)starts_attr->data;
    const float* ends = (const float*)ends_attr->data;
    uint32_t count = 0;
    for (uint32_t i = 0; i < starts_attr->item_count; i++)
    {
        if (fabsf(starts[3 * i + 1] + 1.0f) < 1e-5f &&
            fabsf(ends[3 * i + 1] - 1.0f) < 1e-5f)
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
    DvzVisualAttr* starts_attr = _axis_test_attr(axis->visual, "position_start");
    DvzVisualAttr* ends_attr = _axis_test_attr(axis->visual, "position_end");
    ANN(starts_attr);
    ANN(ends_attr);
    const float* starts = (const float*)starts_attr->data;
    const float* ends = (const float*)ends_attr->data;
    uint32_t count = 0;
    for (uint32_t i = 0; i < starts_attr->item_count; i++)
    {
        if (fabsf(starts[3 * i + 0] + 1.0f) < 1e-5f &&
            fabsf(ends[3 * i + 0] - 1.0f) < 1e-5f)
            count++;
    }
    return count;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_axis_domain_and_ticks(TstSuite* suite, TstItem* item)
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


static int test_axis_minor_ticks(TstSuite* suite, TstItem* item)
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


static int test_axis_tick_density_tracks_panel_size(TstSuite* suite, TstItem* item)
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


int test_panel_data_to_visual_positions(TstSuite* suite, TstItem* item)
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


static int test_axis_plot_margins(TstSuite* suite, TstItem* item)
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


static int test_axis_layout_reserve(TstSuite* suite, TstItem* item)
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


int test_panel_visible_domain(TstSuite* suite, TstItem* item)
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
    dvz_panel_set_panzoom(panel, NULL, 0);
    DvzPanzoom* pz = dvz_panel_panzoom(panel);
    ANN(pz);
    dvz_panzoom_zoom(pz, (vec2){2.0f, 2.0f});
    dvz_panzoom_pan(pz, (vec2){0.5f, 0.0f});

    AT(dvz_panel_visible_domain(panel, DVZ_DIM_X, &min, &max));
    AT(min < 0.0);
    AT(max > 45.0 && max < 50.0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_axis_panzoom_visible_domain(TstSuite* suite, TstItem* item)
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

    dvz_panel_set_panzoom(panel, NULL, 0);
    DvzPanzoom* pz = dvz_panel_panzoom(panel);
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
    DvzVisualAttr* starts_attr = _axis_test_attr(axis->visual, "position_start");
    DvzVisualAttr* ends_attr = _axis_test_attr(axis->visual, "position_end");
    ANN(starts_attr);
    ANN(ends_attr);
    const float* starts = (const float*)starts_attr->data;
    const float* ends = (const float*)ends_attr->data;
    const float x0 = -1.00f;
    const float x1 = +1.00f;
    bool has_grid = false;
    for (uint32_t i = 0; i < starts_attr->item_count; i++)
    {
        if (fabsf(starts[3 * i + 1] + 1.0f) < 1e-5f &&
            fabsf(ends[3 * i + 1] - 1.0f) < 1e-5f)
        {
            has_grid = true;
            AT(starts[3 * i + 0] >= x0 - 1e-5f);
            AT(starts[3 * i + 0] <= x1 + 1e-5f);
        }
    }
    AT(has_grid);

    dvz_panzoom_pan(pz, (vec2){0.45f, 0.0f});
    _scene_prepare_axis_visuals(figure);
    AT(fabs(axis->tick_lstep - step) < 1e-9);
    AT(fabs(axis->tick_lmin - lmin) < 1e-9);
    AT(fabs(axis->tick_covered_min - covered_min) < 1e-9);
    AT(fabs(axis->tick_covered_max - covered_max) < 1e-9);

    dvz_scene_destroy(scene);
    return 0;
}


static int test_axis_zoom_out_in_grid_regression(TstSuite* suite, TstItem* item)
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

    dvz_panel_set_panzoom(panel, NULL, 0);
    DvzPanzoom* pz = dvz_panel_panzoom(panel);
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


static int test_axis_panzoom_resize_visual_smoke(TstSuite* suite, TstItem* item)
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

    dvz_panel_set_panzoom(panel, NULL, 0);
    DvzPanzoom* pz = dvz_panel_panzoom(panel);
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


int test_axis_dynamic_segment_draw_count(TstSuite* suite, TstItem* item)
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
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    AT(dvz_axis_set_tick_policy(
        axis, &(DvzAxisTickPolicy){.target_count = 12, .min_pixel_spacing = 0.0f}));
    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream0);
    uint32_t draw0 = _axis_test_draw_index_count(stream0);
    AT(draw0 > 0);
    dvz_drp2_stream_destroy(stream0);

    dvz_diagnostic_report_init(&report);
    AT(dvz_axis_set_tick_policy(
        axis, &(DvzAxisTickPolicy){.target_count = 2, .min_pixel_spacing = 0.0f}));
    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream1);
    uint32_t draw1 = _axis_test_draw_index_count(stream1);
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

    TEST_SIMPLE(test_axis_domain_and_ticks);
    TEST_SIMPLE(test_axis_minor_ticks);
    TEST_SIMPLE(test_axis_tick_density_tracks_panel_size);
    TEST_SIMPLE(test_panel_data_to_visual_positions);
    TEST_SIMPLE(test_axis_plot_margins);
    TEST_SIMPLE(test_axis_layout_reserve);
    TEST_SIMPLE(test_panel_visible_domain);
    TEST_SIMPLE(test_axis_panzoom_visible_domain);
    TEST_SIMPLE(test_axis_zoom_out_in_grid_regression);
    TEST_SIMPLE(test_axis_panzoom_resize_visual_smoke);
    TEST_SIMPLE(test_axis_dynamic_segment_draw_count);
    return 0;
}
