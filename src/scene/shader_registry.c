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

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
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
    case DVZ_SCENE_BUILTIN_SHADER_POINT_DEPTH_CUE:
        return fragment ? "point_cue_frag" : "point_cue_vert";
    case DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE:
        return fragment ? "point_style_frag" : "point_style_vert";
    case DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE_DEPTH_CUE:
        return fragment ? "point_cue_style_frag" : "point_cue_style_vert";
    case DVZ_SCENE_BUILTIN_SHADER_POINT_SELECTION:
        return fragment ? "point_select_frag" : "point_select_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL:
        return fragment ? "pixel_frag" : "pixel_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL_DEPTH_CUE:
        return fragment ? "pixel_cue_frag" : "pixel_cue_vert";
    case DVZ_SCENE_BUILTIN_SHADER_POINT_PICK:
        return fragment ? "point_pick_frag" : "point_pick_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK:
        return fragment ? "pixel_pick_frag" : "pixel_pick_vert";
    case DVZ_SCENE_BUILTIN_SHADER_POINT_QUERY_U32:
        return fragment ? "point_query_u32_frag" : "point_pick_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL_QUERY_U32:
        return fragment ? "pixel_query_u32_frag" : "pixel_pick_vert";
    case DVZ_SCENE_BUILTIN_SHADER_MARKER:
        return fragment ? "marker_frag" : "marker_vert";
    case DVZ_SCENE_BUILTIN_SHADER_MARKER_SELECTION:
        return fragment ? "marker_select_frag" : "marker_select_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SEGMENT:
        return fragment ? "segment_frag" : "segment_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SEGMENT_PICK:
        return fragment ? "segment_pick_frag" : "segment_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SEGMENT_QUERY_U32:
        return fragment ? "segment_query_u32_frag" : "segment_query_u32_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PATH:
        return fragment ? "path_frag" : "path_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PATH_PICK:
        return fragment ? "path_pick_frag" : "path_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PATH_QUERY_U32:
        return fragment ? "path_query_u32_frag" : "path_query_u32_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SPHERE:
        return fragment ? "sphere_frag" : "sphere_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SPHERE_PICK:
        return fragment ? "sphere_pick_frag" : "sphere_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SPHERE_QUERY_U32:
        return fragment ? "sphere_query_u32_frag" : "sphere_query_u32_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SPHERE_A2C:
        return fragment ? "sphere_a2c_frag" : "sphere_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SPHERE_GBUFFER:
        return fragment ? "sphere_gbuffer_frag" : "sphere_gbuffer_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE:
        return fragment ? "primitive_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED:
        return fragment ? "primitive_frag" : "primitive_instanced_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_PICK:
        return fragment ? "primitive_pick_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_QUERY_U32:
        return fragment ? "primitive_query_u32_frag" : "primitive_query_u32_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT:
        return fragment ? "primitive_lit_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED:
        return fragment ? "primitive_lit_frag" : "primitive_lit_instanced_vert";
    case DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL:
        return fragment ? "gbuffer_normal_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_IMAGE:
        return fragment ? "image_frag" : "image_vert";
    case DVZ_SCENE_BUILTIN_SHADER_IMAGE_PIXEL:
        return fragment ? "image_frag" : "image_pixel_vert";
    case DVZ_SCENE_BUILTIN_SHADER_LABELS_SINT:
        return fragment ? "labels_sint_frag" : "image_vert";
    case DVZ_SCENE_BUILTIN_SHADER_LABELS_UINT:
        return fragment ? "labels_uint_frag" : "image_vert";
    case DVZ_SCENE_BUILTIN_SHADER_GLYPH:
        return fragment ? "glyph_frag" : "glyph_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE:
        return fragment ? "volume_slice_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_MIP:
        return fragment ? "volume_mip_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_COMPOSITE:
        return fragment ? "volume_composite_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH:
        return fragment ? "volume_occlusion_depth_frag" : "volume_slice_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SCENE_OCCLUSION_DEPTH:
        return fragment ? "scene_occlusion_depth_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM:
        return fragment ? "wboit_accum_frag" : "primitive_vert";
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT:
        return fragment ? "wboit_accum_lit_frag" : "primitive_lit_vert";
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE:
        return fragment ? "wboit_resolve_frag" : "fullscreen_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SSAO:
        return fragment ? "ssao_frag" : "fullscreen_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SSAO_BLUR:
        return fragment ? "ssao_blur_frag" : "fullscreen_vert";
    case DVZ_SCENE_BUILTIN_SHADER_SSAO_COMPOSITE:
        return fragment ? "ssao_composite_frag" : "fullscreen_vert";
    case DVZ_SCENE_BUILTIN_SHADER_EDL_RESOLVE:
        return fragment ? "edl_resolve_frag" : "fullscreen_vert";
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
 * Append bytes to an owned shader source buffer.
 *
 * @param dst the source buffer pointer
 * @param len current byte length
 * @param cap current buffer capacity
 * @param src bytes to append
 * @param src_len number of bytes to append
 * @return whether the append succeeded
 */
static bool
_shader_builder_append(char** dst, size_t* len, size_t* cap, const char* src, size_t src_len)
{
    ANN(dst);
    ANN(len);
    ANN(cap);
    ANN(src);
    if (src_len == 0)
        return true;
    if (src_len > SIZE_MAX - *len - 1)
        return false;
    size_t need = *len + src_len + 1;
    if (need > *cap)
    {
        size_t next = *cap == 0 ? 4096 : *cap;
        while (next < need)
        {
            if (next > SIZE_MAX / 2)
                return false;
            next *= 2;
        }
        char* grown = (char*)dvz_realloc(*dst, next);
        if (grown == NULL)
            return false;
        *dst = grown;
        *cap = next;
    }
    dvz_memcpy(*dst + *len, *cap - *len, src, src_len);
    *len += src_len;
    (*dst)[*len] = '\0';
    return true;
}


/**
 * Return an embedded GLSL include source from an include filename.
 *
 * @param include_name include filename such as scene_material.glsl
 * @return embedded source, or NULL when unavailable
 */
static const char* _shader_include_source(const char* include_name)
{
    ANN(include_name);
    char key[128] = {0};
    dvz_strlcpy(key, include_name, sizeof(key));
    char* dot = strrchr(key, '.');
    if (dot != NULL)
        *dot = '\0';
    return _resource_glsl(key);
}


/**
 * Recursively append a GLSL source with local includes resolved.
 *
 * @param dst the source buffer pointer
 * @param len current byte length
 * @param cap current buffer capacity
 * @param glsl input GLSL source
 * @param defines optional top-level defines inserted after #version
 * @param top_level whether this source is the top-level shader
 * @param depth include recursion depth
 * @return whether preprocessing succeeded
 */
static bool _shader_preprocess_into(
    char** dst, size_t* len, size_t* cap, const char* glsl, const char* defines, bool top_level,
    uint32_t depth)
{
    ANN(dst);
    ANN(len);
    ANN(cap);
    ANN(glsl);
    if (depth > 8)
        return false;

    const char* cursor = glsl;
    bool inserted_defines = !top_level || defines == NULL || defines[0] == '\0';
    while (*cursor != '\0')
    {
        const char* line_end = strchr(cursor, '\n');
        size_t line_len = line_end != NULL ? (size_t)(line_end - cursor + 1) : strlen(cursor);

        if (strncmp(cursor, "#include \"", 10) == 0)
        {
            const char* name_start = cursor + 10;
            const char* name_end = strchr(name_start, '"');
            if (name_end == NULL || (line_end != NULL && name_end > line_end))
                return false;
            char include_name[128] = {0};
            size_t name_len = (size_t)(name_end - name_start);
            if (name_len >= sizeof(include_name))
                return false;
            dvz_memcpy(include_name, sizeof(include_name), name_start, name_len);
            const char* include_source = _shader_include_source(include_name);
            if (include_source == NULL)
                return false;
            if (!_shader_preprocess_into(
                    dst, len, cap, include_source, NULL, false, depth + 1))
                return false;
        }
        else
        {
            if (!_shader_builder_append(dst, len, cap, cursor, line_len))
                return false;
            if (!inserted_defines && strncmp(cursor, "#version", 8) == 0)
            {
                if (!_shader_builder_append(dst, len, cap, defines, strlen(defines)))
                    return false;
                inserted_defines = true;
            }
        }
        cursor += line_len;
    }
    if (!inserted_defines)
        return _shader_builder_append(dst, len, cap, defines, strlen(defines));
    return true;
}


/**
 * Create an owned GLSL variant with local includes resolved and optional defines inserted.
 *
 * @param glsl input GLSL source
 * @param defines defines to insert after #version, or NULL
 * @return owned preprocessed source, or NULL on failure
 */
char* _shader_glsl_variant(const char* glsl, const char* defines)
{
    if (glsl == NULL)
        return NULL;
    char* out = NULL;
    size_t len = 0;
    size_t cap = 0;
    if (!_shader_preprocess_into(&out, &len, &cap, glsl, defines, true, 0))
    {
        dvz_free(out);
        return NULL;
    }
    return out;
}


/**
 * Destroy an owned GLSL variant returned by _shader_glsl_variant().
 *
 * @param glsl owned GLSL source
 */
void _shader_glsl_variant_destroy(char* glsl)
{
    dvz_free(glsl);
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
