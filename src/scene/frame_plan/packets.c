/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan DRP2 packet emission                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/drp2.h"
#include "datoviz/scene/frame_packets.h"
#include "emit.h"
#include "frame_plan.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PacketSpan
{
    void* packet;
    uint64_t packet_size;
    void* arena;
    uint64_t arena_size;
} PacketSpan;


struct DvzFramePlanPacketResult
{
    DvzFramePlanPacketStatus status;
    uint64_t resource_version;
    uint64_t frame_index;
    PacketSpan spans[4];
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static PacketSpan* _span(DvzFramePlanPacketResult* result, DvzDrp2PacketKind kind)
{
    if (result == NULL || kind < DVZ_DRP2_PACKET_SETUP || kind > DVZ_DRP2_PACKET_FRAME)
        return NULL;
    return &result->spans[(uint32_t)kind];
}



static const PacketSpan* _span_const(const DvzFramePlanPacketResult* result, DvzDrp2PacketKind kind)
{
    if (result == NULL || kind < DVZ_DRP2_PACKET_SETUP || kind > DVZ_DRP2_PACKET_FRAME)
        return NULL;
    return &result->spans[(uint32_t)kind];
}



static uint64_t _resource_version(const DvzFramePlanEmitter* emitter)
{
    if (emitter == NULL)
        return 0;
    return (uint64_t)emitter->resources.count + (uint64_t)emitter->objects.count;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit split setup/update/frame DRP2 binary packets from a FramePlan.
 */
DvzFramePlanPacketResult* dvz_frame_plan_emitter_emit_drp2_packets(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(plan);

    DvzFramePlanPacketResult* result =
        (DvzFramePlanPacketResult*)dvz_calloc(1, sizeof(DvzFramePlanPacketResult));
    if (result == NULL)
        return NULL;
    result->status = DVZ_FRAME_PLAN_PACKET_STATUS_OK;
    result->frame_index = plan->frame_index;

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, caps, report, cfg);
    if (stream == NULL)
    {
        result->status = DVZ_FRAME_PLAN_PACKET_STATUS_EMIT_ERROR;
        return result;
    }

    result->resource_version = _resource_version(emitter);
    const DvzDrp2PacketKind phases[3] = {
        DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_PACKET_UPDATE,
        DVZ_DRP2_PACKET_FRAME,
    };
    for (uint32_t i = 0; i < 3; i++)
    {
        PacketSpan* span = _span(result, phases[i]);
        ANN(span);
        if (!dvz_drp2_packet_encode_stream_phase(
                stream, phases[i], result->resource_version, result->frame_index, &span->packet,
                &span->packet_size, &span->arena, &span->arena_size))
        {
            result->status = DVZ_FRAME_PLAN_PACKET_STATUS_ENCODE_ERROR;
            break;
        }
    }

    dvz_drp2_stream_destroy(stream);
    return result;
}



/**
 * Destroy a split packet result.
 */
void dvz_frame_plan_packet_result_destroy(DvzFramePlanPacketResult* result)
{
    if (result == NULL)
        return;
    for (uint32_t i = DVZ_DRP2_PACKET_SETUP; i <= DVZ_DRP2_PACKET_FRAME; i++)
    {
        dvz_drp2_packet_destroy(result->spans[i].packet);
        dvz_drp2_packet_destroy(result->spans[i].arena);
    }
    dvz_free(result);
}



/**
 * Return the result status.
 */
DvzFramePlanPacketStatus dvz_frame_plan_packet_result_status(
    const DvzFramePlanPacketResult* result)
{
    if (result == NULL)
        return DVZ_FRAME_PLAN_PACKET_STATUS_EMIT_ERROR;
    return result->status;
}



/**
 * Return the resource version associated with the packet result.
 */
uint64_t dvz_frame_plan_packet_result_resource_version(const DvzFramePlanPacketResult* result)
{
    if (result == NULL)
        return 0;
    return result->resource_version;
}



/**
 * Return the frame index associated with the packet result.
 */
uint64_t dvz_frame_plan_packet_result_frame_index(const DvzFramePlanPacketResult* result)
{
    if (result == NULL)
        return 0;
    return result->frame_index;
}



/**
 * Return one split packet and its companion payload arena.
 */
bool dvz_frame_plan_packet_result_get(
    const DvzFramePlanPacketResult* result, DvzDrp2PacketKind kind, const void** packet,
    uint64_t* packet_size, const void** arena, uint64_t* arena_size)
{
    if (packet != NULL)
        *packet = NULL;
    if (packet_size != NULL)
        *packet_size = 0;
    if (arena != NULL)
        *arena = NULL;
    if (arena_size != NULL)
        *arena_size = 0;
    const PacketSpan* span = _span_const(result, kind);
    if (span == NULL)
        return false;
    if (packet != NULL)
        *packet = span->packet;
    if (packet_size != NULL)
        *packet_size = span->packet_size;
    if (arena != NULL)
        *arena = span->arena;
    if (arena_size != NULL)
        *arena_size = span->arena_size;
    return true;
}
