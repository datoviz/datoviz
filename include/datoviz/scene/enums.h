/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene enums                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Panel link flags.
typedef enum
{
    DVZ_PANEL_LINK_FLAGS_NONE = 0x00,
    DVZ_PANEL_LINK_FLAGS_MODEL = 0x01,
    DVZ_PANEL_LINK_FLAGS_VIEW = 0x02,
    DVZ_PANEL_LINK_FLAGS_PROJECTION = 0x04,
} DvzPanelLinkFlags;



typedef enum
{
    DVZ_FRAME_PLAN_NODE_NONE,
    DVZ_FRAME_PLAN_NODE_UPLOAD,
    DVZ_FRAME_PLAN_NODE_COMPUTE,
    DVZ_FRAME_PLAN_NODE_RENDER,
    DVZ_FRAME_PLAN_NODE_CLEAR,
    DVZ_FRAME_PLAN_NODE_COPY,
    DVZ_FRAME_PLAN_NODE_READBACK,
} DvzFramePlanNodeType;



typedef enum
{
    DVZ_SCENE_SHADER_FORMAT_WGSL,
    DVZ_SCENE_SHADER_FORMAT_GLSL,
} DvzSceneShaderFormat;



// Primitive topology for the `primitive` visual family.
// Maps 1:1 to VK_PRIMITIVE_TOPOLOGY_* / WebGPU primitive topology.
typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
} DvzPrimitiveTopology;
