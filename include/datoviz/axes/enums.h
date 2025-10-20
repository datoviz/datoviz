/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Axis flags.
typedef enum
{
    DVZ_AXIS_FLAGS_NONE = 0x00,
    DVZ_AXIS_FLAGS_DARK = 0x01,
} DvzAxisFlags;



// Ref flags.
typedef enum
{
    DVZ_REF_FLAGS_NONE = 0x00,
    DVZ_REF_FLAGS_EQUAL = 0x01,
} DvzRefFlags;
