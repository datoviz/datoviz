/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual pipeline helpers                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_frame_plan_emit.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_VISUAL_DESC_NONE = 0,
    DVZ_SCENE_VISUAL_DESC_POINT,
    DVZ_SCENE_VISUAL_DESC_PIXEL,
    DVZ_SCENE_VISUAL_DESC_MARKER,
    DVZ_SCENE_VISUAL_DESC_SPHERE,
    DVZ_SCENE_VISUAL_DESC_SEGMENT,
    DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
    DVZ_SCENE_VISUAL_DESC_IMAGE,
    DVZ_SCENE_VISUAL_DESC_GLYPH,
    DVZ_SCENE_VISUAL_DESC_VOLUME,
} DvzSceneVisualDescKind;



typedef enum
{
    DVZ_SCENE_POINT_LIKE_POINT = 0,
    DVZ_SCENE_POINT_LIKE_PIXEL,
    DVZ_SCENE_POINT_LIKE_MARKER,
} DvzScenePointLikeKind;



typedef enum
{
    DVZ_SCENE_POINT_LIKE_LOWERING_NATIVE_POINTS = 0,
    DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS,
} DvzScenePointLikeLowering;



typedef struct DvzSceneVisualDesc
{
    DvzSceneVisualDescKind kind;
    DvzScenePointLikeKind point_like_kind;
    bool has_normal;
    bool depth_test_enabled;
    uint32_t topology;
    uint64_t vbuf_ids[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t vbuf_count;
    uint64_t index_buffer_id;
    uint64_t material_buffer_id;
    uint64_t image_texture_id;
    uint32_t glyph_atlas_encoding;
    float glyph_pixel_range;
    uint64_t volume_texture_id;
    uint64_t volume_transfer_texture_id;
    uint32_t volume_visual_index;
    bool depth_cue_enabled;
    bool point_style_enabled;
    bool volume_transfer_rgba;
    bool scene_occluded;
    DvzSceneOcclusionDesc scene_occlusion;
    bool volume_occluded;
    DvzVolumeOcclusionDesc volume_occlusion;
    DvzVolumeState volume_state;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t instance_count;
    bool has_instance_transform;
    const char* index_format;
} DvzSceneVisualDesc;


typedef struct DvzSceneVisualPassCaps
{
    DvzSceneVisualDescKind kind;
    DvzAlphaMode alpha_mode;
    DvzControllerMode controller_mode;
    bool fixed_controller;
    bool has_normals;
    bool depth_test_enabled;
    bool draws_in_opaque_pass;
    bool draws_in_wboit_pass;
    bool draws_in_depth_peel_pass;
    bool draws_in_transparent_blend_pass;
    bool uses_source_over_blend;
    bool writes_color;
    bool writes_depth;
    bool can_write_depth;
    bool can_depth_test;
    bool samples_depth;
    bool needs_depth_attachment;
    bool eligible_for_depth_postprocess;
    bool eligible_for_gbuffer;
    bool uses_common_set;
    bool needs_material_layout;
    bool uses_material_set;
    bool uses_image_set;
    bool uses_volume_set;
    bool supports_depth_cue;
    bool depth_cue_enabled;
} DvzSceneVisualPassCaps;



typedef struct DvzSceneVisualShaderDesc
{
    char vertex_key[32];
    char fragment_key[32];
    char pipeline_key[48];
    const char* vertex_glsl;
    const char* fragment_glsl;
    const char* vertex_wgsl;
    const char* fragment_wgsl;
    const char* vertex_spirv_key;
    const char* fragment_spirv_key;
    const char* builtin_family;
    const char* builtin_variant;
    const char* builtin_pipeline;
} DvzSceneVisualShaderDesc;



typedef struct DvzSceneVisualPipelineDesc
{
    uint32_t vertex_buffer_count;
    uint32_t topology;
    uint32_t binding_count;
    uint32_t attr_count;
    uint32_t strides[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t step_modes[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t bindings[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t locations[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t formats[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t offsets[DVZ_SCENE_MAX_NODE_RESOURCES];
    bool needs_common_layout;
    bool needs_image_layout;
    bool needs_glyph_layout;
    bool needs_volume_layout;
    bool needs_material_layout;
    bool needs_scene_occlusion_layout;
    bool has_depth_state;
    bool depth_write_enabled;
    uint32_t depth_compare_op;
    bool has_raster_state;
    uint32_t cull_mode;
    uint32_t front_face;
    bool alpha_to_coverage;
} DvzSceneVisualPipelineDesc;



typedef struct DvzSceneVisualBindDesc
{
    bool uses_common_set0;
    bool uses_fixed_common;
    DvzControllerMode controller_mode;
    bool uses_image_set1;
    uint64_t image_texture_id;
    bool uses_glyph_set1;
    uint64_t glyph_texture_id;
    float glyph_pixel_range;
    bool uses_volume_set1;
    uint64_t volume_texture_id;
    uint64_t volume_transfer_texture_id;
    uint64_t volume_depth_texture_id;
    uint32_t volume_visual_index;
    uint32_t volume_bind_variant;
    bool volume_transfer_rgba;
    bool volume_occluded;
    DvzVolumeOcclusionDesc volume_occlusion;
    DvzVolumeState volume_state;
    bool uses_material_set1;
    uint64_t material_buffer_id;
    bool uses_scene_occlusion_set2;
    uint64_t scene_occlusion_depth_texture_id;
    DvzSceneOcclusionDesc scene_occlusion;
} DvzSceneVisualBindDesc;



typedef struct DvzScenePointLoweringDesc
{
    DvzScenePointLikeKind kind;
    DvzScenePointLikeLowering lowering;
    uint32_t topology;
    uint32_t vertex_step_mode;
    uint32_t draw_vertex_count;
    uint32_t draw_instance_count;
} DvzScenePointLikeLoweringDesc;



bool _is_point_visual(const ConverterState* state, const uint64_t* ids, uint32_t n);

bool _is_primitive_visual(const ConverterState* state, const uint64_t* ids, uint32_t n);

bool _is_image_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n,
    uint64_t* out_pos, uint64_t* out_uv, uint64_t* out_tex);

uint64_t _scene_visual_resource_by_role(
    const ConverterState* state, const uint64_t* ids, uint32_t n,
    DvzFramePlanResourceRole role);

bool _emitter_resolve_render_vertex_buffers(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint64_t* out_ids,
    uint32_t* out_count);

bool _scene_render_visual_has_position_resource(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint32_t visual_index);

bool _scene_visual_desc_from_render(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint32_t visual_index,
    DvzSceneVisualDesc* out, const char** error);

bool _scene_visual_pass_caps_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzSceneVisualPassCaps* out);

bool _scene_visual_pass_caps_from_desc(
    const DvzSceneVisualDesc* visual, DvzAlphaMode alpha_mode,
    DvzControllerMode controller_mode, DvzSceneVisualPassCaps* out);

bool _scene_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag,
    DvzSceneVisualShaderDesc* out);

bool _scene_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPipelineDesc* out);

bool _scene_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

bool _scene_point_like_lowering_desc(
    DvzScenePointLikeKind kind, DvzSceneShaderFormat shader_format, uint32_t item_count,
    DvzScenePointLikeLoweringDesc* out);

bool _scene_render_needs_depth(DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render);
