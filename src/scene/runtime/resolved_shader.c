/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime shader resolution                                                    */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_runtime_internal.h"
#include "_scene.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "frame_plan/emit.h"
#include <stdbool.h>
#include <stdint.h>


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static const char* _runtime_shader_format_label(DvzSceneShaderFormat format)
{
    return format == DVZ_SCENE_SHADER_FORMAT_GLSL ? "glsl" : "wgsl";
}



static bool _runtime_shader_report(
    DvzDiagnosticReport* report, const DvzSceneVisualDesc* desc, DvzSceneWorkProviderKey provider,
    const DvzSceneResolvedShaderStage* stage, const char* reason)
{
    ANN(desc);
    ANN(stage);
    ANN(reason);
    char message[384];
    const DvzVisualType type = _scene_visual_desc_default_type(desc->kind);
    const char* visual = _visual_type_name(type);
    int written = dvz_snprintf(
        message, sizeof(message),
        "scene runtime shader validation failed: visual=%s provider=%s format=%s stage=%s key=%s "
        "reason=%s",
        visual, _scene_runtime_work_provider_name(provider),
        stage->format != NULL && stage->format[0] != '\0' ? stage->format : "<empty>",
        stage->stage_label != NULL && stage->stage_label[0] != '\0' ? stage->stage_label
                                                                    : "<empty>",
        stage->key[0] != '\0' ? stage->key : "<empty>", reason);
    if (written < 0 || (size_t)written >= sizeof(message))
        _diagnostic(report, "scene runtime shader validation failed");
    else
        _diagnostic(report, message);
    return false;
}



static void _runtime_shader_stage_resolve(
    const DvzSceneVisualShaderDesc* shader, DvzSceneShaderFormat format, bool fragment,
    DvzSceneResolvedShaderStage* out)
{
    ANN(shader);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneResolvedShaderStage), 0, sizeof(DvzSceneResolvedShaderStage));

    dvz_strlcpy(out->key, fragment ? shader->fragment_key : shader->vertex_key, sizeof(out->key));
    out->stage = fragment ? "FRAGMENT" : "VERTEX";
    out->stage_label = fragment ? "fragment" : "vertex";
    out->format = _runtime_shader_format_label(format);
    out->wgsl = fragment ? shader->fragment_wgsl : shader->vertex_wgsl;
    out->glsl = fragment ? shader->fragment_glsl : shader->vertex_glsl;
    out->spirv_key = fragment ? shader->fragment_spirv_key : shader->vertex_spirv_key;
    out->source = format == DVZ_SCENE_SHADER_FORMAT_WGSL ? out->wgsl : out->glsl;
    out->use_spirv = format != DVZ_SCENE_SHADER_FORMAT_WGSL && out->spirv_key != NULL;
}



static bool _runtime_shader_stage_valid(
    const DvzSceneVisualDesc* desc, DvzSceneWorkProviderKey provider, DvzSceneShaderFormat format,
    const DvzSceneResolvedShaderStage* stage, DvzDiagnosticReport* report)
{
    ANN(desc);
    ANN(stage);

    if (stage->key[0] == '\0')
        return _runtime_shader_report(report, desc, provider, stage, "empty key");
    if (stage->stage == NULL || stage->stage[0] == '\0')
        return _runtime_shader_report(report, desc, provider, stage, "empty stage");
    if (stage->format == NULL || stage->format[0] == '\0')
        return _runtime_shader_report(report, desc, provider, stage, "empty format");
    if (stage->source == NULL || stage->source[0] == '\0')
    {
        const char* reason =
            format == DVZ_SCENE_SHADER_FORMAT_WGSL ? "missing WGSL source" : "missing GLSL source";
        return _runtime_shader_report(report, desc, provider, stage, reason);
    }
    return true;
}



static bool _runtime_shader_emit_stage(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzSceneResolvedShader* shader, const DvzSceneResolvedShaderStage* stage,
    const DvzFramePlanEmitConfig* cfg, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(shader);
    ANN(stage);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, stage->key, &is_new);
    if (id == 0)
        return false;
    *out_id = id;
    if (!is_new)
        return true;

    bool ok = false;
    bool legacy = cfg != NULL && cfg->color_pipeline == DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND;
    if (stage->use_spirv && !legacy)
    {
        ok = _emit_shader_spirv(stream, id, stage->stage, stage->spirv_key, stage->glsl, cfg);
    }
    else
    {
        ok = _emit_shader(stream, id, stage->stage, stage->wgsl, stage->glsl, cfg);
    }

    if (ok && shader->builtin_family != NULL && shader->builtin_variant != NULL)
    {
        ok = dvz_drp2_stream_shader_set_builtin_identity(
            stream, id, shader->builtin_family, shader->builtin_variant,
            shader->builtin_version != 0 ? shader->builtin_version
                                         : DVZ_SCENE_SHADER_BUILTIN_CONTRACT_VERSION);
    }
    return ok;
}


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a diagnostic label for a typed work provider.
 *
 * @param provider work provider key.
 * @return stable provider label.
 */
const char* _scene_runtime_work_provider_name(DvzSceneWorkProviderKey provider)
{
    switch (provider)
    {
    case DVZ_SCENE_WORK_PROVIDER_OPAQUE:
        return "opaque";
    case DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE:
        return "surface_capture";
    case DVZ_SCENE_WORK_PROVIDER_SURFACE_RESOLVE:
        return "surface_resolve";
    case DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION:
        return "volume_occlusion";
    case DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION:
        return "scene_occlusion";
    case DVZ_SCENE_WORK_PROVIDER_GTAO:
        return "gtao";
    case DVZ_SCENE_WORK_PROVIDER_GTAO_DENOISE:
        return "gtao_denoise";
    case DVZ_SCENE_WORK_PROVIDER_GTAO_VISIBILITY_PRESENTATION:
        return "gtao_visibility_present";
    case DVZ_SCENE_WORK_PROVIDER_EDL:
        return "edl";
    case DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION:
        return "wboit_accumulation";
    case DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND:
        return "transparent_blend";
    case DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE:
        return "wboit_resolve";
    case DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT:
        return "depth_peel_init";
    case DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION:
        return "depth_peel_iteration";
    case DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_COMPOSITE:
        return "depth_peel_composite";
    case DVZ_SCENE_WORK_PROVIDER_PRESENTATION:
        return "presentation";
    case DVZ_SCENE_WORK_PROVIDER_NONE:
    default:
        return "unknown";
    }
}



/**
 * Resolve and validate the shader stages used by one render visual.
 *
 * @param shader visual shader descriptor after pass/query policies.
 * @param desc visual descriptor used for diagnostics.
 * @param provider typed work provider.
 * @param format selected shader source format.
 * @param out resolved shader descriptor.
 * @param report diagnostic report receiving validation failures.
 * @return whether both shader stages are valid.
 */
bool _scene_runtime_shader_resolve(
    const DvzSceneVisualShaderDesc* shader, const DvzSceneVisualDesc* desc,
    DvzSceneWorkProviderKey provider, DvzSceneShaderFormat format, DvzSceneResolvedShader* out,
    DvzDiagnosticReport* report)
{
    ANN(shader);
    ANN(desc);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneResolvedShader), 0, sizeof(DvzSceneResolvedShader));

    _runtime_shader_stage_resolve(shader, format, false, &out->vertex);
    _runtime_shader_stage_resolve(shader, format, true, &out->fragment);
    out->builtin_family = shader->builtin_family;
    out->builtin_variant = shader->builtin_variant;
    out->builtin_version = shader->builtin_version;

    return _runtime_shader_stage_valid(desc, provider, format, &out->vertex, report) &&
           _runtime_shader_stage_valid(desc, provider, format, &out->fragment, report);
}



/**
 * Emit resolved shader modules after validation has completed.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param shader resolved shader descriptor.
 * @param cfg optional frame-plan emit configuration.
 * @param out_vs_id output vertex shader object id.
 * @param out_fs_id output fragment shader object id.
 * @return whether both shader modules were emitted or already cached.
 */
bool _scene_runtime_shader_emit(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzSceneResolvedShader* shader, const DvzFramePlanEmitConfig* cfg, uint64_t* out_vs_id,
    uint64_t* out_fs_id)
{
    ANN(shader);
    return _runtime_shader_emit_stage(emitter, stream, shader, &shader->vertex, cfg, out_vs_id) &&
           _runtime_shader_emit_stage(emitter, stream, shader, &shader->fragment, cfg, out_fs_id);
}
