/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 enums                                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_DRP2_COMMAND_NONE,
    DVZ_DRP2_COMMAND_HELLO_RENDERER,
    DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY,
    DVZ_DRP2_COMMAND_CREATE_BUFFER,
    DVZ_DRP2_COMMAND_CREATE_TEXTURE,
    DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE,
    DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE,
    DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE,
    DVZ_DRP2_COMMAND_WRITE_BUFFER,
    DVZ_DRP2_COMMAND_WRITE_TEXTURE,
    DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER,
    DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS,
    DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS,
    DVZ_DRP2_COMMAND_SET_PIPELINE,
    DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER,
    DVZ_DRP2_COMMAND_SET_INDEX_BUFFER,
    DVZ_DRP2_COMMAND_DRAW,
    DVZ_DRP2_COMMAND_DRAW_INDEXED,
    DVZ_DRP2_COMMAND_END_RENDER_PASS,
    DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS,
    DVZ_DRP2_COMMAND_END_COMPUTE_PASS,
    DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE,
    DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER,
    DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE,
    DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER,
    DVZ_DRP2_COMMAND_QUEUE_SUBMIT,
    DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY,
} DvzDrp2CommandType;



typedef enum
{
    DVZ_DRP2_BUFFER_USAGE_NONE = 0x0000,
    DVZ_DRP2_BUFFER_USAGE_COPY_SRC = 0x0001,
    DVZ_DRP2_BUFFER_USAGE_COPY_DST = 0x0002,
    DVZ_DRP2_BUFFER_USAGE_MAP_READ = 0x0004,
    DVZ_DRP2_BUFFER_USAGE_MAP_WRITE = 0x0008,
    DVZ_DRP2_BUFFER_USAGE_VERTEX = 0x0010,
    DVZ_DRP2_BUFFER_USAGE_INDEX = 0x0020,
    DVZ_DRP2_BUFFER_USAGE_UNIFORM = 0x0040,
    DVZ_DRP2_BUFFER_USAGE_STORAGE = 0x0080,
} DvzDrp2BufferUsageFlags;



typedef enum
{
    DVZ_DRP2_TEXTURE_USAGE_NONE = 0x0000,
    DVZ_DRP2_TEXTURE_USAGE_COPY_SRC = 0x0001,
    DVZ_DRP2_TEXTURE_USAGE_COPY_DST = 0x0002,
    DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING = 0x0004,
    DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING = 0x0008,
    DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT = 0x0010,
} DvzDrp2TextureUsageFlags;



typedef enum
{
    DVZ_DRP2_VALIDATION_OK,
    DVZ_DRP2_VALIDATION_INVALID_ARGUMENT,
    DVZ_DRP2_VALIDATION_INVALID_STATE,
    DVZ_DRP2_VALIDATION_OUT_OF_RANGE,
    DVZ_DRP2_VALIDATION_USAGE,
} DvzDrp2ValidationCode;
