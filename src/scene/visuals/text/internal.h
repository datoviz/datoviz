/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Text visual internals                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_emit/visual_lowering.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags);

int _scene_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);

int _scene_text_visual_set_font(DvzVisual* visual, DvzFont* font);

DvzTextRenderer _scene_adornment_text_renderer(DvzTextRenderer renderer);

DvzVisual* _scene_adornment_text_visual(DvzScene* scene, DvzTextRenderer renderer);

int _scene_adornment_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);

bool _scene_text_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out);

bool _scene_text_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

bool _scene_text_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out);

bool _scene_text_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

bool _scene_text_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out);
