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
#define DVZ_DRP2_LABEL_SIZE 128



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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
            uint64_t id;
            uint32_t width;
            uint32_t height;
        } create_texture;
        struct
        {
            uint64_t id;
            char stage[DVZ_DRP2_LABEL_SIZE];
            char code[DVZ_DRP2_LABEL_SIZE];
        } create_shader_module;
        struct
        {
            uint64_t id;
            uint64_t vertex_shader_module_id;
            uint64_t fragment_shader_module_id;
            uint32_t vertex_buffer_slots;
        } create_render_pipeline;
        struct
        {
            uint64_t id;
            uint64_t compute_shader_module_id;
        } create_compute_pipeline;
        struct
        {
            uint64_t buffer_id;
            uint64_t offset;
            uint64_t size;
            char data_base64[DVZ_DRP2_LABEL_SIZE];
        } write_buffer;
        struct
        {
            uint64_t id;
        } begin_command_encoder;
        struct
        {
            uint64_t id;
            uint64_t encoder_id;
            uint64_t texture_id;
        } begin_render_pass;
        struct
        {
            uint64_t id;
            uint64_t encoder_id;
        } begin_compute_pass;
        struct
        {
            uint64_t pass_id;
            uint64_t pipeline_id;
        } set_pipeline;
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
};
