/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan emission internals                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DRP2_ID_COLOR_TARGET 1
#define DRP2_ID_ENCODER 2
#define DRP2_ID_RENDER_PASS 3
#define DRP2_ID_COMMAND_BUFFER 4
#define DRP2_ID_SUBMISSION 5
#define DRP2_ID_COMPUTE_PASS 6
#define DRP2_ID_PIPELINE 10
#define DRP2_ID_COMPUTE_PIPELINE 30
#define DRP2_ID_RESOURCE_BASE 20
#define DRP2_ID_READBACK_BUFFER 12
#define DRP2_ID_BIND_GROUP_LAYOUT 100
#define DRP2_ID_BIND_GROUP 13
#define DRP2_ID_SAMPLER 200
#define DRP2_ID_VERTEX_SHADER 9000
#define DRP2_ID_FRAGMENT_SHADER 9001
#define DRP2_ID_COMPUTE_SHADER 9002
#define DRP2_MAX_FIXTURE_RESOURCES 64
#define DRP2_RUNTIME_TRANSIENT_ID_BASE 10000
#define DRP2_EMITTER_OBJECT_ID_BASE 5000



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResourceId ResourceId;
typedef struct ConverterState ConverterState;
typedef struct SceneRenderStateCache SceneRenderStateCache;

struct ResourceId
{
    char key[DVZ_SCENE_LABEL_SIZE];
    uint64_t id;
    char data_tag[DVZ_SCENE_LABEL_SIZE]; /* attribute name, e.g. "position", "color", "size" */
    uint64_t byte_size;                  /* total bytes uploaded to this buffer               */
    uint32_t usage;                      /* DRP2 buffer usage flags                           */
    uint32_t item_stride;                /* optional element stride (index buffers)          */
    uint32_t topology;                   /* primitive topology hint (UINT32_MAX = unset)      */
};



struct ConverterState
{
    uint32_t count;
    uint64_t next_id;
    uint64_t first_vertex_buffer_id;
    uint64_t first_texture_id;
    uint64_t first_compute_input_id;
    uint64_t first_compute_output_id;
    uint64_t compute_buffer_size;
    ResourceId resources[DRP2_MAX_FIXTURE_RESOURCES];
};



struct SceneRenderStateCache
{
    uint64_t pipeline_id;
    uint64_t bg_set0;
};



struct DvzFramePlanEmitter
{
    ConverterState resources;
    ConverterState objects;
    uint64_t next_transient_id;
    bool handshake_sent;

    /* MVP cache: APPLY slots are panel-specific, FIXED uses a shared identity slot. */
    char mvp_panel_ids[DVZ_SCENE_MAX_PANELS][DVZ_SCENE_LABEL_SIZE];
    DvzMVP mvp_cache[DVZ_SCENE_MAX_PANELS];
    uint32_t mvp_panel_count;
};
