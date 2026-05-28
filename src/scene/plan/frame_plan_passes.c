/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan passes                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_frame_plan_internal.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure graph pass storage has room for one more descriptor.
 *
 * @param plan the FramePlan
 * @return whether storage is available
 */
static bool _ensure_graph_pass_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->graph_passes == NULL || plan->graph_pass_capacity == 0)
    {
        plan->graph_pass_capacity = DVZ_FRAME_PLAN_INITIAL_GRAPH_PASS_CAPACITY;
        plan->graph_passes = (DvzFrameGraphPass*)dvz_calloc(
            plan->graph_pass_capacity, sizeof(DvzFrameGraphPass));
        return plan->graph_passes != NULL;
    }

    if (plan->graph_pass_count < plan->graph_pass_capacity)
        return true;

    if (plan->graph_pass_capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = plan->graph_pass_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzFrameGraphPass), &bytes))
        return false;

    DvzFrameGraphPass* passes = (DvzFrameGraphPass*)dvz_realloc(plan->graph_passes, bytes);
    if (passes == NULL)
        return false;

    plan->graph_pass_capacity = capacity;
    plan->graph_passes = passes;
    return plan->graph_passes != NULL;
}



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
 * Append a typed graph pass descriptor.
 *
 * @param plan the FramePlan
 * @param pass the pass descriptor
 * @return whether the pass was appended
 */
bool dvz_frame_plan_graph_pass(DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    if (plan == NULL || pass == NULL || pass->id[0] == '\0')
        return false;
    if (!_ensure_graph_pass_capacity(plan))
    {
        log_error("cannot grow FramePlan graph pass list");
        return false;
    }

    DvzFrameGraphPass* dst = &plan->graph_passes[plan->graph_pass_count++];
    dvz_memset(dst, sizeof(DvzFrameGraphPass), 0, sizeof(DvzFrameGraphPass));
    dvz_memcpy(dst, sizeof(DvzFrameGraphPass), pass, sizeof(DvzFrameGraphPass));
    return true;
}



/**
 * Return the graph pass count.
 *
 * @param plan the FramePlan
 * @return the graph pass count
 */
uint32_t dvz_frame_plan_graph_pass_count(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return 0;
    return plan->graph_pass_count;
}



/**
 * Return a graph pass descriptor.
 *
 * @param plan the FramePlan
 * @param index the graph pass index
 * @return the pass descriptor, or NULL when index is out of bounds
 */
const DvzFrameGraphPass* dvz_frame_plan_graph_pass_get(const DvzFramePlan* plan, uint32_t index)
{
    if (plan == NULL || index >= plan->graph_pass_count)
        return NULL;
    return &plan->graph_passes[index];
}



/**
 * Add a declared read access to a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param resource_id the resource id
 * @param usage the resource access usage
 * @return whether the read access was appended
 */
bool dvz_frame_graph_pass_read(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage)
{
    if (pass == NULL || resource_id == NULL || resource_id[0] == '\0' ||
        pass->read_count >= DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES)
        return false;

    DvzFrameGraphAccess* access = &pass->reads[pass->read_count++];
    dvz_memset(access, sizeof(DvzFrameGraphAccess), 0, sizeof(DvzFrameGraphAccess));
    _frame_plan_copy_label(access->resource_id, DVZ_SCENE_LABEL_SIZE, resource_id);
    access->usage = usage;
    return true;
}



/**
 * Add a declared write access to a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param resource_id the resource id
 * @param usage the resource access usage
 * @return whether the write access was appended
 */
bool dvz_frame_graph_pass_write(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage)
{
    if (pass == NULL || resource_id == NULL || resource_id[0] == '\0' ||
        pass->write_count >= DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES)
        return false;

    DvzFrameGraphAccess* access = &pass->writes[pass->write_count++];
    dvz_memset(access, sizeof(DvzFrameGraphAccess), 0, sizeof(DvzFrameGraphAccess));
    _frame_plan_copy_label(access->resource_id, DVZ_SCENE_LABEL_SIZE, resource_id);
    access->usage = usage;
    return true;
}



/**
 * Add a color attachment to a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param attachment the color attachment descriptor
 * @return whether the color attachment was appended
 */
bool dvz_frame_graph_pass_color_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment)
{
    if (pass == NULL || attachment == NULL || attachment->resource_id[0] == '\0' ||
        pass->color_attachment_count >= DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS)
        return false;

    DvzFrameGraphAttachment* dst = &pass->color_attachments[pass->color_attachment_count++];
    dvz_memset(dst, sizeof(DvzFrameGraphAttachment), 0, sizeof(DvzFrameGraphAttachment));
    dvz_memcpy(dst, sizeof(DvzFrameGraphAttachment), attachment, sizeof(DvzFrameGraphAttachment));
    return true;
}



/**
 * Set the depth attachment on a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param attachment the depth attachment descriptor
 * @return whether the depth attachment was set
 */
bool dvz_frame_graph_pass_depth_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment)
{
    if (pass == NULL || attachment == NULL || attachment->resource_id[0] == '\0')
        return false;

    dvz_memcpy(
        &pass->depth_attachment, sizeof(DvzFrameGraphAttachment), attachment,
        sizeof(DvzFrameGraphAttachment));
    pass->has_depth_attachment = true;
    return true;
}
