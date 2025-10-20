/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU data management enums                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Tex dims.
typedef enum
{
    DVZ_TEX_NONE,
    DVZ_TEX_1D,
    DVZ_TEX_2D,
    DVZ_TEX_3D,
} DvzTexDims;



// Dat flags.
typedef enum
{
    DVZ_DAT_FLAGS_NONE = 0x0000,               // default: shared, with staging, single copy
    DVZ_DAT_FLAGS_STANDALONE = 0x0100,         // (or shared)
    DVZ_DAT_FLAGS_MAPPABLE = 0x0200,           // (or non-mappable = need staging buffer)
    DVZ_DAT_FLAGS_DUP = 0x0400,                // (or single copy)
    DVZ_DAT_FLAGS_KEEP_ON_RESIZE = 0x1000,     // (or loose the data when resizing the buffer)
    DVZ_DAT_FLAGS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzDatFlags;



// Dat usage.
// TODO: not implemented yet, going from these flags to DvzDatFlags
typedef enum
{
    DVZ_DAT_USAGE_FREQUENT_NONE,
    DVZ_DAT_USAGE_FREQUENT_UPLOAD = 0x0001,
    DVZ_DAT_USAGE_FREQUENT_DOWNLOAD = 0x0002,
    DVZ_DAT_USAGE_FREQUENT_RESIZE = 0x0004,
} DvzDatUsage;



// Dat upload flags.
typedef enum
{
    DVZ_UPLOAD_FLAGS_NOCOPY = 0x0800, // (avoid data copy/free)

} DvzUploadFlags;



// Tex flags.
typedef enum
{
    DVZ_TEX_FLAGS_NONE = 0x0000,               // default
    DVZ_TEX_FLAGS_PERSISTENT_STAGING = 0x2000, // (or recreate the staging buffer every time)
} DvzTexFlags;
