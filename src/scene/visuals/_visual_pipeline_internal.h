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

DvzRenderableKind _scene_visual_meta_renderable_kind(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta);

DvzSceneVisualDescKind _scene_visual_meta_desc_kind(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta);

bool _scene_visual_meta_is_stroked_path(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta);

bool _scene_visual_has_dense_attr(const DvzVisual* visual, const char* name);

bool _scene_visual_desc_is_primitive(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_textured_mesh(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_image(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_volume(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_sphere(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_segment(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_path(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_stroke(DvzSceneVisualDescKind kind);

DvzVisualType _scene_visual_desc_default_type(DvzSceneVisualDescKind kind);

uint64_t _scene_render_visual_resource_id(
    const DvzFramePlanEmitter* emitter, const char* encoded_visual_id,
    DvzFramePlanResourceRole role);
