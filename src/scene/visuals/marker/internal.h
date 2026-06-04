/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual internals                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_emit/visual_lowering.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_marker_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out);

bool _scene_marker_visual_pass_caps(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out);

bool _scene_marker_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error);

bool _scene_marker_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

bool _scene_marker_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out);

bool _scene_marker_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

bool _scene_marker_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out);

bool _scene_marker_visual_validate_attr(
    const DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count);

bool _scene_marker_visual_after_attr_set(
    DvzVisual* visual, const char* attr_name, uint32_t item_count);
