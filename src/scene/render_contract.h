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



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzSceneAttachmentUse
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    DvzSceneAttachmentRole role;
    uint32_t format;
    uint32_t sample_count;
    DvzFrameGraphAttachmentLoadOp load_op;
    DvzFrameGraphAttachmentStoreOp store_op;
    DvzFrameGraphAttachmentAccess access;
    bool read;
    bool write;
    bool clear;
    bool preserve;
} DvzSceneAttachmentUse;


typedef struct DvzSceneDrawContract
{
    uint32_t visual_type;
    DvzAlphaMode alpha_mode;
    DvzFramePlanRenderPassRole pass_role;

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
    bool needs_volume_set;
    bool needs_scene_occlusion_set;
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

bool _scene_draw_contract_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out);

bool _scene_pass_contract_from_render(
    const DvzFramePlan* plan, const DvzPanel* panel, const DvzFramePlanNode* render,
    const DvzFrameGraphPass* graph_pass, DvzScenePassContract* out);

bool _scene_pass_contract_validate(
    const DvzScenePassContract* contract, DvzDiagnosticReport* report);

bool _scene_frame_plan_contracts_validate(
    const DvzFigure* figure, const DvzFramePlan* plan, DvzDiagnosticReport* report);
