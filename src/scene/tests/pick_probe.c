/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene pick and probe tests                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "../_scene.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_PICK_PROBE_GPU_CASE(test)                                                       \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(ctx)                                                  \
    do                                                                                            \
    {                                                                                             \
        if (!_scene_vklite_runtime_available())                                                   \
        {                                                                                         \
            tst_skip((ctx), "Vulkan instance creation failed");                                   \
            return 0;                                                                             \
        }                                                                                         \
    } while (0)




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_process_requests_coalesces_pending_picks_before_execution(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzFigure* other_figure = dvz_figure(scene, 320, 240, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzPanel* other_panel = dvz_panel(
        other_figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);
    ANN(other_panel);

    scene->pending_pick_count = 6;
    scene->pending_picks[0] =
        (DvzPendingPickRequest){.panel = panel, .x = 1.0, .y = 2.0, .freshness_serial = 1};
    scene->pending_picks[1] = (DvzPendingPickRequest){
        .panel = other_panel, .x = 100.0, .y = 200.0, .freshness_serial = 99};
    scene->pending_picks[2] =
        (DvzPendingPickRequest){.panel = panel, .x = 3.0, .y = 4.0, .freshness_serial = 2};
    scene->pending_picks[3] = (DvzPendingPickRequest){
        .panel = panel, .x = 5.0, .y = 6.0, .freshness_serial = 3, .request = {.request_id = 42}};
    scene->pending_picks[4] = (DvzPendingPickRequest){
        .panel = panel, .x = 7.0, .y = 8.0, .freshness_serial = 4, .request = {.request_id = 42}};
    scene->pending_picks[5] = (DvzPendingPickRequest){
        .panel = panel, .x = 9.0, .y = 10.0, .freshness_serial = 5, .request = {.request_id = 43}};

    DvzDrp2Runtime* runtime = (DvzDrp2Runtime*)scene;
    AT(dvz_figure_process_requests(figure, runtime, NULL) == 3);
    AT(scene->pending_pick_count == 1);
    AT(scene->pending_picks[0].panel == other_panel);
    AT(scene->pick_result_count == 3);

    DvzPickResult out = {0};
    AT(dvz_scene_poll_pick(scene, &out));
    AT(out.request_id == 0);
    AC(out.panel_position[0], 3.0, 1e-12);
    AC(out.panel_position[1], 4.0, 1e-12);
    AT(dvz_scene_poll_pick(scene, &out));
    AT(out.request_id == 42);
    AC(out.panel_position[0], 7.0, 1e-12);
    AC(out.panel_position[1], 8.0, 1e-12);
    AT(dvz_scene_poll_pick(scene, &out));
    AT(out.request_id == 43);
    AC(out.panel_position[0], 9.0, 1e-12);
    AC(out.panel_position[1], 10.0, 1e-12);
    AT(!dvz_scene_poll_pick(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure request processing coalesces stale pending probes before execution.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */


int test_scene_process_requests_coalesces_pending_probes_before_execution(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzFigure* other_figure = dvz_figure(scene, 320, 240, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzPanel* other_panel = dvz_panel(
        other_figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);
    ANN(other_panel);

    scene->pending_probe_count = 5;
    scene->pending_probes[0] =
        (DvzPendingProbeRequest){.panel = panel, .x = 1.0, .y = 2.0, .freshness_serial = 1};
    scene->pending_probes[1] = (DvzPendingProbeRequest){
        .panel = other_panel, .x = 100.0, .y = 200.0, .freshness_serial = 99};
    scene->pending_probes[2] =
        (DvzPendingProbeRequest){.panel = panel, .x = 3.0, .y = 4.0, .freshness_serial = 2};
    scene->pending_probes[3] = (DvzPendingProbeRequest){
        .panel = panel, .x = 5.0, .y = 6.0, .freshness_serial = 3, .request = {.request_id = 7}};
    scene->pending_probes[4] = (DvzPendingProbeRequest){
        .panel = panel, .x = 7.0, .y = 8.0, .freshness_serial = 4, .request = {.request_id = 7}};

    DvzDrp2Runtime* runtime = (DvzDrp2Runtime*)scene;
    AT(dvz_figure_process_requests(figure, runtime, NULL) == 2);
    AT(scene->pending_probe_count == 1);
    AT(scene->pending_probes[0].panel == other_panel);
    AT(scene->probe_result_count == 2);

    DvzProbeResult out = {0};
    AT(dvz_scene_poll_probe(scene, &out));
    AT(out.request_id == 0);
    AT(out.source_request_id == 0);
    AT(dvz_scene_poll_probe(scene, &out));
    AT(out.request_id == 7);
    AT(out.source_request_id == 7);
    AT(!dvz_scene_poll_probe(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure unsupported request targets are reported explicitly instead of as generic misses.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pick_probe_unsupported_targets(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_pick(
           panel, 12.0, 34.0,
           &(DvzPickRequest){.request_id = 81, .target = DVZ_SCENE_TARGET_FACE}) == 0);
    AT(dvz_panel_probe(
           panel, 9.0, 8.0,
           &(DvzProbeRequest){.request_id = 82, .target = DVZ_SCENE_TARGET_FACE}) == 0);
    AT(dvz_panel_pick(panel, 700.0, 34.0, &(DvzPickRequest){.request_id = 83}) == 0);
    AT(dvz_panel_probe(panel, 9.0, 500.0, &(DvzProbeRequest){.request_id = 84}) == 0);
    AT(dvz_figure_process_requests(figure, (DvzDrp2Runtime*)scene, NULL) == 4);

    DvzPickResult pick = {0};
    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 81);
    AT(pick.status == DVZ_PICK_STATUS_UNSUPPORTED_TARGET);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_NONE);
    AC(pick.panel_position[0], 12.0, 1e-12);
    AC(pick.panel_position[1], 34.0, 1e-12);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 82);
    AT(probe.status == DVZ_PROBE_STATUS_UNSUPPORTED_TARGET);
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_NONE);
    AC(probe.panel_position[0], 9.0, 1e-12);
    AC(probe.panel_position[1], 8.0, 1e-12);

    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 83);
    AT(pick.status == DVZ_PICK_STATUS_OUTSIDE_PANEL);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 84);
    AT(probe.status == DVZ_PROBE_STATUS_OUTSIDE_PANEL);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pick_probe_queues_and_pinned_readout(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_pick(panel, 12.0, 34.0, &(DvzPickRequest){.request_id = 5}) == 0);
    AT(dvz_panel_probe(panel, 9.0, 8.0, &(DvzProbeRequest){.request_id = 6}) == 0);
    AT(scene->pending_pick_count == 1);
    AT(scene->pending_probe_count == 1);
    AT(scene->pending_picks[0].request.request_id == 5);
    AT(scene->pending_probes[0].request.request_id == 6);

    DvzPickResult pick = {
        .request_id = 5,
        .hit = true,
        .panel_id = 1,
        .visual_id = 2,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 99,
    };
    DvzProbeResult probe = {
        .request_id = 6,
        .hit = true,
        .panel_id = 1,
        .visual_id = 3,
        .target = DVZ_SCENE_TARGET_SAMPLE,
        .target_id = 17,
        .value_kind = DVZ_PROBE_VALUE_SCALAR,
        .scalar = 3.5,
    };
    dvz_snprintf(probe.label, sizeof(probe.label), "%s", "density");

    AT(_dvz_scene_enqueue_pick_result(scene, &pick));
    AT(_dvz_scene_enqueue_probe_result(scene, &probe));

    DvzPickResult out_pick = {0};
    DvzProbeResult out_probe = {0};
    AT(dvz_scene_poll_pick(scene, &out_pick));
    AT(dvz_scene_poll_probe(scene, &out_probe));
    AT(out_pick.resolved_id == 99);
    AT(out_probe.scalar == 3.5);
    AT(!dvz_scene_poll_pick(scene, &out_pick));

    panel->hover.active = true;
    panel->hover.pick = out_pick;
    const DvzHoverState* hover = dvz_scene_hover(scene, panel);
    ANN(hover);
    AT(hover->active);
    AT(hover->pick.resolved_id == 99);

    DvzPinnedReadout* readout = dvz_pinned_readout(panel, &out_probe);
    ANN(readout);
    AT(panel->pinned_readout_count == 1);
    AT(readout->probe.scalar == 3.5);
    AT(strcmp(readout->text, "density: 3.5") == 0);
    AT(readout->card.dirty);
    readout->card.dirty = false;
    dvz_pinned_readout_set_format(
        readout, &(DvzFormatDesc){.precision = 2, .suffix = " u"});
    AT(readout->has_format);
    AT(strcmp(readout->format.suffix, " u") == 0);
    AT(strcmp(readout->text, "density: 3.50 u") == 0);
    AT(readout->card.dirty);

    DvzProbeResult rgba_probe = {
        .request_id = 7,
        .hit = true,
        .panel_id = 1,
        .visual_id = 3,
        .target = DVZ_SCENE_TARGET_PIXEL,
        .target_id = 3,
        .value_kind = DVZ_PROBE_VALUE_VEC4,
        .vector = {0.25, 0.5, 1.0, 0.75},
    };
    dvz_snprintf(rgba_probe.label, sizeof(rgba_probe.label), "%s", "rgba");
    DvzPinnedReadout* rgba_readout = dvz_pinned_readout(panel, &rgba_probe);
    ANN(rgba_readout);
    AT(panel->pinned_readout_count == 2);
    AT(strcmp(rgba_readout->text, "rgba: 0.25 0.5 1 0.75") == 0);

    _scene_prepare_text_visuals(figure);
    ANN(readout->card.background_visual);
    ANN(readout->card.text_visual);
    ANN(readout->card.text_visual->text.glyph_visual);
    ANN(rgba_readout->card.background_visual);
    ANN(rgba_readout->card.text_visual);
    AT(readout->card.background_visual->visible);
    AT(readout->card.text_visual->visible);
    AT(readout->card.text_visual->text.glyph_visual->visible);
    AT(!readout->card.dirty);
    AT(strcmp(readout->card.realized_text, readout->text) == 0);

    DvzVisual* background_visual = readout->card.background_visual;
    DvzVisual* text_visual = readout->card.text_visual;
    DvzVisual* glyph_visual = readout->card.text_visual->text.glyph_visual;
    dvz_pinned_readout_destroy(readout);
    AT(panel->pinned_readout_count == 1);
    AT(!background_visual->visible);
    AT(!text_visual->visible);
    AT(!glyph_visual->visible);
    dvz_pinned_readout_destroy(rgba_readout);
    AT(panel->pinned_readout_count == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_query_api_bridges_pick_and_probe_results(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_query(
           panel, 12.0, 34.0,
           &(DvzQueryRequest){.request_id = 41, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
    AT(scene->pending_pick_count == 1);
    AT(scene->pending_picks[0].request.request_id == 41);
    AT(scene->pending_picks[0].request.target == DVZ_SCENE_TARGET_ITEM);

    AT(dvz_panel_query(
           panel, 56.0, 78.0,
           &(DvzQueryRequest){.request_id = 42, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(scene->pending_probe_count == 1);
    AT(scene->pending_probes[0].request.request_id == 42);
    AT(scene->pending_probes[0].request.target == DVZ_SCENE_TARGET_SAMPLE);

    DvzPickResult pick = {
        .request_id = 41,
        .status = DVZ_PICK_STATUS_HIT,
        .hit = true,
        .panel_id = 1,
        .visual_id = 2,
        .visual_family = DVZ_SCENE_VISUAL_FAMILY_POINT,
        .resolved_target = DVZ_SCENE_TARGET_ITEM,
        .resolved_id = 7,
        .item_id = 7,
        .panel_position = {12.0, 34.0},
    };
    DvzProbeResult probe = {
        .request_id = 42,
        .status = DVZ_PROBE_STATUS_HIT,
        .hit = true,
        .panel_id = 1,
        .visual_id = 3,
        .visual_family = DVZ_SCENE_VISUAL_FAMILY_IMAGE,
        .target = DVZ_SCENE_TARGET_SAMPLE,
        .target_id = 9,
        .value_kind = DVZ_PROBE_VALUE_SCALAR,
        .scalar = 2.25,
        .panel_position = {56.0, 78.0},
        .has_coordinate = true,
        .coordinate = {0.5, 0.25, 0.0},
    };
    dvz_snprintf(probe.label, sizeof(probe.label), "%s", "intensity");

    AT(_dvz_scene_enqueue_pick_result(scene, &pick));
    AT(_dvz_scene_enqueue_probe_result(scene, &probe));

    DvzQueryResult query = {0};
    DvzQueryResult item_query = {0};
    DvzQueryResult value_query = {0};
    AT(dvz_scene_poll_query(scene, &query));
    item_query = query;
    AT(query.request_id == 41);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT);
    AT(query.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(query.resolved_id == 7);
    AT(query.value_kind == DVZ_QUERY_VALUE_NONE);

    AT(dvz_scene_poll_query(scene, &query));
    value_query = query;
    AT(query.request_id == 42);
    AT(query.status == DVZ_QUERY_STATUS_HIT);
    AT(query.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(query.resolved_target == DVZ_SCENE_TARGET_SAMPLE);
    AT(query.resolved_id == 9);
    AT(query.value_kind == DVZ_QUERY_VALUE_SCALAR);
    AC(query.scalar, 2.25, 1e-12);
    AT(query.has_data_position);
    AC(query.data_position[0], 0.5, 1e-12);
    AT(strcmp(query.label, "intensity") == 0);
    AT(!dvz_scene_poll_query(scene, &query));

    DvzSelection* selection = dvz_selection(
        scene, &(DvzSelectionDesc){.mode = DVZ_SELECT_REPLACE, .target = DVZ_SCENE_TARGET_ITEM});
    ANN(selection);
    AT(dvz_selection_apply_query(selection, &item_query) == 0);
    AT(dvz_selection_count(selection) == 1);

    DvzPinnedReadout* readout = dvz_pinned_readout_query(panel, &value_query);
    ANN(readout);
    AT(strcmp(readout->text, "intensity: 2.25") == 0);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure polling resolved pick/probe results clears the consumed queue storage.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_poll_pick_probe_clears_consumed_slots(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzPickResult pick = {
        .request_id = 11,
        .hit = true,
        .resolved_id = 101,
    };
    DvzProbeResult probe = {
        .request_id = 12,
        .source_request_id = 12,
        .hit = true,
        .scalar = 2.5,
    };

    AT(_dvz_scene_enqueue_pick_result(scene, &pick));
    AT(_dvz_scene_enqueue_probe_result(scene, &probe));
    scene->pick_results[0].panel = panel;
    scene->pick_results[0].freshness_serial = 21;
    scene->probe_results[0].panel = panel;
    scene->probe_results[0].freshness_serial = 22;

    DvzPickResult out_pick = {0};
    DvzProbeResult out_probe = {0};
    AT(dvz_scene_poll_pick(scene, &out_pick));
    AT(dvz_scene_poll_probe(scene, &out_probe));
    AT(out_pick.resolved_id == 101);
    AC(out_probe.scalar, 2.5, 1e-12);
    AT(scene->pick_result_count == 0);
    AT(scene->probe_result_count == 0);
    AT(scene->pick_result_head == 1);
    AT(scene->probe_result_head == 1);
    AT(scene->pick_results[0].panel == NULL);
    AT(scene->pick_results[0].freshness_serial == 0);
    AT(scene->pick_results[0].result.request_id == 0);
    AT(scene->pick_results[0].result.resolved_id == 0);
    AT(scene->probe_results[0].panel == NULL);
    AT(scene->probe_results[0].freshness_serial == 0);
    AT(scene->probe_results[0].result.request_id == 0);
    AT(scene->probe_results[0].result.source_request_id == 0);
    AT(!dvz_scene_poll_pick(scene, &out_pick));
    AT(!dvz_scene_poll_probe(scene, &out_probe));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pick_request_same_id_supersedes_older_unresolved(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_pick(panel, 10.0, 20.0, &(DvzPickRequest){.request_id = 77}) == 0);
    AT(dvz_panel_pick(panel, 30.0, 40.0, &(DvzPickRequest){.request_id = 77}) == 0);
    AT(scene->pending_pick_count == 1);
    AT(scene->pending_picks[0].request.request_id == 77);
    AC(scene->pending_picks[0].x, 30.0, 1e-12);
    AC(scene->pending_picks[0].y, 40.0, 1e-12);

    DvzPickResult stale = {.request_id = 77, .hit = true, .resolved_id = 1};
    DvzPickResult fresh = {.request_id = 77, .hit = true, .resolved_id = 2};
    AT(_dvz_scene_enqueue_pick_result(scene, &stale));
    scene->pick_results[0].panel = panel;
    AT(dvz_panel_pick(panel, 50.0, 60.0, &(DvzPickRequest){.request_id = 77}) == 0);
    AT(scene->pick_result_count == 0);
    AT(_dvz_scene_enqueue_pick_result(scene, &fresh));
    scene->pick_results[0].panel = panel;

    DvzPickResult out = {0};
    AT(dvz_scene_poll_pick(scene, &out));
    AT(out.resolved_id == 2);
    AT(!dvz_scene_poll_pick(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_probe_request_zero_id_keeps_newest_unresolved(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_probe(panel, 1.0, 2.0, NULL) == 0);
    AT(dvz_panel_probe(panel, 3.0, 4.0, NULL) == 0);
    AT(scene->pending_probe_count == 1);
    AT(scene->pending_probes[0].request.request_id == 0);
    AC(scene->pending_probes[0].x, 3.0, 1e-12);
    AC(scene->pending_probes[0].y, 4.0, 1e-12);

    DvzProbeResult stale = {.request_id = 0, .hit = true, .scalar = 1.0};
    DvzProbeResult fresh = {.request_id = 0, .hit = true, .scalar = 2.0};
    AT(_dvz_scene_enqueue_probe_result(scene, &stale));
    scene->probe_results[0].panel = panel;
    AT(dvz_panel_probe(panel, 5.0, 6.0, NULL) == 0);
    AT(scene->probe_result_count == 0);
    AT(_dvz_scene_enqueue_probe_result(scene, &fresh));
    scene->probe_results[0].panel = panel;

    DvzProbeResult out = {0};
    AT(dvz_scene_poll_probe(scene, &out));
    AC(out.scalar, 2.0, 1e-12);
    AT(!dvz_scene_poll_probe(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pick_request_distinct_ids_keep_independent_pending_and_results(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_pick(panel, 10.0, 20.0, &(DvzPickRequest){.request_id = 101}) == 0);
    AT(dvz_panel_pick(panel, 30.0, 40.0, &(DvzPickRequest){.request_id = 202}) == 0);
    AT(scene->pending_pick_count == 2);
    AT(scene->pending_picks[0].request.request_id == 101);
    AT(scene->pending_picks[1].request.request_id == 202);
    AT(scene->pending_picks[0].freshness_serial < scene->pending_picks[1].freshness_serial);

    DvzPickResult first = {.request_id = 101, .hit = true, .resolved_id = 11};
    DvzPickResult second = {.request_id = 202, .hit = true, .resolved_id = 22};
    AT(_dvz_scene_enqueue_pick_result(scene, &first));
    AT(_dvz_scene_enqueue_pick_result(scene, &second));
    scene->pick_results[0].panel = panel;
    scene->pick_results[0].freshness_serial = scene->pending_picks[0].freshness_serial;
    scene->pick_results[1].panel = panel;
    scene->pick_results[1].freshness_serial = scene->pending_picks[1].freshness_serial;

    AT(dvz_panel_pick(panel, 50.0, 60.0, &(DvzPickRequest){.request_id = 101}) == 0);
    AT(scene->pending_pick_count == 2);
    AT(scene->pending_picks[0].request.request_id == 202);
    AT(scene->pending_picks[1].request.request_id == 101);
    AT(scene->pick_result_count == 1);
    AT(scene->pick_results[0].result.request_id == 202);

    DvzPickResult out = {0};
    AT(dvz_scene_poll_pick(scene, &out));
    AT(out.request_id == 202);
    AT(out.resolved_id == 22);
    AT(!dvz_scene_poll_pick(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pick_request_same_id_rejects_late_result_after_newer_poll(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_pick(panel, 10.0, 20.0, &(DvzPickRequest){.request_id = 77}) == 0);
    uint64_t stale_serial = scene->pending_picks[0].freshness_serial;
    AT(dvz_panel_pick(panel, 30.0, 40.0, &(DvzPickRequest){.request_id = 77}) == 0);
    AT(scene->pending_pick_count == 1);
    uint64_t fresh_serial = scene->pending_picks[0].freshness_serial;
    AT(fresh_serial > stale_serial);

    DvzPickResult fresh = {.request_id = 77, .hit = true, .resolved_id = 2};
    DvzPickResult stale = {.request_id = 77, .hit = true, .resolved_id = 1};
    AT(_dvz_scene_enqueue_pick_result_scoped(scene, panel, fresh_serial, &fresh));
    DvzPickResult out = {0};
    AT(dvz_scene_poll_pick(scene, &out));
    AT(out.resolved_id == 2);
    AT(_dvz_scene_enqueue_pick_result_scoped(scene, panel, stale_serial, &stale));
    AT(!dvz_scene_poll_pick(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_probe_request_zero_id_rejects_late_result_after_newer_poll(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    AT(dvz_panel_probe(panel, 1.0, 2.0, NULL) == 0);
    uint64_t stale_serial = scene->pending_probes[0].freshness_serial;
    AT(dvz_panel_probe(panel, 3.0, 4.0, NULL) == 0);
    AT(scene->pending_probe_count == 1);
    uint64_t fresh_serial = scene->pending_probes[0].freshness_serial;
    AT(fresh_serial > stale_serial);

    DvzProbeResult fresh = {.request_id = 0, .hit = true, .scalar = 2.0};
    DvzProbeResult stale = {.request_id = 0, .hit = true, .scalar = 1.0};
    AT(_dvz_scene_enqueue_probe_result_scoped(scene, panel, fresh_serial, &fresh));
    DvzProbeResult out = {0};
    AT(dvz_scene_poll_probe(scene, &out));
    AC(out.scalar, 2.0, 1e-12);
    AT(_dvz_scene_enqueue_probe_result_scoped(scene, panel, stale_serial, &stale));
    AT(!dvz_scene_poll_probe(scene, &out));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_probe_transparent_pixel_misses(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("transparent image probe test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 1] = 0;
        pixels[4 * i + 2] = 0;
        pixels[4 * i + 3] = 0;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    tst_log_capture_begin(suite);
    AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = 21}) == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 21);
    AT(probe.status == DVZ_PROBE_STATUS_MISS);
    AT(_captured_log_contains(suite, "returned a transparent GPU pixel"));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure image probes miss when GPU readback fails and no CPU fallback exists.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */


int test_scene_image_probe_gpu_readback_failure_misses(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("image probe GPU failure test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 1] = 0;
        pixels[4 * i + 2] = 0;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    scene->test.force_readback_download_failure = true;
    tst_log_capture_begin(suite);
    AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = 22}) == 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_figure_process_requests(figure, runtime, &caps) == 1);
    scene->test.force_readback_download_failure = false;

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 22);
    AT(probe.status == DVZ_PROBE_STATUS_READBACK_FAILED);
    AT(_captured_log_contains(suite, "scene readback buffer download forced to fail"));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


static int test_scene_volume_slice_probe_cpu_sample(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    float values[27] = {0};
    for (uint32_t i = 0; i < 27; i++)
        values[i] = (float)i;

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 3,
                   .height = 3,
                   .depth = 3,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){
            .data = values,
            .bytes_per_row = 3 * sizeof(float),
            .rows_per_image = 3,
        }));

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    AT(dvz_panel_probe(
           panel, 32.0, 32.0,
           &(DvzProbeRequest){.request_id = 71, .target = DVZ_SCENE_TARGET_SAMPLE}) == 0);
    AT(dvz_figure_process_requests(figure, (DvzDrp2Runtime*)scene, NULL) == 1);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 71);
    AT(probe.status == DVZ_PROBE_STATUS_HIT);
    AT(probe.visual_id == _scene_visual_public_id(scene, volume));
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(probe.target == DVZ_SCENE_TARGET_SAMPLE);
    AT(probe.target_id == 13);
    AT(probe.auxiliary_id == 13);
    AT(probe.value_kind == DVZ_PROBE_VALUE_SCALAR);
    AC(probe.scalar, 13.0, 1e-12);
    AT(probe.has_coordinate);
    AT(probe.has_uvw);
    AC(probe.coordinate[0], 0.0, 1e-12);
    AC(probe.coordinate[1], 0.0, 1e-12);
    AC(probe.coordinate[2], 0.0, 1e-12);
    AC(probe.uvw[0], 0.5, 1e-12);
    AC(probe.uvw[1], 0.5, 1e-12);
    AC(probe.uvw[2], 0.5, 1e-12);
    AT(!dvz_scene_poll_probe(scene, &probe));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_process_pick_probe_requests(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene pick/probe processing test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    dvz_visual_set_pick_capabilities(points, DVZ_PICK_CAPABILITY_ITEM);
    DvzLinkChannel* channel = dvz_link_channel(scene, "pick-links");
    uint64_t point_link_keys[1] = {1234};
    ANN(channel);
    AT(dvz_visual_set_link_keys(points, channel, point_link_keys, 1) == 0);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 1] = 0;
        pixels[4 * i + 2] = 0;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 11}) == 0);
    AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = 12}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 2);

    DvzPickResult pick = {0};
    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(pick.hit);
    AT(pick.request_id == 11);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 0);
    AT(pick.raw_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.raw_id == 0);
    AT(pick.item_id == 0);
    AT(pick.link_key == 1234);
    AT(probe.hit);
    AT(probe.request_id == 12);
    AT(probe.status == DVZ_PROBE_STATUS_HIT);
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(probe.target == DVZ_SCENE_TARGET_PIXEL);
    AT(probe.value_kind == DVZ_PROBE_VALUE_VEC4);
    AC(probe.panel_position[0], 32.0, 1e-12);
    AC(probe.panel_position[1], 32.0, 1e-12);
    AT(probe.vector[0] > 0.9);
    AT(probe.vector[1] < 0.1);
    AT(probe.vector[2] < 0.1);
    AT(probe.vector[3] > 0.9);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure point picking resolves the point at each requested panel coordinate.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_scene_point_pick_quadrants(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene point-pick quadrant test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    dvz_visual_set_pick_capabilities(points, DVZ_PICK_CAPABILITY_ITEM);
    vec3 point_pos[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    DvzColor point_color[4] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 0, 255},
    };
    float point_size[4] = {18.0f, 18.0f, 18.0f, 18.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 4) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 4) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 4) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    const dvec2 coords[4] = {
        {16.0, 16.0},
        {48.0, 16.0},
        {16.0, 48.0},
        {48.0, 48.0},
    };
    const uint64_t expected_ids[4] = {2, 3, 0, 1};
    for (uint32_t i = 0; i < 4; i++)
    {
        AT(dvz_panel_pick(panel, coords[i][0], coords[i][1], &(DvzPickRequest){.request_id = i + 1}) ==
           0);
        AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

        DvzPickResult pick = {0};
        AT(dvz_scene_poll_pick(scene, &pick));
        AT(pick.hit);
        AT(pick.request_id == i + 1);
        AT(pick.status == DVZ_PICK_STATUS_HIT);
        AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT);
        AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
        AT(pick.resolved_id == expected_ids[i]);
        AT(pick.item_id == expected_ids[i]);
        AT(!dvz_scene_poll_pick(scene, &pick));
    }

    dvz_figure_resize(figure, 128, 64);
    AT(dvz_panel_pick(panel, 96.0, 16.0, &(DvzPickRequest){.request_id = 20}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult resized_pick = {0};
    AT(dvz_scene_poll_pick(scene, &resized_pick));
    AT(resized_pick.hit);
    AT(resized_pick.request_id == 20);
    AT(resized_pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT);
    AT(resized_pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(resized_pick.resolved_id == 3);
    AT(!dvz_scene_poll_pick(scene, &resized_pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure GPU point picking uses the circular point mask, not the square sprite bounds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_pick_rejects_disc_corner(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene circular point-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    dvz_visual_set_pick_capabilities(point, DVZ_PICK_CAPABILITY_ITEM);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 255, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(point, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(point, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 42.0, 42.0, &(DvzPickRequest){.request_id = 51}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 51);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure GPU pixel picking keeps square mark semantics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pixel_pick_accepts_square_corner(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene square pixel-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* pixel = dvz_pixel(scene, 0);
    ANN(pixel);
    dvz_visual_set_pick_capabilities(pixel, DVZ_PICK_CAPABILITY_ITEM);
    vec3 pixel_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor pixel_color[1] = {{255, 255, 255, 255}};
    float pixel_size[1] = {24.0f};
    AT(dvz_visual_set_data(pixel, "position", pixel_pos, 1) == 0);
    AT(dvz_visual_set_data(pixel, "color", pixel_color, 1) == 0);
    AT(dvz_visual_set_data(pixel, "size", pixel_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 42.0, 42.0, &(DvzPickRequest){.request_id = 52}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 52);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_PIXEL);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 0);
    AT(pick.item_id == 0);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify marker picking uses the first-slice bounding-box fallback.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_marker_pick_accepts_bbox_corner(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene marker bbox-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* marker = dvz_marker(scene, 0);
    ANN(marker);
    dvz_visual_set_pick_capabilities(marker, DVZ_PICK_CAPABILITY_ITEM);
    vec3 marker_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor marker_color[1] = {{255, 255, 255, 255}};
    float marker_size[1] = {24.0f};
    float marker_angle[1] = {0.0f};
    uint32_t marker_shape[1] = {DVZ_MARKER_SHAPE_DISC};
    AT(dvz_visual_set_data(marker, "position", marker_pos, 1) == 0);
    AT(dvz_visual_set_data(marker, "color", marker_color, 1) == 0);
    AT(dvz_visual_set_data(marker, "size", marker_size, 1) == 0);
    AT(dvz_visual_set_data(marker, "angle", marker_angle, 1) == 0);
    AT(dvz_visual_set_data(marker, "shape", marker_shape, 1) == 0);
    AT(dvz_panel_add_visual(panel, marker, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 42.0, 42.0, &(DvzPickRequest){.request_id = 53}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 53);
    AT(pick.visual_id == 1);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_MARKER);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 0);
    AT(pick.item_id == 0);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure sphere picking resolves item identity through the GPU impostor pick pass.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_pick_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene sphere-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    dvz_visual_set_pick_capabilities(sphere, DVZ_PICK_CAPABILITY_ITEM);
    vec3 positions[2] = {
        {-0.5f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };
    DvzColor colors[2] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
    };
    float radii[2] = {0.2f, 0.25f};
    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 2},
        {.attr_name = "color", .data = colors, .item_count = 2},
        {.attr_name = "radius", .data = radii, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(sphere, sphere_updates, 3) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 61}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 61);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_id == _scene_visual_public_id(scene, sphere));
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_SPHERE);
    AT(pick.raw_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.raw_id == 1);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 1);
    AT(pick.item_id == 1);
    AT(!pick.has_data_position);
    AT(!dvz_scene_poll_pick(scene, &pick));

    AT(dvz_panel_pick(panel, 47.0, 47.0, &(DvzPickRequest){.request_id = 62}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 62);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure segment and stroked path picking resolve item identity through GPU stroke masks.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_stroke_pick_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene stroke-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* segment = dvz_segment(scene, 0);
    ANN(segment);
    dvz_visual_set_pick_capabilities(segment, DVZ_PICK_CAPABILITY_ITEM);
    vec3 segment_start[2] = {
        {-0.75f, -0.5f, 0.0f},
        {-0.75f, 0.0f, 0.0f},
    };
    vec3 segment_end[2] = {
        {0.75f, -0.5f, 0.0f},
        {0.75f, 0.0f, 0.0f},
    };
    DvzColor segment_color[2] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float segment_width[2] = {12.0f, 12.0f};
    AT(dvz_visual_set_data(segment, "position_start", segment_start, 2) == 0);
    AT(dvz_visual_set_data(segment, "position_end", segment_end, 2) == 0);
    AT(dvz_visual_set_data(segment, "color", segment_color, 2) == 0);
    AT(dvz_visual_set_data(segment, "line_width", segment_width, 2) == 0);
    AT(dvz_segment_set_caps(segment, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0);
    AT(dvz_panel_add_visual(panel, segment, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 71}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 71);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_SEGMENT);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 1);
    AT(pick.item_id == 1);
    AT(!dvz_scene_poll_pick(scene, &pick));

    AT(dvz_panel_pick(panel, 32.0, 20.0, &(DvzPickRequest){.request_id = 72}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 72);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);

    scene = dvz_scene();
    ANN(scene);
    figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* path = dvz_path(scene, 0);
    ANN(path);
    dvz_visual_set_pick_capabilities(path, DVZ_PICK_CAPABILITY_ITEM);
    vec3 path_position[3] = {
        {-0.75f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.75f, 0.0f, 0.0f},
    };
    DvzColor path_color[3] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float path_width[3] = {12.0f, 12.0f, 12.0f};
    AT(dvz_visual_set_data(path, "position", path_position, 3) == 0);
    AT(dvz_visual_set_data(path, "color", path_color, 3) == 0);
    AT(dvz_visual_set_data(path, "line_width", path_width, 3) == 0);
    AT(dvz_path_set_caps(path, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) == 0);
    AT(dvz_path_set_join(path, DVZ_PATH_JOIN_MITER, 4.0f) == 0);
    AT(dvz_panel_add_visual(panel, path, NULL) == 0);

    runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    AT(dvz_panel_pick(panel, 48.0, 32.0, &(DvzPickRequest){.request_id = 73}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 73);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_PATH);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 1);
    AT(pick.item_id == 1);
    AT(!dvz_scene_poll_pick(scene, &pick));

    AT(dvz_panel_pick(panel, 48.0, 20.0, &(DvzPickRequest){.request_id = 74}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 74);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure primitive and indexed mesh picking resolve GPU primitive identity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_primitive_pick_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene primitive-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(primitive);
    dvz_visual_set_pick_capabilities(primitive, DVZ_PICK_CAPABILITY_ITEM);
    vec3 primitive_pos[6] = {
        {-0.9f, -0.7f, 0.0f},
        {-0.9f, 0.7f, 0.0f},
        {-0.1f, 0.0f, 0.0f},
        {0.1f, -0.7f, 0.0f},
        {0.1f, 0.7f, 0.0f},
        {0.9f, 0.0f, 0.0f},
    };
    DvzColor primitive_color[6] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    DvzVisualDataUpdate primitive_updates[] = {
        {.attr_name = "position", .data = primitive_pos, .item_count = 6},
        {.attr_name = "color", .data = primitive_color, .item_count = 6},
    };
    AT(dvz_visual_set_data_many(primitive, primitive_updates, 2) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    AT(dvz_panel_pick(panel, 48.0, 32.0, &(DvzPickRequest){.request_id = 81}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 81);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 1);
    AT(pick.item_id == 1);
    AT(!dvz_scene_poll_pick(scene, &pick));

    AT(dvz_panel_pick(panel, 32.0, 8.0, &(DvzPickRequest){.request_id = 82}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 82);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);

    scene = dvz_scene();
    ANN(scene);
    figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    dvz_visual_set_pick_capabilities(mesh, DVZ_PICK_CAPABILITY_ITEM);
    vec3 mesh_pos[4] = {
        {-0.8f, -0.8f, 0.0f},
        {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},
        {0.8f, 0.8f, 0.0f},
    };
    vec3 mesh_normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)));
    DvzVisualDataUpdate mesh_updates[] = {
        {.attr_name = "position", .data = mesh_pos, .item_count = 4},
        {.attr_name = "normal", .data = mesh_normals, .item_count = 4},
    };
    AT(dvz_visual_set_data_many(mesh, mesh_updates, 2) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    AT(dvz_panel_pick(panel, 48.0, 32.0, &(DvzPickRequest){.request_id = 83}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 83);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_MESH);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 1);
    AT(pick.item_id == 1);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure per-item image picking resolves image item identity through GPU quads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_pick_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene image-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_pick_capabilities(image, DVZ_PICK_CAPABILITY_ITEM);
    vec3 positions[2] = {
        {-0.5f, 0.0f, 0.0f},
        {0.5f, 0.0f, 0.0f},
    };
    vec2 extents[2] = {
        {0.4f, 0.4f},
        {0.4f, 0.4f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(image, "extent", extents, 2) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 48.0, 32.0, &(DvzPickRequest){.request_id = 91}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 91);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 1);
    AT(pick.item_id == 1);
    AT(!dvz_scene_poll_pick(scene, &pick));

    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 92}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 92);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure picking honors panel visual order across different visual families.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pick_respects_visual_order_across_families(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene cross-family pick-order test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    dvz_visual_set_pick_capabilities(point, DVZ_PICK_CAPABILITY_ITEM);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 255, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(point, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(point, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    dvz_visual_set_pick_capabilities(image, DVZ_PICK_CAPABILITY_ITEM);
    vec3 image_pos[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 image_extent[1] = {{0.8f, 0.8f}};
    AT(dvz_visual_set_data(image, "position", image_pos, 1) == 0);
    AT(dvz_visual_set_data(image, "extent", image_extent, 1) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 101}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 101);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(pick.visual_id == _scene_visual_public_id(scene, image));
    AT(pick.item_id == 0);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure volume picking resolves visual identity through the GPU proxy geometry.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_pick_resolves_item(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene volume-pick test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    dvz_visual_set_pick_capabilities(volume, DVZ_PICK_CAPABILITY_ITEM);
    double bounds_min[3] = {-0.4, -0.4, -0.4};
    double bounds_max[3] = {+0.4, +0.4, +0.4};
    AT(dvz_volume_set_bounds(volume, bounds_min, bounds_max) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 111}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult pick = {0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(pick.hit);
    AT(pick.request_id == 111);
    AT(pick.status == DVZ_PICK_STATUS_HIT);
    AT(pick.visual_family == DVZ_SCENE_VISUAL_FAMILY_VOLUME);
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 0);
    AT(pick.item_id == 0);
    AT(!dvz_scene_poll_pick(scene, &pick));

    AT(dvz_panel_pick(panel, 4.0, 4.0, &(DvzPickRequest){.request_id = 112}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    pick = (DvzPickResult){0};
    AT(dvz_scene_poll_pick(scene, &pick));
    AT(!pick.hit);
    AT(pick.request_id == 112);
    AT(pick.status == DVZ_PICK_STATUS_MISS);
    AT(!dvz_scene_poll_pick(scene, &pick));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure pick/probe readbacks do not reset the caller-owned DRP2 runtime.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */


int test_scene_process_requests_preserves_caller_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        setup, 900, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);
    dvz_drp2_stream_destroy(setup);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float position[3] = {0};
    DvzColor color = {255, 255, 255, 255};
    float size = 8.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    dvz_visual_set_pick_capabilities(visual, DVZ_PICK_CAPABILITY_ITEM);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = 7}) == 0);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzDrp2CommandStream* cleanup = dvz_drp2_stream();
    ANN(cleanup);
    AT(dvz_drp2_stream_destroy_buffer(cleanup, 900));
    result = dvz_drp2_runtime_execute(runtime, cleanup);
    AT(result.ok);

    dvz_drp2_stream_destroy(cleanup);
    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure repeated image probes reuse one retained request runtime and emitter.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_probe_reuses_retained_request_executor(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 1] = 255;
        pixels[4 * i + 2] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    DvzSceneRequestExecutor executor = {0};
    _scene_request_executor_init(&executor);
    scene->test.force_readback_download_failure = true;

    tst_log_capture_begin(suite);
    AT(dvz_panel_probe(panel, 16.0, 16.0, &(DvzProbeRequest){.request_id = 101}) == 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, _dvz_figure_process_requests_with_executor(figure, runtime, &executor, &caps) == 1);
    AT(executor.runtime_create_count == 1);
    AT(executor.emitter_create_count == 1);
    AT(executor.image_probe_static_upload_count == 1);
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(executor.emitter, "_rb");
    AT(rb_id != 0);
    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);

    AT(dvz_panel_probe(panel, 48.0, 48.0, &(DvzProbeRequest){.request_id = 102}) == 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, _dvz_figure_process_requests_with_executor(figure, runtime, &executor, &caps) == 1);
    AT(executor.runtime_create_count == 1);
    AT(executor.emitter_create_count == 1);
    AT(executor.image_probe_static_upload_count == 1);
    AT(dvz_frame_plan_emitter_object_id(executor.emitter, "_rb") == rb_id);
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);

    pixels[0] = 128;
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = 103}) == 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, _dvz_figure_process_requests_with_executor(figure, runtime, &executor, &caps) == 1);
    AT(executor.runtime_create_count == 1);
    AT(executor.emitter_create_count == 1);
    AT(executor.image_probe_static_upload_count == 2);
    AT(dvz_frame_plan_emitter_object_id(executor.emitter, "_rb") == rb_id);
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);

    scene->test.force_readback_download_failure = false;
    _scene_request_executor_destroy(&executor);
    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure image probes sample the requested panel position, not a fixed image pixel.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */


int test_scene_image_probe_respects_panel_request_position(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("image probe position test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t y = 0; y < 4; y++)
    {
        for (uint32_t x = 0; x < 4; x++)
        {
            uint32_t i = 4 * (y * 4 + x);
            if (x < 2 && y < 2)
            {
                pixels[i + 0] = 255;
                pixels[i + 1] = 0;
                pixels[i + 2] = 0;
                pixels[i + 3] = 255;
            }
            else if (x >= 2 && y < 2)
            {
                pixels[i + 0] = 0;
                pixels[i + 1] = 255;
                pixels[i + 2] = 0;
                pixels[i + 3] = 255;
            }
            else if (x < 2 && y >= 2)
            {
                pixels[i + 0] = 0;
                pixels[i + 1] = 0;
                pixels[i + 2] = 255;
                pixels[i + 3] = 255;
            }
            else
            {
                pixels[i + 0] = 255;
                pixels[i + 1] = 255;
                pixels[i + 2] = 0;
                pixels[i + 3] = 255;
            }
        }
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_probe(panel, 16.0, 16.0, &(DvzProbeRequest){.request_id = 31}) == 0);
    AT(dvz_panel_probe(panel, 48.0, 16.0, &(DvzProbeRequest){.request_id = 32}) == 0);
    AT(dvz_panel_probe(panel, 16.0, 48.0, &(DvzProbeRequest){.request_id = 33}) == 0);
    AT(dvz_panel_probe(panel, 48.0, 48.0, &(DvzProbeRequest){.request_id = 34}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 4);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 31);
    AT(probe.status == DVZ_PROBE_STATUS_HIT);
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(probe.target == DVZ_SCENE_TARGET_PIXEL);
    AT(probe.vector[0] > 0.9);
    AT(probe.vector[1] < 0.1);
    AT(probe.vector[2] < 0.1);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 32);
    AT(probe.vector[0] < 0.1);
    AT(probe.vector[1] > 0.9);
    AT(probe.vector[2] < 0.1);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 33);
    AT(probe.vector[0] < 0.1);
    AT(probe.vector[1] < 0.1);
    AT(probe.vector[2] > 0.9);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 34);
    AT(probe.vector[0] > 0.9);
    AT(probe.vector[1] > 0.9);
    AT(probe.vector[2] < 0.1);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure image probes support retained position/extent image rectangles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_probe_generated_rect_respects_panel_position(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("generated image probe position test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extent[1] = {{2.0f, 2.0f}};
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t y = 0; y < 4; y++)
    {
        for (uint32_t x = 0; x < 4; x++)
        {
            uint32_t i = 4 * (y * 4 + x);
            if (x < 2 && y < 2)
            {
                pixels[i + 0] = 255;
                pixels[i + 3] = 255;
            }
            else if (x >= 2 && y < 2)
            {
                pixels[i + 1] = 255;
                pixels[i + 3] = 255;
            }
            else if (x < 2 && y >= 2)
            {
                pixels[i + 2] = 255;
                pixels[i + 3] = 255;
            }
            else
            {
                pixels[i + 0] = 255;
                pixels[i + 1] = 255;
                pixels[i + 3] = 255;
            }
        }
    }
    AT(dvz_visual_set_data(image, "position", position, 1) == 0);
    AT(dvz_visual_set_data(image, "extent", extent, 1) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_probe(panel, 16.0, 16.0, &(DvzProbeRequest){.request_id = 41}) == 0);
    AT(dvz_panel_probe(panel, 48.0, 16.0, &(DvzProbeRequest){.request_id = 42}) == 0);
    AT(dvz_panel_probe(panel, 16.0, 48.0, &(DvzProbeRequest){.request_id = 43}) == 0);
    AT(dvz_panel_probe(panel, 48.0, 48.0, &(DvzProbeRequest){.request_id = 44}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 4);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 41);
    AT(probe.vector[0] > 0.9);
    AT(probe.vector[1] < 0.1);
    AT(probe.vector[2] < 0.1);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 42);
    AT(probe.vector[0] < 0.1);
    AT(probe.vector[1] > 0.9);
    AT(probe.vector[2] < 0.1);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 43);
    AT(probe.vector[0] < 0.1);
    AT(probe.vector[1] < 0.1);
    AT(probe.vector[2] > 0.9);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 44);
    AT(probe.vector[0] > 0.9);
    AT(probe.vector[1] > 0.9);
    AT(probe.vector[2] < 0.1);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure generated image rectangle probes resolve non-black samples on outer texel bins.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_probe_generated_rect_edge_bins(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    enum
    {
        TEX_WIDTH = 16,
        TEX_HEIGHT = 10,
        FIG_WIDTH = 980,
        FIG_HEIGHT = 680,
    };
    const float image_extent_x = 1.9f;
    const float image_extent_y = 1.2f;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("generated image probe edge test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, FIG_WIDTH, FIG_HEIGHT, 0);
    ANN(figure);
    DvzPanelDesc panel_desc = {.x = 0.045f, .y = 0.06f, .width = 0.91f, .height = 0.88f};
    DvzPanel* panel = dvz_panel(figure, panel_desc);
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extent[1] = {{image_extent_x, image_extent_y}};
    uint8_t pixels[TEX_WIDTH * TEX_HEIGHT * 4] = {0};
    for (uint32_t y = 0; y < TEX_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < TEX_WIDTH; x++)
        {
            uint32_t i = 4 * (y * TEX_WIDTH + x);
            pixels[i + 0] = (uint8_t)(32u + 7u * x);
            pixels[i + 1] = (uint8_t)(48u + 13u * y);
            pixels[i + 2] = 180;
            pixels[i + 3] = 255;
        }
    }
    AT(dvz_visual_set_data(image, "position", position, 1) == 0);
    AT(dvz_visual_set_data(image, "extent", extent, 1) == 0);
    AT(dvz_visual_set_texture(image, pixels, TEX_WIDTH, TEX_HEIGHT) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    double panel_width = (double)panel_desc.width * (double)FIG_WIDTH;
    double panel_height = (double)panel_desc.height * (double)FIG_HEIGHT;
    double image_width = 0.5 * (double)image_extent_x * panel_width;
    double image_height = 0.5 * (double)image_extent_y * panel_height;
    double image_x0 = 0.5 * (panel_width - image_width);
    double image_y0 = 0.5 * (panel_height - image_height);

    uint32_t expected = 0;
    for (uint32_t y = 0; y < TEX_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < TEX_WIDTH; x++)
        {
            bool edge = x == 0 || y == 0 || x == TEX_WIDTH - 1 || y == TEX_HEIGHT - 1;
            if (!edge)
                continue;

            double px = image_x0 + ((double)x + 0.5) * image_width / (double)TEX_WIDTH;
            double py = image_y0 + ((double)y + 0.5) * image_height / (double)TEX_HEIGHT;
            uint64_t request_id = 1000u + y * TEX_WIDTH + x;
            AT(dvz_panel_probe(panel, px, py, &(DvzProbeRequest){.request_id = request_id}) == 0);
            expected++;
        }
    }
    uint32_t processed = dvz_figure_process_requests(figure, runtime, &caps);
    AT(processed == expected);

    for (uint32_t i = 0; i < expected; i++)
    {
        DvzProbeResult probe = {0};
        AT(dvz_scene_poll_probe(scene, &probe));
        AT(probe.hit);
        AT(probe.vector[0] > 0.05);
        AT(probe.vector[1] > 0.05);
        AT(probe.vector[2] > 0.5);
        AT(probe.vector[3] > 0.9);
    }

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure image probes ignore fixed image overlays and sample the data visual below them.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_probe_skips_fixed_image_overlays(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("fixed image overlay probe test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t red_pixels[4 * 4 * 4] = {0};
    uint8_t green_pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        red_pixels[4 * i + 0] = 255;
        red_pixels[4 * i + 3] = 255;
        green_pixels[4 * i + 1] = 255;
        green_pixels[4 * i + 3] = 255;
    }

    DvzVisual* data_image = dvz_image(scene, 0);
    ANN(data_image);
    AT(dvz_visual_set_data(data_image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(data_image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(data_image, red_pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, data_image, &(DvzVisualAttachDesc){.z_layer = 0}) == 0);

    DvzVisual* fixed_overlay = dvz_image(scene, 0);
    ANN(fixed_overlay);
    AT(dvz_visual_set_data(fixed_overlay, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(fixed_overlay, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(fixed_overlay, green_pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, fixed_overlay,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = 35}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 35);
    AT(probe.status == DVZ_PROBE_STATUS_HIT);
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(probe.target == DVZ_SCENE_TARGET_PIXEL);
    AT(probe.vector[0] > 0.9);
    AT(probe.vector[1] < 0.1);
    AT(probe.vector[2] < 0.1);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure labels probes read raw signed integer IDs from the labels field.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_probe_raw_integer_field(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("labels probe test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    vec3 positions[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(labels, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(labels, "texcoords", texcoords, 4) == 0);

    int32_t label_data[4 * 4] = {0};
    label_data[3 * 4 + 1] = -7;
    label_data[3 * 4 + 3] = 17;
    label_data[1 * 4 + 3] = 42;
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = label_data,
                   .bytes_per_row = 4 * sizeof(int32_t),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(labels, "field", field));
    AT(dvz_labels_set_background(labels, 0) == 0);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL, .label = "segments"});
    ANN(scale);
    DvzScaleCategory categories[2] = {
        {.category_id = -7, .order = 0, .label = "negative seven", .color = {255, 0, 0, 255}},
        {.category_id = 17, .order = 1, .label = "seventeen", .color = {0, 255, 0, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));
    AT(dvz_visual_set_scale(labels, "labels", scale) == 0);
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_probe(
           panel, 16.0, 16.0,
           &(DvzProbeRequest){.request_id = 41, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_panel_probe(
           panel, 48.0, 16.0,
           &(DvzProbeRequest){.request_id = 42, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_panel_probe(
           panel, 16.0, 48.0,
           &(DvzProbeRequest){.request_id = 43, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 3);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 41);
    AT(probe.status == DVZ_PROBE_STATUS_HIT);
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_LABELS);
    AT(probe.target == DVZ_SCENE_TARGET_SEGMENT);
    AT(probe.category_id == -7);
    AT(probe.value_kind == DVZ_PROBE_VALUE_LABEL);
    AT(probe.scale == scale);
    AT(strcmp(probe.label, "negative seven") == 0);
    AT(probe.has_uvw);
    AT(probe.uvw[0] > 0.2 && probe.uvw[0] < 0.3);
    AT(probe.uvw[1] > 0.7 && probe.uvw[1] < 0.8);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 42);
    AT(probe.category_id == 17);
    AT(strcmp(probe.label, "seventeen") == 0);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 43);
    AT(probe.status == DVZ_PROBE_STATUS_MISS);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure labels probes preserve high unsigned 32-bit category IDs.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_labels_probe_high_unsigned_id(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    TST_SCENE_PICK_PROBE_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("labels high-id probe test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    vec3 positions[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(labels, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(labels, "texcoords", texcoords, 4) == 0);

    uint32_t label_data[4 * 4] = {0};
    label_data[3 * 4 + 3] = 4000000000u;
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = label_data,
                   .bytes_per_row = 4 * sizeof(uint32_t),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(labels, "field", field));
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzCapabilitySnapshot caps = {0};
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    AT(dvz_panel_probe(
           panel, 48.0, 16.0,
           &(DvzProbeRequest){.request_id = 45, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 45);
    AT(probe.status == DVZ_PROBE_STATUS_HIT);
    AT(probe.visual_family == DVZ_SCENE_VISUAL_FAMILY_LABELS);
    AT(probe.category_id == 4000000000LL);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_probe_plan_rejects_size_overflow(TstContext* suite, const TstCase* item)
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

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4] = {255, 0, 0, 255};
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 1, 1) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);
    ANN(image->field);

    image->field->desc.width = UINT32_MAX;
    image->field->desc.height = UINT32_MAX;

    DvzSceneProbePlan plan = {0};
    DvzPendingProbeRequest pending = {
        .panel = panel,
        .x = 32.0,
        .y = 32.0,
        .request = {.request_id = 90},
        .freshness_serial = 1,
    };
    vec2 request_ndc = {0.0f, 0.0f};

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, !_scene_image_probe_plan(panel, image, &pending, request_ndc, true, &plan));
    AT(_captured_log_contains(suite, "image probe request buffer size overflow"));
    AT(plan.plan == NULL);
    AT(plan.probe_positions == NULL);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Register scene pick and probe tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_pick_probe(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("pick-probe");

    TST_CASE(test_scene_process_requests_coalesces_pending_picks_before_execution);
    TST_CASE(test_scene_process_requests_coalesces_pending_probes_before_execution);
    TST_CASE(test_scene_pick_probe_unsupported_targets);
    TST_CASE(test_scene_pick_probe_queues_and_pinned_readout);
    TST_CASE(test_scene_query_api_bridges_pick_and_probe_results);
    TST_CASE(test_scene_poll_pick_probe_clears_consumed_slots);
    TST_CASE(test_scene_pick_request_same_id_supersedes_older_unresolved);
    TST_CASE(test_scene_probe_request_zero_id_keeps_newest_unresolved);
    TST_CASE(test_scene_pick_request_distinct_ids_keep_independent_pending_and_results);
    TST_CASE(test_scene_pick_request_same_id_rejects_late_result_after_newer_poll);
    TST_CASE(test_scene_probe_request_zero_id_rejects_late_result_after_newer_poll);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_probe_transparent_pixel_misses);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_probe_gpu_readback_failure_misses);
    TST_CASE(test_scene_volume_slice_probe_cpu_sample);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_process_pick_probe_requests);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_point_pick_quadrants);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_point_pick_rejects_disc_corner);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_pixel_pick_accepts_square_corner);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_marker_pick_accepts_bbox_corner);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_sphere_pick_resolves_item);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_stroke_pick_resolves_item);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_primitive_pick_resolves_item);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_pick_resolves_item);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_pick_respects_visual_order_across_families);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_volume_pick_resolves_item);
    TST_CASE(test_scene_process_requests_preserves_caller_runtime);
    TST_CASE(test_scene_image_probe_reuses_retained_request_executor);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_probe_respects_panel_request_position);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_probe_generated_rect_respects_panel_position);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_probe_generated_rect_edge_bins);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_image_probe_skips_fixed_image_overlays);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_labels_probe_raw_integer_field);
    TST_SCENE_PICK_PROBE_GPU_CASE(test_scene_labels_probe_high_unsigned_id);
    TST_CASE(test_scene_image_probe_plan_rejects_size_overflow);

    return 0;
}
