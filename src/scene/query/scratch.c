/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query scratch storage                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_alloc.h"
#include "_log.h"
#include "_overflow.h"
#include "datoviz/math/_cglm.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Destroy a synthetic query frame plan wrapper.
 *
 * @param plan the plan wrapper
 */
void _scene_query_scratch_destroy(DvzSceneQueryScratch* plan)
{
    if (plan == NULL)
        return;
    dvz_frame_plan_destroy(plan->plan);
    plan->plan = NULL;
    dvz_free(plan->query_positions);
    plan->query_positions = NULL;
    dvz_free(plan->query_texcoords);
    plan->query_texcoords = NULL;
    dvz_free(plan->query_colors);
    plan->query_colors = NULL;
    dvz_free(plan->query_ids);
    plan->query_ids = NULL;
    dvz_free(plan->query_position_start);
    plan->query_position_start = NULL;
    dvz_free(plan->query_position_curr);
    plan->query_position_curr = NULL;
    dvz_free(plan->query_position_end);
    plan->query_position_end = NULL;
    dvz_free(plan->query_line_width);
    plan->query_line_width = NULL;
    dvz_free(plan->query_path_flags);
    plan->query_path_flags = NULL;
    dvz_free(plan->query_path_distance);
    plan->query_path_distance = NULL;
    dvz_free(plan->query_indices);
    plan->query_indices = NULL;
}



/**
 * Allocate a temporary query buffer with checked size arithmetic.
 *
 * @param label diagnostic query family label
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @return true when allocation succeeds
 */
bool _dvz_scene_query_alloc(
    const char* label, void** out_ptr, uint64_t count, uint64_t item_size)
{
    ANN(label);
    ANN(out_ptr);
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, item_size, &bytes) || bytes > SIZE_MAX)
    {
        log_error("%s query request buffer size overflow", label);
        return false;
    }
    void* ptr = dvz_calloc((size_t)count, (size_t)item_size);
    if (ptr == NULL && bytes > 0)
    {
        log_error("%s query request buffer allocation failed", label);
        return false;
    }
    *out_ptr = ptr;
    return true;
}



/**
 * Return the offscreen query target extent for one panel.
 *
 * @param figure parent figure
 * @param panel panel receiving the query
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
bool _dvz_scene_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height)
{
    ANN(figure);
    ANN(panel);
    ANN(out_target_width);
    ANN(out_target_height);
    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;
    uint32_t target_width = (uint32_t)(panel_width + 0.5);
    uint32_t target_height = (uint32_t)(panel_height + 0.5);
    *out_target_width = target_width == 0 ? 1 : target_width;
    *out_target_height = target_height == 0 ? 1 : target_height;
    return true;
}



/**
 * Apply the request-centered MVP and viewport to a query render node.
 *
 * @param plan frame plan
 * @param panel panel receiving the query
 * @param request_ndc request coordinate in panel-local NDC
 * @param target_width offscreen target width
 * @param target_height offscreen target height
 */
void _dvz_scene_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc,
    uint32_t target_width, uint32_t target_height)
{
    ANN(plan);
    ANN(panel);
    ANN(request_ndc);
    DvzFramePlanNode* render = dvz_frame_plan_last_render_node(plan);
    if (render == NULL)
        return;

    const DvzPanelAttach* attach = NULL;
    if (visual != NULL)
    {
        for (uint32_t i = 0; i < panel->visual_count; i++)
        {
            if (panel->visuals[i].visual == visual)
            {
                attach = &panel->visuals[i];
                break;
            }
        }
    }
    DvzMVP mvp = {0};
    if (attach == NULL || !_scene_panel_attachment_mvp(panel, visual, attach, NULL, &mvp))
        _scene_panel_apply_mvp(panel, &mvp);
    vec2 target_ndc = {
        -1.0f + 1.0f / (float)target_width,
        1.0f - 1.0f / (float)target_height,
    };
    vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
    mvp.proj[3][0] -= delta[0];
    mvp.proj[3][1] -= delta[1];
    render->u.render.has_mvp = true;
    render->u.render.apply_mvp = mvp;
    render->u.render.has_viewport = true;
    render->u.render.viewport =
        (DvzSceneViewportUniform){0.0f, 0.0f, (float)target_width, (float)target_height};
    render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
}
