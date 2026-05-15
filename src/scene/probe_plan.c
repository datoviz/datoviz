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
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/math/_cglm.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Build a synthetic GPU readback frame plan for one image probe request.
 *
 * @param panel the panel receiving the request
 * @param visual the image visual to probe
 * @param pending the pending probe request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @return true when the plan was assembled
 */
bool _scene_image_probe_plan(
    const DvzPanel* panel, DvzVisual* visual, const DvzPendingProbeRequest* pending,
    const vec2 request_ndc, DvzSceneProbePlan* out_plan)
{
    ANN(panel);
    ANN(visual);
    ANN(pending);
    ANN(request_ndc);
    ANN(out_plan);

    int pos_idx = _attr_index(visual, "position");
    int uv_idx = _attr_index(visual, "texcoords");
    if (pos_idx < 0 || uv_idx < 0)
        return false;
    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
    if (pos_attr->data == NULL || uv_attr->data == NULL || pos_attr->item_count == 0 ||
        uv_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        uv_attr->item_size != sizeof(vec2))
    {
        return false;
    }

    const void* texture_data = NULL;
    uint32_t texture_width = 0;
    uint32_t texture_height = 0;
    if (visual->field != NULL && visual->field->data != NULL &&
        visual->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        texture_data = visual->field->data;
        texture_width = visual->field->desc.width;
        texture_height = visual->field->desc.height;
    }
    else
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
    if (_dvz_mul_u64_overflows(pos_attr->item_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(uv_attr->item_count, uv_attr->item_size, &texcoord_bytes) ||
        _dvz_mul_u64_overflows(texture_width, texture_height, &texture_pixels) ||
        _dvz_mul_u64_overflows(texture_pixels, 4, &texture_bytes))
    {
        log_error("image probe request buffer size overflow");
        return false;
    }

    vec3* probe_positions = (vec3*)dvz_calloc(pos_attr->item_count, sizeof(vec3));
    if (probe_positions == NULL)
    {
        log_error("image probe request position buffer allocation failed");
        return false;
    }

    vec2* probe_texcoords = (vec2*)dvz_calloc(uv_attr->item_count, sizeof(vec2));
    if (probe_texcoords == NULL)
    {
        log_error("image probe request texcoord buffer allocation failed");
        dvz_free(probe_positions);
        return false;
    }

    vec2 target_ndc = {-0.75f, 0.75f};
    vec2 target_uv = {0.5f * (target_ndc[0] + 1.0f), 0.5f * (1.0f - target_ndc[1])};
    vec2 request_uv = {0.5f * (request_ndc[0] + 1.0f), 0.5f * (1.0f - request_ndc[1])};
    vec2 delta_uv = {request_uv[0] - target_uv[0], request_uv[1] - target_uv[1]};
    const vec3* source_positions = (const vec3*)pos_attr->data;
    const vec2* source_texcoords = (const vec2*)uv_attr->data;
    for (uint64_t j = 0; j < pos_attr->item_count; j++)
    {
        probe_positions[j][0] = source_positions[j][0];
        probe_positions[j][1] = source_positions[j][1];
        probe_positions[j][2] = source_positions[j][2];
        probe_texcoords[j][0] = source_texcoords[j][0] + delta_uv[0];
        probe_texcoords[j][1] = source_texcoords[j][1] + delta_uv[1];
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.probe", pending->request.request_id);
    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    bool ok = plan != NULL && emitter != NULL &&
              dvz_frame_plan_upload_bytes(
                  plan, "probe0_position", 0, position_bytes, "position", probe_positions) &&
              dvz_frame_plan_upload_bytes(
                  plan, "probe0_texcoords", 0, texcoord_bytes, "texcoords", probe_texcoords) &&
              dvz_frame_plan_upload_bytes(
                  plan, "probe0_texture", 0, texture_bytes, "texture", texture_data) &&
              dvz_frame_plan_upload_set_texture_extent(plan, texture_width, texture_height) &&
              dvz_frame_plan_upload_set_texture_allocation_extent(
                  plan, texture_width, texture_height) &&
              dvz_frame_plan_render_panel(
                  plan, "panel.probe", "target.probe", false,
                  (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
              dvz_frame_plan_render_visual(plan, "probe0") &&
              dvz_frame_plan_copy(plan, "target.probe", "buf.probe", 4) &&
              dvz_frame_plan_readback(plan, "buf.probe", "request.probe");
    DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
    if (render != NULL)
    {
        DvzMVP mvp = {0};
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
        render->u.render.has_mvp = true;
        render->u.render.apply_mvp = mvp;
        render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
    }
    if (!ok)
    {
        log_error(
            "image probe request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        dvz_frame_plan_emitter_destroy(emitter);
        dvz_free(probe_positions);
        dvz_free(probe_texcoords);
        return false;
    }

    out_plan->plan = plan;
    out_plan->emitter = emitter;
    out_plan->probe_positions = probe_positions;
    out_plan->probe_texcoords = probe_texcoords;
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
    dvz_frame_plan_emitter_destroy(plan->emitter);
    if (plan->probe_positions != NULL)
        dvz_free(plan->probe_positions);
    if (plan->probe_texcoords != NULL)
        dvz_free(plan->probe_texcoords);
    plan->plan = NULL;
    plan->emitter = NULL;
    plan->probe_positions = NULL;
    plan->probe_texcoords = NULL;
}
