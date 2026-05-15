/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"
#include "datoviz/scene/enums.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a capability snapshot.
 *
 * @param snapshot the capability snapshot
 */
DVZ_EXPORT void dvz_capability_snapshot_default(DvzCapabilitySnapshot* snapshot);



/**
 * Copy a capability snapshot.
 *
 * @param dst the destination snapshot
 * @param src the source snapshot
 */
DVZ_EXPORT void
dvz_capability_snapshot_copy(DvzCapabilitySnapshot* dst, const DvzCapabilitySnapshot* src);



/**
 * Initialize a diagnostic report.
 *
 * @param report the diagnostic report
 */
DVZ_EXPORT void dvz_diagnostic_report_init(DvzDiagnosticReport* report);



/**
 * Add a diagnostic message.
 *
 * @param report the diagnostic report
 * @param message the diagnostic message
 * @return whether the message was added
 */
DVZ_EXPORT bool dvz_diagnostic_report_add(DvzDiagnosticReport* report, const char* message);



/**
 * Return a diagnostic count.
 *
 * @param report the diagnostic report
 * @return the number of diagnostic messages
 */
DVZ_EXPORT uint32_t dvz_diagnostic_report_count(const DvzDiagnosticReport* report);



/**
 * Return a diagnostic message.
 *
 * @param report the diagnostic report
 * @param index the diagnostic index
 * @return the diagnostic message, or NULL when index is out of bounds
 */
DVZ_EXPORT const char*
dvz_diagnostic_report_get(const DvzDiagnosticReport* report, uint32_t index);



/**
 * Create an empty FramePlan.
 *
 * @param figure_id the figure id
 * @param frame_index the frame index
 * @return the FramePlan
 */
DVZ_EXPORT DvzFramePlan* dvz_frame_plan(const char* figure_id, uint64_t frame_index);



/**
 * Destroy a FramePlan.
 *
 * @param plan the FramePlan
 */
DVZ_EXPORT void dvz_frame_plan_destroy(DvzFramePlan* plan);



/**
 * Return a FramePlan node count.
 *
 * @param plan the FramePlan
 * @return the node count
 */
DVZ_EXPORT uint32_t dvz_frame_plan_node_count(const DvzFramePlan* plan);



/**
 * Return a FramePlan node.
 *
 * @param plan the FramePlan
 * @param index the node index
 * @return the node, or NULL when index is out of bounds
 */
DVZ_EXPORT const DvzFramePlanNode*
dvz_frame_plan_node_get(const DvzFramePlan* plan, uint32_t index);



/**
 * Return a FramePlan node type.
 *
 * @param node the FramePlan node
 * @return the node type
 */
DVZ_EXPORT DvzFramePlanNodeType dvz_frame_plan_node_type(const DvzFramePlanNode* node);



/**
 * Append an upload node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @param byte_offset the byte offset
 * @param byte_size the byte size
 * @param data_tag the debug data tag
 * @return whether the node was appended
 */
DVZ_EXPORT bool dvz_frame_plan_upload(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset, uint64_t byte_size,
    const char* data_tag);



/**
 * Append an upload node with actual data to be encoded into the DRP2 stream.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @param byte_offset the byte offset
 * @param byte_size the byte size
 * @param data_tag the debug data tag
 * @param data pointer to the bytes to upload (must remain valid until emit time)
 * @return whether the node was appended
 */
DVZ_EXPORT bool dvz_frame_plan_upload_bytes(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset, uint64_t byte_size,
    const char* data_tag, const void* data);



/**
 * Tag the most recently appended upload node with a primitive topology hint.
 *
 * Used by visual families that pick topology at the visual level (`dvz_primitive`).
 * Pass `UINT32_MAX` to clear the hint.
 *
 * @param plan the FramePlan
 * @param topology the primitive topology (DvzPrimitiveTopology), or UINT32_MAX
 * @return whether the hint was applied (false if the most recent node is not an upload)
 */
DVZ_EXPORT bool dvz_frame_plan_upload_set_topology(DvzFramePlan* plan, uint32_t topology);



/**
 * Mark the most recently appended upload node as a 2D texture write of the given extent.
 *
 * `byte_size` on that node should equal `width * height * 4` (RGBA8). When `width` and
 * `height` are non-zero the converter routes the upload to a 2D texture (via
 * CreateTexture + WriteTexture) instead of a vertex buffer. Unless an allocation extent is set
 * separately, this write extent is also used as the texture allocation extent.
 *
 * @param plan the FramePlan
 * @param width written texture-region width in pixels
 * @param height written texture-region height in pixels
 * @return whether the hint was applied (false if the most recent node is not an upload)
 */
DVZ_EXPORT bool dvz_frame_plan_upload_set_texture_extent(
    DvzFramePlan* plan, uint32_t width, uint32_t height);


/**
 * Tag the most recently appended texture upload node with the full allocation extent.
 *
 * Use this when the write extent is a sub-region and the converter must know the complete texture
 * extent without relying on prior cached runtime state.
 *
 * @param plan the FramePlan
 * @param width full texture allocation width in pixels
 * @param height full texture allocation height in pixels
 * @return whether the allocation extent was applied
 */
DVZ_EXPORT bool dvz_frame_plan_upload_set_texture_allocation_extent(
    DvzFramePlan* plan, uint32_t width, uint32_t height);


/**
 * Tag the most recently appended texture upload node with a 2D sub-region origin.
 *
 * Use after `dvz_frame_plan_upload_set_texture_extent()`. The extent still names the upload
 * size, while this call sets the destination origin within the texture.
 *
 * @param plan the FramePlan
 * @param origin_x destination x offset in texels
 * @param origin_y destination y offset in texels
 * @return whether the origin was applied
 */
DVZ_EXPORT bool dvz_frame_plan_upload_set_texture_region(
    DvzFramePlan* plan, uint32_t origin_x, uint32_t origin_y);



/**
 * Append a compute node.
 *
 * @param plan the FramePlan
 * @param shader_key the shader key
 * @param x dispatch workgroup count in X
 * @param y dispatch workgroup count in Y
 * @param z dispatch workgroup count in Z
 * @return whether the node was appended
 */
DVZ_EXPORT bool dvz_frame_plan_compute(
    DvzFramePlan* plan, const char* shader_key, uint32_t x, uint32_t y, uint32_t z);



/**
 * Add a resource read to the most recent compute node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @return whether the resource was appended
 */
DVZ_EXPORT bool dvz_frame_plan_compute_read(DvzFramePlan* plan, const char* resource_id);



/**
 * Add a resource write to the most recent compute node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @return whether the resource was appended
 */
DVZ_EXPORT bool dvz_frame_plan_compute_write(DvzFramePlan* plan, const char* resource_id);



/**
 * Append a render node.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param picking whether the node renders picking output
 * @return whether the node was appended
 */
DVZ_EXPORT bool dvz_frame_plan_render(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking);



/**
 * Append a clear-only render node.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @return whether the node was appended
 */
DVZ_EXPORT bool
dvz_frame_plan_clear(DvzFramePlan* plan, const char* panel_id, const char* render_target_id);



/**
 * Add a visual to the most recent render node.
 *
 * @param plan the FramePlan
 * @param visual_id the visual id
 * @return whether the visual was appended
 */
DVZ_EXPORT bool dvz_frame_plan_render_visual(DvzFramePlan* plan, const char* visual_id);



/**
 * Append a copy node.
 *
 * @param plan the FramePlan
 * @param src_resource_id the source resource id
 * @param dst_resource_id the destination resource id
 * @param byte_size the copy size in bytes
 * @return whether the node was appended
 */
DVZ_EXPORT bool dvz_frame_plan_copy(
    DvzFramePlan* plan, const char* src_resource_id, const char* dst_resource_id,
    uint64_t byte_size);



/**
 * Append a readback node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @param request_id the request id
 * @return whether the node was appended
 */
DVZ_EXPORT bool
dvz_frame_plan_readback(DvzFramePlan* plan, const char* resource_id, const char* request_id);



/**
 * Serialize a FramePlan as deterministic debug JSON.
 *
 * @param plan the FramePlan
 * @return an owned NUL-terminated JSON string
 */
DVZ_EXPORT char* dvz_frame_plan_json(const DvzFramePlan* plan);



/**
 * Emit a DRP2 command stream from a FramePlan in fixture mode.
 *
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @return an owned DRP2 command stream, or NULL on failure
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_frame_plan_emit_drp2(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report);


/**
 * Return the default FramePlan-to-DRP2 emission configuration.
 *
 * @return the default emission configuration
 */
DVZ_EXPORT DvzFramePlanEmitConfig dvz_frame_plan_emit_config(void);


/**
 * Emit a DRP2 command stream from a FramePlan with explicit fixture options.
 *
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned DRP2 command stream, or NULL on failure
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_frame_plan_emit_drp2_ex(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg);


/**
 * Create a persistent FramePlan-to-DRP2 emitter for runtime-mode streams.
 *
 * @return the emitter
 */
DVZ_EXPORT DvzFramePlanEmitter* dvz_frame_plan_emitter(void);


/**
 * Destroy a persistent FramePlan-to-DRP2 emitter.
 *
 * @param emitter the emitter
 */
DVZ_EXPORT void dvz_frame_plan_emitter_destroy(DvzFramePlanEmitter* emitter);


/**
 * Emit a runtime-mode DRP2 command stream from a FramePlan.
 *
 * @param emitter the persistent emitter
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned DRP2 command stream, or NULL on failure
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_frame_plan_emitter_emit_drp2(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report, const DvzFramePlanEmitConfig* cfg);


/**
 * Look up the DRP2 object id assigned to an emitter-internal key.
 *
 * Returns the id previously assigned to `key`, or 0 if the key has not been allocated yet.
 *
 * @param emitter the persistent emitter
 * @param key the internal object key (e.g. "_rb", "_vs", "_pipe0")
 * @return the DRP2 id, or 0 if not found
 */
DVZ_EXPORT uint64_t
dvz_frame_plan_emitter_object_id(const DvzFramePlanEmitter* emitter, const char* key);



/**
 * Destroy a JSON string returned by dvz_frame_plan_json().
 *
 * @param json the JSON string
 */
DVZ_EXPORT void dvz_frame_plan_json_destroy(char* json);

EXTERN_C_OFF
