/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/scene/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_LABEL_SIZE 128
#define DVZ_SCENE_MAX_NODE_RESOURCES 8
#define DVZ_SCENE_MAX_RENDER_VISUALS 8
#define DVZ_SCENE_MAX_DIAGNOSTICS 16
#define DVZ_SCENE_DIAGNOSTIC_SIZE 256



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCapabilitySnapshot DvzCapabilitySnapshot;
typedef struct DvzDiagnosticReport DvzDiagnosticReport;
typedef struct DvzFramePlanEmitConfig DvzFramePlanEmitConfig;
typedef struct DvzFramePlan DvzFramePlan;
typedef struct DvzFramePlanNode DvzFramePlanNode;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCapabilitySnapshot
{
    uint64_t max_buffer_size;
    uint32_t max_texture_dimension_2d;
    uint32_t max_bind_groups;
    uint32_t max_vertex_buffers;
};



struct DvzDiagnosticReport
{
    uint32_t count;
    char messages[DVZ_SCENE_MAX_DIAGNOSTICS][DVZ_SCENE_DIAGNOSTIC_SIZE];
};


struct DvzFramePlanEmitConfig
{
    DvzSceneShaderFormat shader_format;
};
