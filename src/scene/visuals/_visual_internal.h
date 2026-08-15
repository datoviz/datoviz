/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual shared internals                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int _attr_index(const DvzVisual* visual, const char* name);
const char* _visual_type_name(DvzVisualType type);
bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index);
bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action);
const char* _attr_storage_name(DvzVisualType type, const char* name);
bool _attr_is_instance_attribute(DvzVisualType type, const char* name);
uint32_t _attr_item_size(DvzVisualType type, const char* name);
bool _attr_supported(DvzVisualType type, const char* name, uint32_t* item_size);
DvzVisualAttrFormat _attr_default_format(DvzVisualType type, const char* name);
bool _attr_format_supported(
    DvzVisualType type, const char* name, DvzVisualAttrFormat format);
uint32_t _attr_item_size_for_format(
    DvzVisualType type, const char* name, DvzVisualAttrFormat format);
uint32_t _visual_attr_item_size(const DvzVisual* visual, const char* name);
bool _attr_source_supported(DvzVisualType type, const char* name, DvzVisualAttrSource source);
bool _visual_data_update_contains_attr(
    DvzVisualType type, const DvzVisualDataUpdate* updates, uint32_t update_count,
    const char* attr_name);
DvzVisualAttr* _attr_get_or_create(DvzVisual* visual, const char* name, uint32_t item_size);
bool _visual_attr_count_consistent(
    const DvzVisual* visual, const char* attr_name, uint32_t item_count);
DvzVisualBinding* _visual_binding(DvzVisual* visual, DvzVisualBindingKind kind);
void _visual_bump_version(uint64_t* version);
bool _mesh_ensure_default_color(DvzVisual* visual, uint32_t item_count);
void _scene_mesh_visual_set_default_color(DvzVisual* visual, bool is_default);
void _material_params_default(DvzSceneMaterialParams* params);
void _material_state_default(DvzSceneMaterialState* material, DvzVisualType visual_type);
bool _material_visual_supported(DvzVisualType visual_type);
bool _material_model_valid(DvzMaterialModel model);
bool _material_alpha_mode_valid(DvzAlphaMode mode);
bool _material_desc_valid(const DvzMaterialDesc* desc);
void _material_state_apply_desc(DvzSceneMaterialState* material, const DvzMaterialDesc* desc);
void _material_params_sync_state(
    DvzSceneMaterialParams* params, const DvzSceneMaterialState* material);
void _material_params_upload_payload(const DvzVisual* visual, DvzSceneMaterialParams* out_params);
bool _material_depth_cue_supported(DvzVisualType visual_type);
int _material_apply_depth_cue(DvzSceneMaterialState* material, const DvzDepthCueDesc* desc);
void _visual_material_mark_dirty(DvzVisual* visual);
void _sphere_params_sync_mode(DvzVisual* visual);
void _labels_state_default(DvzLabelsState* state);
void _volume_state_default(DvzVolumeState* state);
bool _point_style_enabled(const DvzPointStyleDesc* style);
DvzPointStyleDesc _marker_style_to_point_style(const DvzMarkerStyle* style);
void _point_style_sync_params(DvzSceneMaterialParams* params, const DvzPointStyleDesc* style);
DvzVisual* _scene_alloc_visual(DvzScene* scene, DvzVisualType type, uint32_t flags);
void _scene_visual_reset(DvzVisual* visual, bool release_owned_resources);
uint64_t _scene_visual_public_id(const DvzScene* scene, const DvzVisual* visual);
void _scene_panel_visual_order(const DvzPanel* panel, uint32_t* order);
void _scene_release_visual_scale(DvzVisual* visual);
