/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime render emission */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "_frame_plan_emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_render_pass.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Select the depth-peeling fragment shader variant.
 *
 * @param lit whether the visual carries normals and uses lit shading.
 * @param back_pass whether the pass writes the back-shell accumulation.
 * @return the built-in shader key.
 */
DvzSceneBuiltinShader _depth_peel_fragment_shader(bool lit, bool back_pass)
{
    if (lit)
    {
        return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT
                         : DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT;
    }
    return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK
                     : DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT;
}


/**
 * Return the SPIR-V resource key for one depth-peeling fragment shader.
 *
 * @param shader the built-in shader key.
 * @return the embedded SPIR-V key, or NULL when unsupported.
 */
const char* _depth_peel_fragment_spirv_key(DvzSceneBuiltinShader shader)
{
    switch (shader)
    {
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT:
        return "depth_peel_front_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK:
        return "depth_peel_back_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT:
        return "depth_peel_front_lit_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT:
        return "depth_peel_back_lit_frag";
    default:
        return NULL;
    }
}



/**
 * Attach scene/runtime labels to ids in an emitted DRP2 stream.
 *
 * @param emitter frame-plan emitter carrying scene/resource id maps
 * @param stream emitted DRP2 command stream
 * @param cfg optional emission configuration with borrowed target id
 */
void _emitter_label_stream_ids(
    const DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);

    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        const ResourceId* resource = &emitter->resources.resources[i];
        if (resource->id != 0 && resource->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, resource->id, resource->key);
    }

    for (uint32_t i = 0; i < emitter->objects.count; i++)
    {
        const ResourceId* object = &emitter->objects.resources[i];
        if (object->id != 0 && object->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, object->id, object->key);
    }

    if (cfg != NULL && cfg->color_target_id != 0)
        dvz_drp2_stream_set_label(stream, cfg->color_target_id, "rt");
}



/**
 * Attach the FramePlan pass-contract id to an emitted DRP2 render-pass id.
 *
 * @param stream emitted DRP2 command stream
 * @param pass_id the emitted DRP2 render-pass id
 * @param render the source FramePlan render node
 */
void _label_render_pass_contract(
    DvzDrp2CommandStream* stream, uint64_t pass_id, const DvzFramePlanNode* render)
{
    ANN(stream);
    ANN(render);
    if (pass_id != 0 && render->type == DVZ_FRAME_PLAN_NODE_RENDER &&
        render->u.render.has_pass_contract && render->u.render.pass_contract_id[0] != '\0')
    {
        dvz_drp2_stream_set_label(stream, pass_id, render->u.render.pass_contract_id);
    }
}



/**
 * Append one suffix to a runtime object key, reporting truncation as an emission error.
 *
 * @param key key buffer to append to
 * @param size key buffer size
 * @param suffix suffix to append
 * @param report optional diagnostic report
 * @return whether the suffix was appended without truncation
 */
bool _runtime_key_append(char* key, size_t size, const char* suffix, DvzDiagnosticReport* report)
{
    ANN(key);
    ANN(suffix);
    size_t key_len = strlen(key);
    size_t suffix_len = strlen(suffix);
    if (key_len >= size || suffix_len >= size - key_len)
    {
        _diagnostic(report, "runtime pipeline key suffix would be truncated");
        return false;
    }
    int written = dvz_snprintf(key + key_len, size - key_len, "%s", suffix);
    if (written < 0 || (size_t)written != suffix_len)
    {
        _diagnostic(report, "runtime pipeline key suffix append failed");
        return false;
    }
    return true;
}



/**
 * Append a formatted suffix to a runtime object key, reporting truncation as an emission error.
 *
 * @param key key buffer to append to
 * @param size key buffer size
 * @param report optional diagnostic report
 * @param format suffix format string
 * @return whether the suffix was appended without truncation
 */
bool _runtime_key_appendf(
    char* key, size_t size, DvzDiagnosticReport* report, const char* format, ...)
{
    ANN(key);
    ANN(format);
    char suffix[32];
    va_list args;
    va_start(args, format);
    int written = dvz_vsnprintf(suffix, sizeof(suffix), format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(suffix))
    {
        _diagnostic(report, "runtime pipeline key formatted suffix would be truncated");
        return false;
    }
    return _runtime_key_append(key, size, suffix, report);
}
