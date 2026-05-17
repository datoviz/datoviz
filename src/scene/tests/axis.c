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
#include "../_scene.h"
#include "../_scene_emit.h"
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

    AT(dvz_axis_set_grid(axis, true));
    _scene_prepare_axis_visuals(figure);
    AT(axis->visual->attrs[0].item_count >= 2 * axis->tick_count);

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
    AT(axis->ticks[0] >= -1e-9);
    AT(axis->ticks[axis->tick_count - 1] <= 50.0 + 1e-9);
    double step = axis->tick_lstep;

    AT(dvz_axis_set_grid(axis, true));
    _scene_prepare_axis_visuals(figure);
    DvzVisualAttr* starts_attr = _axis_test_attr(axis->visual, "position_start");
    DvzVisualAttr* ends_attr = _axis_test_attr(axis->visual, "position_end");
    ANN(starts_attr);
    ANN(ends_attr);
    const float* starts = (const float*)starts_attr->data;
    const float* ends = (const float*)ends_attr->data;
    bool has_left_grid = false;
    bool has_right_grid = false;
    for (uint32_t i = 0; i < starts_attr->item_count; i++)
    {
        if (fabsf(starts[3 * i + 1] + 1.0f) < 1e-5f &&
            fabsf(ends[3 * i + 1] - 1.0f) < 1e-5f)
        {
            has_left_grid = has_left_grid || fabsf(starts[3 * i + 0] + 1.0f) < 1e-5f;
            has_right_grid = has_right_grid || fabsf(starts[3 * i + 0] - 1.0f) < 1e-5f;
        }
    }
    AT(has_left_grid);
    AT(has_right_grid);

    dvz_panzoom_pan(pz, (vec2){0.25f, 0.0f});
    _scene_prepare_axis_visuals(figure);
    AT(fabs(axis->tick_lstep - step) < 1e-9);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_axis(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene,axis";

    TEST_SIMPLE(test_axis_domain_and_ticks);
    TEST_SIMPLE(test_axis_panzoom_visible_domain);
    return 0;
}
