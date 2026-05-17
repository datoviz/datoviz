/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene shader registry                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/types.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_BUILTIN_SHADER_FIXTURE,
    DVZ_SCENE_BUILTIN_SHADER_TEXTURE,
    DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY,
    DVZ_SCENE_BUILTIN_SHADER_POINT,
    DVZ_SCENE_BUILTIN_SHADER_POINT_DEPTH_CUE,
    DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE,
    DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE_DEPTH_CUE,
    DVZ_SCENE_BUILTIN_SHADER_PIXEL,
    DVZ_SCENE_BUILTIN_SHADER_PIXEL_DEPTH_CUE,
    DVZ_SCENE_BUILTIN_SHADER_POINT_PICK,
    DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK,
    DVZ_SCENE_BUILTIN_SHADER_SEGMENT,
    DVZ_SCENE_BUILTIN_SHADER_SPHERE,
    DVZ_SCENE_BUILTIN_SHADER_SPHERE_A2C,
    DVZ_SCENE_BUILTIN_SHADER_SPHERE_GBUFFER,
    DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE,
    DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT,
    DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL,
    DVZ_SCENE_BUILTIN_SHADER_IMAGE,
    DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE,
    DVZ_SCENE_BUILTIN_SHADER_VOLUME_MIP,
    DVZ_SCENE_BUILTIN_SHADER_VOLUME_COMPOSITE,
    DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH,
    DVZ_SCENE_BUILTIN_SHADER_SCENE_OCCLUSION_DEPTH,
    DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM,
    DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT,
    DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE,
    DVZ_SCENE_BUILTIN_SHADER_SSAO,
    DVZ_SCENE_BUILTIN_SHADER_SSAO_BLUR,
    DVZ_SCENE_BUILTIN_SHADER_SSAO_COMPOSITE,
    DVZ_SCENE_BUILTIN_SHADER_EDL_RESOLVE,
    DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT,
    DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK,
    DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT,
    DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT,
    DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_COMPOSITE,
} DvzSceneBuiltinShader;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const char* _shader_format_tag(const DvzFramePlanEmitConfig* cfg);

const char* _shader_format_token(const DvzFramePlanEmitConfig* cfg);

const char*
_shader_source(const DvzFramePlanEmitConfig* cfg, const char* wgsl, const char* glsl);

void _render_vertex_shader_source(
    const DvzFramePlanEmitConfig* cfg, const char** wgsl, const char** glsl);

bool _emit_shader(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* wgsl,
    const char* glsl, const DvzFramePlanEmitConfig* cfg);

bool _emit_shader_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const char* spirv_key, const char* glsl, const DvzFramePlanEmitConfig* cfg);

char* _shader_glsl_variant(const char* glsl, const char* defines);

void _shader_glsl_variant_destroy(char* glsl);

const char* _builtin_shader_glsl(DvzSceneBuiltinShader shader, bool fragment);

const char* _builtin_shader_wgsl(DvzSceneBuiltinShader shader, bool fragment);

const char* _fixture_vertex_wgsl(void);

const char* _fullscreen_vertex_wgsl(void);

const char* _fixture_fragment_wgsl(void);

const char* _texture_vertex_wgsl(void);

const char* _texture_fragment_wgsl(void);

const char* _compute_copy_wgsl(void);
