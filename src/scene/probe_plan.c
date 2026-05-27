/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene image probe frame plans                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/math/_cglm.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"



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
static bool _image_probe_generated_rect(
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
 * Expand retained image rectangles to probe-ready position and texcoord buffers.
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
static bool _image_probe_expand_rects(
    const DvzPanel* panel, const DvzVisual* visual, const DvzVisualAttr* pos_attr,
    const DvzVisualAttr* extent_attr, bool pixel_rect, DvzSceneProbePlan* out_plan,
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
        log_error("image probe request vertex count overflow");
        return false;
    }
    out_plan->probe_positions = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
    out_plan->probe_texcoords = (vec2*)dvz_calloc(vertex_count, sizeof(vec2));
    if (out_plan->probe_positions == NULL || out_plan->probe_texcoords == NULL)
    {
        log_error("image probe request geometry allocation failed");
        dvz_free(out_plan->probe_positions);
        dvz_free(out_plan->probe_texcoords);
        out_plan->probe_positions = NULL;
        out_plan->probe_texcoords = NULL;
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
            dvz_memcpy(out_plan->probe_positions[dst], sizeof(vec3), quad_pos[j], sizeof(vec3));
            dvz_memcpy(out_plan->probe_texcoords[dst], sizeof(vec2), quad_uv[j], sizeof(vec2));
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
 * @param visual the image visual to probe
 * @param pending the pending query request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @return true when the plan was assembled
 */
bool _scene_image_probe_plan(
    const DvzPanel* panel, DvzVisual* visual, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, bool include_static_uploads, DvzSceneProbePlan* out_plan)
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
    int extent_idx = -1;
    bool pixel_rect = false;
    bool generated_rect = _image_probe_generated_rect(visual, &extent_idx, &pixel_rect);
    if (generated_rect)
    {
        DvzVisualAttr* extent_attr = &visual->attrs[extent_idx];
        if (include_static_uploads)
        {
            uint64_t vertex_count = 0;
            if (!_image_probe_expand_rects(
                    panel, visual, pos_attr, extent_attr, pixel_rect, out_plan, &vertex_count))
            {
                return false;
            }
            position_data = out_plan->probe_positions;
            texcoord_data = out_plan->probe_texcoords;
            position_item_count = vertex_count;
            texcoord_item_count = vertex_count;
        }
        else
        {
            texcoord_item_count = position_item_count * 6u;
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
    if (include_static_uploads && visual->field != NULL && visual->field->data != NULL &&
        visual->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        texture_data = visual->field->data;
        texture_width = visual->field->desc.width;
        texture_height = visual->field->desc.height;
    }
    else if (include_static_uploads)
    {
        DvzFieldRegion upload_region = {0};
        const void* upload_data = NULL;
        if (!_scene_prepare_image_texture(visual, &upload_region, &upload_data) ||
            visual->texture.rgba == NULL || visual->texture.width == 0 ||
            visual->texture.height == 0)
        {
            return false;
        }
        (void)upload_region;
        (void)upload_data;
        texture_data = visual->texture.rgba;
        texture_width = visual->texture.width;
        texture_height = visual->texture.height;
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
        log_error("image probe request buffer size overflow");
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.probe", pending->request.request_id);
    bool ok = plan != NULL;
    if (include_static_uploads)
    {
        ok = ok && dvz_frame_plan_upload_bytes(
                       plan, "probe0_position", 0, position_bytes, "position", position_data) &&
             dvz_frame_plan_upload_bytes(
                 plan, "probe0_texcoords", 0, texcoord_bytes, "texcoords", texcoord_data) &&
             dvz_frame_plan_upload_bytes(
                 plan, "probe0_texture", 0, texture_bytes, "texture", texture_data) &&
             dvz_frame_plan_upload_set_texture_extent(plan, texture_width, texture_height) &&
             dvz_frame_plan_upload_set_texture_allocation_extent(
                 plan, texture_width, texture_height);
    }
    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.probe", "target.probe", false,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "probe0");
    DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
    if (render != NULL)
    {
        DvzMVP mvp = {0};
        _scene_request_apply_mvp(panel, request_ndc, &mvp);
        render->u.render.has_mvp = true;
        render->u.render.apply_mvp = mvp;
        render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
    }
    ok = ok && dvz_frame_plan_copy(plan, "target.probe", "buf.probe", 4) &&
         dvz_frame_plan_readback(plan, "buf.probe", "request.probe");
    if (!ok)
    {
        log_error(
            "image probe request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        return false;
    }

    out_plan->plan = plan;
    return true;
}



/**
 * Destroy a synthetic image probe frame plan wrapper.
 *
 * @param plan the plan wrapper
 */
void _scene_probe_plan_destroy(DvzSceneProbePlan* plan)
{
    if (plan == NULL)
        return;
    dvz_frame_plan_destroy(plan->plan);
    plan->plan = NULL;
    dvz_free(plan->probe_positions);
    plan->probe_positions = NULL;
    dvz_free(plan->probe_texcoords);
    plan->probe_texcoords = NULL;
    dvz_free(plan->pick_colors);
    plan->pick_colors = NULL;
    dvz_free(plan->pick_ids);
    plan->pick_ids = NULL;
    dvz_free(plan->pick_position_start);
    plan->pick_position_start = NULL;
    dvz_free(plan->pick_position_curr);
    plan->pick_position_curr = NULL;
    dvz_free(plan->pick_position_end);
    plan->pick_position_end = NULL;
    dvz_free(plan->pick_line_width);
    plan->pick_line_width = NULL;
    dvz_free(plan->pick_path_flags);
    plan->pick_path_flags = NULL;
    dvz_free(plan->pick_path_distance);
    plan->pick_path_distance = NULL;
    dvz_free(plan->pick_indices);
    plan->pick_indices = NULL;
}
