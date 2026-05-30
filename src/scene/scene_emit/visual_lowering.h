/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lowering internals                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_scene.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVisualLowering
{
    DvzRenderableKind renderable_kind;
    DvzSceneVisualDescKind desc_kind;
    DvzScenePointLikeKind point_like_kind;
    bool has_point_like_kind;
    bool needs_material_params;
    bool point_style_enabled;
    bool material_params_screen_scaled;
    bool needs_vector_params_sync;
    const char* draw_position_attr;
    const DvzStrokeQuadGpuCache* stroke_quad_cache;
    const DvzPathStrokeGpuCache* path_stroke_cache;
} DvzVisualLowering;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_visual_lowering_resolve(const DvzVisual* visual, DvzVisualLowering* out);

DvzRenderableKind _scene_visual_lowering_renderable_kind(const DvzVisual* visual);

DvzSceneVisualDescKind _scene_visual_lowering_desc_kind(const DvzVisual* visual);

bool _scene_visual_lowering_fill_metadata(
    const DvzVisual* visual, DvzFramePlanVisualMeta* metadata);

bool _scene_visual_lowering_volume_occluded(const DvzVisual* visual);
