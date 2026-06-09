/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene frame artifact internals                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSceneFrameArtifact DvzSceneFrameArtifact;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK = 0,
    DVZ_SCENE_FRAME_ARTIFACT_STATUS_ENCODE_ERROR = 1,
} DvzSceneFrameArtifactStatus;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a frame artifact that owns one DRP2 command stream and encoded split packets.
 *
 * The artifact takes ownership of `stream` on every path. Destroy it with
 * `_scene_frame_artifact_destroy()`.
 *
 * @param stream the owned command stream
 * @param resource_version the retained resource version
 * @param frame_index the frame index
 * @return the owned frame artifact, or NULL
 */
DvzSceneFrameArtifact* _scene_frame_artifact(
    DvzDrp2CommandStream* stream, uint64_t resource_version, uint64_t frame_index);


/**
 * Emit a figure directly into an owned frame artifact.
 *
 * This helper centralizes the legacy stream emission and immediate artifact freeze step. The returned
 * artifact owns the stream snapshot and encoded packet arenas; scene mutation is legal after a
 * successful return because the artifact has already released the emitted stream owner.
 *
 * @param figure the figure to emit
 * @param caps the capability snapshot
 * @param report output diagnostic report
 * @param cfg the emission configuration
 * @param resource_version the retained resource version
 * @param frame_index the frame index
 * @return the owned frame artifact, or NULL
 */
DvzSceneFrameArtifact* _scene_emit_frame_artifact(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg, uint64_t resource_version, uint64_t frame_index);


/**
 * Freeze borrowed payload pointers in a DRP2 stream into owned stream memory.
 *
 * @param stream the stream to freeze
 * @return whether all payloads were frozen successfully
 */
bool _scene_freeze_stream_payloads(DvzDrp2CommandStream* stream);


/**
 * Destroy a frame artifact.
 *
 * @param artifact the frame artifact
 */
void _scene_frame_artifact_destroy(DvzSceneFrameArtifact* artifact);


/**
 * Return the artifact status.
 *
 * @param artifact the frame artifact
 * @return the artifact status
 */
DvzSceneFrameArtifactStatus _scene_frame_artifact_status(
    const DvzSceneFrameArtifact* artifact);


/**
 * Return the resource version associated with an artifact.
 *
 * @param artifact the frame artifact
 * @return the retained resource version
 */
uint64_t _scene_frame_artifact_resource_version(const DvzSceneFrameArtifact* artifact);


/**
 * Return the frame index associated with an artifact.
 *
 * @param artifact the frame artifact
 * @return the frame index
 */
uint64_t _scene_frame_artifact_frame_index(const DvzSceneFrameArtifact* artifact);


/**
 * Return the owned command stream snapshot.
 *
 * @param artifact the frame artifact
 * @return the owned command stream, or NULL
 */
DvzDrp2CommandStream* _scene_frame_artifact_stream(const DvzSceneFrameArtifact* artifact);


/**
 * Return one split packet and companion payload arena.
 *
 * Empty phases return true with NULL packet and zero sizes.
 *
 * @param artifact the frame artifact
 * @param kind setup, update, or frame
 * @param packet output borrowed packet pointer
 * @param packet_size output packet byte size
 * @param arena output borrowed payload arena pointer
 * @param arena_size output arena byte size
 * @return whether `kind` is valid and outputs were populated
 */
bool _scene_frame_artifact_get_packet(
    const DvzSceneFrameArtifact* artifact, DvzDrp2PacketKind kind, const void** packet,
    uint64_t* packet_size, const void** arena, uint64_t* arena_size);
