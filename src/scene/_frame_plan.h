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
#define DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY 16
#define DVZ_FRAME_PLAN_INITIAL_GRAPH_PASS_CAPACITY 16
#define DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES 8
#define DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS 4



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum
{
    DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE = 0,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SHAPE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_FLAGS,
    DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_DISTANCE,
} DvzFramePlanResourceRole;



typedef enum
{
    DVZ_FRAME_PLAN_RESOURCE_KIND_NONE = 0,
    DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
    DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
    DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
} DvzFramePlanResourceKind;


typedef enum
{
    DVZ_FRAME_GRAPH_RESOURCE_NONE = 0,
    DVZ_FRAME_GRAPH_RESOURCE_BUFFER,
    DVZ_FRAME_GRAPH_RESOURCE_TEXTURE,
    DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET,
} DvzFrameGraphResourceKind;



typedef enum
{
    DVZ_FRAME_GRAPH_EXTENT_NONE = 0,
    DVZ_FRAME_GRAPH_EXTENT_FIGURE,
    DVZ_FRAME_GRAPH_EXTENT_PANEL,
    DVZ_FRAME_GRAPH_EXTENT_FIXED,
    DVZ_FRAME_GRAPH_EXTENT_RESOURCE_REF,
} DvzFrameGraphExtentKind;



typedef enum
{
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_NONE = 0,
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED,
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME,
    DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PERSISTENT,
} DvzFrameGraphResourceLifetime;



typedef enum
{
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_NONE = 0x00u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT = 0x01u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT = 0x02u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED = 0x04u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE = 0x08u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC = 0x10u,
    DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST = 0x20u,
} DvzFrameGraphResourceUsage;



typedef enum
{
    DVZ_FRAME_GRAPH_PASS_NONE = 0,
    DVZ_FRAME_GRAPH_PASS_RENDER,
    DVZ_FRAME_GRAPH_PASS_COMPUTE,
    DVZ_FRAME_GRAPH_PASS_COPY,
    DVZ_FRAME_GRAPH_PASS_READBACK,
    DVZ_FRAME_GRAPH_PASS_CLEAR,
} DvzFrameGraphPassKind;



typedef enum
{
    DVZ_FRAME_GRAPH_ACCESS_NONE = 0,
    DVZ_FRAME_GRAPH_ACCESS_SAMPLED,
    DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ,
    DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE,
    DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT,
    DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ,
    DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE,
    DVZ_FRAME_GRAPH_ACCESS_COPY_SRC,
    DVZ_FRAME_GRAPH_ACCESS_COPY_DST,
} DvzFrameGraphAccessUsage;



typedef enum
{
    DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR = 0,
    DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
    DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE,
} DvzFrameGraphAttachmentLoadOp;



typedef enum
{
    DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE = 0,
    DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE,
} DvzFrameGraphAttachmentStoreOp;



typedef enum
{
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE = 0,
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ,
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE,
    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE,
} DvzFrameGraphAttachmentAccess;



typedef struct DvzFrameGraphResource
{
    char id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphResourceKind kind;
    uint32_t format;
    DvzFrameGraphExtentKind extent_kind;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t sample_count;
    char extent_resource_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t usage_flags;
    DvzFrameGraphResourceLifetime lifetime;
} DvzFrameGraphResource;



typedef struct DvzFrameGraphAccess
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphAccessUsage usage;
} DvzFrameGraphAccess;



typedef struct DvzFrameGraphAttachment
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_resource_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t resolve_mode;
    DvzFrameGraphAttachmentLoadOp load_op;
    DvzFrameGraphAttachmentStoreOp store_op;
    DvzFrameGraphAttachmentAccess access;
    float clear_color[4];
    float clear_depth;
    uint32_t clear_stencil;
} DvzFrameGraphAttachment;



typedef struct DvzFrameGraphRect
{
    float x;
    float y;
    float width;
    float height;
} DvzFrameGraphRect;



typedef struct DvzFrameGraphPass
{
    char id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphPassKind kind;
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    bool has_viewport;
    DvzFrameGraphRect viewport;
    bool has_scissor;
    DvzFrameGraphRect scissor;
    uint32_t read_count;
    DvzFrameGraphAccess reads[DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES];
    uint32_t write_count;
    DvzFrameGraphAccess writes[DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES];
    uint32_t color_attachment_count;
    DvzFrameGraphAttachment color_attachments[DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS];
    bool has_depth_attachment;
    DvzFrameGraphAttachment depth_attachment;
    bool has_stencil_attachment;
    DvzFrameGraphAttachment stencil_attachment;
    bool alpha_to_coverage;
    char work_label[DVZ_SCENE_LABEL_SIZE];
} DvzFrameGraphPass;



typedef struct DvzFrameGraphDependency
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t producer_pass_index;
    uint32_t consumer_pass_index;
    char producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    char consumer_pass_id[DVZ_SCENE_LABEL_SIZE];
    DvzFrameGraphAccessUsage producer_usage;
    DvzFrameGraphAccessUsage consumer_usage;
} DvzFrameGraphDependency;



typedef struct DvzFramePlanUploadMeta
{
    bool has_metadata;
    DvzFramePlanResourceKind kind;
    DvzFramePlanResourceRole role;
    uint32_t visual_type;
    uint32_t visual_index;
    uint32_t buffer_index;
    uint64_t logical_item_count;
} DvzFramePlanUploadMeta;



typedef enum DvzFramePlanClipRect
{
    DVZ_FRAME_PLAN_CLIP_RECT_PANEL = 0,
    DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
} DvzFramePlanClipRect;



typedef struct DvzFramePlanVisualMeta
{
    bool has_metadata;
    DvzFramePlanClipRect clip_rect;
    uint32_t visual_type;
    uint32_t visual_index;
    uint32_t buffer_index;
    uint32_t topology;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t instance_count;
    DvzAlphaMode alpha_mode;
    bool depth_test_enabled;
    uint32_t depth_compare_op;
    bool depth_cue_enabled;
    bool point_style_enabled;
    bool has_volume;
    bool has_labels;
    bool image_pixel_space;
    uint32_t field_format;
    uint32_t field_semantic;
    uint32_t field_width;
    uint32_t field_height;
    uint32_t field_depth;
    uint32_t scale_index;
    bool volume_transfer_rgba;
    bool scene_occluder;
    bool scene_occluded;
    bool has_scene_occlusion;
    DvzSceneOcclusionDesc scene_occlusion;
    bool volume_occluded;
    bool has_volume_occlusion;
    DvzVolumeOcclusionDesc volume_occlusion;
    DvzLabelsState labels_state;
    DvzVolumeState volume_state;
    uint32_t glyph_atlas_encoding;
    float glyph_distance_range_px;
    bool has_draw_contract;
    char draw_contract_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t draw_depth_policy;
    uint32_t draw_blend_policy;
    uint32_t draw_shader_feature_mask;
    uint32_t draw_bind_group_layout_mask;
    char draw_volume_occlusion_resource_id[DVZ_SCENE_LABEL_SIZE];
    char draw_volume_occlusion_producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t draw_volume_occlusion_bind_set;
    uint32_t draw_volume_occlusion_bind_binding;
    char draw_scene_occlusion_resource_id[DVZ_SCENE_LABEL_SIZE];
    char draw_scene_occlusion_producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t draw_scene_occlusion_bind_set;
    uint32_t draw_scene_occlusion_bind_binding;
    char position_id[DVZ_SCENE_LABEL_SIZE];
    char position_start_id[DVZ_SCENE_LABEL_SIZE];
    char position_end_id[DVZ_SCENE_LABEL_SIZE];
    char color_id[DVZ_SCENE_LABEL_SIZE];
    char size_id[DVZ_SCENE_LABEL_SIZE];
    char angle_id[DVZ_SCENE_LABEL_SIZE];
    char bounds_id[DVZ_SCENE_LABEL_SIZE];
    char shape_id[DVZ_SCENE_LABEL_SIZE];
    char line_width_id[DVZ_SCENE_LABEL_SIZE];
    char texcoords_id[DVZ_SCENE_LABEL_SIZE];
    char instance_transform_id[DVZ_SCENE_LABEL_SIZE];
    char texture_id[DVZ_SCENE_LABEL_SIZE];
    char volume_texture_id[DVZ_SCENE_LABEL_SIZE];
    char volume_transfer_texture_id[DVZ_SCENE_LABEL_SIZE];
    char volume_label_lookup_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char index_id[DVZ_SCENE_LABEL_SIZE];
    char material_id[DVZ_SCENE_LABEL_SIZE];
    char selection_id[DVZ_SCENE_LABEL_SIZE];
    char path_flags_id[DVZ_SCENE_LABEL_SIZE];
    char path_distance_id[DVZ_SCENE_LABEL_SIZE];
} DvzFramePlanVisualMeta;



typedef struct DvzSceneViewportUniform
{
    float x;
    float y;
    float width;
    float height;
} DvzSceneViewportUniform;



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
            void* owned_data;
            bool external; /* register only; resource is provided by the live runtime */
            uint32_t buffer_usage; /* optional DRP2 buffer-usage mask (0 = vertex default) */
            uint32_t item_stride;  /* optional element stride, used by index buffers */
            /* Optional primitive topology hint, propagated to the converter resource entry.
             * UINT32_MAX = unspecified (default; used by POINT and other typed families). */
            uint32_t topology;
            /* Optional texture write extent. When `texture_width > 0`, the upload targets a
             * texture rather than a vertex buffer. Default 0 = vertex-buffer upload. */
            uint32_t texture_width;
            uint32_t texture_height;
            uint32_t texture_depth;
            uint32_t texture_format;
            uint32_t texture_bytes_per_texel;
            /* Optional full texture allocation extent. Defaults to the write extent when unset. */
            uint32_t texture_alloc_width;
            uint32_t texture_alloc_height;
            uint32_t texture_alloc_depth;
            uint32_t texture_origin_x;
            uint32_t texture_origin_y;
            uint32_t texture_origin_z;
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
            DvzFramePlanRenderPassRole pass_role;
            bool has_pass_contract;
            char pass_contract_id[DVZ_SCENE_LABEL_SIZE];
            DvzPanelDesc desc;
            bool has_plot_desc;
            DvzPanelDesc plot_desc;
            bool has_viewport;
            DvzSceneViewportUniform viewport;
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
            uint32_t src_attachment_index;
            uint32_t src_origin[3];
            uint32_t extent[3];
            uint32_t format;
            uint32_t bytes_per_texel;
            uint64_t bytes_per_row;
            uint32_t rows_per_image;
            uint64_t dst_offset;
            uint64_t byte_size;
            uint64_t request_id;
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
    uint32_t graph_resource_capacity;
    uint32_t graph_resource_count;
    DvzFrameGraphResource* graph_resources;
    uint32_t graph_pass_capacity;
    uint32_t graph_pass_count;
    DvzFrameGraphPass* graph_passes;
};



/*************************************************************************************************/
/*  Internal helpers                                                                            */
/*************************************************************************************************/

bool dvz_frame_plan_render_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc);

bool dvz_frame_plan_render_panel_role(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc, DvzFramePlanRenderPassRole pass_role);

bool dvz_frame_plan_clear_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc);

DvzFramePlanNode* dvz_frame_plan_last_render_node(DvzFramePlan* plan);

bool dvz_frame_plan_render_visual_metadata(
    DvzFramePlan* plan, const DvzFramePlanVisualMeta* metadata);

bool dvz_frame_plan_upload_metadata(DvzFramePlan* plan, const DvzFramePlanUploadMeta* metadata);

bool dvz_frame_plan_graph_resource(DvzFramePlan* plan, const DvzFrameGraphResource* resource);

uint32_t dvz_frame_plan_graph_resource_count(const DvzFramePlan* plan);

const DvzFrameGraphResource*
dvz_frame_plan_graph_resource_get(const DvzFramePlan* plan, uint32_t index);

bool dvz_frame_plan_graph_pass(DvzFramePlan* plan, const DvzFrameGraphPass* pass);

uint32_t dvz_frame_plan_graph_pass_count(const DvzFramePlan* plan);

const DvzFrameGraphPass* dvz_frame_plan_graph_pass_get(const DvzFramePlan* plan, uint32_t index);

uint32_t dvz_frame_plan_graph_dependency_count(const DvzFramePlan* plan);

bool dvz_frame_plan_graph_dependency_get(
    const DvzFramePlan* plan, uint32_t index, DvzFrameGraphDependency* out);

bool dvz_frame_graph_pass_read(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage);

bool dvz_frame_graph_pass_write(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage);

bool dvz_frame_graph_pass_color_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment);

bool dvz_frame_graph_pass_depth_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment);

bool dvz_frame_plan_graph_validate(const DvzFramePlan* plan, DvzDiagnosticReport* report);

char* dvz_frame_plan_graph_dump(const DvzFramePlan* plan);
