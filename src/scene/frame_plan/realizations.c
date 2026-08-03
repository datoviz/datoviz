/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan physical realization map                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_overflow.h"
#include "frame_plan/internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _ensure_realization_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->realizations == NULL || plan->realization_capacity == 0)
    {
        plan->realization_capacity = DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY;
        plan->realizations = (DvzSceneGraphRealization*)dvz_calloc(
            plan->realization_capacity, sizeof(DvzSceneGraphRealization));
        return plan->realizations != NULL;
    }
    if (plan->realization_count < plan->realization_capacity)
        return true;
    if (plan->realization_capacity > UINT32_MAX / 2)
        return false;
    const uint32_t capacity = plan->realization_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzSceneGraphRealization), &bytes))
        return false;
    DvzSceneGraphRealization* realizations =
        (DvzSceneGraphRealization*)dvz_realloc(plan->realizations, bytes);
    if (realizations == NULL)
        return false;
    plan->realization_capacity = capacity;
    plan->realizations = realizations;
    return true;
}



static bool _realization_key_valid(const DvzSceneGraphRealization* realization)
{
    ANN(realization);
    if (realization->panel_id[0] == '\0' ||
        (realization->ref_kind != DVZ_SCENE_RESOURCE_REF_PRODUCT &&
         realization->ref_kind != DVZ_SCENE_RESOURCE_REF_SCRATCH))
        return false;
    if (realization->ref_kind == DVZ_SCENE_RESOURCE_REF_PRODUCT)
        return realization->product_id.value != 0 && realization->scratch_id.value == 0;
    return realization->scratch_id.value != 0 && realization->product_id.value == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append one typed semantic/scratch reference to physical graph-resource realization.
 *
 * @param plan destination frame plan
 * @param realization immutable realization entry
 * @return whether the entry exists identically or was appended
 */
bool _frame_plan_realization_append(
    DvzFramePlan* plan, const DvzSceneGraphRealization* realization)
{
    if (plan == NULL || realization == NULL || !_realization_key_valid(realization) ||
        realization->graph_resource_index >= plan->graph_resource_count)
        return false;
    const DvzSceneGraphRealization* existing = _frame_plan_realization_get(
        plan, realization->panel_id, realization->ref_kind, realization->product_id,
        realization->scratch_id);
    if (existing != NULL)
        return existing->graph_resource_index == realization->graph_resource_index;
    if (!_ensure_realization_capacity(plan))
        return false;
    plan->realizations[plan->realization_count++] = *realization;
    return true;
}



/**
 * Resolve one typed semantic/scratch reference to its physical graph resource.
 *
 * @param plan source frame plan
 * @param panel_id panel identity
 * @param ref_kind product or scratch reference kind
 * @param product_id product identity when ref_kind is product
 * @param scratch_id scratch identity when ref_kind is scratch
 * @return immutable realization entry, or NULL when absent
 */
const DvzSceneGraphRealization* _frame_plan_realization_get(
    const DvzFramePlan* plan, const char* panel_id, DvzSceneResourceRefKind ref_kind,
    DvzRenderProductId product_id, DvzSceneScratchResourceId scratch_id)
{
    if (plan == NULL || panel_id == NULL || panel_id[0] == '\0')
        return NULL;
    for (uint32_t i = 0; i < plan->realization_count; i++)
    {
        const DvzSceneGraphRealization* realization = &plan->realizations[i];
        if (strcmp(realization->panel_id, panel_id) == 0 && realization->ref_kind == ref_kind &&
            (ref_kind != DVZ_SCENE_RESOURCE_REF_PRODUCT ||
             realization->product_id.value == product_id.value) &&
            (ref_kind != DVZ_SCENE_RESOURCE_REF_SCRATCH ||
             realization->scratch_id.value == scratch_id.value))
            return realization;
    }
    return NULL;
}
