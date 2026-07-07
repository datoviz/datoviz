/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family registry                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_shader_registry.h"
#include "_visual_family.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVisualFamilyOps DvzVisualFamilyOps;
typedef struct DvzVisualLowering DvzVisualLowering;

typedef bool (*DvzVisualFamilyLoweringFn)(
    const DvzVisual* visual, DvzVisualLowering* out);

typedef bool (*DvzVisualFamilyBoundsFn)(
    const DvzVisual* visual, DvzBounds* out, bool* out_force_3d);

typedef void (*DvzVisualFamilyOverlayBoundsFn)(const DvzVisual* visual, DvzBounds* bounds);

typedef bool (*DvzVisualFamilyPassCapsFn)(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out);

typedef bool (*DvzVisualFamilyBindDescFn)(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

typedef bool (*DvzVisualFamilyPipelineDescFn)(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out);

typedef bool (*DvzVisualFamilyShaderDescFn)(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

typedef bool (*DvzVisualFamilyDrawDescFn)(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out);

typedef bool (*DvzVisualFamilyDescFn)(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error);

typedef bool (*DvzVisualFamilyMetadataFn)(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata);
typedef void (*DvzVisualFamilyInitFn)(DvzVisual* visual);
typedef void (*DvzVisualFamilyResetFn)(DvzVisual* visual);
typedef bool (*DvzVisualFamilyValidateAttrFn)(
    const DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count);
typedef bool (*DvzVisualFamilyAfterAttrFn)(
    DvzVisual* visual, const char* attr_name, uint32_t item_count);
typedef const char* (*DvzVisualFamilyAttrStorageNameFn)(const char* attr_name);

struct DvzVisualFamilyOps
{
    DvzVisualType type;
    DvzSceneVisualFamily family;
    const char* name;
    DvzRenderableKind renderable_kind;
    DvzSceneVisualDescKind desc_kind;
    DvzMaterialKind default_material_kind;
    DvzMaterialModel default_material_model;
    const DvzVisualFamilyAttrDesc* attrs;
    uint32_t attr_count;
    const char* expected_attrs;
    const char* attr_alias_public;
    const char* attr_alias_storage;
    const char* item_range_attr_name;
    DvzVisualFamilyLoweringFn resolve_lowering;
    DvzVisualFamilyBoundsFn resolve_bounds;
    DvzVisualFamilyOverlayBoundsFn expand_overlay_bounds;
    DvzVisualFamilyPassCapsFn resolve_pass_caps;
    DvzVisualFamilyBindDescFn resolve_bind_desc;
    DvzVisualFamilyPipelineDescFn resolve_pipeline_desc;
    DvzVisualFamilyShaderDescFn resolve_shader_desc;
    DvzVisualFamilyDrawDescFn resolve_draw_desc;
    DvzVisualFamilyDescFn resolve_desc;
    DvzVisualFamilyMetadataFn fill_metadata;
    DvzVisualFamilyInitFn init_state;
    DvzVisualFamilyResetFn reset_state;
    DvzVisualFamilyValidateAttrFn validate_attr;
    DvzVisualFamilyAfterAttrFn after_attr_set;
    DvzVisualFamilyAttrStorageNameFn attr_storage_name;
    bool upload_position_topology;
    bool upload_material_params;
    bool skip_visual_uploads;
    bool panel_clip_rect;
    bool data_coord_uses_plot_clip_rect;
    bool sampled_field_texture_upload;
    bool supports_scale;
    bool supports_scalar_color_scale;
    bool categorical_scale;
    bool supports_material;
    bool supports_depth_cue;
    bool sync_point_style_material;
    bool bounds_resolves_local_transform;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const DvzVisualFamilyOps* _scene_visual_family_ops(DvzVisualType type);

const DvzVisualFamilyOps* _scene_visual_family_ops_for_family(DvzSceneVisualFamily family);

DvzSceneVisualFamily _scene_visual_family_from_type(DvzVisualType type);

uint32_t _scene_visual_family_ops_count(void);

const DvzVisualFamilyOps* _scene_visual_family_ops_at(uint32_t index);

bool _scene_visual_family_ops_registered(DvzVisualType type);

DvzRenderableKind _scene_visual_family_renderable_kind(DvzVisualType type);

DvzSceneVisualDescKind _scene_visual_family_desc_kind(DvzVisualType type);

DvzVisualType _scene_visual_family_desc_default_type(DvzSceneVisualDescKind kind);

bool _scene_visual_default_pass_caps(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out);

bool _scene_visual_shader_desc_resolve(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

void _scene_shader_desc_set_builtin(
    DvzSceneVisualShaderDesc* out, DvzSceneBuiltinShader shader);

void _scene_shader_desc_set_identity(
    DvzSceneVisualShaderDesc* out, const char* family, const char* variant);

bool _scene_visual_default_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out);

void _scene_visual_init_point_style(DvzVisual* visual);
void _scene_segment_visual_init_state(DvzVisual* visual);
void _scene_path_visual_init_state(DvzVisual* visual);
void _scene_vector_visual_init_state(DvzVisual* visual);
void _scene_labels_visual_init_state(DvzVisual* visual);
void _scene_volume_visual_init_state(DvzVisual* visual);
void _scene_text_visual_reset_state(DvzVisual* visual);
bool _scene_splat_visual_validate_attr(
    const DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count);
bool _scene_mesh_visual_after_attr_set(DvzVisual* visual, const char* attr_name, uint32_t item_count);
bool _scene_stroke_visual_after_attr_set(
    DvzVisual* visual, const char* attr_name, uint32_t item_count);
