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

uint64_t _scene_visual_desc_resource(DvzFramePlanEmitter* emitter, const char* key);

bool _scene_visual_desc_append_resource(
    DvzFramePlanEmitter* emitter, DvzSceneVisualDesc* out, const char* key,
    const char* missing_error, const char** error);

bool _scene_visual_desc_set_primary_position(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, const char* key,
    const char* missing_error, DvzSceneVisualDesc* out, const char** error);

uint32_t _scene_visual_desc_resource_topology(DvzFramePlanEmitter* emitter, uint64_t resource_id);

uint64_t _scene_visual_desc_resource_size(DvzFramePlanEmitter* emitter, uint64_t resource_id);

bool _scene_visual_desc_finish_index(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char* overflow_error, const char** error);

bool _scene_visual_has_dense_attr(const DvzVisual* visual, const char* name);

bool _scene_visual_desc_is_primitive(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_textured_mesh(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_image(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_volume(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_sphere(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_glyph(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_segment(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_path(DvzSceneVisualDescKind kind);

bool _scene_visual_desc_is_stroke(DvzSceneVisualDescKind kind);

const char* _scene_visual_desc_kind_name(DvzSceneVisualDescKind kind);

DvzVisualType _scene_visual_desc_default_type(DvzSceneVisualDescKind kind);

void _scene_visual_pipeline_attr(
    DvzSceneVisualPipelineDesc* out, uint32_t index, uint32_t binding, uint32_t location,
    uint32_t format, uint32_t stride);

void _scene_visual_pipeline_instance_transform(
    DvzSceneVisualPipelineDesc* out, uint32_t first_attr, uint32_t binding);

void _scene_visual_pipeline_apply_standard_depth_state(
    const DvzSceneVisualPassCaps* caps, bool pass_needs_depth, bool wboit_accumulation,
    DvzAlphaMode alpha_mode, uint32_t depth_compare_op, DvzSceneVisualPipelineDesc* out);

void _scene_visual_pipeline_desc_apply_query_pick(
    const DvzSceneVisualDesc* visual, uint32_t color_target_format,
    DvzSceneVisualPipelineDesc* pipeline);

void _scene_visual_pipeline_desc_apply_pass_policy(
    const DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role, bool force_point_depth,
    uint32_t pass_sample_count, bool pass_alpha_to_coverage,
    DvzSceneVisualPipelineDesc* pipeline);

bool _scene_visual_shader_desc_apply_query_pick(
    const DvzSceneVisualDesc* visual, uint32_t color_target_format,
    DvzSceneVisualShaderDesc* shader, bool* out_applied);

bool _scene_visual_shader_desc_for_pass(
    DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role, const char* format_tag,
    DvzSceneVisualShaderDesc* shader, char** out_fragment_glsl_variant, bool* out_handled,
    bool* out_skip);

bool _scene_visual_shader_desc_apply_pass_policy(
    const DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role,
    DvzAlphaMode alpha_mode, DvzControllerMode controller_mode, bool picking,
    bool pass_has_depth_attachment, bool force_point_depth, bool wboit_accumulation,
    uint32_t pass_sample_count, bool pass_alpha_to_coverage, bool scene_occluded_shader,
    bool scene_occlusion_uses_set2, DvzSceneVisualShaderDesc* shader,
    char** out_fragment_glsl_variant, bool* out_segment_coverage_blend);

bool _scene_visual_bind_desc_uses_scene_occlusion_set2(
    const DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role);

void _scene_visual_bind_desc_apply_pass_policy(
    DvzSceneVisualBindDesc* bind, DvzFramePlanRenderPassRole pass_role, uint64_t sampled_depth_id,
    bool sampled_depth_is_volume_occlusion, uint64_t scene_occlusion_depth_id);

uint64_t _scene_render_visual_resource_id(
    const DvzFramePlanEmitter* emitter, const char* encoded_visual_id,
    DvzFramePlanResourceRole role);
