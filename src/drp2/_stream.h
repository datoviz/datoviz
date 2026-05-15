/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command stream internals                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/drp2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_INITIAL_COMMAND_CAPACITY 64
#define DVZ_DRP2_LABEL_SIZE 512



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef void (*DvzDrp2StreamOwnerRelease)(void* owner);



struct DvzDrp2Command
{
    DvzDrp2CommandType type;
    union
    {
        struct
        {
            char name[DVZ_DRP2_LABEL_SIZE];
        } handshake;
        struct
        {
            uint64_t id;
            uint64_t size;
            uint32_t usage;
        } create_buffer;
        struct
        {
            uint64_t buffer_id;
        } destroy_buffer;
        struct
        {
            uint64_t id;
            uint32_t width;
            uint32_t height;
            uint32_t depth;  /* 1 for 2D, >1 for 3D */
            uint32_t usage;
        } create_texture;
        struct
        {
            uint64_t texture_id;
        } destroy_texture;
        struct
        {
            uint64_t        id;
            char            stage[DVZ_DRP2_LABEL_SIZE];
            char            format[DVZ_DRP2_LABEL_SIZE];
            char*           code;      /* heap-allocated; freed by stream_destroy */
            const unsigned char* spirv; /* in-process SPIR-V bytes: borrowed, not freed */
            uint64_t        spirv_size; /* byte count */
        } create_shader_module;
        struct
        {
            uint64_t shader_module_id;
        } destroy_shader_module;
        struct
        {
            uint64_t id;
            uint64_t vertex_shader_module_id;
            uint64_t fragment_shader_module_id;
            uint32_t vertex_buffer_slots;
            uint32_t bind_group_layout_count;
            uint64_t bind_group_layout_ids[DVZ_DRP2_MAX_BIND_GROUPS];
            bool has_depth_attachment;
            bool depth_write_enabled;
            uint32_t depth_compare_op;      /* VkCompareOp */
            uint32_t color_target_count;
            DvzDrp2ColorTarget color_targets[DVZ_DRP2_MAX_COLOR_ATTACHMENTS];
            /* Vertex input layout (binding_count==0 → no vertex attributes). */
            uint32_t topology;        /* VkPrimitiveTopology; 0 = TRIANGLE_LIST           */
            uint32_t binding_count;
            uint32_t binding_strides[16]; /* stride in bytes per binding slot             */
            uint32_t binding_step_modes[16]; /* DvzDrp2VertexStepMode per binding slot    */
            uint32_t attr_count;
            uint32_t attr_bindings[16];   /* which binding each attribute reads from      */
            uint32_t attr_locations[16];  /* layout(location=N)                           */
            uint32_t attr_formats[16];    /* VkFormat                                     */
            uint32_t attr_offsets[16];    /* byte offset within the binding stride        */
        } create_render_pipeline;
        struct
        {
            uint64_t render_pipeline_id;
        } destroy_render_pipeline;
        struct
        {
            uint64_t id;
            uint64_t compute_shader_module_id;
            uint32_t bind_group_layout_count;
            uint64_t bind_group_layout_ids[DVZ_DRP2_MAX_BIND_GROUPS];
        } create_compute_pipeline;
        struct
        {
            uint64_t compute_pipeline_id;
        } destroy_compute_pipeline;
        struct
        {
            uint64_t id;
        } create_sampler;
        struct
        {
            uint64_t id;
            uint32_t entry_count;
            DvzDrp2BindGroupLayoutEntry entries[DVZ_DRP2_MAX_BINDINGS];
        } create_bind_group_layout;
        struct
        {
            uint64_t id;
            uint64_t bind_group_layout_id;
            uint32_t entry_count;
            DvzDrp2BindGroupEntry entries[DVZ_DRP2_MAX_BINDINGS];
        } create_bind_group;
        struct
        {
            uint64_t bind_group_layout_id;
        } destroy_bind_group_layout;
        struct
        {
            uint64_t bind_group_id;
        } destroy_bind_group;
        struct
        {
            uint64_t buffer_id;
            uint64_t offset;
            uint64_t size;
            const void* data_raw;  /* in-process path: borrowed pointer, never freed */
            char* data_base64;     /* JSON path: heap-allocated, freed by stream_destroy */
        } write_buffer;
        struct
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
            const void* data_raw;  /* in-process path: borrowed pointer, never freed */
            char* data_base64;     /* JSON path: heap-allocated, freed by stream_destroy */
        } write_texture;
        struct
        {
            uint64_t id;
        } begin_command_encoder;
        struct
        {
            uint64_t id;
            uint64_t encoder_id;
            uint64_t texture_id;
            uint32_t color_attachment_count;
            DvzDrp2ColorAttachment color_attachments[DVZ_DRP2_MAX_COLOR_ATTACHMENTS];
            bool has_depth_attachment;
            float clear_depth;
            float clear_color[4]; /* RGBA clear values; {0,0,0,0} = transparent black */
            float viewport[4];    /* normalized x, y, width, height in [0,1] target space */
            bool clear;
        } begin_render_pass;
        struct
        {
            uint64_t id;
            uint64_t encoder_id;
        } begin_compute_pass;
        struct
        {
            uint64_t pass_id;
            float viewport[4]; /* normalized x, y, width, height in [0,1] target space */
        } set_viewport;
        struct
        {
            uint64_t pass_id;
            float scissor[4]; /* normalized x, y, width, height in [0,1] target space */
        } set_scissor;
        struct
        {
            uint64_t pass_id;
            uint64_t pipeline_id;
        } set_pipeline;
        struct
        {
            uint64_t pass_id;
            uint32_t slot;
            uint64_t bind_group_id;
            uint32_t dynamic_offset_count;
            uint64_t dynamic_offsets[DVZ_DRP2_MAX_BINDINGS];
        } set_bind_group;
        struct
        {
            uint64_t pass_id;
            uint32_t slot;
            uint64_t buffer_id;
            uint64_t offset;
        } set_vertex_buffer;
        struct
        {
            uint64_t pass_id;
            uint64_t buffer_id;
            char index_format[DVZ_DRP2_LABEL_SIZE];
            uint64_t offset;
        } set_index_buffer;
        struct
        {
            uint64_t pass_id;
            uint32_t vertex_count;
            uint32_t instance_count;
            uint32_t first_vertex;
            uint32_t first_instance;
        } draw;
        struct
        {
            uint64_t pass_id;
            uint32_t index_count;
            uint32_t instance_count;
            uint32_t first_index;
            int32_t base_vertex;
            uint32_t first_instance;
        } draw_indexed;
        struct
        {
            uint64_t pass_id;
        } end_render_pass;
        struct
        {
            uint64_t pass_id;
            uint32_t x;
            uint32_t y;
            uint32_t z;
        } dispatch;
        struct
        {
            uint64_t pass_id;
        } end_compute_pass;
        struct
        {
            uint64_t encoder_id;
            uint64_t src_buffer_id;
            uint64_t src_offset;
            uint64_t dst_buffer_id;
            uint64_t dst_offset;
            uint64_t size;
        } copy_buffer_to_buffer;
        struct
        {
            uint64_t encoder_id;
            uint64_t src_buffer_id;
            uint64_t src_offset;
            uint32_t bytes_per_row;
            uint32_t rows_per_image;
            uint64_t dst_texture_id;
            uint32_t dst_mip_level;
            uint32_t dst_origin_x;
            uint32_t dst_origin_y;
            uint32_t dst_origin_z;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
        } copy_buffer_to_texture;
        struct
        {
            uint64_t encoder_id;
            uint64_t src_texture_id;
            uint64_t dst_buffer_id;
            uint64_t dst_offset;
            uint32_t width;
            uint32_t height;
            uint32_t bytes_per_row;
            uint32_t rows_per_image;
        } copy_texture_to_buffer;
        struct
        {
            uint64_t encoder_id;
            uint64_t src_texture_id;
            uint32_t src_mip_level;
            uint32_t src_origin_x;
            uint32_t src_origin_y;
            uint32_t src_origin_z;
            uint64_t dst_texture_id;
            uint32_t dst_mip_level;
            uint32_t dst_origin_x;
            uint32_t dst_origin_y;
            uint32_t dst_origin_z;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
        } copy_texture_to_texture;
        struct
        {
            uint64_t encoder_id;
            uint64_t command_buffer_id;
        } finish_command_encoder;
        struct
        {
            uint64_t command_buffer_id;
            uint64_t submission_id;
            bool has_readback;
            uint64_t buffer_id;
            uint64_t offset;
            uint64_t size;
            char data_base64[DVZ_DRP2_LABEL_SIZE];
        } queue_submit;
    } u;
};



struct DvzDrp2CommandStream
{
    uint32_t capacity;
    uint32_t count;
    DvzDrp2Command* commands;
    void* owner;
    DvzDrp2StreamOwnerRelease owner_release;
    bool owner_released;
};
