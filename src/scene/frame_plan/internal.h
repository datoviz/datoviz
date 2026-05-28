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

const char* _frame_graph_access_usage_name(DvzFrameGraphAccessUsage usage);
