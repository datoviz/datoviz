/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 packet wire structs                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_stream.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PacketWriteBufferBody
{
    uint64_t buffer_id;
    uint64_t offset;
    uint64_t size;
} PacketWriteBufferBody;


typedef struct PacketWriteTextureBody
{
    uint64_t texture_id;
    uint32_t mip_level;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t origin_z;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t bytes_per_row;
    uint32_t rows_per_image;
} PacketWriteTextureBody;


typedef struct PacketShaderBody
{
    uint64_t id;
    char stage[DVZ_DRP2_LABEL_SIZE];
    char format[DVZ_DRP2_LABEL_SIZE];
    char builtin_family[DVZ_DRP2_LABEL_SIZE];
    char builtin_variant[DVZ_DRP2_LABEL_SIZE];
    uint32_t builtin_version;
    uint32_t payload_kind; /* 1=UTF-8 source, 2=SPIR-V bytes. */
    uint64_t payload_size;
} PacketShaderBody;
