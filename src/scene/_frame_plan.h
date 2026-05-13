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

typedef enum
{
    DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE = 0,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING,
} DvzFramePlanResourceRole;



typedef enum
{
    DVZ_FRAME_PLAN_RESOURCE_KIND_NONE = 0,
    DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
    DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
} DvzFramePlanResourceKind;



typedef struct DvzFramePlanUploadMeta
{
    bool has_metadata;
    DvzFramePlanResourceKind kind;
    DvzFramePlanResourceRole role;
    uint32_t visual_type;
    uint32_t visual_index;
    uint32_t buffer_index;
} DvzFramePlanUploadMeta;



typedef struct DvzFramePlanVisualMeta
{
    bool has_metadata;
    uint32_t visual_type;
    uint32_t visual_index;
    uint32_t buffer_index;
    uint32_t topology;
    char position_id[DVZ_SCENE_LABEL_SIZE];
    char color_id[DVZ_SCENE_LABEL_SIZE];
    char size_id[DVZ_SCENE_LABEL_SIZE];
    char texcoords_id[DVZ_SCENE_LABEL_SIZE];
    char texture_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char index_id[DVZ_SCENE_LABEL_SIZE];
    char shading_id[DVZ_SCENE_LABEL_SIZE];
} DvzFramePlanVisualMeta;



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
            uint32_t buffer_usage; /* optional DRP2 buffer-usage mask (0 = vertex default) */
            uint32_t item_stride;  /* optional element stride, used by index buffers */
            /* Optional primitive topology hint, propagated to the converter resource entry.
             * UINT32_MAX = unspecified (default; used by POINT and other typed families). */
            uint32_t topology;
            /* Optional 2D texture extent. When `texture_width > 0`, the upload targets a
             * 2D texture rather than a vertex buffer; `byte_size` is `width * height * 4`
             * (RGBA8). Default 0 = vertex-buffer upload. */
            uint32_t texture_width;
            uint32_t texture_height;
            uint32_t texture_origin_x;
            uint32_t texture_origin_y;
            DvzFramePlanUploadMeta metadata;
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
            DvzFramePlanVisualMeta visual_metadata[DVZ_SCENE_MAX_RENDER_VISUALS];
            bool picking;
            DvzPanelDesc desc;
            bool has_mvp;
            DvzMVP apply_mvp;  /* panel APPLY MVP from panzoom/arcball; identity MVP for FIXED computed by converter */
            DvzControllerMode controller_modes[DVZ_SCENE_MAX_RENDER_VISUALS];  /* parallel to visuals[] */
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

DvzFramePlanNode* dvz_frame_plan_last_render_node(DvzFramePlan* plan);

bool dvz_frame_plan_render_visual_metadata(
    DvzFramePlan* plan, const DvzFramePlanVisualMeta* metadata);

bool dvz_frame_plan_upload_metadata(DvzFramePlan* plan, const DvzFramePlanUploadMeta* metadata);
