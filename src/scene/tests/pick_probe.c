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

#include "_assertions.h"
#include "_compat.h"
#include "../_scene.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_process_requests_coalesces_pending_picks_before_execution(
    TstSuite* suite, TstItem* item)
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
    TstSuite* suite, TstItem* item)
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


int test_scene_pick_probe_queues_and_pinned_readout(TstSuite* suite, TstItem* item)
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
    dvz_pinned_readout_set_format(
        readout, &(DvzFormatDesc){.precision = 2, .suffix = " u"});
    AT(readout->has_format);
    AT(strcmp(readout->format.suffix, " u") == 0);
    dvz_pinned_readout_destroy(readout);
    AT(panel->pinned_readout_count == 0);

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
int test_scene_poll_pick_probe_clears_consumed_slots(TstSuite* suite, TstItem* item)
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
    TstSuite* suite, TstItem* item)
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
    TstSuite* suite, TstItem* item)
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
    TstSuite* suite, TstItem* item)
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
    TstSuite* suite, TstItem* item)
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
    TstSuite* suite, TstItem* item)
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


int test_scene_image_probe_transparent_pixel_misses(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    if (!_scene_vklite_runtime_available())
        return 0;

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
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
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
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 21);
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


int test_scene_image_probe_gpu_readback_failure_misses(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    if (!_scene_vklite_runtime_available())
        return 0;

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
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
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
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    scene->test.force_readback_download_failure = false;

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 22);
    AT(_captured_log_contains(suite, "scene readback buffer download forced to fail"));

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_process_pick_probe_requests(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    if (!_scene_vklite_runtime_available())
        return 0;

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
    float point_pos[1][3] = {{0.0f, 0.0f, 0.0f}};
    uint8_t point_color[1][4] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
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
    AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(pick.resolved_id == 0);
    AT(probe.hit);
    AT(probe.request_id == 12);
    AT(probe.value_kind == DVZ_PROBE_VALUE_VEC4);
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


int test_scene_point_pick_quadrants(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    if (!_scene_vklite_runtime_available())
        return 0;

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
    float point_pos[4][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    uint8_t point_color[4][4] = {
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

    const double coords[4][2] = {
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
        AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
        AT(pick.resolved_id == expected_ids[i]);
        AT(!dvz_scene_poll_pick(scene, &pick));
    }

    dvz_figure_resize(figure, 128, 64);
    AT(dvz_panel_pick(panel, 96.0, 16.0, &(DvzPickRequest){.request_id = 20}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);
    DvzPickResult resized_pick = {0};
    AT(dvz_scene_poll_pick(scene, &resized_pick));
    AT(resized_pick.hit);
    AT(resized_pick.request_id == 20);
    AT(resized_pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(resized_pick.resolved_id == 3);
    AT(!dvz_scene_poll_pick(scene, &resized_pick));

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


int test_scene_process_requests_preserves_caller_runtime(TstSuite* suite, TstItem* item)
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

    AT(dvz_figure_process_requests(figure, runtime, &caps) == 1);

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
 * Ensure image probes sample the requested panel position, not a fixed image pixel.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */


int test_scene_image_probe_respects_panel_request_position(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    if (!_scene_vklite_runtime_available())
        return 0;

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
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
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


int test_scene_image_probe_segment_rgba_hidden_visual(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);
    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("segment probe test skipped because GPU context creation failed");
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
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
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
            uint32_t label = y < 2 ? (x < 2 ? 17u : 258u) : (x < 2 ? 65537u : 0u);
            uint32_t i = 4 * (y * 4 + x);
            pixels[i + 0] = (uint8_t)(label & 0xffu);
            pixels[i + 1] = (uint8_t)((label >> 8) & 0xffu);
            pixels[i + 2] = (uint8_t)((label >> 16) & 0xffu);
            pixels[i + 3] = label == 0 ? 0 : 255;
        }
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    dvz_visual_set_pick_capabilities(image, DVZ_PICK_CAPABILITY_GROUP);
    dvz_visual_set_visible(image, false);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

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
    AT(dvz_panel_probe(
           panel, 48.0, 48.0,
           &(DvzProbeRequest){.request_id = 44, .target = DVZ_SCENE_TARGET_SEGMENT}) == 0);
    AT(dvz_figure_process_requests(figure, runtime, &caps) == 4);

    DvzProbeResult probe = {0};
    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 41);
    AT(probe.target == DVZ_SCENE_TARGET_SEGMENT);
    AT(probe.category_id == 17);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 42);
    AT(probe.category_id == 258);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(probe.hit);
    AT(probe.request_id == 43);
    AT(probe.category_id == 65537);

    AT(dvz_scene_poll_probe(scene, &probe));
    AT(!probe.hit);
    AT(probe.request_id == 44);

    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_probe_plan_rejects_size_overflow(TstSuite* suite, TstItem* item)
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
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
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
        suite, !_scene_image_probe_plan(panel, image, &pending, request_ndc, &plan));
    AT(_captured_log_contains(suite, "image probe request buffer size overflow"));
    AT(plan.plan == NULL);
    AT(plan.emitter == NULL);
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

    TEST_SIMPLE(test_scene_process_requests_coalesces_pending_picks_before_execution);
    TEST_SIMPLE(test_scene_process_requests_coalesces_pending_probes_before_execution);
    TEST_SIMPLE(test_scene_pick_probe_queues_and_pinned_readout);
    TEST_SIMPLE(test_scene_poll_pick_probe_clears_consumed_slots);
    TEST_SIMPLE(test_scene_pick_request_same_id_supersedes_older_unresolved);
    TEST_SIMPLE(test_scene_probe_request_zero_id_keeps_newest_unresolved);
    TEST_SIMPLE(test_scene_pick_request_distinct_ids_keep_independent_pending_and_results);
    TEST_SIMPLE(test_scene_pick_request_same_id_rejects_late_result_after_newer_poll);
    TEST_SIMPLE(test_scene_probe_request_zero_id_rejects_late_result_after_newer_poll);
    TEST_SIMPLE(test_scene_image_probe_transparent_pixel_misses);
    TEST_SIMPLE(test_scene_image_probe_gpu_readback_failure_misses);
    TEST_SIMPLE(test_scene_process_pick_probe_requests);
    TEST_SIMPLE(test_scene_point_pick_quadrants);
    TEST_SIMPLE(test_scene_process_requests_preserves_caller_runtime);
    TEST_SIMPLE(test_scene_image_probe_respects_panel_request_position);
    TEST_SIMPLE(test_scene_image_probe_segment_rgba_hidden_visual);
    TEST_SIMPLE(test_scene_image_probe_plan_rejects_size_overflow);

    return 0;
}
