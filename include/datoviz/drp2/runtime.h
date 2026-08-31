/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime semantic validation                                                            */
/*************************************************************************************************/
/* Advanced/unstable runtime execution and validation API. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2ValidationResult DvzDrp2ValidationResult;
typedef struct DvzDrp2RuntimeConfig DvzDrp2RuntimeConfig;
typedef struct DvzDrp2ExternalBufferDesc DvzDrp2ExternalBufferDesc;
typedef struct DvzDrp2ExternalBufferTimelineDesc DvzDrp2ExternalBufferTimelineDesc;
typedef struct DvzDevice DvzDevice;
typedef struct DvzStreamFrame DvzStreamFrame;
typedef struct DvzVma DvzVma;
typedef struct DvzBuffer DvzBuffer;
typedef struct DvzSemaphore DvzSemaphore;

struct DvzDrp2ValidationResult
{
    bool ok;
    DvzDrp2ValidationCode code;
    /* Index of the failing command. UINT32_MAX when validation succeeds or no command is blamed. */
    uint32_t command_index;
};


struct DvzDrp2RuntimeConfig
{
    uint32_t struct_size;
    uint32_t flags;
    DvzDevice* device;
    DvzVma* allocator;
    bool semantic_only;
};


struct DvzDrp2ExternalBufferDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzBuffer* buffer;
    uint64_t size;
    uint32_t usage;
};


struct DvzDrp2ExternalBufferTimelineDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzSemaphore* semaphore;
    uint64_t wait_value;
    uint64_t signal_value;
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
 * Return a default external-buffer descriptor.
 *
 * @return zeroed descriptor with a valid ABI prologue
 */
DVZ_EXPORT DvzDrp2ExternalBufferDesc dvz_drp2_external_buffer_desc(void);


/**
 * Return a default one-shot external-buffer timeline-handoff descriptor.
 *
 * @return zeroed descriptor with a valid ABI prologue
 */
DVZ_EXPORT DvzDrp2ExternalBufferTimelineDesc dvz_drp2_external_buffer_timeline_desc(void);



/**
 * Create a DRP2 runtime using the vklite backend boundary.
 *
 * The runtime copies the configuration but borrows its device and allocator. Both must remain live
 * until the runtime is destroyed. A configuration with both pointers NULL creates a semantic-only
 * runtime that validates streams without executing backend commands.
 *
 * @param cfg required runtime configuration
 * @return a newly allocated runtime, or NULL on invalid configuration or allocation failure
 */
DVZ_EXPORT DvzDrp2Runtime* dvz_drp2_runtime_vklite(const DvzDrp2RuntimeConfig* cfg);



/**
 * Return the borrowed configuration that was used to create a DRP2 runtime.
 *
 * @param runtime the runtime, or NULL
 * @return the runtime configuration, or zero-initialized fields when runtime is NULL
 */
DVZ_EXPORT DvzDrp2RuntimeConfig dvz_drp2_runtime_get_config(const DvzDrp2Runtime* runtime);



/**
 * Destroy a DRP2 runtime.
 *
 * Vklite-backed runtimes wait for submitted device work before releasing owned backend resources.
 *
 * @param runtime the runtime
 */
DVZ_EXPORT void dvz_drp2_runtime_destroy(DvzDrp2Runtime* runtime);



/**
 * Reset a DRP2 runtime to an empty semantic and backend state.
 *
 * This releases runtime-owned objects while keeping the runtime itself and its borrowed
 * device/allocator configuration alive for reuse. Vklite-backed runtimes wait for submitted device
 * work before releasing owned backend resources. Reset also recovers a runtime that rejected
 * further operations after a backend execution failure.
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
 * Arm one externally-written buffer handoff for the next BufferToTexture copy from this buffer.
 *
 * The runtime borrows `desc->semaphore`. A vklite-backed runtime waits for `wait_value` at the
 * transfer stage, acquires the source buffer for transfer reads, copies it, releases transfer-read
 * access, and signals `signal_value` from that same submission. A second arm is rejected until the
 * pending handoff is consumed. Semantic-only runtimes validate the one-shot state without requiring
 * a semaphore wrapper.
 *
 * @param runtime the runtime
 * @param buffer_id registered external buffer id
 * @param desc timeline handoff values and borrowed semaphore
 * @return true when the handoff was armed
 */
DVZ_EXPORT bool dvz_drp2_runtime_arm_external_buffer_timeline(
    DvzDrp2Runtime* runtime, uint64_t buffer_id,
    const DvzDrp2ExternalBufferTimelineDesc* desc);


/**
 * Return whether an external buffer has an armed handoff awaiting its next BufferToTexture copy.
 *
 * @param runtime the runtime
 * @param buffer_id registered external buffer id
 * @return true when one handoff is pending
 */
DVZ_EXPORT bool dvz_drp2_runtime_external_buffer_timeline_pending(
    const DvzDrp2Runtime* runtime, uint64_t buffer_id);



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
 * A backend execution failure may occur after earlier commands have changed backend state, so the
 * runtime rejects subsequent operations until dvz_drp2_runtime_reset() restores an empty,
 * synchronized state.
 *
 * @param runtime the runtime
 * @param stream the command stream
 * @return the semantic-validation result for semantic-only runtimes, otherwise the backend
 * execution result; `ok` is false when either stage fails
 */
DVZ_EXPORT DvzDrp2ValidationResult
dvz_drp2_runtime_execute(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream);


/**
 * Attach a borrowed stream frame as a runtime render target.
 *
 * The runtime retains the frame's borrowed color image, optional depth image, image views, and
 * command-buffer handles under `texture_id`. They must remain valid until this target is replaced,
 * the runtime is reset, or the runtime is destroyed. The command buffer must already be recording,
 * and the target must be attached again before each later execution that records into a new frame.
 * The runtime records into it but does not begin, end, reset, submit, or destroy it.
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
 * The destination frame must declare `DVZ_STREAM_FRAME_USAGE_COPY_DST`, and its command buffer must
 * already be recording. This call records into the borrowed command buffer without ending,
 * submitting, resetting, or destroying it.
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
 * Must be called after dvz_drp2_runtime_execute() has completed. The requested byte range must fit
 * in a live buffer created with `DVZ_DRP2_BUFFER_USAGE_MAP_READ`. A successful download whose
 * buffer id, offset, and size exactly match the oldest pending QueueSubmit readback acknowledges
 * and consumes that request; an ad-hoc or out-of-order download of another valid range does not
 * release a pending readback pin.
 *
 * @param runtime the vklite runtime
 * @param buffer_id the DRP2 buffer id used in the stream
 * @param offset byte offset within the buffer
 * @param size number of bytes to read
 * @param dst destination CPU buffer (caller-allocated, at least size bytes)
 * @return true when the live buffer and byte range are valid and the bytes were copied
 */
DVZ_EXPORT bool dvz_drp2_runtime_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* dst);

EXTERN_C_OFF
