/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Labels query policy                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
#include "image/internal.h"
#include "colorizer.h"
#include "_visual_lowering.h"
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
 * Return the DRP2 texture format for a labels-query integer field.
 *
 * @param format sampled-field format
 * @param out_texture_format output texture format
 * @param out_bytes_per_texel output texel byte size
 * @return true when the format can carry raw label IDs
 */
static bool _labels_query_integer_format(
    DvzFieldFormat format, uint32_t* out_texture_format, uint32_t* out_bytes_per_texel)
{
    ANN(out_texture_format);
    ANN(out_bytes_per_texel);
    DvzSceneSampleProfile profile = {0};
    if (_scene_sample_profile_resolve(
            format, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_2D, &profile) &&
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
 * Project one visual-space vertex into panel-local NDC.
 *
 * @param mvp panel MVP transform
 * @param position visual-space position
 * @param out_ndc projected NDC coordinate
 * @return true when the projected position is finite
 */
static bool _labels_query_project_vertex(DvzMVP* mvp, const float position[3], double out_ndc[2])
{
    ANN(mvp);
    ANN(position);
    ANN(out_ndc);

    vec4 p = {position[0], position[1], position[2], 1.0f};
    vec4 tmp0 = {0};
    vec4 tmp1 = {0};
    vec4 clip = {0};
    glm_mat4_mulv(mvp->model, p, tmp0);
    glm_mat4_mulv(mvp->view, tmp0, tmp1);
    glm_mat4_mulv(mvp->proj, tmp1, clip);
    if (fabsf(clip[3]) < 1e-12f)
        return false;
    out_ndc[0] = (double)(clip[0] / clip[3]);
    out_ndc[1] = (double)(clip[1] / clip[3]);
    return isfinite(out_ndc[0]) && isfinite(out_ndc[1]);
}



/**
 * Interpolate texture coordinates for a point inside one projected triangle.
 *
 * @param request_ndc requested panel-local NDC point
 * @param p0 first projected triangle vertex
 * @param p1 second projected triangle vertex
 * @param p2 third projected triangle vertex
 * @param uv0 first texture coordinate
 * @param uv1 second texture coordinate
 * @param uv2 third texture coordinate
 * @param out_uv interpolated texture coordinate
 * @return true when the request falls inside the triangle
 */
static bool _labels_query_triangle_uv(
    const vec2 request_ndc, const double p0[2], const double p1[2], const double p2[2],
    const float uv0[2], const float uv1[2], const float uv2[2], double out_uv[2])
{
    ANN(request_ndc);
    ANN(p0);
    ANN(p1);
    ANN(p2);
    ANN(uv0);
    ANN(uv1);
    ANN(uv2);
    ANN(out_uv);

    const double x = (double)request_ndc[0];
    const double y = (double)request_ndc[1];
    const double denom =
        (p1[1] - p2[1]) * (p0[0] - p2[0]) + (p2[0] - p1[0]) * (p0[1] - p2[1]);
    if (fabs(denom) < 1e-18)
        return false;

    const double w0 =
        ((p1[1] - p2[1]) * (x - p2[0]) + (p2[0] - p1[0]) * (y - p2[1])) / denom;
    const double w1 =
        ((p2[1] - p0[1]) * (x - p2[0]) + (p0[0] - p2[0]) * (y - p2[1])) / denom;
    const double w2 = 1.0 - w0 - w1;
    const double eps = 1e-7;
    if (w0 < -eps || w1 < -eps || w2 < -eps)
        return false;

    out_uv[0] = w0 * (double)uv0[0] + w1 * (double)uv1[0] + w2 * (double)uv2[0];
    out_uv[1] = w0 * (double)uv0[1] + w1 * (double)uv1[1] + w2 * (double)uv2[1];
    return isfinite(out_uv[0]) && isfinite(out_uv[1]);
}



/**
 * Test one labels triangle and return the interpolated texture coordinate.
 *
 * @param mvp panel MVP transform
 * @param positions three visual-space positions
 * @param texcoords three texture coordinates
 * @param request_ndc requested panel-local NDC point
 * @param out_uv interpolated texture coordinate
 * @return true when the triangle contains the request
 */
static bool _labels_query_projected_triangle_uv(
    DvzMVP* mvp, const vec3 positions[3], const vec2 texcoords[3], const vec2 request_ndc,
    double out_uv[2])
{
    ANN(mvp);
    ANN(positions);
    ANN(texcoords);
    ANN(request_ndc);
    ANN(out_uv);

    double p0[2] = {0};
    double p1[2] = {0};
    double p2[2] = {0};
    if (
        !_labels_query_project_vertex(mvp, positions[0], p0) ||
        !_labels_query_project_vertex(mvp, positions[1], p1) ||
        !_labels_query_project_vertex(mvp, positions[2], p2))
    {
        return false;
    }
    return _labels_query_triangle_uv(
        request_ndc, p0, p1, p2, texcoords[0], texcoords[1], texcoords[2], out_uv);
}



/**
 * Return whether a visual attribute has dense data of one item size.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param item_size expected item size
 * @param out_attr optional output attribute
 * @return true when matching data exists
 */
static bool _labels_query_attr(
    const DvzVisual* visual, const char* attr_name, uint64_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    if (out_attr != NULL)
        *out_attr = attr;
    return true;
}



/**
 * Resolve the labels texture coordinate for one retained labels visual.
 *
 * @param panel requesting panel
 * @param visual labels visual
 * @param request_ndc requested panel-local NDC coordinate
 * @param out_uv output texture coordinate
 * @return true when the request falls on the labels visual
 */
static bool _labels_query_visual_uv(
    const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc, double out_uv[2])
{
    ANN(panel);
    ANN(visual);
    ANN(request_ndc);
    ANN(out_uv);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_labels_query_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;
    const float* position = (const float*)pos_attr->data;

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);

    const DvzVisualAttr* extent_attr = NULL;
    if (_labels_query_attr(visual, "extent", sizeof(vec2), &extent_attr))
    {
        if (extent_attr->item_count != pos_attr->item_count)
            return false;
        const float* extent = (const float*)extent_attr->data;
        const DvzVisualAttr* anchor_attr = NULL;
        const bool has_anchor =
            _labels_query_attr(visual, "anchor", sizeof(vec2), &anchor_attr) &&
            anchor_attr->item_count == pos_attr->item_count;
        const float* anchor = has_anchor ? (const float*)anchor_attr->data : NULL;
        const DvzVisualAttr* tex_rect_attr = NULL;
        const bool has_tex_rect =
            _labels_query_attr(visual, "tex_rect", 4 * sizeof(float), &tex_rect_attr) &&
            tex_rect_attr->item_count == pos_attr->item_count;
        const float* tex_rect = has_tex_rect ? (const float*)tex_rect_attr->data : NULL;

        for (uint64_t k = pos_attr->item_count; k > 0; k--)
        {
            uint64_t i = k - 1;
            const float x = position[3 * i + 0];
            const float y = position[3 * i + 1];
            const float z = position[3 * i + 2];
            const float w = extent[2 * i + 0];
            const float h = extent[2 * i + 1];
            const float ax = anchor != NULL ? anchor[2 * i + 0] : 0.0f;
            const float ay = anchor != NULL ? anchor[2 * i + 1] : 0.0f;
            const float x0 = x - 0.5f * (ax + 1.0f) * w;
            const float x1 = x0 + w;
            const float y0 = y - 0.5f * (ay + 1.0f) * h;
            const float y1 = y0 + h;
            const float u0 = tex_rect != NULL ? tex_rect[4 * i + 0] : 0.0f;
            const float v0 = tex_rect != NULL ? tex_rect[4 * i + 1] : 0.0f;
            const float u1 = tex_rect != NULL ? tex_rect[4 * i + 2] : 1.0f;
            const float v1 = tex_rect != NULL ? tex_rect[4 * i + 3] : 1.0f;
            const vec3 quad_pos[6] = {
                {x0, y0, z}, {x0, y1, z}, {x1, y0, z},
                {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
            };
            const vec2 quad_uv[6] = {
                {u0, v0}, {u0, v1}, {u1, v0}, {u1, v0}, {u0, v1}, {u1, v1},
            };
            for (uint32_t j = 0; j < 6; j += 3)
            {
                const vec3 tri_pos[3] = {
                    {quad_pos[j + 0][0], quad_pos[j + 0][1], quad_pos[j + 0][2]},
                    {quad_pos[j + 1][0], quad_pos[j + 1][1], quad_pos[j + 1][2]},
                    {quad_pos[j + 2][0], quad_pos[j + 2][1], quad_pos[j + 2][2]},
                };
                const vec2 tri_uv[3] = {
                    {quad_uv[j + 0][0], quad_uv[j + 0][1]},
                    {quad_uv[j + 1][0], quad_uv[j + 1][1]},
                    {quad_uv[j + 2][0], quad_uv[j + 2][1]},
                };
                if (_labels_query_projected_triangle_uv(
                        &mvp, tri_pos, tri_uv, request_ndc, out_uv))
                    return true;
            }
        }
        return false;
    }

    const DvzVisualAttr* uv_attr = NULL;
    if (!_labels_query_attr(visual, "texcoords", sizeof(vec2), &uv_attr))
        return false;
    if (uv_attr->item_count != pos_attr->item_count)
        return false;
    const float* texcoords = (const float*)uv_attr->data;
    if (pos_attr->item_count == 4)
    {
        const uint32_t order[6] = {0, 1, 2, 2, 1, 3};
        for (uint32_t j = 0; j < 6; j += 3)
        {
            const uint32_t i0 = order[j + 0];
            const uint32_t i1 = order[j + 1];
            const uint32_t i2 = order[j + 2];
            const vec3 tri_pos[3] = {
                {position[3 * i0 + 0], position[3 * i0 + 1], position[3 * i0 + 2]},
                {position[3 * i1 + 0], position[3 * i1 + 1], position[3 * i1 + 2]},
                {position[3 * i2 + 0], position[3 * i2 + 1], position[3 * i2 + 2]},
            };
            const vec2 tri_uv[3] = {
                {texcoords[2 * i0 + 0], texcoords[2 * i0 + 1]},
                {texcoords[2 * i1 + 0], texcoords[2 * i1 + 1]},
                {texcoords[2 * i2 + 0], texcoords[2 * i2 + 1]},
            };
            if (_labels_query_projected_triangle_uv(
                    &mvp, tri_pos, tri_uv, request_ndc, out_uv))
                return true;
        }
        return false;
    }
    if (pos_attr->item_count % 3 != 0)
        return false;
    for (uint64_t tri = pos_attr->item_count / 3; tri > 0; tri--)
    {
        uint64_t base = 3 * (tri - 1);
        const vec3 tri_pos[3] = {
            {position[3 * (base + 0) + 0], position[3 * (base + 0) + 1],
             position[3 * (base + 0) + 2]},
            {position[3 * (base + 1) + 0], position[3 * (base + 1) + 1],
             position[3 * (base + 1) + 2]},
            {position[3 * (base + 2) + 0], position[3 * (base + 2) + 1],
             position[3 * (base + 2) + 2]},
        };
        const vec2 tri_uv[3] = {
            {texcoords[2 * (base + 0) + 0], texcoords[2 * (base + 0) + 1]},
            {texcoords[2 * (base + 1) + 0], texcoords[2 * (base + 1) + 1]},
            {texcoords[2 * (base + 2) + 0], texcoords[2 * (base + 2) + 1]},
        };
        if (_labels_query_projected_triangle_uv(&mvp, tri_pos, tri_uv, request_ndc, out_uv))
            return true;
    }
    return false;
}



/**
 * Resolve query-render geometry for a retained labels visual.
 *
 * @param visual labels visual
 * @param scratch output scratch storage
 * @param out_position_data output position buffer pointer
 * @param out_texcoord_data output texture coordinate buffer pointer
 * @param out_vertex_count output vertex count
 * @param out_topology output primitive topology
 * @return true when renderable labels geometry was resolved
 */
static bool _labels_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, const void** out_position_data,
    const void** out_texcoord_data, uint64_t* out_vertex_count, uint32_t* out_topology)
{
    ANN(visual);
    ANN(scratch);
    ANN(out_position_data);
    ANN(out_texcoord_data);
    ANN(out_vertex_count);
    ANN(out_topology);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_labels_query_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;

    const DvzVisualAttr* extent_attr = NULL;
    if (_labels_query_attr(visual, "extent", sizeof(vec2), &extent_attr))
    {
        (void)extent_attr;
        uint64_t vertex_count = 0;
        if (!_image_query_generated_rect_geometry(visual, scratch, false, true, &vertex_count))
            return false;
        *out_position_data = scratch->query_positions;
        *out_texcoord_data = scratch->query_texcoords;
        *out_vertex_count = vertex_count;
        *out_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        return true;
    }

    const DvzVisualAttr* uv_attr = NULL;
    if (!_labels_query_attr(visual, "texcoords", sizeof(vec2), &uv_attr))
        return false;
    if (uv_attr->item_count != pos_attr->item_count)
        return false;
    if (pos_attr->item_count != 4 && pos_attr->item_count % 3 != 0)
        return false;

    *out_position_data = pos_attr->data;
    *out_texcoord_data = uv_attr->data;
    *out_vertex_count = pos_attr->item_count;
    *out_topology = pos_attr->item_count == 4 ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                                              : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    return true;
}



/**
 * Return the byte size of one dense 2D labels field.
 *
 * @param field labels field
 * @param bytes_per_texel field texel byte size
 * @param out_byte_size output byte size
 * @return true when the size is valid
 */
static bool _labels_query_field_byte_size(
    const DvzSampledField* field, uint32_t bytes_per_texel, uint64_t* out_byte_size)
{
    ANN(field);
    ANN(out_byte_size);
    uint64_t pixels = 0;
    if (
        _dvz_mul_u64_overflows(field->desc.width, field->desc.height, &pixels) ||
        _dvz_mul_u64_overflows(pixels, bytes_per_texel, out_byte_size) ||
        *out_byte_size == 0)
    {
        log_error("labels query texture size overflow");
        return false;
    }
    return true;
}



/**
 * Decode an encoded labels-query payload word into a category ID.
 *
 * @param format sampled-field format
 * @param encoded encoded r32uint shader payload
 * @param out_id decoded category ID
 * @return true when the format was decoded
 */
static bool _labels_query_decode_sample(
    DvzFieldFormat format, uint32_t encoded, DvzCategoryId* out_id)
{
    ANN(out_id);
    if (encoded == 0)
        return false;
    uint32_t bits = encoded - 1u;
    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            format, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_2D, &profile))
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
 * Return the display label for one labels-query category.
 *
 * @param visual labels visual
 * @param id category ID
 * @param out_label output display label
 * @param label_size output label capacity
 */
static void _labels_query_category_label(
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
 * Return whether a labels visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _labels_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_LABELS)
        return false;
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_SEGMENT)
        return false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        if (attach->visual == visual && attach->controller_mode == DVZ_CONTROLLER_FIXED)
            return false;
    }
    if ((visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) == 0)
        return false;
    if (visual->field == NULL)
        return false;
    uint32_t texture_format = 0;
    uint32_t bytes_per_texel = 0;
    return _labels_query_integer_format(
        visual->field->desc.format, &texture_format, &bytes_per_texel);
}



/**
 * Build a labels-family rendered integer query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _labels_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    DvzSampledField* field = ctx->visual->field;
    if (field == NULL || field->data == NULL)
        return false;
    if (field->desc.dim != DVZ_FIELD_DIM_2D || field->desc.width == 0 ||
        field->desc.height == 0)
    {
        return false;
    }

    uint32_t texture_format = 0;
    uint32_t bytes_per_texel = 0;
    if (!_labels_query_integer_format(field->desc.format, &texture_format, &bytes_per_texel))
        return false;

    double uv[2] = {0};
    if (!_labels_query_visual_uv(ctx->panel, ctx->visual, ctx->request_ndc, uv))
        return false;
    if (uv[0] < 0.0 || uv[0] > 1.0 || uv[1] < 0.0 || uv[1] > 1.0)
        return false;

    const void* position_data = NULL;
    const void* texcoord_data = NULL;
    uint64_t vertex_count = 0;
    uint32_t topology = 0;
    if (!_labels_query_geometry(
            ctx->visual, &out_plan->scratch, &position_data, &texcoord_data, &vertex_count,
            &topology))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t texture_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec2), &texcoord_bytes) ||
        !_labels_query_field_byte_size(field, bytes_per_texel, &texture_bytes))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.labels", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "labels_query0_position", 0, position_bytes, "position",
                   position_data) &&
         dvz_frame_plan_upload_set_topology(plan, topology) &&
         dvz_frame_plan_upload_bytes(
             plan, "labels_query0_texcoords", 0, texcoord_bytes, "texcoords", texcoord_data) &&
         dvz_frame_plan_upload_bytes(
             plan, "labels_query0_texture", 0, texture_bytes, "texture", field->data) &&
         dvz_frame_plan_upload_set_texture_format(plan, texture_format, bytes_per_texel) &&
         dvz_frame_plan_upload_set_texture_extent(plan, field->desc.width, field->desc.height) &&
         dvz_frame_plan_upload_set_texture_allocation_extent(
             plan, field->desc.width, field->desc.height);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_LABELS;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_TEXTURED_QUAD;
    metadata.desc_kind = (uint32_t)_scene_visual_lowering_desc_kind(ctx->visual);
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    metadata.field_format = field->desc.format;
    metadata.field_width = field->desc.width;
    metadata.field_height = field->desc.height;
    metadata.has_labels = true;
    metadata.labels_state = ctx->visual->labels;
    dvz_strlcpy(metadata.position_id, "labels_query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.texcoords_id, "labels_query0_texcoords", sizeof(metadata.texcoords_id));
    dvz_strlcpy(metadata.texture_id, "labels_query0_texture", sizeof(metadata.texture_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query.labels", "target.query.labels", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "labels_query0") &&
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

    DvzFramePlanCopyDesc copy = {
        .src_resource_id = "target.query.labels",
        .dst_resource_id = "buf.query.labels",
        .extent = {1, 1, 1},
        .format = VK_FORMAT_R32_UINT,
        .bytes_per_texel = sizeof(uint32_t),
        .bytes_per_row = sizeof(uint32_t),
        .rows_per_image = 1,
        .byte_size = sizeof(uint32_t),
        .request_id = ctx->pending->request.request_id,
    };
    ok = ok && dvz_frame_plan_copy_ex(plan, &copy) &&
         dvz_frame_plan_readback(plan, "buf.query.labels", "request.query.labels");
    if (!ok)
    {
        log_error(
            "labels query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    out_plan->field = field;
    out_plan->uvw[0] = uv[0];
    out_plan->uvw[1] = uv[1];
    out_plan->uvw[2] = 0.0;
    out_plan->target_width = 1;
    out_plan->target_height = 1;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    out_plan->schema = (DvzSceneQuerySchema){
        .fields = DVZ_SCENE_QUERY_SCHEMA_FIELD_LABEL_ID | DVZ_SCENE_QUERY_SCHEMA_FIELD_UVW,
        .value_kind = DVZ_QUERY_VALUE_CATEGORY,
        .profile = ctx->profile,
        .format = out_plan->format,
        .byte_size = out_plan->byte_size,
    };
    return true;
}



/**
 * Decode a labels-family integer query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _labels_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->figure);
    ANN(ctx->build->visual);
    ANN(ctx->plan);
    ANN(ctx->plan->field);
    ANN(ctx->bytes);
    ANN(out_result);
    if (ctx->byte_size < ctx->plan->byte_size)
    {
        out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
        return true;
    }

    uint32_t encoded = 0;
    dvz_memcpy(&encoded, sizeof(encoded), ctx->bytes, sizeof(encoded));

    DvzCategoryId label_id = 0;
    if (!_labels_query_decode_sample(ctx->plan->field->desc.format, encoded, &label_id))
    {
        out_result->status = DVZ_QUERY_STATUS_MISS;
        out_result->visual_id =
            _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
        out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_LABELS;
        out_result->raw_target = DVZ_SCENE_TARGET_SEGMENT;
        out_result->resolved_target = DVZ_SCENE_TARGET_SEGMENT;
        out_result->value_kind = DVZ_QUERY_VALUE_NONE;
        return true;
    }
    if (label_id == ctx->build->visual->labels.background_id)
    {
        out_result->status = DVZ_QUERY_STATUS_MISS;
        out_result->visual_id =
            _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
        out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_LABELS;
        out_result->raw_target = DVZ_SCENE_TARGET_SEGMENT;
        out_result->resolved_target = DVZ_SCENE_TARGET_SEGMENT;
        out_result->value_kind = DVZ_QUERY_VALUE_NONE;
        return true;
    }

    uint64_t target_id = label_id >= 0 ? (uint64_t)label_id : 0;
    out_result->status = DVZ_QUERY_STATUS_HIT;
    out_result->hit = true;
    out_result->visual_id = _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
    out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_LABELS;
    out_result->payload_version = 1;
    out_result->raw_target = DVZ_SCENE_TARGET_SEGMENT;
    out_result->raw_id = target_id;
    out_result->resolved_target = DVZ_SCENE_TARGET_SEGMENT;
    out_result->resolved_id = target_id;
    out_result->group_id = target_id;
    out_result->category_id = label_id;
    out_result->scale = ctx->build->visual->scale;
    out_result->has_uvw = true;
    out_result->uvw[0] = ctx->plan->uvw[0];
    out_result->uvw[1] = ctx->plan->uvw[1];
    out_result->uvw[2] = ctx->plan->uvw[2];
    out_result->value_kind = DVZ_QUERY_VALUE_CATEGORY;
    _labels_query_category_label(
        ctx->build->visual, label_id, out_result->label, sizeof(out_result->label));
    return true;
}



/**
 * Complete labels-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _labels_query_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(result);
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return labels visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_labels_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "labels",
        .family = DVZ_SCENE_VISUAL_FAMILY_LABELS,
        .eligible = _labels_query_eligible,
        .build = _labels_query_build,
        .decode = _labels_query_decode,
        .readout = _labels_query_readout,
    };
    return &ops;
}
