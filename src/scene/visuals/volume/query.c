/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Volume query policy                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
#include "colorizer.h"
#include "scene_emit/visual_lowering.h"
#include "../../query/internal.h"
#include "sample_profile.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_VOLUME_QUERY_QUANT_MAX 16777214.0
#define DVZ_VOLUME_QUERY_UVW_QUANT_MAX 1023.0

static const uint8_t VOLUME_QUERY_DUMMY_TRANSFER_RGBA[4] = {255, 255, 255, 255};



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
static bool _volume_query_attr(
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
 * Return the offscreen query target extent for one panel.
 *
 * @param figure parent figure
 * @param panel panel receiving the query
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
static bool _volume_query_target_extent(
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
 * Return whether a scalar volume texture format can be sampled by the float query shader.
 *
 * @param format retained field format
 * @param out_texture_format output Vulkan texture format
 * @param out_bytes_per_texel output texture byte size
 * @return true when the sample query shader supports the field format
 */
static bool _volume_query_scalar_sample_format(
    DvzFieldFormat format, uint32_t* out_texture_format, uint32_t* out_bytes_per_texel)
{
    ANN(out_texture_format);
    ANN(out_bytes_per_texel);
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
    case DVZ_FIELD_FORMAT_R8_SNORM:
    case DVZ_FIELD_FORMAT_R16_UNORM:
    case DVZ_FIELD_FORMAT_R16_SNORM:
    case DVZ_FIELD_FORMAT_R16_FLOAT:
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        return _field_format_texture_format(format, out_texture_format) &&
               _field_format_bytes_per_texel(format, out_bytes_per_texel);
    default:
        *out_texture_format = 0;
        *out_bytes_per_texel = 0;
        return false;
    }
}



/**
 * Return whether a label volume texture format can be sampled by the integer query shader.
 *
 * @param format retained field format
 * @param out_texture_format output Vulkan texture format
 * @param out_bytes_per_texel output texture byte size
 * @return true when the sample query shader supports the field format
 */
static bool _volume_query_label_sample_format(
    DvzFieldFormat format, uint32_t* out_texture_format, uint32_t* out_bytes_per_texel)
{
    ANN(out_texture_format);
    ANN(out_bytes_per_texel);
    DvzSceneSampleProfile profile = {0};
    if (_scene_sample_profile_resolve(
            format, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_3D, &profile) &&
        _scene_sample_profile_is_integer_label(&profile))
    {
        return _field_format_texture_format(format, out_texture_format) &&
               _field_format_bytes_per_texel(format, out_bytes_per_texel);
    }
    *out_texture_format = 0;
    *out_bytes_per_texel = 0;
    return false;
}



/**
 * Decode a raw label volume payload word into a category ID.
 *
 * @param format sampled-field format
 * @param encoded raw r32uint shader payload; zero is reserved as miss
 * @param out_id decoded category ID
 * @return true when the format was decoded
 */
static bool _volume_query_decode_label_sample(
    DvzFieldFormat format, uint32_t encoded, DvzCategoryId* out_id)
{
    ANN(out_id);
    if (encoded == 0)
        return false;
    uint32_t bits = encoded;
    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            format, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_3D, &profile))
    {
        return false;
    }
    if (_scene_sample_profile_is_unsigned_label(&profile))
    {
        *out_id = (DvzCategoryId)bits;
        return true;
    }
    if (_scene_sample_profile_is_signed_label(&profile))
    {
        int32_t v = 0;
        dvz_memcpy(&v, sizeof(v), &bits, sizeof(bits));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    return false;
}



/**
 * Return the display label for one label-volume query category.
 *
 * @param visual volume visual
 * @param id category ID
 * @param out_label output display label
 * @param label_size output label capacity
 */
static void _volume_query_category_label(
    const DvzVisual* visual, DvzCategoryId id, char* out_label, uint64_t label_size)
{
    ANN(visual);
    ANN(out_label);
    DvzSceneColorizer colorizer = {0};
    if (_scene_colorizer_from_scale(
            visual->scale, DVZ_SCENE_COLORIZER_CATEGORICAL, &colorizer))
        (void)_scene_colorizer_category_label(&colorizer, id, out_label, label_size);
    else
        dvz_snprintf(out_label, label_size, "label %" PRIi64, id);
}



/**
 * Return the byte size of a dense 3D volume field.
 *
 * @param field retained sampled field
 * @param bytes_per_texel field texel byte size
 * @param out_byte_size output byte size
 * @return true when the size was computed without overflow
 */
static bool _volume_query_field_byte_size(
    const DvzSampledField* field, uint32_t bytes_per_texel, uint64_t* out_byte_size)
{
    ANN(field);
    ANN(out_byte_size);
    uint64_t texel_count = 0;
    if (
        _dvz_mul_u64_overflows(field->desc.width, field->desc.height, &texel_count) ||
        _dvz_mul_u64_overflows(texel_count, field->desc.depth, &texel_count))
    {
        return false;
    }
    return !_dvz_mul_u64_overflows(texel_count, bytes_per_texel, out_byte_size);
}



/**
 * Decode one 10-bit quantized UVW component from a packed query word.
 *
 * @param packed packed UVW payload
 * @param shift bit shift of the component
 * @return normalized UVW component
 */
static double _volume_query_decode_uvw_component(uint32_t packed, uint32_t shift)
{
    uint32_t code = (packed >> shift) & 0x3ffu;
    return (double)code / DVZ_VOLUME_QUERY_UVW_QUANT_MAX;
}



/**
 * Return one axis-selected UVW component.
 *
 * @param axis axis index
 * @param uvw normalized volume coordinate
 * @return selected component, clamped by caller
 */
static double _volume_query_axis_value(uint32_t axis, const double uvw[3])
{
    axis = axis <= 2 ? axis : 0;
    return uvw[axis];
}



/**
 * Map normalized volume UVW to normalized texture UVW using retained axis mapping.
 *
 * @param state retained volume state
 * @param uvw normalized volume coordinate
 * @param out_texture_uvw normalized texture coordinate
 */
static void _volume_query_texture_uvw(
    const DvzVolumeState* state, const double uvw[3], double out_texture_uvw[3])
{
    ANN(state);
    ANN(uvw);
    ANN(out_texture_uvw);
    for (uint32_t i = 0; i < 3; i++)
    {
        uint32_t axis = state->axis_order[i] <= 2 ? state->axis_order[i] : i;
        double value = _volume_query_axis_value(axis, uvw);
        if (state->axis_flip[i])
            value = 1.0 - value;
        if (value < 0.0)
            value = 0.0;
        if (value > 1.0)
            value = 1.0;
        out_texture_uvw[i] = value;
    }
}



/**
 * Decode a linear voxel/sample id from GPU-returned UVW.
 *
 * @param field sampled field
 * @param state retained volume state
 * @param uvw normalized volume coordinate
 * @param out_voxel_id output linear voxel id
 * @return true when the id was computed
 */
static bool _volume_query_voxel_id(
    const DvzSampledField* field, const DvzVolumeState* state, const double uvw[3],
    uint64_t* out_voxel_id)
{
    ANN(field);
    ANN(state);
    ANN(uvw);
    ANN(out_voxel_id);
    if (field->desc.width == 0 || field->desc.height == 0 || field->desc.depth == 0)
        return false;

    double texture_uvw[3] = {0};
    _volume_query_texture_uvw(state, uvw, texture_uvw);
    uint32_t dims[3] = {field->desc.width, field->desc.height, field->desc.depth};
    uint32_t coord[3] = {0};
    for (uint32_t i = 0; i < 3; i++)
    {
        double scaled = texture_uvw[i] * (double)dims[i];
        coord[i] = scaled >= (double)dims[i] ? dims[i] - 1u : (uint32_t)scaled;
    }
    *out_voxel_id =
        (uint64_t)coord[2] * (uint64_t)field->desc.width * (uint64_t)field->desc.height +
        (uint64_t)coord[1] * (uint64_t)field->desc.width + (uint64_t)coord[0];
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
static void _volume_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const vec2 request_ndc, uint32_t target_width,
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
 * Build a volume-family rendered scalar sample query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _volume_query_build_sample(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    DvzSampledField* field = ctx->visual->field;
    if (field == NULL || field->data == NULL || field->desc.dim != DVZ_FIELD_DIM_3D ||
        field->desc.width == 0 || field->desc.height == 0 || field->desc.depth == 0)
    {
        return false;
    }

    uint32_t texture_format = 0;
    uint32_t bytes_per_texel = 0;
    DvzSceneSampleProfile sample_profile = {0};
    if (!_scene_sample_profile_resolve(
            field->desc.format, field->desc.semantic, field->desc.dim, &sample_profile))
    {
        return false;
    }
    bool label_profile = _scene_sample_profile_is_integer_label(&sample_profile);
    if (label_profile && ctx->profile != DVZ_QUERY_PROFILE_U32_R32)
        return false;
    bool valid_format = label_profile
                            ? _volume_query_label_sample_format(
                                  field->desc.format, &texture_format, &bytes_per_texel)
                            : _volume_query_scalar_sample_format(
                                  field->desc.format, &texture_format, &bytes_per_texel);
    if (!valid_format)
    {
        return false;
    }

    const DvzVisualAttr* pos_attr = NULL;
    const DvzVisualAttr* uvw_attr = NULL;
    if (!_volume_query_attr(ctx->visual, "position", sizeof(vec3), &pos_attr) ||
        !_volume_query_attr(ctx->visual, "texcoords", sizeof(vec3), &uvw_attr) ||
        uvw_attr->item_count != pos_attr->item_count || pos_attr->item_count > UINT32_MAX)
    {
        return false;
    }
    uint64_t vertex_count = pos_attr->item_count;

    uint64_t position_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t texture_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &texcoord_bytes) ||
        !_volume_query_field_byte_size(field, bytes_per_texel, &texture_bytes))
    {
        log_error("volume sample query request buffer size overflow");
        return false;
    }

    DvzFramePlan* plan =
        dvz_frame_plan("figure.query.volume.sample", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "volume_query0_position", 0, position_bytes, "position",
                   pos_attr->data) &&
         dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) &&
         dvz_frame_plan_upload_bytes(
             plan, "volume_query0_texcoords", 0, texcoord_bytes, "texcoords", uvw_attr->data) &&
         dvz_frame_plan_upload_bytes(
             plan, "volume_query0_texture", 0, texture_bytes, "texture", field->data) &&
         dvz_frame_plan_upload_set_texture_format(plan, texture_format, bytes_per_texel) &&
         dvz_frame_plan_upload_set_texture_3d_extent(
             plan, field->desc.width, field->desc.height, field->desc.depth) &&
         dvz_frame_plan_upload_set_texture_3d_allocation_extent(
             plan, field->desc.width, field->desc.height, field->desc.depth) &&
         dvz_frame_plan_upload_set_texture_3d_region(plan, 0, 0, 0) &&
         dvz_frame_plan_upload_bytes(
             plan, "volume_query0_transfer", 0, sizeof(VOLUME_QUERY_DUMMY_TRANSFER_RGBA),
             "volume_transfer", VOLUME_QUERY_DUMMY_TRANSFER_RGBA) &&
         dvz_frame_plan_upload_set_texture_format(plan, VK_FORMAT_R8G8B8A8_UNORM, 4) &&
         dvz_frame_plan_upload_set_texture_extent(plan, 1, 1) &&
         dvz_frame_plan_upload_set_texture_allocation_extent(plan, 1, 1) &&
         dvz_frame_plan_upload_set_texture_region(plan, 0, 0);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_VOLUME;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_VOLUME_PROXY;
    metadata.desc_kind = (uint32_t)_scene_visual_lowering_desc_kind(ctx->visual);
    metadata.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    metadata.field_format = field->desc.format;
    metadata.field_semantic = field->desc.semantic;
    metadata.field_width = field->desc.width;
    metadata.field_height = field->desc.height;
    metadata.field_depth = field->desc.depth;
    metadata.has_volume = true;
    metadata.volume_state = ctx->visual->volume;
    metadata.volume_transfer_rgba = false;
    dvz_strlcpy(metadata.position_id, "volume_query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.texcoords_id, "volume_query0_texcoords", sizeof(metadata.texcoords_id));
    dvz_strlcpy(metadata.texture_id, "volume_query0_texture", sizeof(metadata.texture_id));
    dvz_strlcpy(
        metadata.volume_texture_id, "volume_query0_texture", sizeof(metadata.volume_texture_id));
    dvz_strlcpy(
        metadata.volume_transfer_texture_id, "volume_query0_transfer",
        sizeof(metadata.volume_transfer_texture_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query.volume.sample", "target.query.volume.sample", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "volume_query0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
    if (render != NULL)
    {
        DvzMVP mvp = {0};
        _scene_request_apply_mvp(ctx->panel, ctx->request_ndc, &mvp);
        render->u.render.has_mvp = true;
        render->u.render.apply_mvp = mvp;
        render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
    }

    bool rg32_profile = !label_profile && ctx->profile == DVZ_QUERY_PROFILE_U64_RG32;
    uint32_t query_format = rg32_profile ? VK_FORMAT_R32G32_UINT : VK_FORMAT_R32_UINT;
    uint32_t query_byte_size = rg32_profile ? 2u * sizeof(uint32_t) : sizeof(uint32_t);
    DvzFramePlanCopyDesc copy = {
        .src_resource_id = "target.query.volume.sample",
        .dst_resource_id = "buf.query.volume.sample",
        .extent = {1, 1, 1},
        .format = query_format,
        .bytes_per_texel = query_byte_size,
        .bytes_per_row = query_byte_size,
        .rows_per_image = 1,
        .byte_size = query_byte_size,
        .request_id = ctx->pending->request.request_id,
    };
    ok = ok && dvz_frame_plan_copy_ex(plan, &copy) &&
         dvz_frame_plan_readback(plan, "buf.query.volume.sample", "request.query.volume.sample");
    if (!ok)
    {
        log_error(
            "volume sample query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    out_plan->field = field;
    out_plan->target_width = 1;
    out_plan->target_height = 1;
    out_plan->format = query_format;
    out_plan->byte_size = query_byte_size;
    out_plan->schema = (DvzSceneQuerySchema){
        .fields = label_profile ? DVZ_SCENE_QUERY_SCHEMA_FIELD_LABEL_ID
                                : (DVZ_SCENE_QUERY_SCHEMA_FIELD_SAMPLE_VALUE |
                                   (rg32_profile ? (DVZ_SCENE_QUERY_SCHEMA_FIELD_UVW |
                                                    DVZ_SCENE_QUERY_SCHEMA_FIELD_VOXEL_COORD)
                                                 : 0)),
        .value_kind = label_profile ? DVZ_QUERY_VALUE_CATEGORY : DVZ_QUERY_VALUE_SCALAR,
        .profile = ctx->profile,
        .format = out_plan->format,
        .byte_size = out_plan->byte_size,
    };
    return true;
}



/**
 * Return whether a volume visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _volume_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return false;
    if (request->target == DVZ_SCENE_TARGET_SAMPLE)
    {
        if ((visual->query_capabilities & DVZ_QUERY_CAPABILITY_SAMPLE) == 0)
            return false;
        if (visual->volume.render_mode != DVZ_VOLUME_RENDER_SLICE)
            return false;
        if (visual->field == NULL || visual->field->data == NULL)
            return false;
        uint32_t texture_format = 0;
        uint32_t bytes_per_texel = 0;
        DvzSceneSampleProfile profile = {0};
        if (!_scene_sample_profile_resolve(
                visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
                &profile))
        {
            return false;
        }
        if (_scene_sample_profile_is_integer_label(&profile))
            return _volume_query_label_sample_format(
                visual->field->desc.format, &texture_format, &bytes_per_texel);
        return _volume_query_scalar_sample_format(
            visual->field->desc.format, &texture_format, &bytes_per_texel);
    }
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_ITEM &&
        request->target != DVZ_SCENE_TARGET_OBJECT)
    {
        return false;
    }
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a volume-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _volume_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    if (ctx->pending->request.target == DVZ_SCENE_TARGET_SAMPLE)
        return _volume_query_build_sample(ctx, out_plan);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_volume_query_attr(ctx->visual, "position", sizeof(vec3), &pos_attr) ||
        pos_attr->item_count > UINT32_MAX)
    {
        return false;
    }
    uint64_t vertex_count = pos_attr->item_count;

    if ((out_plan->scratch.query_ids = (uint32_t*)dvz_calloc(vertex_count, sizeof(uint32_t))) == NULL)
        return false;
    for (uint64_t i = 0; i < vertex_count; i++)
        out_plan->scratch.query_ids[i] = 1u;

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_volume_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
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
        log_error("volume query request buffer size overflow");
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.volume", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position", 0, position_bytes, "position", pos_attr->data);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.query_ids);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_PRIMITIVE;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_INDEXED_MESH;
    metadata.desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
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
        _volume_query_apply_render_state(
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
            "volume query request %" PRIu64 " failed to assemble the GPU readback plan",
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
 * Decode a volume-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _volume_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->figure);
    ANN(ctx->build->visual);
    ANN(ctx->bytes);
    ANN(out_result);
    if (ctx->build->pending->request.target == DVZ_SCENE_TARGET_SAMPLE)
    {
        if (ctx->byte_size < sizeof(uint32_t))
        {
            out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
            return true;
        }

        uint32_t encoded = 0;
        dvz_memcpy(&encoded, sizeof(encoded), ctx->bytes, sizeof(encoded));
        out_result->visual_id =
            _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
        out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_VOLUME;
        out_result->raw_target = DVZ_SCENE_TARGET_SAMPLE;
        out_result->resolved_target = DVZ_SCENE_TARGET_SAMPLE;
        if (encoded == 0)
        {
            out_result->status = DVZ_QUERY_STATUS_MISS;
            out_result->value_kind = DVZ_QUERY_VALUE_NONE;
            return true;
        }

        DvzSceneSampleProfile sample_profile = {0};
        bool label_profile =
            ctx->plan != NULL && ctx->plan->field != NULL &&
            _scene_sample_profile_resolve(
                ctx->plan->field->desc.format, ctx->plan->field->desc.semantic,
                ctx->plan->field->desc.dim, &sample_profile) &&
            _scene_sample_profile_is_integer_label(&sample_profile);
        if (label_profile)
        {
            DvzCategoryId label_id = 0;
            if (!_volume_query_decode_label_sample(
                    ctx->plan->field->desc.format, encoded, &label_id) ||
                label_id == 0)
            {
                out_result->status = DVZ_QUERY_STATUS_MISS;
                out_result->value_kind = DVZ_QUERY_VALUE_NONE;
                return true;
            }
            uint64_t target_id = label_id >= 0 ? (uint64_t)label_id : 0;
            out_result->status = DVZ_QUERY_STATUS_HIT;
            out_result->hit = true;
            out_result->payload_version = 1;
            out_result->raw_target = DVZ_SCENE_TARGET_SAMPLE;
            out_result->raw_id = target_id;
            out_result->resolved_target = DVZ_SCENE_TARGET_SAMPLE;
            out_result->resolved_id = target_id;
            out_result->group_id = target_id;
            out_result->category_id = label_id;
            out_result->scale = ctx->build->visual->scale;
            out_result->value_kind = DVZ_QUERY_VALUE_CATEGORY;
            _volume_query_category_label(
                ctx->build->visual, label_id, out_result->label, sizeof(out_result->label));
            return true;
        }

        double t = (double)(encoded - 1u) / DVZ_VOLUME_QUERY_QUANT_MAX;
        const DvzVolumeState* state = &ctx->build->visual->volume;
        double value = state->value_min + t * (state->value_max - state->value_min);
        out_result->status = DVZ_QUERY_STATUS_HIT;
        out_result->hit = true;
        out_result->payload_version = 1;
        out_result->value_kind = DVZ_QUERY_VALUE_SCALAR;
        out_result->scalar = value;
        out_result->vector[0] = value;
        if (ctx->plan->format == VK_FORMAT_R32G32_UINT)
        {
            if (ctx->byte_size < 2u * sizeof(uint32_t))
            {
                out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
                out_result->hit = false;
                out_result->value_kind = DVZ_QUERY_VALUE_NONE;
                return true;
            }
            uint32_t packed_uvw = 0;
            dvz_memcpy(
                &packed_uvw, sizeof(packed_uvw), ctx->bytes + sizeof(uint32_t),
                sizeof(packed_uvw));
            out_result->has_uvw = true;
            out_result->uvw[0] = _volume_query_decode_uvw_component(packed_uvw, 0);
            out_result->uvw[1] = _volume_query_decode_uvw_component(packed_uvw, 10);
            out_result->uvw[2] = _volume_query_decode_uvw_component(packed_uvw, 20);
            DvzSampledField* field = ctx->plan->field;
            uint64_t voxel_id = 0;
            if (
                field != NULL &&
                _volume_query_voxel_id(field, state, out_result->uvw, &voxel_id))
            {
                out_result->raw_id = voxel_id;
                out_result->resolved_id = voxel_id;
                out_result->voxel_id = voxel_id;
                out_result->texel_id = voxel_id;
            }
        }
        dvz_strlcpy(out_result->label, "scalar", sizeof(out_result->label));
        return true;
    }

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
    out_result->status = DVZ_QUERY_STATUS_HIT;
    out_result->hit = true;
    out_result->visual_id = _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
    out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_VOLUME;
    out_result->payload_version = 1;
    out_result->raw_target = DVZ_SCENE_TARGET_ITEM;
    out_result->raw_id = item_id;
    out_result->resolved_target = DVZ_SCENE_TARGET_ITEM;
    out_result->resolved_id = item_id;
    out_result->item_id = item_id;
    if (ctx->build->visual->link_keys != NULL && item_id < ctx->build->visual->link_key_count)
        out_result->link_key = ctx->build->visual->link_keys[item_id];
    return true;
}



/**
 * Complete volume-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _volume_query_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(result);
    if (result->resolved_target != DVZ_SCENE_TARGET_SAMPLE)
        result->value_kind = DVZ_QUERY_VALUE_NONE;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether the volume family supports a selected query profile.
 *
 * @param ctx query build context
 * @param profile selected query profile
 * @return whether the profile is supported
 */
static bool _volume_query_supports_profile(
    const DvzSceneQueryBuildContext* ctx, DvzQueryProfile profile)
{
    ANN(ctx);
    if (profile == DVZ_QUERY_PROFILE_U32_R32)
        return true;
    return profile == DVZ_QUERY_PROFILE_U64_RG32 &&
           ctx->pending->request.target == DVZ_SCENE_TARGET_SAMPLE;
}



/**
 * Return volume visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_volume_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "volume",
        .family = DVZ_SCENE_VISUAL_FAMILY_VOLUME,
        .eligible = _volume_query_eligible,
        .supports_profile = _volume_query_supports_profile,
        .build = _volume_query_build,
        .decode = _volume_query_decode,
        .readout = _volume_query_readout,
    };
    return &ops;
}
