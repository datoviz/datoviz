/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan graph internals                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "frame_plan/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _frame_plan_graph_resource_index(const DvzFramePlan* plan, const char* resource_id, uint32_t* index);

bool _frame_plan_graph_pass_id_exists_before(const DvzFramePlan* plan, const char* pass_id, uint32_t end);

uint32_t _frame_plan_graph_usage_flag(DvzFrameGraphAccessUsage usage);

bool _frame_plan_graph_access_reads(DvzFrameGraphAccessUsage usage);

bool _frame_plan_graph_access_writes(DvzFrameGraphAccessUsage usage);

bool _frame_plan_graph_attachment_reads(const DvzFrameGraphAttachment* attachment);

bool _frame_plan_graph_attachment_writes(const DvzFrameGraphAttachment* attachment);

bool _frame_plan_graph_pass_writes_resource(const DvzFrameGraphPass* pass, const char* resource_id);

bool _frame_plan_graph_resource_is_per_frame(const DvzFramePlan* plan, const char* resource_id);

DvzFrameGraphAccessUsage
_frame_plan_graph_color_attachment_usage(const DvzFrameGraphAttachment* attachment);

DvzFrameGraphAccessUsage
_frame_plan_graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment);

bool _frame_plan_graph_pass_write_count_resource(
    const DvzFrameGraphPass* pass, const char* resource_id, uint32_t* count,
    DvzFrameGraphAccessUsage* usage);

bool _frame_plan_graph_find_last_writer_before(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index,
    uint32_t* producer_index, DvzFrameGraphAccessUsage* producer_usage);

bool _frame_plan_graph_find_first_writer_after(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index,
    uint32_t* producer_index, DvzFrameGraphAccessUsage* producer_usage);

bool _frame_plan_graph_dependency_from_access(
    const DvzFramePlan* plan, const char* resource_id, DvzFrameGraphAccessUsage consumer_usage,
    uint32_t consumer_index, DvzFrameGraphDependency* out);

uint32_t _frame_plan_graph_pass_dependency_count(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t pass_index,
    uint32_t target_index, DvzFrameGraphDependency* out);

bool _frame_plan_graph_resource_written_before(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index);

bool _frame_plan_graph_resource_is_color_attachment_compatible(const DvzFrameGraphResource* resource);

bool _frame_plan_graph_resource_is_depth_attachment_compatible(const DvzFrameGraphResource* resource);

uint32_t _frame_plan_graph_resource_sample_count(const DvzFrameGraphResource* resource);

bool _frame_plan_graph_resource_sample_count_valid(uint32_t sample_count);

bool _frame_plan_graph_resource_extent_matches(
    const DvzFrameGraphResource* a, const DvzFrameGraphResource* b);

bool _frame_plan_graph_report(DvzDiagnosticReport* report, const char* fmt, ...);
