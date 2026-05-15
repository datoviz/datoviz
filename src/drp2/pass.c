/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime passes                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <volk.h>

#include "_assertions.h"
#include "_runtime.h"
#include "_stream.h"
#include "datoviz/vklite/sync.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a normalized float to the closed [0, 1] interval.
 *
 * @param value the input value
 * @return the clamped value
 */
static float _clamp_unit(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}


/**
 * Convert a normalized [0, 1] viewport to an in-bounds pixel render area.
 *
 * @param target_width render-target width in pixels
 * @param target_height render-target height in pixels
 * @param viewport normalized x/y/width/height
 * @param out_x output left pixel coordinate
 * @param out_y output top pixel coordinate
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
static void _render_area_from_viewport(
    uint32_t target_width, uint32_t target_height, const float viewport[4], uint32_t* out_x,
    uint32_t* out_y, uint32_t* out_width, uint32_t* out_height)
{
    ANN(viewport);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);

    float x0 = _clamp_unit(viewport[0]);
    float y0 = _clamp_unit(viewport[1]);
    float x1 = _clamp_unit(viewport[0] + viewport[2]);
    float y1 = _clamp_unit(viewport[1] + viewport[3]);
    if (x1 < x0)
        x1 = x0;
    if (y1 < y0)
        y1 = y0;

    uint32_t px0 = (uint32_t)(x0 * (float)target_width);
    uint32_t py0 = (uint32_t)(y0 * (float)target_height);
    uint32_t px1 = (uint32_t)(x1 * (float)target_width);
    uint32_t py1 = (uint32_t)(y1 * (float)target_height);

    if (px0 > target_width)
        px0 = target_width;
    if (py0 > target_height)
        py0 = target_height;
    if (px1 > target_width)
        px1 = target_width;
    if (py1 > target_height)
        py1 = target_height;

    if (px1 <= px0 && target_width > px0)
        px1 = px0 + 1;
    if (py1 <= py0 && target_height > py0)
        py1 = py0 + 1;

    *out_x = px0;
    *out_y = py0;
    *out_width = px1 > px0 ? px1 - px0 : 0;
    *out_height = py1 > py0 ? py1 - py0 : 0;
}



/**
 * Find the active borrowed frame target depth image for same-size intermediate passes.
 *
 * @param state vklite runtime state
 * @param width required target width
 * @param height required target height
 * @return borrowed frame target with live depth resources, or NULL
 */
static Drp2VkliteObject* _active_borrowed_depth_target(
    Drp2VkliteState* state, uint32_t width, uint32_t height)
{
    ANN(state);
    if (state->active_borrowed_command_buffer == VK_NULL_HANDLE)
        return NULL;

    for (uint32_t i = 0; i < state->count; i++)
    {
        Drp2VkliteObject* object = &state->objects[i];
        if (object->destroyed || !object->borrowed_frame_target)
            continue;
        if (object->width != width || object->height != height)
            continue;
        if (object->depth_images == NULL || object->depth_views == NULL)
            continue;
        return object;
    }
    return NULL;
}



static VkAttachmentLoadOp _vklite_attachment_load_op(DvzDrp2AttachmentLoadOp op)
{
    switch (op)
    {
    case DVZ_DRP2_ATTACHMENT_LOAD_CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case DVZ_DRP2_ATTACHMENT_LOAD_LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    }
}



static VkAttachmentStoreOp _vklite_attachment_store_op(DvzDrp2AttachmentStoreOp op)
{
    switch (op)
    {
    case DVZ_DRP2_ATTACHMENT_STORE_STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_STORE_OP_STORE;
    }
}


/**
 * Begin a vklite dynamic-rendering pass for a DRP2 BeginRenderPass command.
 *
 * @param state vklite runtime state
 * @param command DRP2 BeginRenderPass command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_begin_render_pass(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint32_t color_count = command->u.begin_render_pass.color_attachment_count;
    if (color_count == 0)
        color_count = 1;
    if (color_count > DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* targets[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    VkImageView target_views[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    for (uint32_t i = 0; i < color_count; i++)
    {
        uint64_t texture_id = command->u.begin_render_pass.color_attachment_count > 0
                                  ? command->u.begin_render_pass.color_attachments[i].texture_id
                                  : command->u.begin_render_pass.texture_id;
        targets[i] = _vklite_find(state, texture_id);
        target_views[i] = _vklite_object_image_view(targets[i]);
        if (targets[i] == NULL || targets[i]->images == NULL ||
            target_views[i] == VK_NULL_HANDLE)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    Drp2VkliteObject* target = targets[0];
    if (target->borrowed_frame_target && color_count > 1)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 1; i < color_count; i++)
    {
        if (targets[i]->borrowed_frame_target || targets[i]->width != target->width ||
            targets[i]->height != target->height)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    Drp2VkliteObject* pass =
        _vklite_add(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    for (uint32_t i = 0; i < color_count; i++)
    {
        uint64_t texture_id = command->u.begin_render_pass.color_attachment_count > 0
                                  ? command->u.begin_render_pass.color_attachments[i].texture_id
                                  : command->u.begin_render_pass.texture_id;
        targets[i] = _vklite_find(state, texture_id);
        target_views[i] = _vklite_object_image_view(targets[i]);
        if (targets[i] == NULL || targets[i]->images == NULL ||
            target_views[i] == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    target = targets[0];

    DvzCommands* cmds = NULL;
    if (state->active_borrowed_command_buffer != VK_NULL_HANDLE)
    {
        cmds = _vklite_borrowed_frame_commands_create(
            state->runtime->device, state->active_borrowed_command_buffer);
        if (cmds == NULL)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        pass->borrowed_commands = true;
    }
    else if (target->borrowed_frame_target)
    {
        cmds = _vklite_borrowed_frame_commands_create(
            state->runtime->device, target->command_buffer);
        if (cmds == NULL)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        pass->borrowed_commands = true;
    }
    else
    {
        cmds = _vklite_owned_commands_create(state->runtime->device);
        if (cmds == NULL)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    pass->commands = cmds;
    pass->width = target->width;
    pass->height = target->height;
    pass->color_target_count = color_count;
    for (uint32_t i = 0; i < color_count; i++)
        pass->color_target_ids[i] = targets[i]->id;
    pass->viewport_x = command->u.begin_render_pass.viewport[0];
    pass->viewport_y = command->u.begin_render_pass.viewport[1];
    pass->viewport_width = command->u.begin_render_pass.viewport[2];
    pass->viewport_height = command->u.begin_render_pass.viewport[3];
    pass->scissor_x = command->u.begin_render_pass.viewport[0];
    pass->scissor_y = command->u.begin_render_pass.viewport[1];
    pass->scissor_width = command->u.begin_render_pass.viewport[2];
    pass->scissor_height = command->u.begin_render_pass.viewport[3];

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    if (rendering == NULL)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->rendering = rendering;

    dvz_rendering(rendering);
    dvz_rendering_area(rendering, 0, 0, target->width, target->height);
    for (uint32_t i = 0; i < color_count; i++)
    {
        const DvzDrp2ColorAttachment* attachment =
            command->u.begin_render_pass.color_attachment_count > 0
                ? &command->u.begin_render_pass.color_attachments[i]
                : NULL;
        bool clear_attachment =
            attachment != NULL ? attachment->clear : command->u.begin_render_pass.clear;
        DvzDrp2AttachmentLoadOp load_op =
            attachment != NULL ? attachment->load_op :
                                 (clear_attachment ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR :
                                                     DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
        DvzDrp2AttachmentStoreOp store_op =
            attachment != NULL ? attachment->store_op : DVZ_DRP2_ATTACHMENT_STORE_STORE;
        const float* clear_color =
            attachment != NULL ? attachment->clear_color : command->u.begin_render_pass.clear_color;
        VkClearValue clear = {
            .color.float32 = {clear_color[0], clear_color[1], clear_color[2], clear_color[3]}};
        DvzAttachment* catt = dvz_rendering_color(rendering, i);
        dvz_attachment_image(catt, target_views[i], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_attachment_ops(
            catt, _vklite_attachment_load_op(load_op), _vklite_attachment_store_op(store_op));
        if (load_op == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR)
            dvz_attachment_clear(catt, clear);
    }
    if (command->u.begin_render_pass.has_depth_attachment)
    {
        Drp2VkliteObject* borrowed_depth_owner =
            target->borrowed_frame_target
                ? NULL
                : _active_borrowed_depth_target(state, target->width, target->height);
        Drp2VkliteObject* depth_owner =
            target->borrowed_frame_target ? target :
                                            (borrowed_depth_owner != NULL ? borrowed_depth_owner :
                                                                           pass);
        bool load_existing_depth = borrowed_depth_owner != NULL;
        VkImageView depth_view = VK_NULL_HANDLE;

        if (load_existing_depth)
        {
            depth_view = dvz_image_views_handle(depth_owner->depth_views, 0);
        }
        else
        {
            DvzImages* depth_images = dvz_images_create_wrapper();
            DvzImageViews* depth_views = dvz_image_views_create_wrapper();
            if (depth_images == NULL || depth_views == NULL)
            {
                if (depth_views != NULL)
                    dvz_image_views_free(depth_views);
                if (depth_images != NULL)
                    dvz_images_free(depth_images);
                return _vklite_fail_destroy_object(
                    pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            dvz_images(
                state->runtime->device, state->runtime->allocator, VK_IMAGE_TYPE_2D, 1,
                depth_images);
            dvz_images_format(depth_images, VK_FORMAT_D32_SFLOAT);
            dvz_images_size(depth_images, target->width, target->height, 1);
            dvz_images_mip(depth_images, 1);
            dvz_images_layers(depth_images, 1);
            dvz_images_samples(depth_images, VK_SAMPLE_COUNT_1_BIT);
            dvz_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            if (dvz_images_create(depth_images) != 0)
            {
                dvz_image_views_free(depth_views);
                dvz_images_free(depth_images);
                return _vklite_fail_destroy_object(
                    pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            dvz_image_views(depth_images, depth_views);
            dvz_image_views_aspect(depth_views, VK_IMAGE_ASPECT_DEPTH_BIT);
            dvz_image_views_create(depth_views);
            depth_view = dvz_image_views_handle(depth_views, 0);
            if (depth_view == VK_NULL_HANDLE)
            {
                dvz_image_views_free(depth_views);
                dvz_images_destroy(depth_images);
                dvz_images_free(depth_images);
                return _vklite_fail_destroy_object(
                    pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            if (depth_owner->depth_images != NULL || depth_owner->depth_views != NULL)
            {
                if (!_vklite_retire_frame_target_depth(state, depth_owner))
                {
                    dvz_image_views_destroy(depth_views);
                    dvz_image_views_free(depth_views);
                    dvz_images_destroy(depth_images);
                    dvz_images_free(depth_images);
                    return _vklite_fail_destroy_object(
                        pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
                }
            }
            depth_owner->depth_images = depth_images;
            depth_owner->depth_views = depth_views;
        }
        if (depth_view == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        DvzAttachment* datt = dvz_rendering_depth(rendering);
        dvz_attachment_image(datt, depth_view, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
        DvzDrp2AttachmentLoadOp depth_load_op = command->u.begin_render_pass.depth_load_op;
        if (!command->u.begin_render_pass.depth_ops_explicit && load_existing_depth)
            depth_load_op = DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
        dvz_attachment_ops(
            datt, _vklite_attachment_load_op(depth_load_op),
            _vklite_attachment_store_op(command->u.begin_render_pass.depth_store_op));
        if (depth_load_op == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR)
        {
            dvz_attachment_clear(
                datt, (VkClearValue){.depthStencil = {command->u.begin_render_pass.clear_depth, 0}});
        }
    }

    if (!pass->borrowed_commands && dvz_cmd_begin_result(cmds) != 0)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 0; i < state->count; i++)
    {
        Drp2VkliteObject* object = &state->objects[i];
        bool is_color_target = false;
        for (uint32_t j = 0; j < color_count; j++)
        {
            if (object == targets[j])
            {
                is_color_target = true;
                break;
            }
        }
        if (object->kind == DRP2_OBJECT_TEXTURE && !is_color_target && object->views != NULL &&
            !object->borrowed_frame_target &&
            (object->usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0)
        {
            _vklite_transition_image(
                cmds, object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        }
    }
    for (uint32_t i = 0; i < color_count; i++)
    {
        if (!targets[i]->borrowed_frame_target)
        {
            _vklite_transition_image(
                cmds, targets[i], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        }
    }
    if (pass->depth_images != NULL)
    {
        DvzBarriers barriers = {0};
        dvz_barriers(&barriers);
        DvzBarrierImage* bimg =
            dvz_barriers_image(&barriers, dvz_image_handle(pass->depth_images, 0));
        ANN(bimg);
        dvz_barrier_image_stage(
            bimg,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);
        dvz_barrier_image_access(
            bimg, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        dvz_barrier_image_layout(
            bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
        dvz_barrier_image_aspect(bimg, VK_IMAGE_ASPECT_DEPTH_BIT);
        dvz_cmd_barriers(cmds, &barriers);
    }
    else if (target->borrowed_frame_target && target->depth_images != NULL)
    {
        DvzBarriers barriers = {0};
        dvz_barriers(&barriers);
        DvzBarrierImage* bimg =
            dvz_barriers_image(&barriers, dvz_image_handle(target->depth_images, 0));
        ANN(bimg);
        dvz_barrier_image_stage(
            bimg,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);
        dvz_barrier_image_access(
            bimg, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        dvz_barrier_image_layout(
            bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
        dvz_barrier_image_aspect(bimg, VK_IMAGE_ASPECT_DEPTH_BIT);
        dvz_cmd_barriers(cmds, &barriers);
    }
    else if (command->u.begin_render_pass.has_depth_attachment)
    {
        Drp2VkliteObject* depth_owner =
            _active_borrowed_depth_target(state, target->width, target->height);
        if (depth_owner != NULL)
        {
            DvzBarriers barriers = {0};
            dvz_barriers(&barriers);
            DvzBarrierImage* bimg =
                dvz_barriers_image(&barriers, dvz_image_handle(depth_owner->depth_images, 0));
            ANN(bimg);
            dvz_barrier_image_stage(
                bimg,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);
            dvz_barrier_image_access(
                bimg, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
            dvz_barrier_image_layout(
                bimg, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
            dvz_barrier_image_aspect(bimg, VK_IMAGE_ASPECT_DEPTH_BIT);
            dvz_cmd_barriers(cmds, &barriers);
        }
    }
    dvz_cmd_rendering_begin(cmds, rendering);
    return _drp2_ok();
}


/**
 * Begin a vklite command buffer for a DRP2 compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 BeginComputePass command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_begin_compute_pass(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* pass =
        _vklite_add(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->commands = cmds;

    if (dvz_cmd_begin_result(cmds) != 0)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Bind a vklite pipeline within a DRP2 render or compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetPipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_pipeline.pass_id);
    Drp2VkliteObject* pipeline = _vklite_find(state, command->u.set_pipeline.pipeline_id);
    if (pass == NULL || pass->commands == NULL || pipeline == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (pass->kind == DRP2_OBJECT_RENDER_PASS && pipeline->kind == DRP2_OBJECT_RENDER_PIPELINE &&
        pipeline->graphics != NULL)
    {
        uint32_t viewport_x = 0, viewport_y = 0, viewport_width = pass->width,
                 viewport_height = pass->height;
        float viewport[4] = {
            pass->viewport_x,
            pass->viewport_y,
            pass->viewport_width,
            pass->viewport_height,
        };
        uint32_t scissor_x = 0, scissor_y = 0, scissor_width = pass->width,
                 scissor_height = pass->height;
        float scissor[4] = {
            pass->scissor_x,
            pass->scissor_y,
            pass->scissor_width,
            pass->scissor_height,
        };
        _render_area_from_viewport(
            pass->width, pass->height, viewport, &viewport_x, &viewport_y, &viewport_width,
            &viewport_height);
        _render_area_from_viewport(
            pass->width, pass->height, scissor, &scissor_x, &scissor_y, &scissor_width,
            &scissor_height);
        dvz_graphics_viewport(
            pipeline->graphics, (float)viewport_x, (float)viewport_y, (float)viewport_width,
            (float)viewport_height, 0, 1,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_graphics_scissor(
            pipeline->graphics, (int32_t)scissor_x, (int32_t)scissor_y, scissor_width,
            scissor_height,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_cmd_bind_graphics(pass->commands, pipeline->graphics);
        pass->current_pipeline_id = command->u.set_pipeline.pipeline_id;
        return _drp2_ok();
    }
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS && pipeline->kind == DRP2_OBJECT_COMPUTE_PIPELINE &&
        pipeline->compute != NULL)
    {
        dvz_cmd_bind_compute(pass->commands, pipeline->compute);
        pass->current_pipeline_id = command->u.set_pipeline.pipeline_id;
        return _drp2_ok();
    }
    return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
}


/**
 * Bind a vertex buffer within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetVertexBuffer command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_vertex_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_vertex_buffer.pass_id);
    Drp2VkliteObject* buffer = _vklite_find(state, command->u.set_vertex_buffer.buffer_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL ||
        buffer == NULL || buffer->buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSize offset = command->u.set_vertex_buffer.offset;
    dvz_cmd_bind_vertex_buffers(
        pass->commands, command->u.set_vertex_buffer.slot, 1, buffer->buffer, &offset);
    return _drp2_ok();
}


/**
 * Bind an index buffer within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetIndexBuffer command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_index_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_index_buffer.pass_id);
    Drp2VkliteObject* buffer = _vklite_find(state, command->u.set_index_buffer.buffer_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL ||
        buffer == NULL || buffer->buffer == NULL)
    {
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    VkIndexType index_type = VK_INDEX_TYPE_UINT32;
    if (strcmp(command->u.set_index_buffer.index_format, "uint16") == 0)
        index_type = VK_INDEX_TYPE_UINT16;
    else if (strcmp(command->u.set_index_buffer.index_format, "uint32") != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    dvz_cmd_bind_index_buffer(
        pass->commands, buffer->buffer, command->u.set_index_buffer.offset, index_type);
    return _drp2_ok();
}


/**
 * Set a dynamic viewport within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetViewport command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_viewport(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_viewport.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->viewport_x = command->u.set_viewport.viewport[0];
    pass->viewport_y = command->u.set_viewport.viewport[1];
    pass->viewport_width = command->u.set_viewport.viewport[2];
    pass->viewport_height = command->u.set_viewport.viewport[3];

    uint32_t x = 0, y = 0, width = pass->width, height = pass->height;
    _render_area_from_viewport(
        pass->width, pass->height, command->u.set_viewport.viewport, &x, &y, &width, &height);
    VkViewport viewport = {
        .x = (float)x,
        .y = (float)y,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(dvz_commands_handle(pass->commands), 0, 1, &viewport);
    return _drp2_ok();
}



/**
 * Set a dynamic scissor within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetScissor command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_scissor(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_scissor.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->scissor_x = command->u.set_scissor.scissor[0];
    pass->scissor_y = command->u.set_scissor.scissor[1];
    pass->scissor_width = command->u.set_scissor.scissor[2];
    pass->scissor_height = command->u.set_scissor.scissor[3];

    uint32_t x = 0, y = 0, width = pass->width, height = pass->height;
    _render_area_from_viewport(
        pass->width, pass->height, command->u.set_scissor.scissor, &x, &y, &width, &height);
    VkRect2D scissor = {
        .offset = {.x = (int32_t)x, .y = (int32_t)y},
        .extent = {.width = width, .height = height},
    };
    vkCmdSetScissor(dvz_commands_handle(pass->commands), 0, 1, &scissor);
    return _drp2_ok();
}


/**
 * Bind a vklite descriptor set within a DRP2 render or compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetBindGroup command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_bind_group(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_bind_group.pass_id);
    Drp2VkliteObject* bind_group = _vklite_find(state, command->u.set_bind_group.bind_group_id);
    if (pass == NULL || pass->commands == NULL || bind_group == NULL ||
        bind_group->descriptors == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2VkliteObject* pipeline = _vklite_find(state, pass->current_pipeline_id);
    if (pipeline == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS)
        bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    else if (pass->kind != DRP2_OBJECT_RENDER_PASS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (pipeline->combined_pipeline_layout != VK_NULL_HANDLE)
    {
        VkDescriptorSet descriptor_set = dvz_descriptors_handle(bind_group->descriptors, 0);
        uint32_t dynamic_offsets[DVZ_DRP2_MAX_BINDINGS] = {0};
        for (uint32_t i = 0; i < command->u.set_bind_group.dynamic_offset_count; i++)
            dynamic_offsets[i] = (uint32_t)command->u.set_bind_group.dynamic_offsets[i];
        vkCmdBindDescriptorSets(
            dvz_commands_handle(pass->commands), bind_point, pipeline->combined_pipeline_layout,
            command->u.set_bind_group.slot, 1, &descriptor_set,
            command->u.set_bind_group.dynamic_offset_count, dynamic_offsets);
    }
    else
    {
        uint32_t dynamic_offsets[DVZ_DRP2_MAX_BINDINGS] = {0};
        for (uint32_t i = 0; i < command->u.set_bind_group.dynamic_offset_count; i++)
            dynamic_offsets[i] = (uint32_t)command->u.set_bind_group.dynamic_offsets[i];
        dvz_cmd_bind_descriptors(
            pass->commands, bind_point, bind_group->descriptors, command->u.set_bind_group.slot, 1,
            command->u.set_bind_group.dynamic_offset_count, dynamic_offsets);
    }
    return _drp2_ok();
}


/**
 * Record a direct draw within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 Draw command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_draw(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.draw.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_draw(
        pass->commands, command->u.draw.first_vertex, command->u.draw.vertex_count,
        command->u.draw.first_instance, command->u.draw.instance_count);
    return _drp2_ok();
}


/**
 * Record an indexed draw within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 DrawIndexed command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_draw_indexed(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.draw_indexed.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_draw_indexed(
        pass->commands, command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex,
        command->u.draw_indexed.index_count, command->u.draw_indexed.first_instance,
        command->u.draw_indexed.instance_count);
    return _drp2_ok();
}


/**
 * Record a compute dispatch within a vklite compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 DispatchWorkgroups command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_dispatch_workgroups(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.dispatch.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_dispatch(
        pass->commands, command->u.dispatch.x, command->u.dispatch.y, command->u.dispatch.z);
    return _drp2_ok();
}


/**
 * End and submit a vklite dynamic-rendering pass.
 *
 * @param state vklite runtime state
 * @param pass_id DRP2 render pass id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult
_vklite_end_render_pass(Drp2VkliteState* state, uint64_t pass_id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* pass = _vklite_find(state, pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_rendering_end(pass->commands);
    for (uint32_t i = 0; i < pass->color_target_count; i++)
    {
        Drp2VkliteObject* target = _vklite_find(state, pass->color_target_ids[i]);
        if (target == NULL || target->borrowed_frame_target || target->images == NULL)
            continue;
        if ((target->usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) == 0)
            continue;
        _vklite_transition_image(
            pass->commands, target, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    }
    if (!pass->borrowed_commands)
    {
        DvzDrp2ValidationResult result =
            _vklite_owned_commands_end_submit(pass->commands, command_index);
        if (!result.ok)
        {
            _vklite_destroy_object_slot(state, pass);
            return result;
        }
    }
    else if (pass->depth_images != NULL &&
             state->active_borrowed_command_buffer != VK_NULL_HANDLE &&
             _vklite_defer_destroy_object(state, pass, state->active_borrowed_command_buffer))
    {
        return _drp2_ok();
    }
    _vklite_destroy_object_slot(state, pass);
    return _drp2_ok();
}


/**
 * End and submit a vklite compute pass.
 *
 * @param state vklite runtime state
 * @param pass_id DRP2 compute pass id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult
_vklite_end_compute_pass(Drp2VkliteState* state, uint64_t pass_id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* pass = _vklite_find(state, pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(pass->commands, command_index);
    if (!result.ok)
    {
        _vklite_destroy_object_slot(state, pass);
        return result;
    }
    _vklite_destroy_object_slot(state, pass);
    return _drp2_ok();
}



#endif
