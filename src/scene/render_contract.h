/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render contract helpers                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/types.h"
#include "_frame_plan.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_MAX_CONTRACT_ATTACHMENTS 16



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_PASS_KIND_RASTER = 0,
    DVZ_SCENE_PASS_KIND_FULLSCREEN,
    DVZ_SCENE_PASS_KIND_COMPUTE,
} DvzScenePassKind;


typedef enum
{
    DVZ_SCENE_ATTACHMENT_COLOR = 0,
    DVZ_SCENE_ATTACHMENT_DEPTH,
    DVZ_SCENE_ATTACHMENT_STORAGE,
    DVZ_SCENE_ATTACHMENT_SAMPLED,
} DvzSceneAttachmentRole;


typedef enum
{
    DVZ_SCENE_DEPTH_POLICY_NONE = 0,
    DVZ_SCENE_DEPTH_POLICY_TEST = 1u << 0,
    DVZ_SCENE_DEPTH_POLICY_WRITE = 1u << 1,
    DVZ_SCENE_DEPTH_POLICY_SAMPLE = 1u << 2,
} DvzSceneDepthPolicy;


typedef enum
{
    DVZ_SCENE_BLEND_POLICY_NONE = 0,
    DVZ_SCENE_BLEND_POLICY_OPAQUE,
    DVZ_SCENE_BLEND_POLICY_SOURCE_OVER,
    DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE,
    DVZ_SCENE_BLEND_POLICY_WBOIT,
    DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL,
} DvzSceneBlendPolicy;


typedef enum
{
    DVZ_SCENE_SHADER_FEATURE_NONE = 0,
    DVZ_SCENE_SHADER_FEATURE_SAMPLE_DEPTH = 1u << 0,
    DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION = 1u << 1,
    DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION = 1u << 2,
    DVZ_SCENE_SHADER_FEATURE_WRITE_VOLUME_OCCLUSION = 1u << 3,
    DVZ_SCENE_SHADER_FEATURE_WRITE_SCENE_OCCLUSION = 1u << 4,
} DvzSceneShaderFeature;


typedef enum
{
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_NONE = 0,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON = 1u << 0,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL = 1u << 1,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_IMAGE = 1u << 2,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_GLYPH = 1u << 3,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME = 1u << 4,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION = 1u << 5,
    DVZ_SCENE_BIND_GROUP_REQUIREMENT_LABELS = 1u << 6,
} DvzSceneBindGroupRequirement;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzSceneAttachmentUse
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    char producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    DvzSceneAttachmentRole role;
    uint32_t format;
    uint32_t sample_count;
    uint32_t requested_sample_count;
    uint32_t resolved_sample_count;
    DvzFrameGraphAttachmentLoadOp load_op;
    DvzFrameGraphAttachmentStoreOp store_op;
    DvzFrameGraphAttachmentAccess access;
    bool read;
    bool write;
    bool clear;
    bool preserve;
} DvzSceneAttachmentUse;


typedef struct DvzSceneBlendTargetContract
{
    uint32_t target_index;
    uint32_t format;
    bool blend_enabled;
    uint32_t src_color_blend_factor;
    uint32_t dst_color_blend_factor;
    uint32_t color_blend_op;
    uint32_t src_alpha_blend_factor;
    uint32_t dst_alpha_blend_factor;
    uint32_t alpha_blend_op;
    uint32_t color_write_mask;
} DvzSceneBlendTargetContract;


typedef struct DvzSceneDrawFacts
{
    uint32_t visual_type;
    DvzAlphaMode alpha_mode;

    bool can_depth_test;
    bool can_write_depth;
    bool writes_depth;
    bool samples_depth;
    bool volume_occluded;
    bool scene_occluded;
    bool scene_occluder;
    bool uses_segment_pipeline;

    bool uses_common_set;
    bool uses_material_set;
    bool uses_image_set;
    bool uses_volume_set;
} DvzSceneDrawFacts;


typedef struct DvzSceneDrawContract
{
    uint32_t visual_type;
    DvzAlphaMode alpha_mode;
    DvzFramePlanRenderPassRole pass_role;

    uint32_t depth_policy;
    DvzSceneBlendPolicy blend_policy;
    DvzSceneBlendTargetContract blend_targets[DVZ_DRP2_MAX_COLOR_ATTACHMENTS];
    uint32_t blend_target_count;
    uint32_t shader_feature_mask;
    uint32_t bind_group_layout_mask;
    bool has_raster_state;
    uint32_t cull_mode;
    uint32_t front_face;

    bool depth_test;
    bool depth_write;
    bool samples_depth;
    bool samples_volume_occlusion;
    bool samples_scene_occlusion;
    bool writes_volume_occlusion_depth;
    bool writes_scene_occlusion_depth;

    bool needs_common_set;
    bool needs_material_set;
    bool needs_image_set;
    bool needs_labels_set;
    bool needs_glyph_set;
    bool needs_volume_set;
    bool needs_scene_occlusion_set;

    char volume_occlusion_resource_id[DVZ_SCENE_LABEL_SIZE];
    char volume_occlusion_producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t volume_occlusion_bind_set;
    uint32_t volume_occlusion_bind_binding;
    char scene_occlusion_resource_id[DVZ_SCENE_LABEL_SIZE];
    char scene_occlusion_producer_pass_id[DVZ_SCENE_LABEL_SIZE];
    uint32_t scene_occlusion_bind_set;
    uint32_t scene_occlusion_bind_binding;
} DvzSceneDrawContract;


typedef struct DvzScenePassContract
{
    DvzScenePassKind kind;
    DvzFramePlanRenderPassRole role;
    char id[DVZ_SCENE_LABEL_SIZE];
    char panel_id[DVZ_SCENE_LABEL_SIZE];

    DvzSceneAttachmentUse attachments[DVZ_SCENE_MAX_CONTRACT_ATTACHMENTS];
    uint32_t attachment_count;

    DvzSceneDrawContract draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count;

    uint32_t color_attachment_count;
    uint32_t sampled_read_count;
    uint32_t sampled_depth_read_count;
    bool has_depth_attachment;

    bool needs_common_set;
    bool needs_material_set;
    bool needs_image_set;
    bool needs_labels_set;
    bool needs_glyph_set;
    bool needs_volume_set;
    bool needs_scene_occlusion_set;
    bool needs_wboit_resolve_layout;
    bool needs_depth_peel_sampled_layout;
    uint32_t sampled_texture_binding_count;

    bool source_over_blend;
    bool wboit_accumulation;
    bool depth_peel;
    bool fullscreen_resolve;
} DvzScenePassContract;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_draw_contract_resolve(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out);

bool _scene_draw_contract_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out);

bool _scene_pass_contract_from_render(
    const DvzFramePlan* plan, const DvzPanel* panel, const DvzFramePlanNode* render,
    const DvzFrameGraphPass* graph_pass, DvzScenePassContract* out);

bool _scene_pass_contract_from_render_ex(
    const DvzFramePlan* plan, const DvzPanel* panel, const DvzFramePlanNode* render,
    const DvzFrameGraphPass* graph_pass, const DvzCapabilitySnapshot* caps,
    DvzScenePassContract* out);

bool _scene_pass_contract_validate(
    const DvzScenePassContract* contract, DvzDiagnosticReport* report);

bool _scene_frame_plan_contracts_validate(
    const DvzFigure* figure, const DvzFramePlan* plan, DvzDiagnosticReport* report);

bool _scene_frame_plan_contracts_validate_ex(
    const DvzFigure* figure, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report);

bool _scene_frame_plan_drp2_contracts_validate(
    const DvzFramePlan* plan, const DvzDrp2CommandStream* stream, DvzDiagnosticReport* report);
