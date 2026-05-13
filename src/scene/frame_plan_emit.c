/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan emission helpers                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_emit.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether the requested shader format is supported by the capability snapshot.
 *
 * @param caps the capability snapshot
 * @param cfg the emission config
 * @return whether the shader format is supported
 */
static bool
_validate_shader_format(const DvzCapabilitySnapshot* caps, const DvzFramePlanEmitConfig* cfg)
{
    ANN(caps);
    DvzSceneShaderFormat format =
        cfg != NULL ? cfg->shader_format : DVZ_SCENE_SHADER_FORMAT_WGSL;
    if (format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return caps->shader_format_glsl;
    return caps->shader_format_wgsl;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Fill a base64 string representing zero-initialized bytes.
 *
 * @param byte_size the decoded byte size
 * @param out the output string
 * @param out_size the output string capacity
 * @return whether the string was written
 */
bool _zero_base64(uint64_t byte_size, char* out, uint64_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return false;

    uint64_t groups = byte_size / 3;
    uint64_t remainder = byte_size % 3;
    uint64_t extra = remainder == 0 ? 0 : 4;
    if (groups > (UINT64_MAX - extra) / 4)
        return false;

    uint64_t count = groups * 4 + extra;
    if (count == UINT64_MAX || count + 1 > out_size)
        return false;

    for (uint64_t i = 0; i < count; i++)
        out[i] = 'A';
    if (remainder == 1)
    {
        out[count - 2] = '=';
        out[count - 1] = '=';
    }
    else if (remainder == 2)
    {
        out[count - 1] = '=';
    }
    out[count] = '\0';
    return true;
}



/**
 * Allocate a base64 string representing zero-initialized bytes.
 *
 * @param byte_size the decoded byte size
 * @return the owned base64 string, or NULL on failure
 */
char* _zero_base64_alloc(uint64_t byte_size)
{
    uint64_t groups = byte_size / 3;
    uint64_t remainder = byte_size % 3;
    uint64_t extra = remainder == 0 ? 0 : 4;
    if (groups > (UINT64_MAX - extra) / 4)
        return NULL;

    uint64_t count = groups * 4 + extra;
    if (count == UINT64_MAX)
        return NULL;

    char* out = (char*)dvz_malloc(count + 1);
    if (out == NULL)
        return NULL;
    if (!_zero_base64(byte_size, out, count + 1))
    {
        dvz_free(out);
        return NULL;
    }
    return out;
}



/**
 * Return the first node of a given type.
 *
 * @param plan the FramePlan
 * @param type the node type
 * @return the first matching node, or NULL
 */
const DvzFramePlanNode* _first_node_of_type(
    const DvzFramePlan* plan, DvzFramePlanNodeType type)
{
    ANN(plan);
    for (uint32_t i = 0; i < plan->count; i++)
    {
        if (plan->nodes[i].type == type)
            return &plan->nodes[i];
    }
    return NULL;
}



/**
 * Return whether a render node targets the fixture texture-sampling path.
 *
 * @param node the render node
 * @return true when one visual id names an image or texture visual
 */
bool _render_uses_texture(const DvzFramePlanNode* node)
{
    ANN(node);
    for (uint32_t i = 0; i < node->u.render.visual_count; i++)
    {
        const char* visual = node->u.render.visuals[i];
        if (strstr(visual, "image") != NULL || strstr(visual, "texture") != NULL)
            return true;
    }
    return false;
}



/**
 * Add a converter diagnostic message.
 *
 * @param report the optional diagnostic report
 * @param message the diagnostic message
 */
void _diagnostic(DvzDiagnosticReport* report, const char* message)
{
    ANN(message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Validate FramePlan conversion against a capability snapshot.
 *
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param cfg the emission config
 * @param report the optional diagnostic report
 * @return whether the plan can be emitted
 */
bool _validate_capabilities(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(caps);

    if (!_validate_shader_format(caps, cfg))
    {
        _diagnostic(report, "unsupported shader format");
        return false;
    }
    if (caps->max_texture_dimension_2d < 4)
    {
        _diagnostic(report, "max_texture_dimension_2d is too small for fixture render target");
        return false;
    }

    uint32_t upload_count = 0;
    bool has_compute = false;
    bool has_texture_render = false;
    bool has_scene_render = false;  /* scene nodes do per-visual draws, not one composite draw */
    uint64_t max_readback_size = 0;
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (render != NULL)
    {
        has_texture_render = _render_uses_texture(render);
        has_scene_render   = render->u.render.visual_count > 0;
    }

    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        switch (node->type)
        {
        case DVZ_FRAME_PLAN_NODE_UPLOAD:
            upload_count++;
            uint64_t upload_end = 0;
            if (_dvz_add_u64_overflows(
                    node->u.upload.byte_offset, node->u.upload.byte_size, &upload_end) ||
                upload_end > caps->max_buffer_size)
            {
                _diagnostic(report, "upload buffer exceeds max_buffer_size");
                return false;
            }
            break;
        case DVZ_FRAME_PLAN_NODE_COMPUTE:
            has_compute = true;
            break;
        case DVZ_FRAME_PLAN_NODE_COPY:
            if (node->u.copy.byte_size > max_readback_size)
                max_readback_size = node->u.copy.byte_size;
            break;
        default:
            break;
        }
    }

    /* For the fixture (non-scene) render pipeline, all uploads become vertex buffers in one draw.
     * For scene render nodes, each visual uses at most DVZ_SCENE_MAX_NODE_RESOURCES buffers per
     * draw, so check that bound rather than total upload count. */
    if (!has_texture_render)
    {
        uint32_t effective_count =
            has_scene_render ? DVZ_SCENE_MAX_NODE_RESOURCES : upload_count;
        if (effective_count > caps->max_vertex_buffers)
        {
            _diagnostic(report, "max_vertex_buffers is too small for fixture render pipeline");
            return false;
        }
    }
    if ((has_texture_render || has_compute) && caps->max_bind_groups < 1)
    {
        _diagnostic(report, "max_bind_groups is too small for fixture bind groups");
        return false;
    }
    if (has_texture_render && caps->max_texture_dimension_2d < 2)
    {
        _diagnostic(report, "max_texture_dimension_2d is too small for fixture texture upload");
        return false;
    }
    if (max_readback_size > caps->max_buffer_size)
    {
        _diagnostic(report, "readback buffer exceeds max_buffer_size");
        return false;
    }
    return true;
}



/**
 * Return the configured DRP2 color target id.
 *
 * @param cfg the emission config
 * @return the color target id
 */
uint64_t _color_target_id(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->color_target_id != 0)
        return cfg->color_target_id;
    return DRP2_ID_COLOR_TARGET;
}
