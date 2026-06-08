/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene packet artifact internals                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_overflow.h"
#include "datoviz/drp2.h"
#include "frame_artifact_internal.h"
#include "../../drp2/_stream.h"



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


struct DvzScenePacketArtifact
{
    DvzScenePacketArtifactStatus status;
    uint64_t resource_version;
    uint64_t frame_index;
    DvzDrp2CommandStream* stream;
    PacketSpan spans[4];
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static PacketSpan* _span(DvzScenePacketArtifact* artifact, DvzDrp2PacketKind kind)
{
    if (artifact == NULL || kind < DVZ_DRP2_PACKET_SETUP || kind > DVZ_DRP2_PACKET_FRAME)
        return NULL;
    return &artifact->spans[(uint32_t)kind];
}


static const PacketSpan* _span_const(
    const DvzScenePacketArtifact* artifact, DvzDrp2PacketKind kind)
{
    if (artifact == NULL || kind < DVZ_DRP2_PACKET_SETUP || kind > DVZ_DRP2_PACKET_FRAME)
        return NULL;
    return &artifact->spans[(uint32_t)kind];
}


static bool _write_texture_payload_size(const DvzDrp2Command* command, uint64_t* out_size)
{
    ANN(command);
    ANN(out_size);
    *out_size = 0;
    if (command->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        return false;

    uint64_t row_bytes = command->u.write_texture.bytes_per_row;
    uint64_t rows = command->u.write_texture.rows_per_image;
    uint64_t depth = command->u.write_texture.depth > 0 ? command->u.write_texture.depth : 1;
    if (row_bytes == 0 || rows == 0)
        return false;

    uint64_t image_bytes = 0;
    if (_dvz_mul_u64_overflows(row_bytes, rows, &image_bytes))
        return false;
    if (_dvz_mul_u64_overflows(image_bytes, depth, out_size))
        return false;
    return *out_size > 0;
}


static bool _freeze_stream_payloads(DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (
            command->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE ||
            command->u.write_texture.data_raw == NULL || command->u.write_texture.data_raw_owned)
        {
            continue;
        }

        uint64_t byte_size = 0;
        if (!_write_texture_payload_size(command, &byte_size) || byte_size > SIZE_MAX)
            return false;
        void* copy = dvz_malloc((size_t)byte_size);
        if (copy == NULL)
            return false;
        dvz_memcpy(copy, (size_t)byte_size, command->u.write_texture.data_raw, (size_t)byte_size);
        command->u.write_texture.data_raw = copy;
        command->u.write_texture.data_raw_owned = true;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a packet artifact that owns one DRP2 command stream and encoded split packets.
 */
DvzScenePacketArtifact* _scene_packet_artifact(
    DvzDrp2CommandStream* stream, uint64_t resource_version, uint64_t frame_index)
{
    if (stream == NULL)
        return NULL;

    DvzScenePacketArtifact* artifact =
        (DvzScenePacketArtifact*)dvz_calloc(1, sizeof(DvzScenePacketArtifact));
    if (artifact == NULL)
    {
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }

    artifact->status = DVZ_SCENE_PACKET_ARTIFACT_STATUS_OK;
    artifact->resource_version = resource_version;
    artifact->frame_index = frame_index;
    artifact->stream = stream;

    if (!_freeze_stream_payloads(stream))
    {
        artifact->status = DVZ_SCENE_PACKET_ARTIFACT_STATUS_ENCODE_ERROR;
        return artifact;
    }
    _dvz_drp2_stream_release_owner(stream);

    const DvzDrp2PacketKind phases[3] = {
        DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_PACKET_UPDATE,
        DVZ_DRP2_PACKET_FRAME,
    };
    for (uint32_t i = 0; i < 3; i++)
    {
        PacketSpan* span = _span(artifact, phases[i]);
        ANN(span);
        if (!dvz_drp2_packet_encode_stream_phase(
                stream, phases[i], resource_version, frame_index, &span->packet,
                &span->packet_size, &span->arena, &span->arena_size))
        {
            artifact->status = DVZ_SCENE_PACKET_ARTIFACT_STATUS_ENCODE_ERROR;
            break;
        }
    }

    return artifact;
}



/**
 * Destroy a packet artifact.
 */
void _scene_packet_artifact_destroy(DvzScenePacketArtifact* artifact)
{
    if (artifact == NULL)
        return;

    for (uint32_t i = DVZ_DRP2_PACKET_SETUP; i <= DVZ_DRP2_PACKET_FRAME; i++)
    {
        dvz_drp2_packet_destroy(artifact->spans[i].packet);
        dvz_drp2_packet_destroy(artifact->spans[i].arena);
    }
    if (artifact->stream != NULL)
        dvz_drp2_stream_destroy(artifact->stream);
    dvz_free(artifact);
}



/**
 * Return the artifact status.
 */
DvzScenePacketArtifactStatus _scene_packet_artifact_status(
    const DvzScenePacketArtifact* artifact)
{
    if (artifact == NULL)
        return DVZ_SCENE_PACKET_ARTIFACT_STATUS_ENCODE_ERROR;
    return artifact->status;
}



/**
 * Return the resource version associated with an artifact.
 */
uint64_t _scene_packet_artifact_resource_version(const DvzScenePacketArtifact* artifact)
{
    return artifact != NULL ? artifact->resource_version : 0;
}



/**
 * Return the frame index associated with an artifact.
 */
uint64_t _scene_packet_artifact_frame_index(const DvzScenePacketArtifact* artifact)
{
    return artifact != NULL ? artifact->frame_index : 0;
}



/**
 * Return the owned command stream snapshot.
 */
DvzDrp2CommandStream* _scene_packet_artifact_stream(const DvzScenePacketArtifact* artifact)
{
    return artifact != NULL ? artifact->stream : NULL;
}



/**
 * Return one split packet and companion payload arena.
 */
bool _scene_packet_artifact_get(
    const DvzScenePacketArtifact* artifact, DvzDrp2PacketKind kind, const void** packet,
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

    const PacketSpan* span = _span_const(artifact, kind);
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
