/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 types                                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/enums.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

#define DVZ_DRP2_MAX_BIND_GROUPS 4
#define DVZ_DRP2_MAX_BINDINGS 16

typedef struct DvzDrp2CommandStream DvzDrp2CommandStream;
typedef struct DvzDrp2Command DvzDrp2Command;
typedef struct DvzDrp2Runtime DvzDrp2Runtime;
typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;
typedef struct DvzDrp2BindGroupLayoutEntry DvzDrp2BindGroupLayoutEntry;
typedef struct DvzDrp2BindGroupEntry DvzDrp2BindGroupEntry;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDrp2BindGroupLayoutEntry
{
    uint32_t binding;
    DvzDrp2BindingType binding_type;
    uint32_t visibility;
    DvzDrp2BindingAccess access;
    bool has_dynamic_offset;
};



struct DvzDrp2BindGroupEntry
{
    uint32_t binding;
    DvzDrp2BindingType binding_type;
    DvzDrp2BindingResourceKind resource_kind;
    uint64_t resource_id;
    uint64_t offset;
    uint64_t size;
};
