/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Adornment workspace allocation regressions                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "annotation/prepare_internal.h"
#include "datoviz/scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Reject the large legend workspace while preserving other allocations.
 *
 * @param count allocation count
 * @param size element size
 * @return allocated storage, or NULL for the workspace
 */
static void* _legend_reject_workspace_calloc(DvzSize count, DvzSize size)
{
    if (count == 1 && size > 64 * 1024)
        return NULL;
    return dvz_system_allocator()->calloc_fn(count, size);
}



/**
 * Reject the five-string pointer array used by the legend fixture.
 *
 * @param count allocation count
 * @param size element size
 * @return allocated storage, or NULL for the string pointer array
 */
static void* _legend_reject_strings_calloc(DvzSize count, DvzSize size)
{
    if (count == 5 && size == sizeof(const char*))
        return NULL;
    return dvz_system_allocator()->calloc_fn(count, size);
}



/**
 * Verify a clean legend retries a failed resize update without another mutation.
 *
 * @param reject allocation callback selecting the failure
 * @param workspace whether the rejected allocation precedes mark updates
 * @return zero on success
 */
static int _legend_allocation_retry(DvzCallocFn reject, bool workspace)
{
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 500, 320, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[4] = {
        {.category_id = 1, .label = "One", .color = {255, 0, 0, 255}},
        {.category_id = 2, .label = "Two", .color = {0, 255, 0, 255}},
        {.category_id = 3, .label = "Three", .color = {0, 0, 255, 255}},
        {.category_id = 4, .label = "Four", .color = {255, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 4) == DVZ_OK);
    DvzLegend* legend = dvz_legend(
        panel, scale,
        &(DvzLegendDesc){DVZ_STRUCT_INIT_FIELDS(DvzLegendDesc),
                        .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                        .title = "Classes",
                        .reserve_px = 150.0f,
                        .mark_size_px = 12.0f});
    ANN(legend);

    _scene_prepare_legend_visuals(figure, NULL);
    AT(legend->mark_visual != NULL && legend->text_visual != NULL);
    AT(legend->mark_visual->visible && legend->text_visual->visible);
    AT(legend->text_count == 5);
    AT(!legend->dirty);
    AT(dvz_figure_resize(figure, 600, 320) == DVZ_OK);
    AT(!legend->dirty);

    const DvzAllocator* previous = dvz_get_allocator();
    DvzAllocator rejecting = *dvz_system_allocator();
    rejecting.calloc_fn = reject;
    dvz_set_allocator(&rejecting);
    _scene_prepare_legend_visuals(figure, NULL);
    dvz_set_allocator(previous);
    AT(!legend->text_visual->visible);
    AT(legend->mark_visual->visible == !workspace);
    AT(legend->dirty);

    _scene_prepare_legend_visuals(figure, NULL);
    AT(legend->mark_visual->visible && legend->text_visual->visible);
    AT(!legend->dirty);
    DvzVisualDataView positions = {0};
    AT(dvz_visual_data(legend->text_visual, "position", &positions) == 0);
    AT(positions.item_count == 5);
    dvz_scene_destroy(scene);
    return 0;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Recover from a legend workspace allocation failure during a clean resize update.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
int test_scene_legend_workspace_allocation_retry(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    return _legend_allocation_retry(_legend_reject_workspace_calloc, true);
}



/**
 * Recover from a legend strings allocation failure without another scene mutation.
 *
 * @param suite active test context
 * @param item active test case
 * @return zero on success
 */
int test_scene_legend_strings_allocation_retry(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    return _legend_allocation_retry(_legend_reject_strings_calloc, false);
}
