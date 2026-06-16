/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Image sample query frame plans                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/math/_cglm.h"
#include "image/internal.h"
#include "../../query/internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "domain/field_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether an image visual uses retained per-item rectangles.
 *
 * @param visual the image visual
 * @param out_extent_idx optional extent attribute index
 * @param out_pixel_rect optional pixel-rect flag
 * @return true when the visual uses generated image rectangles
 */
static bool _image_query_generated_rect(
    const DvzVisual* visual, int* out_extent_idx, bool* out_pixel_rect)
{
    ANN(visual);
    int extent_idx = _attr_index(visual, "extent");
    int extent_px_idx = _attr_index(visual, "extent_px");
    bool has_extent = extent_idx >= 0 && visual->attrs[extent_idx].data != NULL;
    bool has_extent_px = extent_px_idx >= 0 && visual->attrs[extent_px_idx].data != NULL;
    if (has_extent && has_extent_px)
        return false;
    if (!has_extent && !has_extent_px)
        return false;
    if (out_extent_idx != NULL)
        *out_extent_idx = has_extent_px ? extent_px_idx : extent_idx;
    if (out_pixel_rect != NULL)
        *out_pixel_rect = has_extent_px;
    return true;
}


/**
 * Expand retained image rectangles to query-ready position and texcoord buffers.
 *
 * @param panel the panel receiving the request
 * @param visual the image visual
 * @param pos_attr retained position attribute
 * @param extent_attr retained extent attribute
 * @param pixel_rect whether the rectangle is expressed in logical pixels
 * @param out_plan output plan scratch buffers
 * @param out_vertex_count expanded vertex count
 * @return true when geometry was expanded
 */
static bool _image_query_expand_rects(
    const DvzPanel* panel, const DvzVisual* visual, const DvzVisualAttr* pos_attr,
    const DvzVisualAttr* extent_attr, bool pixel_rect, DvzSceneQueryScratch* out_plan,
    uint64_t* out_vertex_count)
{
    ANN(panel);
    ANN(visual);
    ANN(pos_attr);
    ANN(extent_attr);
    ANN(out_plan);
    ANN(out_vertex_count);

    if (
        pos_attr->data == NULL || extent_attr->data == NULL || pos_attr->item_count == 0 ||
        extent_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        extent_attr->item_size != sizeof(vec2))
    {
        return false;
    }

    uint64_t vertex_count = 0;
    if (_dvz_mul_u64_overflows(pos_attr->item_count, 6, &vertex_count))
    {
        log_error("image query request vertex count overflow");
        return false;
    }
    out_plan->query_positions = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
    out_plan->query_texcoords = (vec2*)dvz_calloc(vertex_count, sizeof(vec2));
    if (out_plan->query_positions == NULL || out_plan->query_texcoords == NULL)
    {
        log_error("image query request geometry allocation failed");
        dvz_free(out_plan->query_positions);
        dvz_free(out_plan->query_texcoords);
        out_plan->query_positions = NULL;
        out_plan->query_texcoords = NULL;
        return false;
    }

    int anchor_idx = _attr_index(visual, "anchor");
    int tex_rect_idx = _attr_index(visual, "tex_rect");
    const DvzVisualAttr* anchor_attr = anchor_idx >= 0 ? &visual->attrs[anchor_idx] : NULL;
    const DvzVisualAttr* tex_rect_attr = tex_rect_idx >= 0 ? &visual->attrs[tex_rect_idx] : NULL;
    const float* anchor =
        anchor_attr != NULL && anchor_attr->data != NULL &&
                anchor_attr->item_count == pos_attr->item_count &&
                anchor_attr->item_size == sizeof(vec2)
            ? (const float*)anchor_attr->data
            : NULL;
    const float* tex_rect =
        tex_rect_attr != NULL && tex_rect_attr->data != NULL &&
                tex_rect_attr->item_count == pos_attr->item_count &&
                tex_rect_attr->item_size == 4 * sizeof(float)
            ? (const float*)tex_rect_attr->data
            : NULL;

    float pixel_scale_x = 1.0f;
    float pixel_scale_y = 1.0f;
    if (pixel_rect && panel->figure != NULL)
    {
        pixel_scale_x = panel->figure->device_scale_x > 0.0f
                            ? panel->figure->device_scale_x * panel->figure->render_scale
                            : 1.0f;
        pixel_scale_y = panel->figure->device_scale_y > 0.0f
                            ? panel->figure->device_scale_y * panel->figure->render_scale
                            : 1.0f;
        if (pixel_scale_x <= 0.0f || !isfinite(pixel_scale_x))
            pixel_scale_x = 1.0f;
        if (pixel_scale_y <= 0.0f || !isfinite(pixel_scale_y))
            pixel_scale_y = 1.0f;
    }

    const float* position = (const float*)pos_attr->data;
    const float* extent = (const float*)extent_attr->data;
    for (uint64_t i = 0; i < pos_attr->item_count; i++)
    {
        const float x = position[3 * i + 0] * pixel_scale_x;
        const float y = position[3 * i + 1] * pixel_scale_y;
        const float z = position[3 * i + 2];
        const float w = extent[2 * i + 0] * pixel_scale_x;
        const float h = extent[2 * i + 1] * pixel_scale_y;
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

        const float quad_pos[6][3] = {
            {x0, y0, z}, {x0, y1, z}, {x1, y0, z}, {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
        };
        const float quad_uv[6][2] = {
            {u0, v0}, {u0, v1}, {u1, v0}, {u1, v0}, {u0, v1}, {u1, v1},
        };

        for (uint32_t j = 0; j < 6; j++)
        {
            uint64_t dst = 6 * i + j;
            dvz_memcpy(out_plan->query_positions[dst], sizeof(vec3), quad_pos[j], sizeof(vec3));
            dvz_memcpy(out_plan->query_texcoords[dst], sizeof(vec2), quad_uv[j], sizeof(vec2));
        }
    }

    *out_vertex_count = vertex_count;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Build a synthetic GPU readback frame plan for one image query request.
 *
 * @param panel the panel receiving the request
 * @param visual the image visual to query
 * @param pending the pending query request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @return true when the plan was assembled
 */
bool _scene_image_query_plan(
    const DvzPanel* panel, DvzVisual* visual, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, bool include_static_uploads, DvzSceneQueryScratch* out_plan)
{
    ANN(panel);
    ANN(visual);
    ANN(pending);
    ANN(request_ndc);
    ANN(out_plan);

    int pos_idx = _attr_index(visual, "position");
    if (pos_idx < 0)
        return false;
    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];

    const void* position_data = pos_attr->data;
    const void* texcoord_data = NULL;
    uint64_t position_item_count = pos_attr->item_count;
    uint64_t texcoord_item_count = 0;
    uint64_t draw_vertex_count = position_item_count;
    int extent_idx = -1;
    bool pixel_rect = false;
    bool generated_rect = _image_query_generated_rect(visual, &extent_idx, &pixel_rect);
    if (generated_rect)
    {
        DvzVisualAttr* extent_attr = &visual->attrs[extent_idx];
        if (include_static_uploads)
        {
            uint64_t vertex_count = 0;
            if (!_image_query_expand_rects(
                    panel, visual, pos_attr, extent_attr, pixel_rect, out_plan, &vertex_count))
            {
                return false;
            }
            position_data = out_plan->query_positions;
            texcoord_data = out_plan->query_texcoords;
            position_item_count = vertex_count;
            texcoord_item_count = vertex_count;
            draw_vertex_count = vertex_count;
        }
        else
        {
            if (_dvz_mul_u64_overflows(position_item_count, 6u, &draw_vertex_count))
            {
                log_error("image query request vertex count overflow");
                return false;
            }
            texcoord_item_count = draw_vertex_count;
        }
    }
    else
    {
        int uv_idx = _attr_index(visual, "texcoords");
        if (uv_idx < 0)
            return false;
        DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
        if (
            pos_attr->data == NULL || uv_attr->data == NULL || pos_attr->item_count == 0 ||
            uv_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
            uv_attr->item_size != sizeof(vec2))
        {
            return false;
        }
        texcoord_data = uv_attr->data;
        texcoord_item_count = uv_attr->item_count;
    }

    const void* texture_data = NULL;
    uint32_t texture_width = 0;
    uint32_t texture_height = 0;
    DvzColorRole texture_color_role = DVZ_COLOR_ROLE_SRGB_COLOR;
    if (include_static_uploads && _visual_family_state(visual)->field != NULL && _visual_family_state(visual)->field->data != NULL &&
        _visual_family_state(visual)->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        texture_data = _visual_family_state(visual)->field->data;
        texture_width = _visual_family_state(visual)->field->desc.width;
        texture_height = _visual_family_state(visual)->field->desc.height;
        texture_color_role = _visual_family_state(visual)->field->desc.color_role;
    }
    else if (include_static_uploads)
    {
        DvzImageTextureUploadPayload payload = {0};
        if (!_image_texture_upload_payload(visual, &payload) ||
            _visual_family_state(visual)->texture.rgba == NULL ||
            _visual_family_state(visual)->texture.width == 0 ||
            _visual_family_state(visual)->texture.height == 0)
        {
            return false;
        }
        texture_data = payload.data;
        texture_width = payload.region.width;
        texture_height = payload.region.height;
        texture_color_role = payload.color_role;
    }

    uint64_t position_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t texture_pixels = 0;
    uint64_t texture_bytes = 0;
    if (include_static_uploads &&
        (_dvz_mul_u64_overflows(position_item_count, sizeof(vec3), &position_bytes) ||
         _dvz_mul_u64_overflows(texcoord_item_count, sizeof(vec2), &texcoord_bytes) ||
         _dvz_mul_u64_overflows(texture_width, texture_height, &texture_pixels) ||
         _dvz_mul_u64_overflows(texture_pixels, 4, &texture_bytes)))
    {
        log_error("image query request buffer size overflow");
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.image.sample", pending->request.request_id);
    bool ok = plan != NULL;
    if (include_static_uploads)
    {
        ok = ok && dvz_frame_plan_upload_bytes(
                       plan, "query0_position", 0, position_bytes, "position", position_data);
        ok = ok && dvz_frame_plan_upload_set_topology(
                       plan, generated_rect ? DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                                             : DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        ok = ok && dvz_frame_plan_upload_bytes(
                       plan, "query0_texcoords", 0, texcoord_bytes, "texcoords", texcoord_data);
        ok = ok && dvz_frame_plan_upload_bytes(
                       plan, "query0_texture", 0, texture_bytes, "texture", texture_data) &&
             dvz_frame_plan_upload_metadata(
                 plan,
                 &(DvzFramePlanUploadMeta){
                     .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
                     .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                     .color_role = texture_color_role,
                     .visual_index = UINT32_MAX,
                     .buffer_index = UINT32_MAX,
                 }) &&
             dvz_frame_plan_upload_set_texture_extent(plan, texture_width, texture_height) &&
             dvz_frame_plan_upload_set_texture_allocation_extent(
                 plan, texture_width, texture_height);
    }
    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query.image", "target.query.image", false,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "query0");
    DvzFramePlanVisualMeta metadata = {
        .has_metadata = true,
        .visual_type = (uint32_t)DVZ_VISUAL_TYPE_IMAGE,
        .renderable_kind = (uint32_t)DVZ_RENDERABLE_TEXTURED_QUAD,
        .desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_IMAGE,
        .topology = generated_rect ? DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                                   : DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .vertex_count = draw_vertex_count <= UINT32_MAX ? (uint32_t)draw_vertex_count : 0,
        .alpha_mode = DVZ_ALPHA_OPAQUE,
        .depth_test_enabled = false,
        .field_format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
        .field_semantic = DVZ_FIELD_SEMANTIC_COLOR,
        .field_width = texture_width,
        .field_height = texture_height,
        .field_depth = 1,
        .image_nearest_sampler = pending->request.target == DVZ_SCENE_TARGET_PIXEL,
        .image_color_role = texture_color_role,
    };
    dvz_strlcpy(metadata.position_id, "query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.texcoords_id, "query0_texcoords", sizeof(metadata.texcoords_id));
    dvz_strlcpy(metadata.texture_id, "query0_texture", sizeof(metadata.texture_id));
    ok = ok && dvz_frame_plan_render_visual_metadata(plan, &metadata);
    DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
    if (render != NULL)
    {
        DvzMVP mvp = {0};
        _scene_panel_apply_mvp(panel, &mvp);
        vec2 target_ndc = {0.0f, 0.0f};
        vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
        mvp.proj[3][0] -= delta[0];
        mvp.proj[3][1] -= delta[1];
        render->u.render.has_mvp = true;
        render->u.render.apply_mvp = mvp;
        render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
    }
    ok = ok && dvz_frame_plan_copy(plan, "target.query.image", "buf.query.image", 4) &&
         dvz_frame_plan_readback(plan, "buf.query.image", "request.query.image");
    if (!ok)
    {
        log_error(
            "image query request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        return false;
    }

    out_plan->plan = plan;
    return true;
}
