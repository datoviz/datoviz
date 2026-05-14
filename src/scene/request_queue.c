/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene pick/probe request queues                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static bool _scene_request_ids_share_scope(uint64_t lhs_request_id, uint64_t rhs_request_id);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether two request ids belong to the same coalescing scope.
 *
 * Anonymous zero-id requests use one latest-wins scope per panel/kind. Explicit non-zero request
 * ids only coalesce with matching ids on the same panel/kind.
 *
 * @param lhs_request_id the first request id
 * @param rhs_request_id the second request id
 * @return true when the ids belong to the same coalescing scope
 */
static bool _scene_request_ids_share_scope(uint64_t lhs_request_id, uint64_t rhs_request_id)
{
    if (lhs_request_id == 0 || rhs_request_id == 0)
        return lhs_request_id == 0 && rhs_request_id == 0;
    return lhs_request_id == rhs_request_id;
}



/*************************************************************************************************/
/*  Result queues                                                                                */
/*************************************************************************************************/

/**
 * Append one resolved pick result to the scene queue.
 *
 * @param scene the scene
 * @param panel the owning panel, or NULL for synthetic test injection
 * @param freshness_serial the originating request freshness serial
 * @param result the result payload
 * @return true on success, false when the queue is full
 */
bool _scene_push_pick_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzPickResult* result)
{
    ANN(scene);
    ANN(result);
    if (panel != NULL &&
        !_scene_pick_request_is_current(scene, panel, result->request_id, freshness_serial))
    {
        return true;
    }
    if (scene->pick_result_count >= DVZ_SCENE_MAX_PICK_RESULTS)
    {
        log_error("pick result queue is full");
        return false;
    }
    uint32_t index =
        (scene->pick_result_head + scene->pick_result_count) % DVZ_SCENE_MAX_PICK_RESULTS;
    scene->pick_results[index].panel = panel;
    scene->pick_results[index].freshness_serial = freshness_serial;
    scene->pick_results[index].result = *result;
    scene->pick_result_count++;
    return true;
}



/**
 * Append one resolved probe result to the scene queue.
 *
 * @param scene the scene
 * @param panel the owning panel, or NULL for synthetic test injection
 * @param freshness_serial the originating request freshness serial
 * @param result the result payload
 * @return true on success, false when the queue is full
 */
bool _scene_push_probe_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzProbeResult* result)
{
    ANN(scene);
    ANN(result);
    if (panel != NULL &&
        !_scene_probe_request_is_current(scene, panel, result->request_id, freshness_serial))
    {
        return true;
    }
    if (scene->probe_result_count >= DVZ_SCENE_MAX_PROBE_RESULTS)
    {
        log_error("probe result queue is full");
        return false;
    }
    uint32_t index =
        (scene->probe_result_head + scene->probe_result_count) % DVZ_SCENE_MAX_PROBE_RESULTS;
    scene->probe_results[index].panel = panel;
    scene->probe_results[index].freshness_serial = freshness_serial;
    scene->probe_results[index].result = *result;
    scene->probe_result_count++;
    return true;
}



/**
 * Push one resolved pick result into the internal scene queue.
 *
 * @param scene the scene
 * @param result the resolved result
 * @return true on success
 */
bool _dvz_scene_enqueue_pick_result(DvzScene* scene, const DvzPickResult* result)
{
    return _scene_push_pick_result(scene, NULL, 0, result);
}



/**
 * Push one scoped pick result into the internal scene queue.
 *
 * @param scene the scene
 * @param panel the owning panel
 * @param freshness_serial the originating request freshness serial
 * @param result the resolved result
 * @return true on success
 */
bool _dvz_scene_enqueue_pick_result_scoped(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzPickResult* result)
{
    return _scene_push_pick_result(scene, panel, freshness_serial, result);
}



/**
 * Push one resolved probe result into the internal scene queue.
 *
 * @param scene the scene
 * @param result the resolved result
 * @return true on success
 */
bool _dvz_scene_enqueue_probe_result(DvzScene* scene, const DvzProbeResult* result)
{
    return _scene_push_probe_result(scene, NULL, 0, result);
}



/**
 * Push one scoped probe result into the internal scene queue.
 *
 * @param scene the scene
 * @param panel the owning panel
 * @param freshness_serial the originating request freshness serial
 * @param result the resolved result
 * @return true on success
 */
bool _dvz_scene_enqueue_probe_result_scoped(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzProbeResult* result)
{
    return _scene_push_probe_result(scene, panel, freshness_serial, result);
}



/*************************************************************************************************/
/*  Pending request queues                                                                       */
/*************************************************************************************************/

/**
 * Coalesce pending pick requests for one figure before execution.
 *
 * For the current v0.4 slice, anonymous zero-id requests keep only the newest request per panel,
 * while explicit non-zero ids keep only the newest request for that same panel/id pair.
 *
 * @param scene the scene
 * @param figure the figure being processed
 */
void _scene_coalesce_pending_pick_requests(DvzScene* scene, const DvzFigure* figure)
{
    ANN(scene);
    ANN(figure);
    bool keep[DVZ_SCENE_MAX_PENDING_REQUESTS] = {0};
    uint32_t write = 0;
    uint32_t old_count = scene->pending_pick_count;

    for (int32_t i = (int32_t)scene->pending_pick_count - 1; i >= 0; i--)
    {
        const DvzPendingPickRequest* pending = &scene->pending_picks[i];
        bool keep_pending = true;
        if (pending->panel != NULL && pending->panel->figure == figure)
        {
            for (uint32_t j = (uint32_t)i + 1; j < scene->pending_pick_count; j++)
            {
                const DvzPendingPickRequest* newer = &scene->pending_picks[j];
                if (!keep[j])
                    continue;
                if (newer->panel != pending->panel)
                    continue;
                if (!_scene_request_ids_share_scope(
                        newer->request.request_id, pending->request.request_id))
                {
                    continue;
                }
                keep_pending = false;
                break;
            }
        }
        keep[i] = keep_pending;
    }

    for (uint32_t read = 0; read < old_count; read++)
    {
        if (!keep[read])
            continue;
        if (write != read)
            scene->pending_picks[write] = scene->pending_picks[read];
        write++;
    }
    for (uint32_t i = write; i < old_count; i++)
    {
        dvz_memset(
            &scene->pending_picks[i], sizeof(DvzPendingPickRequest), 0,
            sizeof(DvzPendingPickRequest));
    }
    scene->pending_pick_count = write;
}



/**
 * Coalesce pending probe requests for one figure before execution.
 *
 * For the current v0.4 slice, anonymous zero-id requests keep only the newest request per panel,
 * while explicit non-zero ids keep only the newest request for that same panel/id pair.
 *
 * @param scene the scene
 * @param figure the figure being processed
 */
void _scene_coalesce_pending_probe_requests(DvzScene* scene, const DvzFigure* figure)
{
    ANN(scene);
    ANN(figure);
    bool keep[DVZ_SCENE_MAX_PENDING_REQUESTS] = {0};
    uint32_t write = 0;
    uint32_t old_count = scene->pending_probe_count;

    for (int32_t i = (int32_t)scene->pending_probe_count - 1; i >= 0; i--)
    {
        const DvzPendingProbeRequest* pending = &scene->pending_probes[i];
        bool keep_pending = true;
        if (pending->panel != NULL && pending->panel->figure == figure)
        {
            for (uint32_t j = (uint32_t)i + 1; j < scene->pending_probe_count; j++)
            {
                const DvzPendingProbeRequest* newer = &scene->pending_probes[j];
                if (!keep[j])
                    continue;
                if (newer->panel != pending->panel)
                    continue;
                if (!_scene_request_ids_share_scope(
                        newer->request.request_id, pending->request.request_id))
                {
                    continue;
                }
                keep_pending = false;
                break;
            }
        }
        keep[i] = keep_pending;
    }

    for (uint32_t read = 0; read < old_count; read++)
    {
        if (!keep[read])
            continue;
        if (write != read)
            scene->pending_probes[write] = scene->pending_probes[read];
        write++;
    }
    for (uint32_t i = write; i < old_count; i++)
    {
        dvz_memset(
            &scene->pending_probes[i], sizeof(DvzPendingProbeRequest), 0,
            sizeof(DvzPendingProbeRequest));
    }
    scene->pending_probe_count = write;
}



/**
 * Remove one pending pick request by queue index.
 *
 * @param scene the scene
 * @param index the queue index
 */
void _scene_remove_pending_pick_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_pick_count);
    for (uint32_t i = index + 1; i < scene->pending_pick_count; i++)
        scene->pending_picks[i - 1] = scene->pending_picks[i];
    scene->pending_pick_count--;
    dvz_memset(
        &scene->pending_picks[scene->pending_pick_count], sizeof(DvzPendingPickRequest), 0,
        sizeof(DvzPendingPickRequest));
}



/**
 * Remove one pending probe request by queue index.
 *
 * @param scene the scene
 * @param index the queue index
 */
void _scene_remove_pending_probe_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_probe_count);
    for (uint32_t i = index + 1; i < scene->pending_probe_count; i++)
        scene->pending_probes[i - 1] = scene->pending_probes[i];
    scene->pending_probe_count--;
    dvz_memset(
        &scene->pending_probes[scene->pending_probe_count], sizeof(DvzPendingProbeRequest), 0,
        sizeof(DvzPendingProbeRequest));
}
