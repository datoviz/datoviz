/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mesh query policy                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
#include "_visual_pipeline.h"
#include "../../query/internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one retained attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
static bool _mesh_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    return _dvz_scene_query_dense_attr(visual, attr_name, item_size, out_attr);
}



/**
 * Allocate one temporary mesh query buffer with checked size arithmetic.
 *
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @return true when allocation succeeds
 */
static bool _mesh_query_alloc(void** out_ptr, uint64_t count, uint64_t item_size)
{
    return _dvz_scene_query_alloc("mesh", out_ptr, count, item_size);
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
static bool _mesh_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height)
{
    return _dvz_scene_query_target_extent(figure, panel, out_target_width, out_target_height);
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
static void _mesh_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const vec2 request_ndc, uint32_t target_width,
    uint32_t target_height)
{
    _dvz_scene_query_apply_render_state(plan, panel, request_ndc, target_width, target_height);
}



/**
 * Return one source vertex index from either direct or indexed visual geometry.
 *
 * @param visual visual carrying an optional index buffer binding
 * @param draw_index draw-order vertex index
 * @param vertex_count source vertex count
 * @param out_source_index output source vertex index
 * @return true when the source index is valid
 */
static bool _mesh_query_source_vertex_index(
    const DvzVisual* visual, uint64_t draw_index, uint64_t vertex_count,
    uint64_t* out_source_index)
{
    ANN(visual);
    ANN(out_source_index);
    uint64_t source_index = draw_index;
    if (visual->buffer != NULL && visual->buffer->data != NULL)
    {
        uint32_t stride = visual->buffer->desc.stride;
        uint64_t offset = 0;
        if (
            stride == 0 ||
            _dvz_mul_u64_overflows(draw_index, stride, &offset) ||
            offset > visual->buffer->desc.byte_size ||
            stride > visual->buffer->desc.byte_size - offset)
        {
            return false;
        }
        const uint8_t* data = (const uint8_t*)visual->buffer->data + offset;
        if (stride == sizeof(uint16_t))
        {
            uint16_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else if (stride == sizeof(uint32_t))
        {
            uint32_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else
            return false;
    }
    if (source_index >= vertex_count)
        return false;
    *out_source_index = source_index;
    return true;
}



/**
 * Build temporary GPU mesh buffers for one retained mesh visual.
 *
 * @param visual mesh visual
 * @param scratch output scratch plan storage
 * @param out_vertex_count output derived vertex count
 * @param out_topology output draw topology
 * @return true when derived buffers were created
 */
static bool _mesh_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count,
    uint32_t* out_topology)
{
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);
    ANN(out_topology);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_mesh_query_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;
    uint64_t vertex_count = pos_attr->item_count;

    uint64_t source_index_count = vertex_count;
    if (visual->buffer != NULL && visual->buffer->data != NULL &&
        visual->buffer->desc.byte_size > 0 && visual->buffer->desc.stride > 0)
    {
        uint32_t stride = visual->buffer->desc.stride;
        if (stride != sizeof(uint16_t) && stride != sizeof(uint32_t))
        {
            log_error("mesh query request index stride must be 16-bit or 32-bit");
            return false;
        }
        if (visual->buffer->desc.byte_size % stride != 0)
        {
            log_error("mesh query request index buffer size is not stride-aligned");
            return false;
        }
        source_index_count = visual->buffer->desc.byte_size / stride;
    }

    uint64_t mesh_count = 0;
    uint64_t draw_vertex_count = 0;
    uint32_t draw_topology = (uint32_t)visual->topology;
    switch (visual->topology)
    {
    case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
        mesh_count = source_index_count;
        draw_vertex_count = mesh_count;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
        mesh_count = source_index_count / 2;
        if (_dvz_mul_u64_overflows(mesh_count, 2, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        if (source_index_count < 2)
            return false;
        mesh_count = source_index_count - 1;
        if (_dvz_mul_u64_overflows(mesh_count, 2, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        mesh_count = source_index_count / 3;
        if (_dvz_mul_u64_overflows(mesh_count, 3, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        if (source_index_count < 3)
            return false;
        mesh_count = source_index_count - 2;
        if (_dvz_mul_u64_overflows(mesh_count, 3, &draw_vertex_count))
            return false;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    default:
        return false;
    }
    if (mesh_count == 0 || draw_vertex_count == 0 || draw_vertex_count > UINT32_MAX)
        return false;

    if (!_mesh_query_alloc(
            (void**)&scratch->query_positions, draw_vertex_count, sizeof(vec3)) ||
        !_mesh_query_alloc((void**)&scratch->query_ids, draw_vertex_count, sizeof(uint32_t)))
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    for (uint64_t prim = 0; prim < mesh_count; prim++)
    {
        uint64_t draw_indices[3] = {0, 0, 0};
        uint32_t prim_vertex_count = 1;
        switch (visual->topology)
        {
        case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
            draw_indices[0] = prim;
            prim_vertex_count = 1;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
            draw_indices[0] = 2 * prim + 0;
            draw_indices[1] = 2 * prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            draw_indices[0] = 3 * prim + 0;
            draw_indices[1] = 3 * prim + 1;
            draw_indices[2] = 3 * prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            draw_indices[0] = 0;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        default:
            _scene_query_scratch_destroy(scratch);
            return false;
        }

        for (uint32_t j = 0; j < prim_vertex_count; j++)
        {
            uint64_t source_index = 0;
            if (!_mesh_query_source_vertex_index(
                    visual, draw_indices[j], vertex_count, &source_index))
            {
                _scene_query_scratch_destroy(scratch);
                return false;
            }
            uint64_t dst = prim * prim_vertex_count + j;
            dvz_memcpy(
                scratch->query_positions[dst], sizeof(vec3), &position[3 * source_index],
                sizeof(vec3));
            scratch->query_ids[dst] = (uint32_t)prim + 1u;
        }
    }

    *out_vertex_count = draw_vertex_count;
    *out_topology = draw_topology;
    return true;
}



/**
 * Return whether a mesh visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _mesh_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_MESH)
        return false;
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_ITEM &&
        request->target != DVZ_SCENE_TARGET_OBJECT)
    {
        return false;
    }
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a mesh-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _mesh_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    uint64_t vertex_count = 0;
    uint32_t topology = 0;
    if (!_mesh_query_geometry(ctx->visual, &out_plan->scratch, &vertex_count, &topology))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_mesh_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t id_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &id_bytes))
    {
        log_error("mesh query request buffer size overflow");
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.mesh", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position", 0, position_bytes, "position",
                   out_plan->scratch.query_positions);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, topology);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.query_ids);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_MESH;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_INDEXED_MESH;
    metadata.desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    metadata.topology = topology;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    dvz_strlcpy(metadata.position_id, "query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "query0_id", sizeof(metadata.color_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query", "target.query", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "query0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _mesh_query_apply_render_state(
            plan, ctx->panel, ctx->request_ndc, target_width, target_height);

    DvzFramePlanCopyDesc copy = {
        .src_resource_id = "target.query",
        .dst_resource_id = "buf.query",
        .extent = {1, 1, 1},
        .format = VK_FORMAT_R32_UINT,
        .bytes_per_texel = sizeof(uint32_t),
        .bytes_per_row = sizeof(uint32_t),
        .rows_per_image = 1,
        .byte_size = sizeof(uint32_t),
        .request_id = ctx->pending->request.request_id,
    };
    ok = ok && dvz_frame_plan_copy_ex(plan, &copy) &&
         dvz_frame_plan_readback(plan, "buf.query", "request.query");
    if (!ok)
    {
        log_error(
            "mesh query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    out_plan->target_width = target_width;
    out_plan->target_height = target_height;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    return true;
}



/**
 * Decode a mesh-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _mesh_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_MESH, out_result);
}



/**
 * Complete mesh-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _mesh_query_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(result);
    result->value_kind = DVZ_QUERY_VALUE_NONE;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return mesh visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_mesh_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "mesh",
        .family = DVZ_SCENE_VISUAL_FAMILY_MESH,
        .eligible = _mesh_query_eligible,
        .build = _mesh_query_build,
        .decode = _mesh_query_decode,
        .readout = _mesh_query_readout,
    };
    return &ops;
}
