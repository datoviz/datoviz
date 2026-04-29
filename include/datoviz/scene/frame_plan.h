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
 * Destroy a JSON string returned by dvz_frame_plan_json().
 *
 * @param json the JSON string
 */
DVZ_EXPORT void dvz_frame_plan_json_destroy(char* json);

EXTERN_C_OFF
