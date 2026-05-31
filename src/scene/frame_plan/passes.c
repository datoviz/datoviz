/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan node passes                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append a compute node.
 *
 * @param plan the FramePlan
 * @param shader_key the shader key
 * @param x dispatch workgroup count in X
 * @param y dispatch workgroup count in Y
 * @param z dispatch workgroup count in Z
 * @return whether the node was appended
 */
bool dvz_frame_plan_compute(
    DvzFramePlan* plan, const char* shader_key, uint32_t x, uint32_t y, uint32_t z)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(
        node->u.compute.shader_key, DVZ_SCENE_LABEL_SIZE, shader_key ? shader_key : "");
    node->u.compute.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    node->u.compute.dispatch[0] = x;
    node->u.compute.dispatch[1] = y;
    node->u.compute.dispatch[2] = z;
    return true;
}



/**
 * Add a resource read to the most recent compute node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @return whether the resource was appended
 */
bool dvz_frame_plan_compute_read(DvzFramePlan* plan, const char* resource_id)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL || node->u.compute.read_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    _frame_plan_copy_label(
        node->u.compute.reads[node->u.compute.read_count], DVZ_SCENE_LABEL_SIZE,
        resource_id ? resource_id : "");
    node->u.compute.read_count++;
    return true;
}



/**
 * Add a resource write to the most recent compute node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @return whether the resource was appended
 */
bool dvz_frame_plan_compute_write(DvzFramePlan* plan, const char* resource_id)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL || node->u.compute.write_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    _frame_plan_copy_label(
        node->u.compute.writes[node->u.compute.write_count], DVZ_SCENE_LABEL_SIZE,
        resource_id ? resource_id : "");
    node->u.compute.write_count++;
    return true;
}



/**
 * Append a render node.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param picking whether the node renders picking output
 * @return whether the node was appended
 */
bool dvz_frame_plan_render(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking)
{
    return dvz_frame_plan_render_panel(
        plan, panel_id, render_target_id, picking, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
}



/**
 * Append a render node with explicit panel bounds.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param picking whether the node renders picking output
 * @param desc the panel bounds
 * @return whether the node was appended
 */
bool dvz_frame_plan_render_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc)
{
    return dvz_frame_plan_render_panel_role(
        plan, panel_id, render_target_id, picking, desc,
        picking ? DVZ_FRAME_PLAN_RENDER_PASS_PICKING : DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
}



/**
 * Append a render node with explicit panel bounds and pass role.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param picking whether the node renders picking output
 * @param desc the panel bounds
 * @param pass_role the render pass role
 * @return whether the node was appended
 */
bool dvz_frame_plan_render_panel_role(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc, DvzFramePlanRenderPassRole pass_role)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(
        node->u.render.panel_id, DVZ_SCENE_LABEL_SIZE, panel_id ? panel_id : "");
    _frame_plan_copy_label(
        node->u.render.render_target_id, DVZ_SCENE_LABEL_SIZE,
        render_target_id ? render_target_id : "");
    node->u.render.picking = picking;
    node->u.render.pass_role = picking ? DVZ_FRAME_PLAN_RENDER_PASS_PICKING : pass_role;
    node->u.render.desc = desc;
    return true;
}



/**
 * Return the most recently appended render node.
 *
 * @param plan the FramePlan
 * @return the render node, or NULL
 */
DvzFramePlanNode* dvz_frame_plan_last_render_node(DvzFramePlan* plan)
{
    return _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
}



/**
 * Append a clear-only render node.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @return whether the node was appended
 */
bool dvz_frame_plan_clear(DvzFramePlan* plan, const char* panel_id, const char* render_target_id)
{
    return dvz_frame_plan_clear_panel(
        plan, panel_id, render_target_id, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
}



/**
 * Append a clear-only render node with explicit panel bounds.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param desc the panel bounds
 * @return whether the node was appended
 */
bool dvz_frame_plan_clear_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(node->u.clear.panel_id, DVZ_SCENE_LABEL_SIZE, panel_id ? panel_id : "");
    _frame_plan_copy_label(
        node->u.clear.render_target_id, DVZ_SCENE_LABEL_SIZE,
        render_target_id ? render_target_id : "");
    node->u.clear.desc = desc;
    return true;
}



/**
 * Add a visual to the most recent render node.
 *
 * @param plan the FramePlan
 * @param visual_id the visual id
 * @return whether the visual was appended
 */
bool dvz_frame_plan_render_visual(DvzFramePlan* plan, const char* visual_id)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL || node->u.render.visual_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;
    _frame_plan_copy_label(
        node->u.render.visuals[node->u.render.visual_count], DVZ_SCENE_LABEL_SIZE,
        visual_id ? visual_id : "");
    node->u.render.visual_count++;
    return true;
}



/**
 * Attach typed metadata to the most recently appended render visual.
 *
 * @param plan the FramePlan
 * @param metadata the visual metadata
 * @return whether the metadata was attached
 */
bool dvz_frame_plan_render_visual_metadata(
    DvzFramePlan* plan, const DvzFramePlanVisualMeta* metadata)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL || metadata == NULL || node->u.render.visual_count == 0)
        return false;
    uint32_t index = node->u.render.visual_count - 1;
    dvz_memcpy(
        &node->u.render.visual_metadata[index], sizeof(DvzFramePlanVisualMeta), metadata,
        sizeof(DvzFramePlanVisualMeta));
    node->u.render.visual_metadata[index].has_metadata = true;
    return true;
}



/**
 * Return whether every render visual carries typed metadata.
 *
 * @param plan the FramePlan
 * @return true when all render visuals have typed metadata
 */
bool dvz_frame_plan_render_metadata_complete(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return true;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        for (uint32_t j = 0; j < node->u.render.visual_count; j++)
        {
            if (!node->u.render.visual_metadata[j].has_metadata)
                return false;
        }
    }
    return true;
}
