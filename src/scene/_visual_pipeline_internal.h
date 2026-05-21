/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual pipeline shared internals                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

uint64_t _scene_visual_resource_lookup_label(const ConverterState* state, const char* key);

bool _scene_visual_meta_is_primitive(uint32_t visual_type);

bool _scene_visual_meta_is_stroked_path(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta);

bool _scene_visual_has_dense_attr(const DvzVisual* visual, const char* name);

bool _scene_visual_desc_is_primitive(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_image(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_volume(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_sphere(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_segment(DvzSceneVisualDescKind kind);

bool _scene_visual_meta_point_like_kind(uint32_t visual_type, DvzScenePointLikeKind* out);

uint64_t _scene_render_visual_resource_id(
    const DvzFramePlanEmitter* emitter, const char* encoded_visual_id,
    DvzFramePlanResourceRole role);
