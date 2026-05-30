/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Image query policy                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
#include "image/internal.h"
#include "_visual_pipeline.h"
#include "registry/registry.h"
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
 * Allocate one temporary image query buffer with checked size arithmetic.
 *
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @return true when allocation succeeds
 */
static bool _image_query_alloc(void** out_ptr, uint64_t count, uint64_t item_size)
{
    return _dvz_scene_query_alloc("image", out_ptr, count, item_size);
}



/**
 * Return image-query static resource versions for one visual.
 *
 * @param visual the image visual
 * @param out_position_version position attribute version
 * @param out_texcoord_version texcoord attribute version
 * @param out_texture_version texture payload version
 * @return true when required static resources exist
 */
static bool _image_query_static_versions(
    const DvzVisual* visual, uint64_t* out_position_version, uint64_t* out_texcoord_version,
    uint64_t* out_texture_version)
{
    ANN(visual);
    ANN(out_position_version);
    ANN(out_texcoord_version);
    ANN(out_texture_version);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_image_query_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;

    uint64_t texcoord_version = 0;
    int extent_idx = _attr_index(visual, "extent");
    int extent_px_idx = _attr_index(visual, "extent_px");
    bool has_extent = extent_idx >= 0 && visual->attrs[extent_idx].data != NULL;
    bool has_extent_px = extent_px_idx >= 0 && visual->attrs[extent_px_idx].data != NULL;
    if (has_extent || has_extent_px)
    {
        if (has_extent && has_extent_px)
            return false;
        const DvzVisualAttr* extent_attr =
            has_extent_px ? &visual->attrs[extent_px_idx] : &visual->attrs[extent_idx];
        if (
            extent_attr->item_count != pos_attr->item_count ||
            extent_attr->item_size != sizeof(vec2))
        {
            return false;
        }
        texcoord_version = extent_attr->version;

        int anchor_idx = _attr_index(visual, "anchor");
        if (anchor_idx >= 0 && visual->attrs[anchor_idx].data != NULL)
            texcoord_version ^= visual->attrs[anchor_idx].version;
        int tex_rect_idx = _attr_index(visual, "tex_rect");
        if (tex_rect_idx >= 0 && visual->attrs[tex_rect_idx].data != NULL)
            texcoord_version ^= visual->attrs[tex_rect_idx].version;
    }
    else
    {
        const DvzVisualAttr* uv_attr = NULL;
        if (!_image_query_attr(visual, "texcoords", sizeof(vec2), &uv_attr))
            return false;
        if (uv_attr->item_count != pos_attr->item_count)
            return false;
        texcoord_version = uv_attr->version;
    }

    *out_position_version = pos_attr->version;
    *out_texcoord_version = texcoord_version;
    *out_texture_version = _visual_family_state(visual)->texture.version;
    return true;
}



/**
 * Return whether retained image-query static uploads must be refreshed.
 *
 * @param executor retained query executor
 * @param visual image visual
 * @param position_version position attribute version
 * @param texcoord_version texcoord attribute version
 * @param texture_version texture payload version
 * @return true when the static resources should be uploaded
 */
static bool _image_query_needs_static_upload(
    const DvzSceneRequestExecutor* executor, const DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version)
{
    ANN(visual);
    if (executor == NULL)
        return true;
    return executor->query_static_cache_family != DVZ_SCENE_VISUAL_FAMILY_IMAGE ||
           executor->query_static_cache_visual != visual ||
           executor->query_static_cache_key_count != 3 ||
           executor->query_static_cache_keys[0] != position_version ||
           executor->query_static_cache_keys[1] != texcoord_version ||
           executor->query_static_cache_keys[2] != texture_version;
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
static bool _image_query_target_extent(
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
static void _image_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const vec2 request_ndc, uint32_t target_width,
    uint32_t target_height)
{
    _dvz_scene_query_apply_render_state(plan, panel, request_ndc, target_width, target_height);
}



/**
 * Build temporary GPU image identity quads for one retained image visual.
 *
 * @param visual image visual
 * @param scratch output scratch plan storage
 * @param out_vertex_count output derived vertex count
 * @return true when derived buffers were created
 */
static bool _image_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count)
{
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_image_query_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;
    const float* position = (const float*)pos_attr->data;

    uint64_t vertex_count = 6;
    const DvzVisualAttr* extent_attr = NULL;
    bool generated_quads = _image_query_attr(visual, "extent", sizeof(vec2), &extent_attr);
    if (generated_quads)
    {
        (void)extent_attr;
        return _image_query_generated_rect_geometry(
            visual, scratch, true, false, out_vertex_count);
    }
    else if (pos_attr->item_count != 4 && pos_attr->item_count != 6)
    {
        return false;
    }

    if (!_image_query_alloc((void**)&scratch->query_positions, vertex_count, sizeof(vec3)) ||
        !_image_query_alloc((void**)&scratch->query_ids, vertex_count, sizeof(uint32_t)))
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    if (pos_attr->item_count == 4)
    {
        const uint32_t order[6] = {0, 1, 2, 2, 1, 3};
        for (uint32_t j = 0; j < 6; j++)
        {
            dvz_memcpy(
                scratch->query_positions[j], sizeof(vec3), &position[3 * order[j]],
                sizeof(vec3));
            scratch->query_ids[j] = 1u;
        }
    }
    else
    {
        for (uint32_t j = 0; j < 6; j++)
        {
            dvz_memcpy(
                scratch->query_positions[j], sizeof(vec3), &position[3 * j], sizeof(vec3));
            scratch->query_ids[j] = 1u;
        }
    }

    *out_vertex_count = vertex_count;
    return true;
}



/**
 * Return whether an image visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _image_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
        return false;
    if (
        request->target == DVZ_SCENE_TARGET_PIXEL ||
        request->target == DVZ_SCENE_TARGET_SAMPLE)
    {
        for (uint32_t i = 0; i < panel->visual_count; i++)
        {
            const DvzPanelAttach* attach = &panel->visuals[i];
            if (attach->visual == visual && attach->controller_mode == DVZ_CONTROLLER_FIXED)
                return false;
        }
        uint32_t capability = request->target == DVZ_SCENE_TARGET_SAMPLE
                                  ? DVZ_QUERY_CAPABILITY_SAMPLE
                                  : DVZ_QUERY_CAPABILITY_PIXEL;
        return (visual->query_capabilities & capability) != 0;
    }
    return _dvz_scene_query_item_target_eligible(visual, request, DVZ_VISUAL_TYPE_IMAGE);
}


/**
 * Return an explicit image-query unsupported status for native pixel/sample requests.
 *
 * @param visual image visual
 * @param request query request
 * @param out_status output unsupported status
 * @return whether the family owns the rejection
 */
static bool _image_query_reject_unsupported(
    const DvzVisual* visual, const DvzQueryRequest* request, DvzQueryStatus* out_status)
{
    ANN(visual);
    ANN(request);
    ANN(out_status);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
        return false;
    if (
        request->target != DVZ_SCENE_TARGET_PIXEL &&
        request->target != DVZ_SCENE_TARGET_SAMPLE)
    {
        return false;
    }
    uint32_t capability = request->target == DVZ_SCENE_TARGET_SAMPLE
                              ? DVZ_QUERY_CAPABILITY_SAMPLE
                              : DVZ_QUERY_CAPABILITY_PIXEL;
    if ((visual->query_capabilities & capability) == 0)
        return false;
    *out_status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
    return true;
}



/**
 * Build an image-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _image_query_build(const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    if (
        ctx->pending->request.target == DVZ_SCENE_TARGET_PIXEL ||
        ctx->pending->request.target == DVZ_SCENE_TARGET_SAMPLE)
    {
        uint64_t position_version = 0;
        uint64_t texcoord_version = 0;
        uint64_t texture_version = 0;
        if (!_image_query_static_versions(
                ctx->visual, &position_version, &texcoord_version, &texture_version))
        {
            return false;
        }
        bool include_static_uploads = _image_query_needs_static_upload(
            ctx->executor, ctx->visual, position_version, texcoord_version, texture_version);

        if (!_scene_image_query_plan(
                ctx->panel, ctx->visual, ctx->pending, ctx->request_ndc, include_static_uploads,
                &out_plan->scratch))
        {
            _scene_query_scratch_destroy(&out_plan->scratch);
            return false;
        }
        if (include_static_uploads)
        {
            out_plan->mark_static_cache_uploaded = true;
            out_plan->static_cache_family = DVZ_SCENE_VISUAL_FAMILY_IMAGE;
            out_plan->static_cache_visual = ctx->visual;
            out_plan->static_cache_key_count = 3;
            out_plan->static_cache_keys[0] = position_version;
            out_plan->static_cache_keys[1] = texcoord_version;
            out_plan->static_cache_keys[2] = texture_version;
        }
        out_plan->target_width = 1;
        out_plan->target_height = 1;
        out_plan->format = 0;
        out_plan->byte_size = 4;
        out_plan->schema = (DvzSceneQuerySchema){
            .fields = DVZ_SCENE_QUERY_SCHEMA_FIELD_DISPLAY_RGBA,
            .value_kind = DVZ_QUERY_VALUE_VEC4,
            .profile = ctx->profile,
            .format = out_plan->format,
            .byte_size = out_plan->byte_size,
        };
        return true;
    }

    uint64_t vertex_count = 0;
    if (!_image_query_geometry(ctx->visual, &out_plan->scratch, &vertex_count))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_image_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
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
        log_error("image query request buffer size overflow");
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.image", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position", 0, position_bytes, "position",
                   out_plan->scratch.query_positions);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.query_ids);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_PRIMITIVE;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_INDEXED_MESH;
    metadata.desc_kind = (uint32_t)_scene_visual_family_desc_kind(DVZ_VISUAL_TYPE_PRIMITIVE);
    metadata.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
        _image_query_apply_render_state(
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
            "image query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    out_plan->target_width = target_width;
    out_plan->target_height = target_height;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    out_plan->schema = (DvzSceneQuerySchema){
        .fields = DVZ_SCENE_QUERY_SCHEMA_FIELD_VISUAL_ID | DVZ_SCENE_QUERY_SCHEMA_FIELD_ITEM_ID,
        .value_kind = DVZ_QUERY_VALUE_NONE,
        .profile = ctx->profile,
        .format = out_plan->format,
        .byte_size = out_plan->byte_size,
    };
    return true;
}



/**
 * Decode an image-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _image_query_decode(const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->figure);
    ANN(ctx->build->visual);
    ANN(ctx->bytes);
    ANN(out_result);
    if (
        ctx->build->pending->request.target == DVZ_SCENE_TARGET_PIXEL ||
        ctx->build->pending->request.target == DVZ_SCENE_TARGET_SAMPLE)
    {
        DvzSceneTargetKind target = ctx->build->pending->request.target;
        if (ctx->byte_size < 4)
        {
            out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
            return true;
        }
        if (ctx->bytes[3] == 0)
        {
            out_result->status = DVZ_QUERY_STATUS_MISS;
            out_result->visual_id =
                _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
            out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_IMAGE;
            out_result->raw_target = target;
            out_result->resolved_target = target;
            out_result->value_kind = DVZ_QUERY_VALUE_NONE;
            return true;
        }

        out_result->status = DVZ_QUERY_STATUS_HIT;
        out_result->hit = true;
        out_result->visual_id =
            _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
        out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_IMAGE;
        out_result->payload_version = 1;
        out_result->raw_target = target;
        out_result->resolved_target = target;
        out_result->value_kind = DVZ_QUERY_VALUE_VEC4;
        for (uint32_t i = 0; i < 4; i++)
        {
            out_result->vector[i] = ctx->bytes[i] / 255.0;
            out_result->display_rgba[i] = out_result->vector[i];
        }
        out_result->has_display_rgba = true;
        dvz_strlcpy(out_result->label, "rgba", sizeof(out_result->label));
        return true;
    }

    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_IMAGE, out_result);
}



/**
 * Complete image-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _image_query_readout(const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(result);
    if (
        result->resolved_target != DVZ_SCENE_TARGET_PIXEL &&
        result->resolved_target != DVZ_SCENE_TARGET_SAMPLE)
    {
        result->value_kind = DVZ_QUERY_VALUE_NONE;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return image visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_image_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "image",
        .family = DVZ_SCENE_VISUAL_FAMILY_IMAGE,
        .eligible = _image_query_eligible,
        .build = _image_query_build,
        .reject_unsupported = _image_query_reject_unsupported,
        .decode = _image_query_decode,
        .readout = _image_query_readout,
    };
    return &ops;
}
