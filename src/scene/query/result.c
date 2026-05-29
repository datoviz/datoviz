/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query result helpers                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether two request ids belong to the same query freshness scope.
 *
 * @param lhs_request_id the first request id
 * @param rhs_request_id the second request id
 * @return true when the ids share one latest-wins scope
 */
static bool _query_result_request_ids_share_scope(
    uint64_t lhs_request_id, uint64_t rhs_request_id)
{
    if (lhs_request_id == 0 || rhs_request_id == 0)
        return lhs_request_id == 0 && rhs_request_id == 0;
    return lhs_request_id == rhs_request_id;
}



/**
 * Return the latest query freshness serial for one request scope.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @return latest freshness serial, or zero when absent
 */
static uint64_t _query_result_latest_request_serial(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    for (uint32_t i = 0; i < scene->query_scope_count; i++)
    {
        const DvzRequestFreshnessScope* scope = &scene->query_scopes[i];
        if (scope->panel == panel &&
            _query_result_request_ids_share_scope(scope->request_id, request_id))
        {
            return scope->freshness_serial;
        }
    }
    return 0;
}



/**
 * Return whether a query result scope is still current.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @param freshness_serial the originating freshness serial
 * @return true when the result is current
 */
static bool _query_result_is_current(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return true;
    uint64_t latest_serial = _query_result_latest_request_serial(scene, panel, request_id);
    return latest_serial == 0 || latest_serial == freshness_serial;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Drop queued query results superseded by a newer panel query.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the new request id
 */
void _dvz_scene_query_drop_superseded_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    DvzQueuedQueryResult kept[DVZ_SCENE_MAX_QUERY_RESULTS] = {0};
    uint32_t kept_count = 0;
    for (uint32_t i = 0; i < scene->query_result_count; i++)
    {
        uint32_t index = (scene->query_result_head + i) % DVZ_SCENE_MAX_QUERY_RESULTS;
        DvzQueuedQueryResult queued = scene->query_results[index];
        if (queued.panel == panel &&
            _query_result_request_ids_share_scope(queued.result.request_id, request_id))
        {
            continue;
        }
        kept[kept_count++] = queued;
    }
    dvz_memset(scene->query_results, sizeof(scene->query_results), 0, sizeof(scene->query_results));
    for (uint32_t i = 0; i < kept_count; i++)
        scene->query_results[i] = kept[i];
    scene->query_result_head = 0;
    scene->query_result_count = kept_count;
}



/**
 * Push one native query result.
 *
 * @param scene the scene
 * @param panel the panel
 * @param freshness_serial the originating freshness serial
 * @param result the result
 * @return true on success
 */
bool _dvz_scene_query_push_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzQueryResult* result)
{
    ANN(scene);
    ANN(result);
    if (panel != NULL &&
        !_query_result_is_current(scene, panel, result->request_id, freshness_serial))
    {
        return true;
    }
    if (scene->query_result_count >= DVZ_SCENE_MAX_QUERY_RESULTS)
    {
        log_error("query result queue is full");
        return false;
    }
    uint32_t index =
        (scene->query_result_head + scene->query_result_count) % DVZ_SCENE_MAX_QUERY_RESULTS;
    scene->query_results[index].panel = panel;
    scene->query_results[index].freshness_serial = freshness_serial;
    scene->query_results[index].result = *result;
    scene->query_result_count++;
    return true;
}



/**
 * Decode a standard r32uint item-id query payload.
 *
 * @param ctx decode context
 * @param family resolved visual family
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
bool _dvz_scene_query_decode_item_id(
    const DvzSceneQueryDecodeContext* ctx, DvzSceneVisualFamily family,
    DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->figure);
    ANN(ctx->build->visual);
    ANN(ctx->bytes);
    ANN(out_result);
    if (ctx->byte_size < sizeof(uint32_t))
    {
        out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
        return true;
    }

    uint32_t encoded = 0;
    dvz_memcpy(&encoded, sizeof(encoded), ctx->bytes, sizeof(encoded));
    if (encoded == 0)
        return false;

    uint64_t item_id = (uint64_t)encoded - 1u;
    DvzVisual* visual = ctx->build->visual;
    out_result->status = DVZ_QUERY_STATUS_HIT;
    out_result->hit = true;
    out_result->visual_id = _scene_visual_public_id(ctx->build->figure->scene, visual);
    out_result->visual_family = family;
    out_result->payload_version = 1;
    out_result->raw_target = DVZ_SCENE_TARGET_ITEM;
    out_result->raw_id = item_id;
    out_result->resolved_target = DVZ_SCENE_TARGET_ITEM;
    out_result->resolved_id = item_id;
    out_result->item_id = item_id;
    if (visual->link_keys != NULL && item_id < visual->link_key_count)
        out_result->link_key = visual->link_keys[item_id];
    return true;
}



/**
 * Poll one resolved query result.
 *
 * @param scene the scene
 * @param out_result output query result
 * @return true when a result was written
 */
bool dvz_scene_poll_query(DvzScene* scene, DvzQueryResult* out_result)
{
    ANN(scene);
    ANN(out_result);

    if (scene->query_result_count > 0)
    {
        uint32_t index = scene->query_result_head;
        *out_result = scene->query_results[index].result;
        dvz_memset(
            &scene->query_results[index], sizeof(DvzQueuedQueryResult), 0,
            sizeof(DvzQueuedQueryResult));
        scene->query_result_head = (index + 1) % DVZ_SCENE_MAX_QUERY_RESULTS;
        scene->query_result_count--;
        return true;
    }

    return false;
}
