/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Primitive visual shader descriptors                                                          */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "primitive/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve primitive visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_primitive_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    if (visual->kind != DVZ_SCENE_VISUAL_DESC_PRIMITIVE)
        return false;

    bool lit = visual->has_normal;
    bool instanced = visual->has_instance_transform;
    bool item_state = visual->has_item_state && !picking && !wboit_accumulation;
    if (picking)
    {
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_prim_pick%s%s",
            instanced ? "_inst" : "", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim_pick%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_pick_t%u%s%s",
            visual->topology, instanced ? "_inst" : "", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_PICK);
        if (instanced)
        {
            out->vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED, false);
            out->vertex_wgsl =
                _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED, false);
            out->vertex_spirv_key = "primitive_instanced_vert";
        }
        _scene_shader_desc_set_identity(
            out, "scene.primitive", instanced ? "pick_instanced" : "pick");
        out->fragment_spirv_key = "primitive_pick_frag";
        return true;
    }
    if (wboit_accumulation)
    {
        DvzSceneBuiltinShader shader =
            lit ? DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT : DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM;
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_wboit_accum_n%u%s", lit ? 1u : 0u,
            format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key), "_fs_wboit_accum_n%u%s", lit ? 1u : 0u,
            format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_wboit_accum_t%u_n%u%s",
            visual->topology, lit ? 1u : 0u, format_tag);
        _scene_shader_desc_set_builtin(out, shader);
        if (instanced)
        {
            DvzSceneBuiltinShader vertex_shader =
                lit ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED
                    : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED;
            out->vertex_glsl = _builtin_shader_glsl(vertex_shader, false);
            out->vertex_wgsl = _builtin_shader_wgsl(vertex_shader, false);
            out->vertex_spirv_key =
                lit ? "primitive_lit_instanced_vert" : "primitive_instanced_vert";
        }
        _scene_shader_desc_set_identity(
            out, "scene.primitive",
            instanced ? (lit ? "wboit_lit_instanced" : "wboit_instanced")
                      : (lit ? "wboit_lit" : "wboit"));
        return true;
    }

    if (lit)
    {
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_prim_lit%s%s%s",
            instanced ? "_inst" : "", item_state ? "_item_state" : "", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim_lit%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_lit_t%u%s%s%s",
            visual->topology, instanced ? "_inst" : "", item_state ? "_item_state" : "",
            format_tag);
        _scene_shader_desc_set_builtin(
            out,
            instanced ? (item_state ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED_ITEM_STATE
                                    : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED)
                      : (item_state ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_ITEM_STATE
                                    : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT));
        _scene_shader_desc_set_identity(
            out, "scene.primitive",
            instanced ? (item_state ? "lit_instanced_item_state" : "lit_instanced")
                      : (item_state ? "lit_item_state" : "lit"));
        if (item_state)
        {
            out->vertex_spirv_key = instanced ? "primitive_lit_instanced_item_state_vert"
                                              : "primitive_lit_item_state_vert";
            out->fragment_spirv_key = "primitive_lit_frag";
        }
        return true;
    }

    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_prim%s%s%s", instanced ? "_inst" : "",
        item_state ? "_item_state" : "", format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim%s", format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_t%u%s%s%s", visual->topology,
        instanced ? "_inst" : "", item_state ? "_item_state" : "", format_tag);
    _scene_shader_desc_set_builtin(
        out,
        instanced ? (item_state ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED_ITEM_STATE
                                : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED)
                  : (item_state ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_ITEM_STATE
                                : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE));
    _scene_shader_desc_set_identity(
        out, "scene.primitive",
        instanced ? (item_state ? "instanced_item_state" : "instanced")
                  : (item_state ? "item_state" : "default"));
    out->vertex_spirv_key = instanced ? (item_state ? "primitive_instanced_item_state_vert"
                                                    : "primitive_instanced_vert")
                                     : (item_state ? "primitive_item_state_vert"
                                                   : "primitive_vert");
    out->fragment_spirv_key = "primitive_frag";
    return true;
}
