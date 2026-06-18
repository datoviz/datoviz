/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 binary packets                                                                          */
/*************************************************************************************************/

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
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_DRP2_PACKET_NONE = 0,
    DVZ_DRP2_PACKET_SETUP = 1,
    DVZ_DRP2_PACKET_UPDATE = 2,
    DVZ_DRP2_PACKET_FRAME = 3,
} DvzDrp2PacketKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2PacketInfo
{
    DvzDrp2PacketKind kind;
    uint32_t command_count;
    uint64_t command_bytes;
    uint64_t arena_size;
    uint64_t resource_version;
    uint64_t frame_index;
} DvzDrp2PacketInfo;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default packet phase for a command type.
 *
 * @param type the command type
 * @return the packet phase, or DVZ_DRP2_PACKET_NONE for unsupported/unknown commands
 */
DVZ_EXPORT DvzDrp2PacketKind dvz_drp2_packet_command_kind(DvzDrp2CommandType type);


/**
 * Encode only commands whose default phase matches `kind`.
 *
 * This is the native split-packet bridge used before browser execution is changed. Empty phases
 * return true with NULL packet and zero sizes.
 *
 * @param stream the command stream
 * @param kind the packet phase to select and write into the packet header
 * @param resource_version retained resource version associated with the packet
 * @param frame_index frame counter associated with the packet
 * @param packet output encoded packet bytes, or NULL when the phase is empty
 * @param packet_size output encoded packet byte size
 * @param arena output payload arena bytes, or NULL when empty
 * @param arena_size output payload arena byte size
 * @return whether the matching commands were encoded
 */
DVZ_EXPORT bool dvz_drp2_packet_encode_stream_phase(
    const DvzDrp2CommandStream* stream, DvzDrp2PacketKind kind, uint64_t resource_version,
    uint64_t frame_index, void** packet, uint64_t* packet_size, void** arena,
    uint64_t* arena_size);

/**
 * Encode a DRP2 command stream as a binary packet plus payload arena.
 *
 * The returned `packet` and `arena` buffers are owned by the caller and must be released with
 * `dvz_drp2_packet_destroy()`. JSON/base64-only payload commands are intentionally rejected; the
 * runtime packet path requires raw payload bytes.
 *
 * @param stream the command stream
 * @param kind the packet phase
 * @param resource_version retained resource version associated with the packet
 * @param frame_index frame counter associated with the packet
 * @param packet output encoded packet bytes, or NULL when empty
 * @param packet_size output encoded packet byte size
 * @param arena output payload arena bytes, or NULL when empty
 * @param arena_size output payload arena byte size
 * @return whether the stream was encoded
 */
DVZ_EXPORT bool dvz_drp2_packet_encode_stream(
    const DvzDrp2CommandStream* stream, DvzDrp2PacketKind kind, uint64_t resource_version,
    uint64_t frame_index, void** packet, uint64_t* packet_size, void** arena,
    uint64_t* arena_size);


/**
 * Decode a binary packet plus payload arena into a DRP2 command stream.
 *
 * The returned command stream is owned by the caller and must be destroyed with
 * `dvz_drp2_stream_destroy()`. The payload arena must remain alive while the decoded stream is used.
 *
 * @param packet encoded packet bytes
 * @param packet_size encoded packet byte size
 * @param arena payload arena bytes, or NULL when empty
 * @param arena_size payload arena byte size
 * @param info optional output packet metadata
 * @return a decoded command stream, or NULL when validation fails
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_drp2_packet_decode_stream(
    const void* packet, uint64_t packet_size, const void* arena, uint64_t arena_size,
    DvzDrp2PacketInfo* info);


/**
 * Destroy a buffer returned by `dvz_drp2_packet_encode_stream()`.
 *
 * @param ptr encoded packet or arena pointer
 */
DVZ_EXPORT void dvz_drp2_packet_destroy(void* ptr);

EXTERN_C_OFF
