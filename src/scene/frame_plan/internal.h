/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan private helpers                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "frame_plan/frame_plan.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void _frame_plan_copy_label(char* dst, uint64_t dst_size, const char* src);

DvzFramePlanNode* _frame_plan_append_node(DvzFramePlan* plan, DvzFramePlanNodeType type);

DvzFramePlanNode* _frame_plan_last_node(DvzFramePlan* plan, DvzFramePlanNodeType type);

bool _frame_plan_composition_validate(
    const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report);

uint64_t _frame_plan_composition_work_fingerprint(
    const DvzPanelCompositionSnapshot* snapshot);

bool _scene_panel_composition_contract_validate(
    const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report);

bool _frame_plan_composition_append(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot,
    DvzDiagnosticReport* report);

const DvzPanelCompositionSnapshot* _frame_plan_composition_get(
    const DvzFramePlan* plan, const char* panel_id);

bool _frame_plan_render_visual_reserve(DvzFramePlanNode* node, uint32_t count);

const char* _frame_graph_access_usage_name(DvzFrameGraphAccessUsage usage);

const char* _frame_plan_product_kind_name(DvzRenderProductKind kind);

const char* _frame_plan_product_domain_name(DvzRenderProductDomain domain);

const char* _frame_plan_product_extent_name(DvzRenderProductExtentPolicy extent);

const char* _frame_plan_product_rounding_name(DvzRenderProductRoundingPolicy rounding);

const char* _frame_plan_product_format_name(DvzRenderProductFormatClass format_class);

const char* _frame_plan_product_samples_name(DvzRenderProductSampleDomain sample_domain);

const char* _frame_plan_product_resolve_name(DvzRenderProductResolvePolicy resolve_policy);

const char* _frame_plan_product_coordinates_name(DvzRenderProductCoordinateSpace coordinates);

const char* _frame_plan_product_encoding_name(DvzRenderProductEncoding encoding);

const char* _frame_plan_product_alpha_name(DvzRenderProductAlpha alpha);

const char* _frame_plan_product_coverage_name(DvzRenderProductCoverage coverage);

const char* _frame_plan_product_validity_name(DvzRenderProductValidity validity);

const char* _frame_plan_product_validity_requirement_name(
    DvzRenderProductValidityRequirement requirement);
