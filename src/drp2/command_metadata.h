/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command metadata                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/drp2/packet.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2CommandMetadata
{
    DvzDrp2CommandType type;
    const char* name;
    DvzDrp2PacketKind packet_kind;
    uint64_t fixed_body_size;
} DvzDrp2CommandMetadata;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return static metadata for a DRP2 command type.
 *
 * @param type the command type
 * @return command metadata, or metadata for DVZ_DRP2_COMMAND_NONE for unsupported commands
 */
const DvzDrp2CommandMetadata* _dvz_drp2_command_metadata(DvzDrp2CommandType type);


/**
 * Return the schema/prose command discriminator for a DRP2 command type.
 *
 * @param type the command type
 * @return command discriminator, or "None" for unsupported commands
 */
const char* _dvz_drp2_command_name(DvzDrp2CommandType type);


/**
 * Return the default packet phase for a command type.
 *
 * @param type the command type
 * @return packet phase, or DVZ_DRP2_PACKET_NONE for unsupported commands
 */
DvzDrp2PacketKind _dvz_drp2_command_packet_kind(DvzDrp2CommandType type);


/**
 * Return the fixed binary packet body size for a command type.
 *
 * @param type the command type
 * @return fixed body size, or 0 for unsupported commands
 */
uint64_t _dvz_drp2_command_fixed_body_size(DvzDrp2CommandType type);
