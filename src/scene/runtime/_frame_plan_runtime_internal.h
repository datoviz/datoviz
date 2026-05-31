/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime shared internals                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_scene.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SceneDrawVertexBuffer SceneDrawVertexBuffer;
typedef struct SceneDrawPacket SceneDrawPacket;
typedef struct SceneRenderBatch SceneRenderBatch;
typedef struct SceneGraphRuntimeTarget SceneGraphRuntimeTarget;
typedef struct SceneGraphRuntimeTargets SceneGraphRuntimeTargets;
typedef struct SceneGBufferTargets SceneGBufferTargets;
typedef struct SceneSsaoTargets SceneSsaoTargets;
typedef struct SceneEdlTargets SceneEdlTargets;
typedef struct SceneWboitTargets SceneWboitTargets;
typedef struct SceneDepthPeelTargets SceneDepthPeelTargets;

typedef struct DvzSceneOcclusionUniform
{
    float params[4];
} DvzSceneOcclusionUniform;


typedef struct DvzSceneGlyphUniform
{
    float params[4];
} DvzSceneGlyphUniform;


struct SceneDrawVertexBuffer
{
    uint32_t slot;
    uint64_t buffer_id;
    uint32_t stride;
    uint32_t step_mode;
    uint64_t logical_count;
    const char* role;
    bool validates_draw;
};


struct SceneDrawPacket
{
    DvzSceneVisualDescKind kind;
    uint64_t pipeline_id;
    uint64_t bg_set0; /* MVP bg; 0 = none */
    uint64_t bg_set1; /* image texture or material bg; 0 = none */
    uint64_t bg_set2; /* scene occlusion bg; 0 = none */
    uint64_t bg_set3; /* depth-peel sampled bg; 0 = none */
    DvzFramePlanClipRect clip_rect;
    SceneDrawVertexBuffer vertex_buffers[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t vertex_buffer_count;
    uint32_t validation_binding_count;
    uint64_t index_buffer_id;
    const char* index_format;
    uint32_t index_stride;
    uint64_t index_logical_count;
    uint32_t first_vertex;
    uint32_t vertex_count;
    uint32_t first_instance;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t base_vertex;
    uint32_t index_count;
    bool indexed;
};


struct SceneRenderBatch
{
    const DvzFramePlanNode* render;
    SceneDrawPacket draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count;
};


struct SceneGraphRuntimeTarget
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    uint64_t texture_id;
    uint32_t sample_count;
};


struct SceneGraphRuntimeTargets
{
    SceneGraphRuntimeTarget targets[DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY];
    uint32_t count;
};


struct SceneGBufferTargets
{
    uint64_t normal_id;
    uint64_t object_id;
    uint64_t depth_id;
    SceneGraphRuntimeTargets graph;
};


struct SceneEdlTargets
{
    uint64_t color_id;
    uint64_t depth_id;
    uint64_t params_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t resolve_bgl_id;
    uint64_t resolve_bg_id;
    uint64_t resolve_pipeline_id;
};


struct SceneSsaoTargets
{
    uint64_t normal_id;
    uint64_t depth_id;
    uint64_t occlusion_id;
    uint64_t blur_id;
    uint64_t composite_input_id;
    uint64_t params_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t ssao_bgl_id;
    uint64_t ssao_bg_id;
    uint64_t ssao_pipeline_id;
    uint64_t blur_bgl_id;
    uint64_t blur_bg_id;
    uint64_t blur_pipeline_id;
    uint64_t composite_bgl_id;
    uint64_t composite_bg_id;
    uint64_t composite_pipeline_id;
};


struct SceneWboitTargets
{
    uint64_t color_id;
    uint64_t accum_id;
    uint64_t weight_id;
    uint64_t depth_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t resolve_bgl_id;
    uint64_t resolve_bg_id;
    uint64_t resolve_pipeline_id;
};


struct SceneDepthPeelTargets
{
    uint64_t color_id;
    uint64_t depth_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t composite_bgl_id;
    uint64_t iter_bgl_id;
    uint64_t composite_bg_id;
    uint64_t iter_bg_ids[DVZ_SCENE_DEPTH_PEEL_ITERATIONS];
    uint64_t dummy_bg_id;
    uint64_t composite_pipeline_id;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzSceneBuiltinShader _depth_peel_fragment_shader(bool lit, bool back_pass);
const char* _depth_peel_fragment_spirv_key(DvzSceneBuiltinShader shader);
void _pipeline_bind_group_layouts(
    const DvzSceneVisualPipelineDesc* pipeline, uint64_t common_bgl_id, uint64_t image_bgl_id,
    uint64_t labels_bgl_id, uint64_t glyph_bgl_id, uint64_t volume_bgl_id,
    uint64_t material_bgl_id, uint64_t item_state_style_bgl_id, uint64_t scene_occlusion_bgl_id,
    bool scene_occlusion_uses_set2, uint64_t* out_layouts, uint32_t* out_count);
bool _resolve_material_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id);
bool _resolve_item_state_style_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id);
bool _resolve_textured_mesh_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id);
bool _resolve_item_state_style_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id,
    uint64_t material_buffer_id, uint64_t item_state_style_buffer_id, uint64_t* out_id);
bool _resolve_textured_mesh_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id,
    uint64_t material_buffer_id, uint64_t texture_id, uint64_t sampler_id, uint64_t* out_id);
bool _create_glyph_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);
bool _create_labels_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);
bool _create_volume_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);
bool _create_scene_occlusion_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);
bool _create_dummy_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);
void _scene_occlusion_uniform_from_desc(
    const DvzSceneOcclusionDesc* desc, DvzSceneOcclusionUniform* out);
bool _resolve_glyph_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id);
void _labels_uniform_from_state(const DvzLabelsState* state, DvzSceneLabelsUniform* out);
bool _resolve_labels_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id);
bool _resolve_scene_occlusion_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id);
bool _resolve_volume_dummy_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id);
bool _resolve_volume_dummy_transfer(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id);
void _volume_uniform_from_state(
    const DvzVolumeState* state, bool transfer_rgba, const DvzVolumeOcclusionDesc* occlusion,
    DvzSceneVolumeUniform* out);
bool _resolve_volume_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id);
void _emitter_label_stream_ids(
    const DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg);
void _label_render_pass_contract(
    DvzDrp2CommandStream* stream, uint64_t pass_id, const DvzFramePlanNode* render);
bool _runtime_key_append(char* key, size_t size, const char* suffix, DvzDiagnosticReport* report);
bool _runtime_key_appendf(
    char* key, size_t size, DvzDiagnosticReport* report, const char* format, ...);
bool _scene_draw_packet_init(
    const ConverterState* state, const DvzSceneVisualDesc* visual,
    const DvzSceneVisualPipelineDesc* pipeline, uint64_t pipeline_id, uint64_t bg_set0,
    uint64_t bg_set1, uint64_t bg_set2, uint64_t bg_set3, DvzFramePlanClipRect clip_rect,
    DvzSceneShaderFormat shader_format, DvzDiagnosticReport* report, SceneDrawPacket* out);
bool _scene_draw_packet_emit(
    DvzDrp2CommandStream* stream, uint64_t render_pass_id, const SceneDrawPacket* packet);
bool _emitter_prepare_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, bool pass_has_depth_attachment, bool force_point_depth,
    uint64_t sampled_depth_id, bool sampled_depth_is_volume_occlusion,
    uint64_t scene_occlusion_depth_id, uint64_t depth_peel_sampled_bgl_id,
    uint64_t depth_peel_sampled_bg_id, uint64_t depth_peel_dummy_bg_id, uint32_t pass_sample_count,
    bool pass_alpha_to_coverage, DvzDiagnosticReport* report, SceneDrawPacket* draws,
    uint32_t* draw_count_out);
DvzPanelDesc _render_desc_framebuffer_rect(
    const DvzPanelDesc* desc, const DvzFramePlanEmitConfig* cfg);
bool _emitter_emit_render_multi_draws(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, uint64_t render_pass_id, const SceneDrawPacket* draws,
    uint32_t draw_count, SceneRenderStateCache* cache);
void _emit_target_extent(const DvzFramePlanEmitConfig* cfg, uint32_t* width, uint32_t* height);
const DvzFrameGraphResource*
_graph_resource_by_id(const DvzFramePlan* plan, const char* resource_id);
const DvzFrameGraphPass*
_graph_pass_by_panel_work(const DvzFramePlan* plan, const char* panel_id, const char* work_label);
const DvzFrameGraphPass* _graph_pass_by_id(const DvzFramePlan* plan, const char* pass_id);
const DvzFrameGraphPass*
_graph_pass_for_render(const DvzFramePlan* plan, const DvzFramePlanNode* render);
const DvzFramePlanNode*
_graph_render_for_pass(const DvzFramePlan* plan, const DvzFrameGraphPass* pass);
uint32_t _graph_texture_usage_to_drp2(uint32_t usage_flags);
uint32_t _graph_access_usage_to_drp2(DvzFrameGraphAccessUsage usage);
DvzFrameGraphAccessUsage _graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment);
uint32_t _graph_declared_texture_usage_to_drp2(const DvzFramePlan* plan, const char* resource_id);
DvzDrp2AttachmentLoadOp _graph_load_op_to_drp2(DvzFrameGraphAttachmentLoadOp op);
DvzDrp2AttachmentStoreOp _graph_store_op_to_drp2(DvzFrameGraphAttachmentStoreOp op);
DvzDrp2AttachmentAccess _graph_attachment_access_to_drp2(DvzFrameGraphAttachmentAccess access);
uint32_t _graph_resource_sample_count(const DvzFrameGraphResource* resource);
uint32_t _sample_count_lowered(uint32_t sample_count, uint32_t max_sample_count);
uint32_t _graph_resource_lowered_sample_count(
    const DvzFramePlanEmitter* emitter, const DvzFrameGraphResource* resource);
uint32_t _graph_render_pass_sample_count(
    const DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzFrameGraphPass* pass);
bool _stream_apply_graph_color_ops(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets);
bool _stream_apply_graph_depth(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t depth_id);
bool _graph_runtime_targets_add(
    SceneGraphRuntimeTargets* targets, const char* resource_id, uint64_t texture_id);
uint64_t
_graph_runtime_targets_get(const SceneGraphRuntimeTargets* targets, const char* resource_id);
uint64_t _graph_runtime_texture_id_for_resource(
    const char* resource_id, uint64_t final_color_id, const SceneGraphRuntimeTargets* targets,
    uint64_t fallback_id);
uint64_t _graph_color_attachment_texture_id(
    const DvzFrameGraphPass* pass, uint32_t attachment_index, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets, uint64_t fallback_id);
uint64_t _graph_sampled_read_texture_id(
    const DvzFrameGraphPass* pass, uint32_t read_index, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets, uint64_t fallback_id);
bool _graph_volume_occlusion_read_index(const DvzFrameGraphPass* pass, uint32_t* out_read_index);
bool _graph_resolve_volume_occlusion_read(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphPass* pass, uint64_t* out_id);
bool _graph_scene_occlusion_read_index(const DvzFrameGraphPass* pass, uint32_t* out_read_index);
bool _graph_resolve_scene_occlusion_read(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphPass* pass, uint64_t* out_id);
bool _graph_prepare_render_color_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFrameGraphPass* pass, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* out);
bool _runtime_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const char* key, uint32_t width,
    uint32_t height, uint32_t format, uint32_t usage, uint32_t sample_count, uint64_t* out_id);
void _runtime_scope_key(
    const DvzFramePlanEmitConfig* cfg, const char* base_key, char* out_key, size_t out_size);
bool _graph_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphResource* resource, uint32_t width,
    uint32_t height, uint32_t fallback_format, uint64_t* out_id);
bool _graph_resolve_render_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    const DvzFrameGraphPass** graph_pass, uint64_t* out_depth_id);
bool _emitter_prepare_gbuffer_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg, SceneGBufferTargets* out);
uint64_t _edl_bind_group_fingerprint(
    uint64_t color_id, uint64_t depth_id, uint64_t sampler_id, uint64_t params_id);
bool _emitter_prepare_edl_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg, SceneEdlTargets* out);
uint64_t _ssao_bind_group_fingerprint(
    uint64_t first_id, uint64_t second_id, uint64_t third_id, uint64_t sampler_id,
    uint64_t params_id);
bool _emitter_prepare_ssao_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg, SceneSsaoTargets* out);
uint64_t _wboit_bind_group_fingerprint(uint64_t accum_id, uint64_t weight_id, uint64_t sampler_id);
bool _emitter_prepare_wboit_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneWboitTargets* out);
uint64_t _depth_peel_bind_group_fingerprint(
    const uint64_t* texture_ids, uint32_t texture_count, uint64_t sampler_id);
bool _depth_peel_resolve_sampled_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass,
    const SceneGraphRuntimeTargets* targets, const char* key, uint64_t bgl_id, uint64_t sampler_id,
    uint64_t* out_bg_id);
bool _emitter_prepare_depth_peel_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneDepthPeelTargets* out);
const SceneGBufferTargets* _gbuffer_targets_for_panel(
    const SceneGBufferTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id);
const SceneEdlTargets* _edl_targets_for_panel(
    const SceneEdlTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id);
const SceneSsaoTargets* _ssao_targets_for_panel(
    const SceneSsaoTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id);
const SceneWboitTargets* _wboit_targets_for_panel(
    const SceneWboitTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id);
const SceneDepthPeelTargets* _depth_peel_targets_for_panel(
    const SceneDepthPeelTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id);
const SceneRenderBatch* _render_batch_for_node(
    const SceneRenderBatch* batches, uint32_t count, const DvzFramePlanNode* render);
bool _emitter_emit_wboit_resolve(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, uint64_t render_pass_id, const SceneWboitTargets* targets);
bool _emitter_emit_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report);
bool _emitter_emit_scene_figure_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg, bool needs_depth,
    DvzDiagnosticReport* report);
bool _plan_has_graph_render_passes(const DvzFramePlan* plan);
bool _emitter_emit_scene_graph_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report);
bool _emitter_emit_render_compat(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const uint64_t* vertex_buffer_ids,
    uint32_t vertex_buffer_count, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report);
bool _emitter_emit_plain_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const uint64_t* fallback_vertex_buffer_ids, uint32_t fallback_vertex_buffer_count,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report);
bool _emitter_emit_compute_passes(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, DvzDiagnosticReport* report);
bool _emitter_emit_clear_only(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* clear_node,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg);
bool _emitter_emit_texture_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t texture_id,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg);
bool _emitter_emit_compute_assisted_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* compute,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg);
