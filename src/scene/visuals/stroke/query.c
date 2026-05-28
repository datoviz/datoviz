/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stroke visual query helpers                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one retained stroke attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
bool _stroke_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    ANN(out_attr);
    int attr_idx = _attr_index(visual, attr_name);
    if (attr_idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[attr_idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    *out_attr = attr;
    return true;
}



/**
 * Allocate one temporary stroke query buffer with checked size arithmetic.
 *
 * @param label diagnostic family label
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @return true when allocation succeeds
 */
bool _stroke_query_alloc(
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
 * Return the offscreen stroke query target extent for one panel.
 *
 * @param figure parent figure
 * @param panel panel receiving the query
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
bool _stroke_query_target_extent(
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
 * Apply the request-centered MVP and viewport to a stroke query render node.
 *
 * @param plan frame plan
 * @param panel panel receiving the query
 * @param request_ndc request coordinate in panel-local NDC
 * @param target_width offscreen target width
 * @param target_height offscreen target height
 */
void _stroke_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const float* request_ndc, uint32_t target_width,
    uint32_t target_height)
{
    ANN(plan);
    ANN(panel);
    ANN(request_ndc);
    DvzFramePlanNode* render = dvz_frame_plan_last_render_node(plan);
    if (render == NULL)
        return;

    DvzMVP mvp = {0};
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



/**
 * Mark the most recent stroke query upload node as an index buffer.
 *
 * @param plan the frame plan
 * @param stride index item stride in bytes
 */
void _stroke_query_mark_last_upload_index(DvzFramePlan* plan, uint32_t stride)
{
    ANN(plan);
    DvzFramePlanNode* node = plan->count > 0 ? &plan->nodes[plan->count - 1] : NULL;
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
    node->u.upload.item_stride = stride;
}



/**
 * Mark the most recent stroke query upload node as a material uniform buffer.
 *
 * @param plan the frame plan
 */
void _stroke_query_mark_last_upload_uniform(DvzFramePlan* plan)
{
    ANN(plan);
    DvzFramePlanNode* node = plan->count > 0 ? &plan->nodes[plan->count - 1] : NULL;
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_UNIFORM;
    node->u.upload.item_stride = sizeof(DvzSceneMaterialParams);
}
