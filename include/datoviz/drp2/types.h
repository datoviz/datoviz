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
#include "datoviz/render_types.h"



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
typedef struct DvzDrp2TextureDesc DvzDrp2TextureDesc;
typedef struct DvzDrp2RenderPipelineDesc DvzDrp2RenderPipelineDesc;



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


struct DvzDrp2TextureDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint64_t id;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    DvzFormat format;
    uint32_t usage;
    uint32_t sample_count;
    DvzDrp2ColorRole color_role;
};


struct DvzDrp2RenderPipelineDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint64_t id;
    uint64_t vertex_shader_module_id;
    uint64_t fragment_shader_module_id;
    uint32_t vertex_buffer_slots;
    DvzPrimitiveTopology topology;
    uint32_t bind_group_layout_count;
    const uint64_t* bind_group_layout_ids;
    uint32_t binding_count;
    const uint32_t* binding_strides;
    const uint32_t* binding_step_modes;
    uint32_t attr_count;
    const uint32_t* attr_bindings;
    const uint32_t* attr_locations;
    const DvzFormat* attr_formats;
    const uint32_t* attr_offsets;
};
