/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime semantic validation                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if DVZ_DRP2_HAS_VKLITE
#include <volk.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_log.h"
#include "_overflow.h"
#include "_runtime.h"
#include "_stream.h"

#if DVZ_DRP2_HAS_VKLITE
#include "datoviz/stream/frame_stream.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/compute.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "datoviz/vklite/sync.h"
#endif



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_DRP2_HAS_VKLITE
#define DVZ_DRP2_HAS_VKLITE 0
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#if DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data);
#endif


#if DVZ_DRP2_HAS_VKLITE
/**
 * Ensure a runtime has a vklite backend state object ready for direct registration.
 *
 * @param runtime the runtime
 * @return whether the state object is available
 */
static bool _vklite_runtime_state_ensure(DvzDrp2Runtime* runtime)
{
    ANN(runtime);
    if (runtime->vklite_state != NULL)
        return true;
    runtime->vklite_state = (Drp2VkliteState*)dvz_calloc(1, sizeof(Drp2VkliteState));
    if (runtime->vklite_state == NULL)
        return false;
    runtime->vklite_state->runtime = runtime;
    return true;
}
#endif


/**
 * Return the smaller of two 32-bit unsigned integers.
 *
 * @param a the first value
 * @param b the second value
 * @return the smaller value
 */
static uint32_t _min_u32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}


#if DVZ_DRP2_HAS_VKLITE
/**
 * Wait for submitted vklite work before releasing runtime-owned backend resources.
 *
 * @param runtime the runtime
 */
static void _runtime_wait_backend_idle(DvzDrp2Runtime* runtime)
{
    ANN(runtime);
    if (!runtime->semantic_only && runtime->device != NULL)
        dvz_device_wait(runtime->device);
}
#endif


/**
 * Return whether a borrowed stream frame can be exposed as a render target.
 *
 * @param texture_id the DRP2 texture id assigned to the frame
 * @param frame the borrowed stream frame
 * @return whether the frame has the required target handles and extent
 */
#if DVZ_DRP2_HAS_VKLITE
bool _drp2_frame_target_valid(uint64_t texture_id, const DvzStreamFrame* frame)
{
    if (texture_id == 0 || frame == NULL)
        return false;
    if (frame->image == VK_NULL_HANDLE || frame->image_view == VK_NULL_HANDLE ||
        frame->command_buffer == VK_NULL_HANDLE)
        return false;
    if (!frame->image_borrowed || !frame->image_view_borrowed || !frame->command_buffer_borrowed)
        return false;
    if (!frame->command_buffer_recording)
        return false;
    if (frame->color_format == VK_FORMAT_UNDEFINED)
        return false;
    if (frame->image_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        return false;
    if ((frame->usage & DVZ_STREAM_FRAME_USAGE_RENDER_TARGET) == 0)
        return false;
    return frame->extent.width != 0 && frame->extent.height != 0;
}
#endif







/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_RUNTIME_CONFIG_KNOWN_FLAGS 0u
#define DVZ_DRP2_EXTERNAL_BUFFER_DESC_KNOWN_FLAGS 0u



static bool _drp2_runtime_config_validate(const DvzDrp2RuntimeConfig* cfg)
{
    if (cfg == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(cfg, DvzDrp2RuntimeConfig, DVZ_DRP2_RUNTIME_CONFIG_KNOWN_FLAGS))
    {
        log_error("invalid DvzDrp2RuntimeConfig ABI prologue");
        return false;
    }
    return true;
}



static bool _drp2_external_buffer_desc_validate(const DvzDrp2ExternalBufferDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(
            desc, DvzDrp2ExternalBufferDesc, DVZ_DRP2_EXTERNAL_BUFFER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzDrp2ExternalBufferDesc ABI prologue");
        return false;
    }
    return true;
}



/**
 * Return a DRP2 runtime configuration for a vklite-backed runtime.
 *
 * @param device the borrowed Vulkan device wrapper
 * @param allocator the borrowed Vulkan allocator wrapper
 * @return the runtime configuration
 */
DvzDrp2RuntimeConfig dvz_drp2_runtime_vklite_config(DvzDevice* device, DvzVma* allocator)
{
    DvzDrp2RuntimeConfig cfg = {DVZ_STRUCT_INIT_FIELDS(DvzDrp2RuntimeConfig)};
    cfg.device = device;
    cfg.allocator = allocator;
    return cfg;
}



DvzDrp2ExternalBufferDesc dvz_drp2_external_buffer_desc(void)
{
    return (DvzDrp2ExternalBufferDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
    };
}



/**
 * Create a DRP2 runtime using the vklite backend boundary.
 *
 * @param cfg the runtime configuration
 * @return the runtime, or NULL on invalid configuration
 */
DvzDrp2Runtime* dvz_drp2_runtime_vklite(const DvzDrp2RuntimeConfig* cfg)
{
    if (!_drp2_runtime_config_validate(cfg))
        return NULL;
#if !DVZ_DRP2_HAS_VKLITE
    if (!cfg->semantic_only)
        return NULL;
#endif
    if (!cfg->semantic_only && (cfg->device == NULL || cfg->allocator == NULL))
        return NULL;

    DvzDrp2Runtime* runtime = (DvzDrp2Runtime*)dvz_calloc(1, sizeof(DvzDrp2Runtime));
    ANN(runtime);
    runtime->device = cfg->device;
    runtime->allocator = cfg->allocator;
    runtime->semantic_only = cfg->semantic_only;
    return runtime;
}



/**
 * Return the borrowed configuration that was used to create a DRP2 runtime.
 *
 * @param runtime the runtime
 * @return the runtime configuration, or zero-initialized fields when runtime is NULL
 */
DvzDrp2RuntimeConfig dvz_drp2_runtime_get_config(const DvzDrp2Runtime* runtime)
{
    DvzDrp2RuntimeConfig cfg = {DVZ_STRUCT_INIT_FIELDS(DvzDrp2RuntimeConfig)};
    if (runtime == NULL)
        return cfg;
    cfg.device = runtime->device;
    cfg.allocator = runtime->allocator;
    cfg.semantic_only = runtime->semantic_only;
    return cfg;
}



/**
 * Destroy a DRP2 runtime.
 *
 * @param runtime the runtime
 */
void dvz_drp2_runtime_destroy(DvzDrp2Runtime* runtime)
{
    if (runtime == NULL)
        return;
#if DVZ_DRP2_HAS_VKLITE
    _runtime_wait_backend_idle(runtime);
#endif
    _drp2_runtime_state_cleanup(runtime->semantic_state);
    dvz_free(runtime->semantic_state);
#if DVZ_DRP2_HAS_VKLITE
    _vklite_state_cleanup(runtime->vklite_state);
    dvz_free(runtime->vklite_state);
#endif
    dvz_free(runtime);
}



/**
 * Reset a DRP2 runtime to its empty reusable state.
 *
 * @param runtime the runtime
 */
void dvz_drp2_runtime_reset(DvzDrp2Runtime* runtime)
{
    if (runtime == NULL)
        return;

#if DVZ_DRP2_HAS_VKLITE
    _runtime_wait_backend_idle(runtime);
#endif

    if (runtime->semantic_state != NULL)
    {
        _drp2_runtime_state_cleanup(runtime->semantic_state);
        dvz_free(runtime->semantic_state);
        runtime->semantic_state = NULL;
    }

#if DVZ_DRP2_HAS_VKLITE
    if (runtime->vklite_state != NULL)
    {
        _vklite_state_cleanup(runtime->vklite_state);
        dvz_free(runtime->vklite_state);
        runtime->vklite_state = NULL;
    }
#endif
}


/**
 * Register a runtime-provided buffer under a DRP2 buffer id.
 *
 * @param runtime the runtime
 * @param buffer_id the DRP2 buffer id
 * @param desc the external buffer descriptor
 * @return true on success
 */
bool dvz_drp2_runtime_register_external_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, const DvzDrp2ExternalBufferDesc* desc)
{
    if (!_drp2_external_buffer_desc_validate(desc))
        return false;
    if (runtime == NULL || buffer_id == 0 || desc->size == 0 ||
        desc->usage == DVZ_DRP2_BUFFER_USAGE_NONE)
    {
        return false;
    }
    if (!_drp2_runtime_state_ensure(runtime))
        return false;

    Drp2Object* existing = _drp2_find_any_object(runtime->semantic_state, buffer_id);
    if (existing != NULL && !existing->destroyed)
        return false;

#if DVZ_DRP2_HAS_VKLITE
    if (!runtime->semantic_only)
    {
        if (desc->buffer == NULL)
            return false;
        if (desc->size > dvz_buffer_size_value(desc->buffer))
            return false;
        if (!_vklite_runtime_state_ensure(runtime))
            return false;
        if (_vklite_find(runtime->vklite_state, buffer_id) != NULL)
            return false;
    }
#else
    if (!runtime->semantic_only)
        return false;
#endif

    Drp2Object* semantic = _drp2_add_object(runtime->semantic_state, buffer_id, DRP2_OBJECT_BUFFER);
    if (semantic == NULL)
        return false;
    semantic->size = desc->size;
    semantic->usage = desc->usage;

#if DVZ_DRP2_HAS_VKLITE
    if (!runtime->semantic_only)
    {
        Drp2VkliteObject* object = _vklite_add(
            runtime->vklite_state, buffer_id, DRP2_OBJECT_BUFFER);
        if (object == NULL)
        {
            semantic->destroyed = true;
            return false;
        }
        object->buffer = desc->buffer;
        object->borrowed_buffer = true;
    }
#endif
    return true;
}


#if DVZ_DRP2_HAS_VKLITE
/**
 * Download bytes from a live vklite buffer owned by a DRP2 runtime.
 *
 * @param runtime the DRP2 runtime
 * @param buffer_id the DRP2 buffer id
 * @param offset the byte offset
 * @param size the byte count to download
 * @param data the output buffer
 * @return true when the buffer exists and the download was requested
 */
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data)
{
    if (runtime == NULL || runtime->vklite_state == NULL || data == NULL || size == 0)
        return false;

    Drp2VkliteObject* object = _vklite_find(runtime->vklite_state, buffer_id);
    if (object == NULL || object->buffer == NULL)
        return false;
    if (runtime->semantic_state == NULL)
        return false;

    Drp2Object* semantic = _drp2_find_any_object(runtime->semantic_state, buffer_id);
    if (semantic == NULL || semantic->kind != DRP2_OBJECT_BUFFER)
        return false;
    if (_drp2_range_overflows(offset, size, semantic->size))
    {
        log_error(
            "runtime buffer download [%" PRIu64 ", %" PRIu64 ") exceeds buffer %" PRIu64
            " size %" PRIu64,
            offset, offset + size, buffer_id, semantic->size);
        return false;
    }

    dvz_buffer_download(object->buffer, offset, size, data);
    return true;
}
#endif



/**
 * Execute a command stream through a DRP2 runtime.
 *
 * @param runtime the runtime
 * @param stream the command stream
 * @return the validation result after semantic validation and backend execution
 */
DvzDrp2ValidationResult
dvz_drp2_runtime_execute(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream)
{
    if (runtime == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);
    if (stream == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    Drp2RuntimeState next_state = {0};
    DvzDrp2ValidationResult result = _drp2_runtime_validate_stream(runtime, stream, &next_state);
    if (!result.ok)
    {
        _drp2_runtime_state_cleanup(&next_state);
        return result;
    }

    if (!_drp2_runtime_state_ensure(runtime))
    {
        _drp2_runtime_state_cleanup(&next_state);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
    }

    if (runtime->semantic_only)
    {
        if (!_drp2_runtime_state_commit(runtime, &next_state))
        {
            _drp2_runtime_state_cleanup(&next_state);
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
        }
        return result;
    }

#if DVZ_DRP2_HAS_VKLITE
    DvzDrp2ValidationResult backend_result = _vklite_execute(runtime, stream);
    if (!backend_result.ok)
    {
        _drp2_runtime_state_cleanup(&next_state);
        return backend_result;
    }
    if (!_drp2_runtime_state_commit(runtime, &next_state))
    {
        _drp2_runtime_state_cleanup(&next_state);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
    }
    return backend_result;
#else
    _drp2_runtime_state_cleanup(&next_state);
    return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
#endif
}


/**
 * Attach a borrowed stream frame as a runtime render target.
 *
 * @param runtime the runtime
 * @param texture_id the DRP2 texture id to expose for render passes
 * @param frame the borrowed stream frame whose command buffer is currently recording
 * @return whether the frame target was attached
 */
bool dvz_drp2_runtime_attach_frame_target(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame)
{
#if DVZ_DRP2_HAS_VKLITE
    if (runtime == NULL || !_drp2_frame_target_valid(texture_id, frame))
        return false;

    if (runtime->semantic_state == NULL)
    {
        runtime->semantic_state = (Drp2RuntimeState*)dvz_calloc(1, sizeof(Drp2RuntimeState));
        if (runtime->semantic_state == NULL)
            return false;
    }

    Drp2Object* object = _drp2_find_any_object(runtime->semantic_state, texture_id);
    if (object != NULL && object->kind != DRP2_OBJECT_TEXTURE)
        return false;
    if (object == NULL && !_drp2_runtime_state_ensure_capacity(runtime->semantic_state))
        return false;

    if (!runtime->semantic_only)
    {
#if DVZ_DRP2_HAS_VKLITE
        if (!_vklite_attach_frame_target(runtime, texture_id, frame))
            return false;
#else
        return false;
#endif
    }

    object = _drp2_find_any_object(runtime->semantic_state, texture_id);
    if (object == NULL)
        object = _drp2_add_object(runtime->semantic_state, texture_id, DRP2_OBJECT_TEXTURE);
    if (object == NULL || object->kind != DRP2_OBJECT_TEXTURE)
        return false;

    object->destroyed = false;
    object->width = frame->extent.width;
    object->height = frame->extent.height;
    object->depth = 1;
    object->format = (uint32_t)frame->color_format;
    object->sample_count = 1;
    object->usage = DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    return true;
#else
    (void)runtime;
    (void)texture_id;
    (void)frame;
    return false;
#endif
}


/**
 * Record a copy from a runtime-owned texture into a borrowed stream frame.
 *
 * @param runtime the runtime
 * @param texture_id the DRP2 texture id to copy from
 * @param frame the borrowed stream frame whose command buffer is currently recording
 * @return whether the copy commands were recorded
 */
bool dvz_drp2_runtime_copy_texture_to_frame(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame)
{
#if DVZ_DRP2_HAS_VKLITE
    if (runtime == NULL || runtime->vklite_state == NULL || frame == NULL)
        return false;
    if (!_drp2_frame_target_valid(texture_id, frame))
        return false;
    if ((frame->usage & DVZ_STREAM_FRAME_USAGE_COPY_DST) == 0)
        return false;

    Drp2VkliteObject* source = _vklite_find(runtime->vklite_state, texture_id);
    if (source == NULL || source->images == NULL)
        return false;

    uint32_t width = _min_u32(source->width, frame->extent.width);
    uint32_t height = _min_u32(source->height, frame->extent.height);
    if (width == 0 || height == 0)
        return false;

    DvzCommands* cmds =
        _vklite_borrowed_frame_commands_create(runtime->device, frame->command_buffer);
    if (cmds == NULL)
        return false;

    _vklite_transition_image_access(cmds, source, DRP2_TEXTURE_ACCESS_TRANSFER_READ);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* dst = dvz_barriers_image(&barriers, frame->image);
    ANN(dst);
    dvz_barrier_image_stage(
        dst, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    dvz_barrier_image_access(
        dst, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    dvz_barrier_image_layout(
        dst, frame->image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_cmd_barriers(cmds, &barriers);

    DvzImageCopy* copy = dvz_image_copy_create();
    if (copy == NULL)
    {
        _vklite_borrowed_frame_commands_free(cmds);
        return false;
    }
    dvz_cmd_copy_source(
        copy, dvz_image_handle(source->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_READ), 0, 0, 0, width,
        height, 1);
    dvz_cmd_copy_destination(
        copy, frame->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0);
    dvz_cmd_copy_image(cmds, copy);
    dvz_image_copy_free(copy);

    dvz_barriers(&barriers);
    dst = dvz_barriers_image(&barriers, frame->image);
    ANN(dst);
    dvz_barrier_image_stage(
        dst, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_image_access(
        dst, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, frame->image_layout);
    dvz_cmd_barriers(cmds, &barriers);

    _vklite_borrowed_frame_commands_free(cmds);
    return true;
#else
    (void)runtime;
    (void)texture_id;
    (void)frame;
    return false;
#endif
}



/*************************************************************************************************/
/*  Public GLSL compilation utility                                                              */
/*************************************************************************************************/

DVZ_EXPORT uint32_t* dvz_compile_glsl(const char* stage, const char* glsl, uint64_t* out_size)
{
    ANN(stage);
    ANN(glsl);
    ANN(out_size);
    *out_size = 0;
#if DVZ_DRP2_HAS_VKLITE
    uint32_t* spv = NULL;
    uint64_t spv_size = 0;
    if (!_vklite_compile_glsl(stage, glsl, &spv, &spv_size))
        return NULL;
    *out_size = spv_size;
    return spv;
#else
    (void)stage;
    (void)glsl;
    return NULL;
#endif
}


bool dvz_drp2_runtime_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* dst)
{
#if DVZ_DRP2_HAS_VKLITE
    return _dvz_drp2_runtime_vklite_download_buffer(runtime, buffer_id, offset, size, dst);
#else
    (void)runtime;
    (void)buffer_id;
    (void)offset;
    (void)size;
    (void)dst;
    return false;
#endif
}
