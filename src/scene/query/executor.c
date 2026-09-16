/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query executor                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/drp2/runtime.h"
#include "../../drp2/_runtime.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether two runtime configs describe the same backend.
 *
 * @param a the first runtime config
 * @param b the second runtime config
 * @return true when the reusable query runtime can be kept
 */
static bool _query_runtime_config_matches(
    const DvzDrp2RuntimeConfig* a, const DvzDrp2RuntimeConfig* b)
{
    ANN(a);
    ANN(b);
    return a->device == b->device && a->allocator == b->allocator &&
           a->semantic_only == b->semantic_only;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a retained scene query executor.
 *
 * @param executor the query executor
 */
void _scene_request_executor_init(DvzSceneRequestExecutor* executor)
{
    ANN(executor);
    dvz_memset(executor, sizeof(DvzSceneRequestExecutor), 0, sizeof(DvzSceneRequestExecutor));
}



/**
 * Destroy a retained scene query executor.
 *
 * @param executor the query executor
 */
void _scene_request_executor_destroy(DvzSceneRequestExecutor* executor)
{
    if (executor == NULL)
        return;
    const bool timing_enabled = executor->timing_enabled;
    const uint64_t pending_superseded_count = executor->pending_superseded_count;
    const DvzSceneQueryTiming last_timing = executor->last_timing;
    if (executor->runtime != NULL)
        dvz_drp2_runtime_destroy(executor->runtime);
    if (executor->emitter != NULL)
        dvz_frame_plan_emitter_destroy(executor->emitter);
    dvz_memset(executor, sizeof(DvzSceneRequestExecutor), 0, sizeof(DvzSceneRequestExecutor));
    executor->timing_enabled = timing_enabled;
    executor->pending_superseded_count = pending_superseded_count;
    executor->last_timing = last_timing;
}



/**
 * Ensure a retained query executor is ready for the caller runtime's backend.
 *
 * @param executor the retained query executor
 * @param source_runtime the caller's main DRP2 runtime
 * @return true when the executor is ready
 */
bool _scene_request_executor_prepare(DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime)
{
    ANN(executor);
    ANN(source_runtime);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_get_config(source_runtime);
    if (executor->runtime != NULL && executor->emitter != NULL &&
        _query_runtime_config_matches(&executor->runtime_cfg, &runtime_cfg))
    {
        return true;
    }

    _scene_request_executor_destroy(executor);
    executor->emitter = dvz_frame_plan_emitter();
    if (executor->emitter == NULL)
    {
        log_error("scene query emitter creation failed");
        return false;
    }
    executor->emitter_create_count++;

    executor->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (executor->runtime == NULL)
    {
        log_error("scene query runtime creation failed");
        _scene_request_executor_destroy(executor);
        return false;
    }
    executor->runtime_cfg = runtime_cfg;
    executor->runtime_create_count++;
    _dvz_drp2_runtime_timing_enable(executor->runtime, executor->timing_enabled);
    return true;
}



void _dvz_scene_query_timing_enable(DvzScene* scene, bool enabled)
{
    ANN(scene);
    DvzSceneRequestExecutor* executor = &scene->query_executor;
    executor->timing_enabled = enabled;
    if (enabled)
        executor->pending_superseded_count = 0;
    if (executor->runtime != NULL)
        _dvz_drp2_runtime_timing_enable(executor->runtime, enabled);
    if (!enabled)
    {
        dvz_memset(
            &executor->last_timing, sizeof(DvzSceneQueryTiming), 0,
            sizeof(DvzSceneQueryTiming));
    }
}



bool _dvz_scene_query_timing_get(const DvzScene* scene, DvzSceneQueryTiming* timing)
{
    if (scene == NULL || timing == NULL || !scene->query_executor.timing_enabled)
        return false;
    *timing = scene->query_executor.last_timing;
    return true;
}
