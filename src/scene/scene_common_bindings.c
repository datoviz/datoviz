/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene common bind group helpers                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_emit.h"
#include "_scene_common_bindings.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene/panzoom.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create the shared scene-common bind group layout.
 *
 * @param stream the DRP2 command stream
 * @param id the bind group layout id
 * @return whether the command was appended
 */
static bool _create_common_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    const uint32_t visibility =
        DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT;
    DvzDrp2BindGroupLayoutEntry entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = visibility,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = visibility,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 2, entries);
}



/**
 * Fill the viewport uniform from a render node.
 *
 * @param render the render node
 * @param out the output viewport uniform
 */
static void _viewport_uniform_from_render(
    const DvzFramePlanNode* render, DvzSceneViewportUniform* out)
{
    ANN(render);
    ANN(out);
    if (render->u.render.has_viewport)
    {
        *out = render->u.render.viewport;
        return;
    }
    out->x = render->u.render.desc.x;
    out->y = render->u.render.desc.y;
    out->width = render->u.render.desc.width;
    out->height = render->u.render.desc.height;
}



/**
 * Fill the identity MVP used by fixed-controller visuals.
 *
 * @param out the output MVP
 */
static void _identity_mvp(DvzMVP* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzMVP), 0, sizeof(DvzMVP));
    glm_mat4_identity(out->model);
    glm_mat4_identity(out->view);
    glm_mat4_identity(out->proj);
    out->time = 0.0f;
    out->flags = 0;
}



/**
 * Copy an MVP into a deterministic uniform payload with zeroed padding.
 *
 * @param dst the destination MVP
 * @param src the source MVP
 */
static void _mvp_uniform_copy(DvzMVP* dst, const DvzMVP* src)
{
    ANN(dst);
    ANN(src);

    dvz_memset(dst, sizeof(DvzMVP), 0, sizeof(DvzMVP));
    dvz_memcpy(dst->model, sizeof(dst->model), src->model, sizeof(src->model));
    dvz_memcpy(dst->view, sizeof(dst->view), src->view, sizeof(src->view));
    dvz_memcpy(dst->proj, sizeof(dst->proj), src->proj, sizeof(src->proj));
    dst->time = src->time;
    dst->flags = src->flags;
}



/**
 * Resolve one scene-common bind group for a panel/controller-mode pair.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param render the render node
 * @param common_bgl_id the shared common bind group layout id
 * @param mode_tag the controller mode tag
 * @param fixed whether the MVP should be identity
 * @param out_bg_id the resolved bind group id
 * @return whether the common bind group was resolved
 */
static bool _resolve_common_set(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t common_bgl_id, const char* mode_tag, bool fixed, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(mode_tag);
    ANN(out_bg_id);

    *out_bg_id = 0;

    char mvp_buf_key[128], viewport_buf_key[128], bg_key[128];
    char mvp_slot_key[128], viewport_slot_key[128];
    dvz_snprintf(
        mvp_buf_key, sizeof(mvp_buf_key), "_common_mvp_buf_%s_%s",
        render->u.render.panel_id, mode_tag);
    dvz_snprintf(
        viewport_buf_key, sizeof(viewport_buf_key), "_common_viewport_buf_%s_%s",
        render->u.render.panel_id, mode_tag);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_common_bg_%s_%s", render->u.render.panel_id, mode_tag);
    dvz_snprintf(
        mvp_slot_key, sizeof(mvp_slot_key), "%s_%s", render->u.render.panel_id, mode_tag);
    dvz_snprintf(
        viewport_slot_key, sizeof(viewport_slot_key), "%s_%s", render->u.render.panel_id,
        mode_tag);

    bool is_new = false;
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;

    uint64_t mvp_buf_id = _obj_id(emitter, mvp_buf_key, &is_new);
    if (mvp_buf_id == 0)
        return false;
    bool mvp_buf_new = is_new;
    if (mvp_buf_new && !dvz_drp2_stream_create_buffer(
                           stream, mvp_buf_id, sizeof(DvzMVP), usage))
        return false;

    uint64_t viewport_buf_id = _obj_id(emitter, viewport_buf_key, &is_new);
    if (viewport_buf_id == 0)
        return false;
    bool viewport_buf_new = is_new;
    if (viewport_buf_new && !dvz_drp2_stream_create_buffer(
                                stream, viewport_buf_id,
                                sizeof(DvzSceneViewportUniform), usage))
        return false;

    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[2] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = mvp_buf_id,
                .offset = 0,
                .size = sizeof(DvzMVP),
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = viewport_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneViewportUniform),
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(
                stream, bg_id, common_bgl_id, 2, entries))
            return false;
    }

    DvzMVP local_identity = {0};
    const DvzMVP* mvp_src = &render->u.render.apply_mvp;
    DvzMVP* mvp_slot = _emitter_mvp_slot(emitter, mvp_slot_key);
    if (mvp_slot == NULL)
        return false;
    if (fixed)
    {
        _identity_mvp(&local_identity);
        mvp_src = &local_identity;
    }
    _mvp_uniform_copy(mvp_slot, mvp_src);
    mvp_src = mvp_slot;

    DvzSceneViewportUniform local_viewport = {0};
    _viewport_uniform_from_render(render, &local_viewport);
    DvzSceneViewportUniform* viewport_slot =
        _emitter_viewport_slot(emitter, viewport_slot_key);
    if (viewport_slot == NULL)
        return false;
    *viewport_slot = local_viewport;

    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, mvp_buf_id, 0, sizeof(DvzMVP), mvp_src))
        return false;
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, viewport_buf_id, 0, sizeof(DvzSceneViewportUniform), viewport_slot))
        return false;

    *out_bg_id = bg_id;
    return true;
}



/**
 * Resolve common bind groups needed by all visuals in one panel render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param render the render node for one panel
 * @param out_bgl_id the shared common bind group layout id
 * @param out_apply_bg_id the panel apply-common bind group id
 * @param out_fixed_bg_id the panel fixed-common bind group id
 * @return whether the common bindings were resolved
 */
bool _scene_common_bindings_resolve_panel_sets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t* out_bgl_id, uint64_t* out_apply_bg_id, uint64_t* out_fixed_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(out_bgl_id);
    ANN(out_apply_bg_id);
    ANN(out_fixed_bg_id);

    *out_bgl_id = 0;
    *out_apply_bg_id = 0;
    *out_fixed_bg_id = 0;

    bool ok = true;
    bool is_new = false;

    /* Common bind group infrastructure: one BGL shared across all panels. */
    uint64_t common_bgl_id = _obj_id(emitter, "_bgl_scene_common", &is_new);
    if (common_bgl_id == 0)
        return false;
    if (is_new)
        ok = ok && _create_common_bind_group_layout(stream, common_bgl_id);

    /* Determine which modes are needed for this panel. */
    bool needs_apply = false, needs_fixed = false;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
            needs_fixed = true;
        else
            needs_apply = true;
    }

    uint64_t apply_bg_id = 0, fixed_bg_id = 0;
    if (needs_apply && ok)
        ok = _resolve_common_set(
            emitter, stream, render, common_bgl_id, "apply", false, &apply_bg_id);
    if (needs_fixed && ok)
        ok = _resolve_common_set(
            emitter, stream, render, common_bgl_id, "fixed", true, &fixed_bg_id);

    if (!ok)
        return false;
    *out_bgl_id = common_bgl_id;
    *out_apply_bg_id = apply_bg_id;
    *out_fixed_bg_id = fixed_bg_id;
    return true;
}



/**
 * Resolve the common bind group used by a single-draw GLSL render path.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param render the render node
 * @param out_bgl_id the shared common bind group layout id
 * @param out_bg_id the resolved common bind group id
 * @return whether the common binding was resolved
 */
bool _scene_common_bindings_resolve_single_set(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t* out_bgl_id, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(out_bgl_id);
    ANN(out_bg_id);

    *out_bgl_id = 0;
    *out_bg_id = 0;

    bool ok = true;
    bool common_bgl_new = false;
    uint64_t common_bgl_id = _obj_id(emitter, "_bgl_scene_common", &common_bgl_new);
    if (common_bgl_id == 0)
        return false;
    if (common_bgl_new)
        ok = ok && _create_common_bind_group_layout(stream, common_bgl_id);

    const char* mode_tag =
        (render->u.render.controller_modes[0] == DVZ_CONTROLLER_FIXED) ? "fixed" : "apply";
    uint64_t common_bg_id = 0;
    ok = ok && _resolve_common_set(
                   emitter, stream, render, common_bgl_id, mode_tag,
                   render->u.render.controller_modes[0] == DVZ_CONTROLLER_FIXED, &common_bg_id);

    if (!ok)
        return false;
    *out_bgl_id = common_bgl_id;
    *out_bg_id = common_bg_id;
    return true;
}
