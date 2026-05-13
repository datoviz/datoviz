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
    DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
    DVZ_SCENE_VISUAL_DESC_IMAGE,
} DvzSceneVisualDescKind;



typedef struct DvzSceneVisualDesc
{
    DvzSceneVisualDescKind kind;
    bool has_normal;
    uint32_t topology;
    uint64_t vbuf_ids[DVZ_SCENE_MAX_NODE_RESOURCES];
    uint32_t vbuf_count;
    uint64_t index_buffer_id;
    uint64_t shading_buffer_id;
    uint64_t image_texture_id;
    uint32_t vertex_count;
    uint32_t index_count;
    const char* index_format;
} DvzSceneVisualDesc;



typedef struct DvzSceneVisualShaderDesc
{
    char vertex_key[32];
    char fragment_key[32];
    char pipeline_key[48];
    const char* vertex_glsl;
    const char* fragment_glsl;
    const char* vertex_spirv_key;
    const char* fragment_spirv_key;
} DvzSceneVisualShaderDesc;



typedef struct DvzSceneVisualPipelineDesc
{
    uint32_t vertex_buffer_count;
    uint32_t topology;
    uint32_t binding_count;
    uint32_t attr_count;
    uint32_t strides[3];
    uint32_t bindings[3];
    uint32_t locations[3];
    uint32_t formats[3];
    uint32_t offsets[3];
    bool needs_mvp_layout;
    bool needs_image_layout;
    bool needs_shading_layout;
    bool has_depth_state;
    bool depth_write_enabled;
    uint32_t depth_compare_op;
} DvzSceneVisualPipelineDesc;



typedef struct DvzSceneVisualBindDesc
{
    bool uses_mvp_set0;
    bool uses_fixed_mvp;
    bool uses_image_set0;
    uint64_t image_texture_id;
    bool uses_shading_set1;
    uint64_t shading_buffer_id;
} DvzSceneVisualBindDesc;



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

bool _scene_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, const char* format_tag,
    DvzSceneVisualShaderDesc* out);

bool _scene_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    DvzSceneVisualPipelineDesc* out);

bool _scene_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

bool _scene_render_needs_depth(DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render);
