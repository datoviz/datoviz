/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan DRP2 packet emission                                                         */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/packet.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFramePlanPacketResult DvzFramePlanPacketResult;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_FRAME_PLAN_PACKET_STATUS_OK = 0,
    DVZ_FRAME_PLAN_PACKET_STATUS_EMIT_ERROR = 1,
    DVZ_FRAME_PLAN_PACKET_STATUS_ENCODE_ERROR = 2,
} DvzFramePlanPacketStatus;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit split setup/update/frame DRP2 binary packets from a FramePlan.
 *
 * This native boundary is the first step toward the break-compatible WebGPU/WASM runtime path. JSON
 * export remains separate fixture/debug output and is not used here.
 *
 * @param emitter the persistent FramePlan-to-DRP2 emitter
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned packet result; destroy with dvz_frame_plan_packet_result_destroy()
 */
DVZ_EXPORT DvzFramePlanPacketResult* dvz_frame_plan_emitter_emit_drp2_packets(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report, const DvzFramePlanEmitConfig* cfg);


/**
 * Destroy a split packet result.
 *
 * @param result the packet result
 */
DVZ_EXPORT void dvz_frame_plan_packet_result_destroy(DvzFramePlanPacketResult* result);


/**
 * Return the result status.
 *
 * @param result the packet result
 * @return the packet emission status
 */
DVZ_EXPORT DvzFramePlanPacketStatus
dvz_frame_plan_packet_result_status(const DvzFramePlanPacketResult* result);


/**
 * Return the resource version associated with the packet result.
 *
 * @param result the packet result
 * @return the retained resource version
 */
DVZ_EXPORT uint64_t dvz_frame_plan_packet_result_resource_version(
    const DvzFramePlanPacketResult* result);


/**
 * Return the frame index associated with the packet result.
 *
 * @param result the packet result
 * @return the frame index
 */
DVZ_EXPORT uint64_t dvz_frame_plan_packet_result_frame_index(
    const DvzFramePlanPacketResult* result);


/**
 * Return one split packet and its companion payload arena.
 *
 * Empty phases return true with NULL packet and zero sizes.
 *
 * @param result the packet result
 * @param kind setup, update, or frame
 * @param packet output borrowed packet pointer
 * @param packet_size output packet byte size
 * @param arena output borrowed payload arena pointer
 * @param arena_size output arena byte size
 * @return whether `kind` is valid and outputs were populated
 */
DVZ_EXPORT bool dvz_frame_plan_packet_result_get(
    const DvzFramePlanPacketResult* result, DvzDrp2PacketKind kind, const void** packet,
    uint64_t* packet_size, const void** arena, uint64_t* arena_size);

EXTERN_C_OFF
