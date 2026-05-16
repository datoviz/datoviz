/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene shader registry                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_shader_registry.h"

#include "datoviz/drp2/stream.h"
#include "datoviz/fileio/fileio.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HAS_GLSL_SHADERS
#define DVZ_HAS_GLSL_SHADERS 0
#endif

#ifndef DVZ_HAS_PRECOMPILED_SHADERS
#define DVZ_HAS_PRECOMPILED_SHADERS 0
#endif

#ifndef DVZ_HAS_WGSL_SHADERS
#define DVZ_HAS_WGSL_SHADERS 0
#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return an embedded GLSL shader source by registry key.
 *
 * @param key the generated shader resource key
 * @return the shader source, or NULL when unavailable
 */
static const char* _resource_glsl(const char* key)
{
#if DVZ_HAS_GLSL_SHADERS
    unsigned long size = 0;
    const char* source = dvz_resource_glsl(key, &size);
    if (source != NULL && size > 0)
        return source;
#else
    (void)key;
#endif
    return NULL;
}



/**
 * Return an embedded WGSL shader source by registry key.
 *
 * @param key the generated shader resource key
 * @return the shader source, or NULL when unavailable
 */
static const char* _resource_wgsl(const char* key)
{
#if DVZ_HAS_WGSL_SHADERS
    unsigned long size = 0;
    const char* source = dvz_resource_wgsl(key, &size);
    if (source != NULL && size > 0)
        return source;
#else
    (void)key;
#endif
    return NULL;
}



/**
 * Return the embedded shader resource key for a built-in shader stage.
 *
 * @param shader the built-in shader key
 * @param fragment whether to return the fragment-stage variant
 * @return the generated shader resource key, or NULL when unsupported
 */
static const char* _builtin_shader_resource_key(DvzSceneBuiltinShader shader, bool fragment)
{
    switch (shader)
    {
    case DVZ_SCENE_BUILTIN_SHADER_FIXTURE:
        return fragment ? "fixture_frag" : "fixture_vert";
    case DVZ_SCENE_BUILTIN_SHADER_TEXTURE:
        return fragment ? "texture_frag" : "texture_vert";
    case DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY:
        return fragment ? NULL : "compute_copy";
    case DVZ_SCENE_BUILTIN_SHADER_POINT:
        return fragment ? "point_frag" : "point_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL:
        return fragment ? "pixel_frag" : "pixel_vert";
    case DVZ_SCENE_BUILTIN_SHADER_POINT_PICK:
        return fragment ? "point_pick_frag" : "point_pick_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE:
        return fragment ? "primitive_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT:
        return fragment ? "primitive_lit_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL:
        return fragment ? "gbuffer_normal_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_IMAGE:
        return fragment ? "image_frag" : "image_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE:
        return fragment ? "volume_slice_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_MIP:
        return fragment ? "volume_mip_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_COMPOSITE:
        return fragment ? "volume_composite_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM:
        return fragment ? "wboit_accum_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT:
        return fragment ? "wboit_accum_lit_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE:
        return fragment ? "wboit_resolve_frag" : "fullscreen_vert";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT:
        return fragment ? "depth_peel_front_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK:
        return fragment ? "depth_peel_back_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT:
        return fragment ? "depth_peel_front_lit_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT:
        return fragment ? "depth_peel_back_lit_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_COMPOSITE:
        return fragment ? "depth_peel_composite_frag" : "fullscreen_vert";
    default:
        return NULL;
    }
}



/**
 * Return the short shader-format tag used in runtime cache keys.
 *
 * @param cfg the emission configuration
 * @return "g" for GLSL, "w" otherwise
 */
const char* _shader_format_tag(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return "g";
    return "w";
}



/**
 * Return the DRP2 shader-format token for a scene fixture emission config.
 *
 * @param cfg the emission config
 * @return the shader-format token
 */
const char* _shader_format_token(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return "glsl";
    return "wgsl";
}



/**
 * Select shader source for a scene fixture emission config.
 *
 * @param cfg the emission config
 * @param wgsl the WGSL shader source
 * @param glsl the GLSL shader source
 * @return the selected shader source
 */
const char*
_shader_source(const DvzFramePlanEmitConfig* cfg, const char* wgsl, const char* glsl)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return glsl;
    return wgsl;
}



/**
 * Return the vertex shader source used for simple render emissions.
 *
 * @param cfg the emission config
 * @param wgsl output WGSL shader source
 * @param glsl output GLSL shader source
 */
void _render_vertex_shader_source(
    const DvzFramePlanEmitConfig* cfg, const char** wgsl, const char** glsl)
{
    if (cfg != NULL && cfg->fullscreen_triangle)
    {
        *wgsl = _resource_wgsl("fullscreen_vert");
        *glsl = _resource_glsl("fullscreen_vert");
    }
    else
    {
        *wgsl = _resource_wgsl("fixture_vert");
        *glsl = _resource_glsl("fixture_vert");
    }
}



/**
 * Emit a shader module command with the configured source format.
 *
 * @param stream the DRP2 stream
 * @param id the shader id
 * @param stage the shader stage
 * @param wgsl the WGSL shader source
 * @param glsl the GLSL shader source
 * @param cfg the emission config
 * @return whether the command was appended
 */
bool _emit_shader(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* wgsl,
    const char* glsl, const DvzFramePlanEmitConfig* cfg)
{
    return dvz_drp2_stream_create_shader_module_format(
        stream, id, stage, _shader_format_token(cfg), _shader_source(cfg, wgsl, glsl));
}



/**
 * Emit a shader using precompiled SPIR-V when available, or runtime GLSL otherwise.
 *
 * @param stream the DRP2 stream
 * @param id the shader id
 * @param stage the shader stage
 * @param spirv_key the embedded SPIR-V resource key
 * @param glsl the GLSL fallback source
 * @param cfg the emission config
 * @return whether the command was appended
 */
bool _emit_shader_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const char* spirv_key, const char* glsl, const DvzFramePlanEmitConfig* cfg)
{
    (void)cfg;
#if DVZ_HAS_PRECOMPILED_SHADERS
    unsigned long spv_size = 0;
    const unsigned char* spv = dvz_resource_shader(spirv_key, &spv_size);
    if (spv != NULL && spv_size > 0)
        return dvz_drp2_stream_create_shader_module_spirv(
            stream, id, stage, spv, (uint64_t)spv_size);
#else
    (void)spirv_key;
#endif
    return dvz_drp2_stream_create_shader_module_format(stream, id, stage, "glsl", glsl);
}



/**
 * Return the fallback GLSL source for a built-in scene shader.
 *
 * @param shader the built-in shader key
 * @param fragment whether to return the fragment stage variant
 * @return the GLSL source, or NULL when the key/stage pair is invalid
 */
const char* _builtin_shader_glsl(DvzSceneBuiltinShader shader, bool fragment)
{
    const char* key = _builtin_shader_resource_key(shader, fragment);
    if (key == NULL)
        return NULL;
    return _resource_glsl(key);
}



/**
 * Return the WGSL source for a built-in scene shader.
 *
 * @param shader the built-in shader key
 * @param fragment whether to return the fragment stage variant
 * @return the WGSL source, or NULL when the key/stage pair has no WGSL variant
 */
const char* _builtin_shader_wgsl(DvzSceneBuiltinShader shader, bool fragment)
{
    const char* key = _builtin_shader_resource_key(shader, fragment);
    if (key == NULL)
        return NULL;
    return _resource_wgsl(key);
}



/**
 * Return the fixture vertex WGSL source.
 *
 * @return the WGSL source
 */
const char* _fixture_vertex_wgsl(void)
{
    return _resource_wgsl("fixture_vert");
}



/**
 * Return the fullscreen triangle WGSL source.
 *
 * @return the WGSL source
 */
const char* _fullscreen_vertex_wgsl(void)
{
    return _resource_wgsl("fullscreen_vert");
}



/**
 * Return the fixture fragment WGSL source.
 *
 * @return the WGSL source
 */
const char* _fixture_fragment_wgsl(void)
{
    return _resource_wgsl("fixture_frag");
}



/**
 * Return the texture vertex WGSL source.
 *
 * @return the WGSL source
 */
const char* _texture_vertex_wgsl(void)
{
    return _resource_wgsl("texture_vert");
}



/**
 * Return the texture fragment WGSL source.
 *
 * @return the WGSL source
 */
const char* _texture_fragment_wgsl(void)
{
    return _resource_wgsl("texture_frag");
}



/**
 * Return the compute-copy WGSL source.
 *
 * @return the WGSL source
 */
const char* _compute_copy_wgsl(void)
{
    return _resource_wgsl("compute_copy");
}
