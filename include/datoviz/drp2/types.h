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
#define DVZ_DRP2_MAX_COLOR_ATTACHMENTS 4

typedef struct DvzDrp2CommandStream DvzDrp2CommandStream;
typedef struct DvzDrp2Command DvzDrp2Command;
typedef struct DvzDrp2Runtime DvzDrp2Runtime;
typedef struct DvzDrp2BindGroupLayoutEntry DvzDrp2BindGroupLayoutEntry;
typedef struct DvzDrp2BindGroupEntry DvzDrp2BindGroupEntry;
typedef struct DvzDrp2ColorAttachment DvzDrp2ColorAttachment;
typedef struct DvzDrp2ColorTarget DvzDrp2ColorTarget;



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


struct DvzDrp2ColorAttachment
{
    uint64_t texture_id;
    uint64_t resolve_texture_id;
    uint32_t resolve_mode;
    bool clear;
    DvzDrp2AttachmentLoadOp load_op;
    DvzDrp2AttachmentStoreOp store_op;
    DvzDrp2AttachmentAccess access;
    float clear_color[4];
};


struct DvzDrp2ColorTarget
{
    uint32_t format;
    bool blend_enabled;
    uint32_t src_color_blend_factor;
    uint32_t dst_color_blend_factor;
    uint32_t color_blend_op;
    uint32_t src_alpha_blend_factor;
    uint32_t dst_alpha_blend_factor;
    uint32_t alpha_blend_op;
    uint32_t color_write_mask;
};
