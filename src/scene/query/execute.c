/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query execution                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "../_scene.h"
#include "_assertions.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Execute one native query with a visual-family operation table.
 *
 * @param figure the figure
 * @param runtime the caller runtime
 * @param executor retained query executor
 * @param caps capability snapshot
 * @param pending pending query request
 * @param request_ndc panel-local NDC coordinate
 * @param profile selected query profile
 * @param visual candidate visual
 * @param ops family operation table
 * @param out_result output result
 * @return true when the family produced a terminal result
 */
bool _dvz_scene_query_execute_family(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, DvzQueryProfile profile, DvzVisual* visual,
    const DvzSceneQueryFamilyOps* ops, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(executor);
    ANN(caps);
    ANN(pending);
    ANN(request_ndc);
    ANN(visual);
    ANN(ops);
    ANN(out_result);
    if (ops->build == NULL || ops->decode == NULL)
        return false;
    if (profile != DVZ_QUERY_PROFILE_U32_R32)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE;
        return true;
    }
    if (runtime == NULL)
    {
        out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        return true;
    }
    if (!_scene_request_executor_prepare(executor, runtime))
    {
        out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        return true;
    }

    DvzSceneQueryBuildContext build = {
        .figure = figure,
        .panel = pending->panel,
        .visual = visual,
        .pending = pending,
        .caps = caps,
        .profile = profile,
    };
    build.request_ndc[0] = request_ndc[0];
    build.request_ndc[1] = request_ndc[1];

    DvzSceneQueryPlan plan = {0};
    if (!ops->build(&build, &plan))
    {
        _scene_probe_plan_destroy(&plan.scratch);
        return false;
    }

    uint8_t bytes[DVZ_SCENE_QUERY_PAYLOAD_WORDS * sizeof(uint32_t)] = {0};
    bool executed = false;
    bool ok = false;
    if (ops->execute != NULL)
    {
        ok = ops->execute(&build, executor, caps, &plan, bytes, plan.byte_size, &executed);
    }
    else
    {
        ok = _dvz_scene_query_execute_readback(
            figure->scene, executor, caps, plan.scratch.plan, plan.target_width,
            plan.target_height, plan.format, bytes, plan.byte_size, &executed);
    }
    if (!ok)
    {
        out_result->status =
            executed ? DVZ_QUERY_STATUS_READBACK_FAILED : DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        _scene_probe_plan_destroy(&plan.scratch);
        return true;
    }

    DvzSceneQueryDecodeContext decode = {
        .build = &build,
        .plan = &plan,
        .bytes = bytes,
        .byte_size = plan.byte_size,
    };
    if (!ops->decode(&decode, out_result))
    {
        _scene_probe_plan_destroy(&plan.scratch);
        return false;
    }

    if (ops->readout != NULL)
    {
        DvzSceneQueryReadoutContext readout = {
            .build = &build,
            .plan = &plan,
        };
        if (!ops->readout(&readout, out_result))
            out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
    }

    _scene_probe_plan_destroy(&plan.scratch);
    return true;
}
