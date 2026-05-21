/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene technique shared internals                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_frame_graph_has_resource(const DvzFramePlan* plan, const char* resource_id);
bool _scene_frame_graph_color_written(const DvzFramePlan* plan, const char* resource_id);
bool _scene_frame_graph_resource_once(DvzFramePlan* plan, const DvzFrameGraphResource* resource);
bool _scene_msaa_sample_count_valid(uint32_t sample_count);
void _scene_frame_graph_color_attachment(
    DvzFrameGraphAttachment* attachment, const char* resource_id,
    DvzFrameGraphAttachmentLoadOp load_op, bool clear);
void _scene_frame_graph_depth_attachment(
    DvzFrameGraphAttachment* attachment, const char* resource_id,
    DvzFrameGraphAttachmentLoadOp load_op, DvzFrameGraphAttachmentAccess access);
bool _scene_caps_support_gbuffer(const DvzSceneVisualPassCaps* caps);
float _clampf(float value, float min_value, float max_value);
bool _scene_visual_samples_depth(const DvzVisual* visual, const DvzPanelAttach* attach);
