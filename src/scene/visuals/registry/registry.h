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

typedef bool (*DvzVisualFamilyPassCapsFn)(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out);

typedef bool (*DvzVisualFamilyBindDescFn)(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out);

typedef bool (*DvzVisualFamilyPipelineDescFn)(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPipelineDesc* out);

typedef bool (*DvzVisualFamilyShaderDescFn)(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out);

typedef bool (*DvzVisualFamilyDrawDescFn)(
    const DvzSceneVisualDesc* visual, DvzSceneVisualDrawDesc* out);

typedef bool (*DvzVisualFamilyMetadataFn)(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata);

struct DvzVisualFamilyOps
{
    DvzVisualType type;
    const char* name;
    DvzVisualFamilyLoweringFn resolve_lowering;
    DvzVisualFamilyBoundsFn resolve_bounds;
    DvzVisualFamilyPassCapsFn resolve_pass_caps;
    DvzVisualFamilyBindDescFn resolve_bind_desc;
    DvzVisualFamilyPipelineDescFn resolve_pipeline_desc;
    DvzVisualFamilyShaderDescFn resolve_shader_desc;
    DvzVisualFamilyDrawDescFn resolve_draw_desc;
    DvzVisualFamilyMetadataFn fill_metadata;
    bool upload_position_topology;
    bool upload_material_params;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const DvzVisualFamilyOps* _scene_visual_family_ops(DvzVisualType type);

uint32_t _scene_visual_family_ops_count(void);

const DvzVisualFamilyOps* _scene_visual_family_ops_at(uint32_t index);

bool _scene_visual_family_ops_registered(DvzVisualType type);

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
    const DvzSceneVisualDesc* visual, DvzSceneVisualDrawDesc* out);
