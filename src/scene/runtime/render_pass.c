/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render-pass helpers                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_frame_plan_emit.h"
#include "_render_pass.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve or create the color target used by a runtime render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param cfg the emission config
 * @param out_id the resolved color target id
 * @return whether the color target was resolved
 */
bool _render_pass_resolve_color_target(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    *out_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        *out_id = cfg->color_target_id;
        return true;
    }

    bool needs_create = false;
    ResourceId* resource = _resource_entry(&emitter->objects, "_ct", &needs_create);
    if (resource == NULL || resource->id == 0)
        return false;
    uint32_t width = cfg != NULL && cfg->target_width > 0 ? cfg->target_width : 4;
    uint32_t height = cfg != NULL && cfg->target_height > 0 ? cfg->target_height : 4;
    uint32_t format = cfg != NULL && cfg->color_target_format != 0 ?
                          cfg->color_target_format :
                          VK_FORMAT_R8G8B8A8_UNORM;
    if (
        needs_create || resource->texture_width == 0 || resource->texture_height == 0 ||
        resource->texture_depth == 0)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        needs_create = true;
    }
    else if (
        width != resource->texture_width || height != resource->texture_height ||
        resource->texture_depth != 1 || resource->texture_format != format)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        needs_create = true;
    }
    if (needs_create)
    {
        uint32_t usage =
            DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, resource->id, width, height, format, usage))
            return false;
    }
    *out_id = resource->id;
    return true;
}



/**
 * Resolve or create the optional readback buffer for a render-pass copy node.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param copy the optional readback copy node
 * @param out_id the resolved readback buffer id
 * @return whether the readback buffer was resolved
 */
bool _render_pass_resolve_readback_buffer(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* copy,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    *out_id = 0;
    if (copy == NULL)
        return true;

    bool is_new = false;
    uint64_t rb_id = _obj_buffer_id(emitter, "_rb", copy->u.copy.byte_size, &is_new);
    if (rb_id == 0)
        return false;
    if (is_new)
    {
        uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
        if (!dvz_drp2_stream_create_buffer(stream, rb_id, copy->u.copy.byte_size, usage))
            return false;
    }
    *out_id = rb_id;
    return true;
}



/**
 * Allocate transient command ids for one runtime render pass submission.
 *
 * @param emitter the persistent emitter
 * @param encoder_id the command encoder id
 * @param render_pass_id the render pass id
 * @param command_buffer_id the command buffer id
 * @param submission_id the queue submission id
 */
void _render_pass_next_ids(
    DvzFramePlanEmitter* emitter, uint64_t* encoder_id, uint64_t* render_pass_id,
    uint64_t* command_buffer_id, uint64_t* submission_id)
{
    ANN(emitter);
    ANN(encoder_id);
    ANN(render_pass_id);
    ANN(command_buffer_id);
    ANN(submission_id);

    *encoder_id = _emitter_next_transient_id(emitter);
    *render_pass_id = _emitter_next_transient_id(emitter);
    *command_buffer_id = _emitter_next_transient_id(emitter);
    *submission_id = _emitter_next_transient_id(emitter);
}



/**
 * Finish a render encoder and submit it, optionally with a readback copy.
 *
 * @param stream the DRP2 command stream
 * @param encoder_id the command encoder id
 * @param command_buffer_id the command buffer id
 * @param submission_id the queue submission id
 * @param color_id the rendered color texture id
 * @param readback_buffer_id the optional readback buffer id
 * @param copy the optional readback copy node
 * @return whether the copy, finish, and submit commands were emitted
 */
bool _render_pass_copy_finish_submit(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t command_buffer_id,
    uint64_t submission_id, uint64_t color_id, uint64_t readback_buffer_id,
    const DvzFramePlanNode* copy)
{
    ANN(stream);

    bool ok = true;
    if (copy != NULL)
    {
        if (color_id == 0 || readback_buffer_id == 0)
            return false;
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, readback_buffer_id,
                       copy->u.copy.dst_offset, copy->u.copy.extent[0], copy->u.copy.extent[1],
                       (uint32_t)copy->u.copy.bytes_per_row, copy->u.copy.rows_per_image);
    }

    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (copy != NULL)
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, readback_buffer_id, 0,
                       copy->u.copy.byte_size);
    else
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    return ok;
}
