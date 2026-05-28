/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render contract internals                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "render_contract.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _draw_pass_role_matches(const DvzSceneDrawContract* draw);

void _draw_blend_target_contracts(
    DvzSceneBlendPolicy blend_policy, DvzSceneBlendTargetContract* targets,
    uint32_t* target_count);

void _draw_raster_state_contract(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role,
    bool* out_has_raster_state, uint32_t* out_cull_mode, uint32_t* out_front_face);

void _contract_report(DvzDiagnosticReport* report, const char* message);

const DvzFrameGraphPass* _contract_graph_pass_for_render(
    const DvzFramePlan* plan, const DvzFramePlanNode* render);

bool _contract_validate_graph_backed_render_nodes(
    const DvzFramePlan* plan, DvzDiagnosticReport* report);

DvzSceneAttachmentUse* _contract_append_attachment(
    DvzScenePassContract* contract, const char* resource_id, DvzSceneAttachmentRole role);

bool _contract_append_color_attachment(
    const DvzFramePlan* plan, DvzScenePassContract* contract,
    const DvzFrameGraphAttachment* attachment, const DvzCapabilitySnapshot* caps);

bool _contract_append_depth_attachment(
    const DvzFramePlan* plan, DvzScenePassContract* contract,
    const DvzFrameGraphAttachment* attachment, const DvzCapabilitySnapshot* caps);

bool _contract_append_read(
    const DvzFramePlan* plan, DvzScenePassContract* contract, const char* consumer_pass_id,
    const DvzFrameGraphAccess* read, const DvzCapabilitySnapshot* caps);

bool _contract_has_depth_attachment(const DvzScenePassContract* contract);

bool _contract_has_sampled_depth_resource(const DvzScenePassContract* contract);

uint32_t _contract_attachment_count(
    const DvzScenePassContract* contract, DvzSceneAttachmentRole role);

const DvzSceneAttachmentUse* _contract_attachment_suffix(
    const DvzScenePassContract* contract, DvzSceneAttachmentRole role, const char* suffix);

bool _contract_reads_resource_suffix(const DvzScenePassContract* contract, const char* suffix);

const DvzSceneAttachmentUse* _contract_sampled_resource_use(
    const DvzScenePassContract* contract, const char* resource_id);

bool _contract_needs_depth(const DvzScenePassContract* contract);
