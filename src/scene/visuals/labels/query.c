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

#include "datoviz/math/_cglm.h"
#include "../../../drp2/_stream.h"
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
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UINT:
    case DVZ_FIELD_FORMAT_R8_SINT:
    case DVZ_FIELD_FORMAT_R16_UINT:
    case DVZ_FIELD_FORMAT_R16_SINT:
    case DVZ_FIELD_FORMAT_R32_UINT:
    case DVZ_FIELD_FORMAT_R32_SINT:
        return _field_format_texture_format(format, out_texture_format) &&
               _field_format_bytes_per_texel(format, out_bytes_per_texel);
    default:
        *out_texture_format = 0;
        *out_bytes_per_texel = 0;
        return false;
    }
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
 * Decode raw labels-query texel bytes into a category ID.
 *
 * @param format sampled-field format
 * @param sample raw texel bytes
 * @param out_id decoded category ID
 * @return true when the format was decoded
 */
static bool _labels_query_decode_sample(
    DvzFieldFormat format, const uint8_t sample[4], DvzCategoryId* out_id)
{
    ANN(sample);
    ANN(out_id);
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UINT:
        *out_id = (DvzCategoryId)sample[0];
        return true;
    case DVZ_FIELD_FORMAT_R8_SINT:
    {
        int8_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R16_UINT:
    {
        uint16_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R16_SINT:
    {
        int16_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R32_UINT:
    {
        uint32_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R32_SINT:
    {
        int32_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    default:
        return false;
    }
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
    if (visual->scale != NULL)
    {
        for (uint32_t i = 0; i < visual->scale->category_count; i++)
        {
            const DvzScaleCategoryState* category = &visual->scale->categories[i];
            if (category->category_id == id && category->has_label)
            {
                dvz_strlcpy(out_label, category->label, label_size);
                return;
            }
        }
    }
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
    if ((request->flags & DVZ_SCENE_QUERY_FLAG_COMPAT_PROBE) != 0)
        return true;
    return (visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) != 0;
}



/**
 * Build a labels-family direct integer readback query plan.
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

    uint32_t texel_x = (uint32_t)floor(uv[0] * (double)field->desc.width);
    uint32_t texel_y = (uint32_t)floor(uv[1] * (double)field->desc.height);
    if (texel_x >= field->desc.width)
        texel_x = field->desc.width - 1;
    if (texel_y >= field->desc.height)
        texel_y = field->desc.height - 1;

    out_plan->field = field;
    out_plan->texel_x = texel_x;
    out_plan->texel_y = texel_y;
    out_plan->uvw[0] = uv[0];
    out_plan->uvw[1] = uv[1];
    out_plan->uvw[2] = 0.0;
    out_plan->format = texture_format;
    out_plan->byte_size = bytes_per_texel;
    return true;
}



/**
 * Execute a labels-family direct integer texture readback.
 *
 * @param ctx build context
 * @param executor retained query executor
 * @param caps capability snapshot
 * @param plan query plan
 * @param bytes output byte buffer
 * @param byte_size output byte buffer size
 * @param out_executed whether the stream executed successfully before download
 * @return true when the selected sample was downloaded
 */
static bool _labels_query_execute(
    const DvzSceneQueryBuildContext* ctx, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzSceneQueryPlan* plan, uint8_t* bytes,
    uint32_t byte_size, bool* out_executed)
{
    ANN(ctx);
    ANN(executor);
    ANN(caps);
    ANN(plan);
    ANN(plan->field);
    ANN(bytes);
    ANN(out_executed);
    (void)caps;
    *out_executed = false;
    if (executor->runtime == NULL || byte_size < plan->byte_size)
        return false;

    const DvzSampledField* field = plan->field;
    uint32_t bytes_per_texel = 0;
    uint32_t texture_format = 0;
    if (!_labels_query_integer_format(field->desc.format, &texture_format, &bytes_per_texel))
        return false;
    if (bytes_per_texel != plan->byte_size)
        return false;

    uint64_t row_bytes = 0;
    uint64_t buffer_size = 0;
    if (
        _dvz_mul_u64_overflows(field->desc.width, bytes_per_texel, &row_bytes) ||
        _dvz_mul_u64_overflows(row_bytes, field->desc.height, &buffer_size) ||
        row_bytes > UINT32_MAX || buffer_size == 0)
    {
        log_error("labels query readback buffer size overflow");
        return false;
    }

    uint64_t sample_offset = 0;
    uint64_t row_offset = 0;
    uint64_t texel_offset = 0;
    if (
        _dvz_mul_u64_overflows(plan->texel_y, row_bytes, &row_offset) ||
        _dvz_mul_u64_overflows(plan->texel_x, bytes_per_texel, &texel_offset) ||
        _dvz_add_u64_overflows(row_offset, texel_offset, &sample_offset))
    {
        log_error("labels query readback sample offset overflow");
        return false;
    }

    dvz_drp2_runtime_reset(executor->runtime);
    executor->image_probe_visual = NULL;
    executor->image_probe_position_version = 0;
    executor->image_probe_texcoord_version = 0;
    executor->image_probe_texture_version = 0;

    const uint64_t texture_id = 7001;
    const uint64_t buffer_id = 7002;
    const uint64_t encoder_id = 7003;
    const uint64_t command_buffer_id = 7004;
    const uint64_t submission_id = 7005;
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    if (stream == NULL)
        return false;

    bool ok = dvz_drp2_stream_hello_renderer(stream, "scene-labels-query") &&
              dvz_drp2_stream_renderer_hello_reply(stream, "datoviz") &&
              dvz_drp2_stream_create_texture_2d_format_usage(
                  stream, texture_id, field->desc.width, field->desc.height, texture_format,
                  DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) &&
              dvz_drp2_stream_write_texture_2d_bytes(
                  stream, texture_id, 0, field->desc.width, field->desc.height,
                  (uint32_t)row_bytes, field->desc.height, field->data) &&
              dvz_drp2_stream_create_buffer(
                  stream, buffer_id, buffer_size,
                  DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ) &&
              dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
              dvz_drp2_stream_copy_texture_to_buffer(
                  stream, encoder_id, texture_id, buffer_id, 0, field->desc.width,
                  field->desc.height, (uint32_t)row_bytes, field->desc.height) &&
              dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id) &&
              dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    if (!ok)
    {
        log_error("labels query readback stream assembly failed");
        dvz_drp2_stream_destroy(stream);
        return false;
    }

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(executor->runtime, stream);
    if (!result.ok)
    {
        log_error(
            "labels query readback runtime execution failed (code=%d command=%u)",
            (int)result.code, result.command_index);
        dvz_drp2_stream_destroy(stream);
        return false;
    }
    *out_executed = true;

    ok = false;
    if (ctx->figure != NULL && ctx->figure->scene != NULL &&
        ctx->figure->scene->test.force_readback_download_failure)
    {
        log_error("labels query readback buffer download forced to fail");
    }
    else
    {
        ok = dvz_drp2_runtime_download_buffer(
            executor->runtime, buffer_id, sample_offset, bytes_per_texel, bytes);
        if (!ok)
            log_error("labels query readback buffer download failed");
    }
    dvz_drp2_stream_destroy(stream);
    return ok;
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

    DvzCategoryId label_id = 0;
    if (!_labels_query_decode_sample(ctx->plan->field->desc.format, ctx->bytes, &label_id))
    {
        out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
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
        .execute = _labels_query_execute,
        .decode = _labels_query_decode,
        .readout = _labels_query_readout,
    };
    return &ops;
}
