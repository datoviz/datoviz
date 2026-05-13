/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene MVP bind group helpers                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_emit.h"
#include "_mvp_bindings.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene/panzoom.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve MVP bind groups needed by all visuals in one panel render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param render the render node for one panel
 * @param out_bgl_id the shared MVP bind group layout id
 * @param out_apply_bg_id the panel apply-MVP bind group id
 * @param out_fixed_bg_id the shared fixed-MVP bind group id
 * @return whether the MVP bindings were resolved
 */
bool _mvp_bindings_resolve_panel_sets(
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

    /* MVP UBO infrastructure: one BGL shared across all panels. */
    uint64_t mvp_bgl_id = _obj_id(emitter, "_bgl_mvp", &is_new);
    if (mvp_bgl_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, mvp_bgl_id);

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
    {
        char buf_key[128], bg_key[128], slot_key[128];
        dvz_snprintf(buf_key, sizeof(buf_key), "_mvp_buf_%s_apply", render->u.render.panel_id);
        dvz_snprintf(bg_key, sizeof(bg_key), "_mvp_bg_%s_apply", render->u.render.panel_id);
        dvz_snprintf(slot_key, sizeof(slot_key), "%s_apply", render->u.render.panel_id);

        uint64_t buf_id = _obj_id(emitter, buf_key, &is_new);
        if (buf_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                             DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, buf_id, sizeof(DvzMVP), usage);
        }
        uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
        if (bg_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, bg_id, mvp_bgl_id, buf_id, 0, sizeof(DvzMVP));

        DvzMVP* slot = _emitter_mvp_slot(emitter, slot_key);
        if (slot != NULL)
            *slot = render->u.render.apply_mvp;
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, buf_id, 0, sizeof(DvzMVP),
                       slot ? slot : &render->u.render.apply_mvp);
        apply_bg_id = bg_id;
    }

    if (needs_fixed && ok)
    {
        const char* buf_key = "_mvp_buf_fixed";
        const char* bg_key = "_mvp_bg_fixed";
        const char* slot_key = "_fixed";

        uint64_t buf_id = _obj_id(emitter, buf_key, &is_new);
        if (buf_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                             DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, buf_id, sizeof(DvzMVP), usage);
        }
        uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
        if (bg_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, bg_id, mvp_bgl_id, buf_id, 0, sizeof(DvzMVP));

        DvzMVP* slot = _emitter_mvp_slot(emitter, slot_key);
        if (slot != NULL)
        {
            glm_mat4_identity(slot->model);
            glm_mat4_identity(slot->view);
            glm_mat4_identity(slot->proj);
            slot->time  = 0.0f;
            slot->flags = 0;
        }
        DvzMVP local_identity = {0};
        glm_mat4_identity(local_identity.model);
        glm_mat4_identity(local_identity.view);
        glm_mat4_identity(local_identity.proj);
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, buf_id, 0, sizeof(DvzMVP),
                       slot ? slot : &local_identity);
        fixed_bg_id = bg_id;
    }

    if (!ok)
        return false;
    *out_bgl_id = mvp_bgl_id;
    *out_apply_bg_id = apply_bg_id;
    *out_fixed_bg_id = fixed_bg_id;
    return true;
}



/**
 * Resolve the MVP bind group used by a single-draw GLSL render path.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param render the render node
 * @param out_bgl_id the shared MVP bind group layout id
 * @param out_bg_id the resolved MVP bind group id
 * @return whether the MVP binding was resolved
 */
bool _mvp_bindings_resolve_single_set(
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
    bool mvp_bgl_new = false;
    uint64_t mvp_bgl_id = _obj_id(emitter, "_bgl_mvp", &mvp_bgl_new);
    if (mvp_bgl_id == 0)
        return false;
    if (mvp_bgl_new)
        ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, mvp_bgl_id);

    const char* mode_tag =
        (render->u.render.controller_modes[0] == DVZ_CONTROLLER_FIXED) ? "fixed" : "apply";
    char mvp_buf_key[128], mvp_bg_key[128];
    dvz_snprintf(
        mvp_buf_key, sizeof(mvp_buf_key), "_mvp_buf_%s_%s", render->u.render.panel_id,
        mode_tag);
    dvz_snprintf(
        mvp_bg_key, sizeof(mvp_bg_key), "_mvp_bg_%s_%s", render->u.render.panel_id, mode_tag);

    bool mvp_buf_new = false;
    uint64_t mvp_buf_id = _obj_id(emitter, mvp_buf_key, &mvp_buf_new);
    if (mvp_buf_id == 0)
        return false;
    if (mvp_buf_new)
    {
        uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                         DVZ_DRP2_BUFFER_USAGE_COPY_DST;
        ok = ok && dvz_drp2_stream_create_buffer(stream, mvp_buf_id, sizeof(DvzMVP), usage);
    }

    bool mvp_bg_new = false;
    uint64_t mvp_bg_id = _obj_id(emitter, mvp_bg_key, &mvp_bg_new);
    if (mvp_bg_id == 0)
        return false;
    if (mvp_bg_new)
        ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                       stream, mvp_bg_id, mvp_bgl_id, mvp_buf_id, 0, sizeof(DvzMVP));

    /* Copy MVP into the emitter's per-(panel, controller_mode) cache (persists past
     * frame plan destruction so write_buffer_bytes' borrowed pointer stays valid). */
    char mvp_slot_key[128];
    dvz_snprintf(mvp_slot_key, sizeof(mvp_slot_key), "%s_%s", render->u.render.panel_id, mode_tag);
    DvzMVP* mvp_slot = _emitter_mvp_slot(emitter, mvp_slot_key);
    if (mvp_slot != NULL)
        *mvp_slot = render->u.render.apply_mvp;
    ok = ok && dvz_drp2_stream_write_buffer_bytes(
                   stream, mvp_buf_id, 0, sizeof(DvzMVP),
                   mvp_slot ? mvp_slot : &render->u.render.apply_mvp);

    if (!ok)
        return false;
    *out_bgl_id = mvp_bgl_id;
    *out_bg_id = mvp_bg_id;
    return true;
}
