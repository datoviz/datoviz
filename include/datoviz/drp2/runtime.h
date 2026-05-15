/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime semantic validation                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"
#include "datoviz/stream/frame_stream.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2ValidationResult DvzDrp2ValidationResult;
typedef struct DvzDrp2RuntimeConfig DvzDrp2RuntimeConfig;

struct DvzDrp2ValidationResult
{
    bool ok;
    DvzDrp2ValidationCode code;
    uint32_t command_index;
};


struct DvzDrp2RuntimeConfig
{
    DvzDevice* device;
    DvzVma* allocator;
    bool semantic_only;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a DRP2 runtime configuration for a vklite-backed runtime.
 *
 * @param device the borrowed Vulkan device wrapper
 * @param allocator the borrowed Vulkan allocator wrapper
 * @return the runtime configuration
 */
DVZ_EXPORT DvzDrp2RuntimeConfig
dvz_drp2_runtime_vklite_config(DvzDevice* device, DvzVma* allocator);



/**
 * Create a DRP2 runtime using the vklite backend boundary.
 *
 * @param cfg the runtime configuration
 * @return the runtime, or NULL on invalid configuration
 */
DVZ_EXPORT DvzDrp2Runtime* dvz_drp2_runtime_vklite(const DvzDrp2RuntimeConfig* cfg);



/**
 * Return the borrowed configuration that was used to create a DRP2 runtime.
 *
 * @param runtime the runtime
 * @return the runtime configuration, or zero-initialized fields when runtime is NULL
 */
DVZ_EXPORT DvzDrp2RuntimeConfig dvz_drp2_runtime_config(const DvzDrp2Runtime* runtime);



/**
 * Destroy a DRP2 runtime.
 *
 * @param runtime the runtime
 */
DVZ_EXPORT void dvz_drp2_runtime_destroy(DvzDrp2Runtime* runtime);



/**
 * Reset a DRP2 runtime to an empty semantic and backend state.
 *
 * This releases runtime-owned objects while keeping the runtime itself and its
 * borrowed device/allocator configuration alive for reuse.
 *
 * @param runtime the runtime
 */
DVZ_EXPORT void dvz_drp2_runtime_reset(DvzDrp2Runtime* runtime);


/**
 * Register a runtime-provided buffer under a DRP2 buffer id.
 *
 * The registration is live-runtime state and is not portable DRP2 stream data. Semantic-only
 * runtimes use `size` and `usage` without requiring a backend buffer. Vklite-backed runtimes borrow
 * `desc->buffer`; the caller must keep it alive until the runtime is reset or destroyed.
 *
 * @param runtime the runtime
 * @param buffer_id the DRP2 buffer id to register
 * @param desc the external buffer descriptor
 * @return true on success
 */
DVZ_EXPORT bool dvz_drp2_runtime_register_external_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, const DvzDrp2ExternalBufferDesc* desc);



/**
 * Validate a DRP2 command stream against the backend-agnostic semantic rules.
 *
 * @param stream the command stream
 * @return the validation result
 */
DVZ_EXPORT DvzDrp2ValidationResult
dvz_drp2_validate_stream(const DvzDrp2CommandStream* stream);



/**
 * Execute a command stream through a DRP2 runtime skeleton.
 *
 * @param runtime the runtime
 * @param stream the command stream
 * @return the validation result before backend execution
 */
DVZ_EXPORT DvzDrp2ValidationResult
dvz_drp2_runtime_execute(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream);


/**
 * Attach a borrowed stream frame as a runtime render target.
 *
 * @param runtime the runtime
 * @param texture_id the DRP2 texture id to expose for render passes
 * @param frame the borrowed stream frame whose command buffer is currently recording
 * @return whether the frame target was attached
 */
DVZ_EXPORT bool dvz_drp2_runtime_attach_frame_target(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame);


/**
 * Record a copy from a runtime-owned texture into a borrowed stream frame.
 *
 * @param runtime the runtime
 * @param texture_id the DRP2 texture id to copy from
 * @param frame the borrowed stream frame whose command buffer is currently recording
 * @return whether the copy commands were recorded
 */
DVZ_EXPORT bool dvz_drp2_runtime_copy_texture_to_frame(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame);


/**
 * Download bytes from a DRP2 buffer into CPU memory.
 *
 * Must be called after dvz_drp2_runtime_execute() has completed.
 * The buffer must have been created with DVZ_DRP2_BUFFER_USAGE_COPY_DST usage.
 *
 * @param runtime the vklite runtime
 * @param buffer_id the DRP2 buffer id used in the stream
 * @param offset byte offset within the buffer
 * @param size number of bytes to read
 * @param dst destination CPU buffer (caller-allocated, at least size bytes)
 * @return true on success
 */
DVZ_EXPORT bool dvz_drp2_runtime_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* dst);

EXTERN_C_OFF
