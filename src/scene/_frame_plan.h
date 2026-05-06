/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan internals                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY 32



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFramePlanNode
{
    DvzFramePlanNodeType type;
    union
    {
        struct
        {
            char resource_id[DVZ_SCENE_LABEL_SIZE];
            uint64_t byte_offset;
            uint64_t byte_size;
            char data_tag[DVZ_SCENE_LABEL_SIZE];
            const void* data; /* optional: if non-NULL, actual bytes to upload */
        } upload;
        struct
        {
            char shader_key[DVZ_SCENE_LABEL_SIZE];
            uint32_t dispatch[3];
            uint32_t read_count;
            char reads[DVZ_SCENE_MAX_NODE_RESOURCES][DVZ_SCENE_LABEL_SIZE];
            uint32_t write_count;
            char writes[DVZ_SCENE_MAX_NODE_RESOURCES][DVZ_SCENE_LABEL_SIZE];
        } compute;
        struct
        {
            char panel_id[DVZ_SCENE_LABEL_SIZE];
            char render_target_id[DVZ_SCENE_LABEL_SIZE];
            uint32_t visual_count;
            char visuals[DVZ_SCENE_MAX_RENDER_VISUALS][DVZ_SCENE_LABEL_SIZE];
            bool picking;
            DvzPanelDesc desc;
        } render;
        struct
        {
            char panel_id[DVZ_SCENE_LABEL_SIZE];
            char render_target_id[DVZ_SCENE_LABEL_SIZE];
            DvzPanelDesc desc;
        } clear;
        struct
        {
            char src_resource_id[DVZ_SCENE_LABEL_SIZE];
            char dst_resource_id[DVZ_SCENE_LABEL_SIZE];
            uint64_t byte_size;
        } copy;
        struct
        {
            char resource_id[DVZ_SCENE_LABEL_SIZE];
            char request_id[DVZ_SCENE_LABEL_SIZE];
        } readback;
    } u;
};



struct DvzFramePlan
{
    char figure_id[DVZ_SCENE_LABEL_SIZE];
    uint64_t frame_index;
    uint32_t capacity;
    uint32_t count;
    DvzFramePlanNode* nodes;
};



/*************************************************************************************************/
/*  Internal helpers                                                                            */
/*************************************************************************************************/

bool dvz_frame_plan_render_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc);

bool dvz_frame_plan_clear_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc);
