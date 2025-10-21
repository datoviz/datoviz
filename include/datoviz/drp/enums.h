/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP enums                                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Recorder command type.
typedef enum
{
    DVZ_RECORDER_NONE,
    DVZ_RECORDER_BEGIN,
    DVZ_RECORDER_DRAW,
    DVZ_RECORDER_DRAW_INDEXED,
    DVZ_RECORDER_DRAW_INDIRECT,
    DVZ_RECORDER_DRAW_INDEXED_INDIRECT,
    DVZ_RECORDER_VIEWPORT,
    DVZ_RECORDER_PUSH,
    DVZ_RECORDER_END,
    DVZ_RECORDER_COUNT, // Number of different recorder types
} DvzRecorderCommandType;



// Request action.
typedef enum
{
    DVZ_REQUEST_ACTION_NONE,
    DVZ_REQUEST_ACTION_CREATE,
    DVZ_REQUEST_ACTION_DELETE,
    DVZ_REQUEST_ACTION_RESIZE,
    DVZ_REQUEST_ACTION_UPDATE,
    DVZ_REQUEST_ACTION_BIND,
    DVZ_REQUEST_ACTION_RECORD,
    DVZ_REQUEST_ACTION_UPLOAD,
    DVZ_REQUEST_ACTION_UPFILL,
    DVZ_REQUEST_ACTION_DOWNLOAD,
    DVZ_REQUEST_ACTION_SET,
    DVZ_REQUEST_ACTION_GET,
} DvzRequestAction;



// Request object.
typedef enum
{
    DVZ_REQUEST_OBJECT_NONE,
    // DVZ_REQUEST_OBJECT_BOARD = 100,
    DVZ_REQUEST_OBJECT_CANVAS = 101,
    DVZ_REQUEST_OBJECT_DAT,
    DVZ_REQUEST_OBJECT_TEX,
    DVZ_REQUEST_OBJECT_SAMPLER,
    DVZ_REQUEST_OBJECT_COMPUTE,
    DVZ_REQUEST_OBJECT_PRIMITIVE,
    DVZ_REQUEST_OBJECT_DEPTH,
    DVZ_REQUEST_OBJECT_BLEND,
    DVZ_REQUEST_OBJECT_MASK,
    DVZ_REQUEST_OBJECT_POLYGON,
    DVZ_REQUEST_OBJECT_CULL,
    DVZ_REQUEST_OBJECT_FRONT,
    DVZ_REQUEST_OBJECT_SHADER,
    DVZ_REQUEST_OBJECT_VERTEX,
    DVZ_REQUEST_OBJECT_VERTEX_ATTR,
    DVZ_REQUEST_OBJECT_SLOT,
    DVZ_REQUEST_OBJECT_PUSH,
    DVZ_REQUEST_OBJECT_SPECIALIZATION,
    DVZ_REQUEST_OBJECT_GRAPHICS,
    DVZ_REQUEST_OBJECT_INDEX,
    DVZ_REQUEST_OBJECT_BACKGROUND,

    DVZ_REQUEST_OBJECT_RECORD, // use recorder.h
} DvzRequestObject;



// Buffer type.
// NOTE: the enum index should correspond to the buffer index in the context->buffers container
typedef enum
{
    DVZ_BUFFER_TYPE_UNDEFINED,
    DVZ_BUFFER_TYPE_STAGING,  // 1
    DVZ_BUFFER_TYPE_VERTEX,   // 2
    DVZ_BUFFER_TYPE_INDEX,    // 3
    DVZ_BUFFER_TYPE_STORAGE,  // 4
    DVZ_BUFFER_TYPE_UNIFORM,  // 5
    DVZ_BUFFER_TYPE_INDIRECT, // 6
} DvzBufferType;

#define DVZ_BUFFER_TYPE_COUNT 6



// Blend type.
typedef enum
{
    DVZ_BLEND_DISABLE,
    DVZ_BLEND_STANDARD,
    DVZ_BLEND_DESTINATION,
    DVZ_BLEND_OIT,
} DvzBlendType;



// Depth test.
typedef enum
{
    DVZ_DEPTH_TEST_DISABLE,
    DVZ_DEPTH_TEST_ENABLE,
} DvzDepthTest;



// Graphics builtins
// TODO: remove?
typedef enum
{
    DVZ_GRAPHICS_NONE,
    DVZ_GRAPHICS_POINT,
    DVZ_GRAPHICS_TRIANGLE,
    DVZ_GRAPHICS_CUSTOM,
} DvzGraphicsType;



// Tex dims.
typedef enum
{
    DVZ_TEX_NONE,
    DVZ_TEX_1D,
    DVZ_TEX_2D,
    DVZ_TEX_3D,
} DvzTexDims;
