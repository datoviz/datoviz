/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query tests                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "../query/internal.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_query_registry_covers_active_visual_families(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    const DvzSceneVisualFamily families[] = {
        DVZ_SCENE_VISUAL_FAMILY_POINT,     DVZ_SCENE_VISUAL_FAMILY_PIXEL,
        DVZ_SCENE_VISUAL_FAMILY_MARKER,    DVZ_SCENE_VISUAL_FAMILY_SPHERE,
        DVZ_SCENE_VISUAL_FAMILY_SEGMENT,   DVZ_SCENE_VISUAL_FAMILY_PATH,
        DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE, DVZ_SCENE_VISUAL_FAMILY_MESH,
        DVZ_SCENE_VISUAL_FAMILY_IMAGE,     DVZ_SCENE_VISUAL_FAMILY_LABELS,
        DVZ_SCENE_VISUAL_FAMILY_VOLUME,    DVZ_SCENE_VISUAL_FAMILY_TEXT,
        DVZ_SCENE_VISUAL_FAMILY_GLYPH,
    };
    const uint32_t family_count = sizeof(families) / sizeof(families[0]);
    AT(_dvz_scene_query_registry_count() == family_count);

    for (uint32_t i = 0; i < family_count; i++)
    {
        const DvzSceneQueryFamilyOps* ops = _dvz_scene_query_registry_find(families[i]);
        ANN(ops);
        AT(ops->family == families[i]);
        AT(ops->name != NULL);
        AT(ops->pick_capabilities != 0);
    }
    AT(_dvz_scene_query_registry_find(DVZ_SCENE_VISUAL_FAMILY_NONE) == NULL);
    return 0;
}



int test_scene_query_queue_processes_native_results(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_query(
           panel, 8.0, 8.0,
           &(DvzQueryRequest){.request_id = 11, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(scene->pending_query_count == 1);
    AT(scene->pending_pick_count == 0);
    AT(scene->pending_probe_count == 0);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);
    AT(scene->pending_query_count == 0);
    AT(scene->query_result_count == 1);

    DvzQueryResult result = {0};
    AT(dvz_scene_poll_query(scene, &result));
    AT(result.request_id == 11);
    AT(result.status == DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL);
    AT(result.panel_id == 1);
    AC(result.panel_position[0], 8.0, 1e-12);
    AT(!dvz_scene_poll_query(scene, &result));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_query_queue_coalesces_pending_requests(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_query(
           panel, 8.0, 8.0,
           &(DvzQueryRequest){.request_id = 12, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(dvz_panel_query(
           panel, 16.0, 16.0,
           &(DvzQueryRequest){.request_id = 12, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(scene->pending_query_count == 1);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    AT(dvz_figure_process_queries(figure, (DvzDrp2Runtime*)scene, &caps) == 1);

    DvzQueryResult result = {0};
    AT(dvz_scene_poll_query(scene, &result));
    AT(result.request_id == 12);
    AC(result.panel_position[0], 16.0, 1e-12);
    AC(result.panel_position[1], 16.0, 1e-12);
    AT(!dvz_scene_poll_query(scene, &result));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Register scene query tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_query(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("query");

    TST_CASE(test_scene_query_registry_covers_active_visual_families);
    TST_CASE(test_scene_query_queue_processes_native_results);
    TST_CASE(test_scene_query_queue_coalesces_pending_requests);

    return 0;
}
