/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual untyped compatibility classifiers                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/*
 * Return true when vertex_buffer_ids[0..n-1] carry data_tags "position", "color", "size"
 * (in any order), which identifies a DvzPoint visual.
 */
bool _scene_untyped_compat_is_point_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n < 3)
        return false;
    bool has_pos = false, has_col = false, has_sz = false;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
            has_pos = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)
            has_col = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE)
            has_sz = true;
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
            has_pos = true;
        if (strcmp(tag, "color") == 0)
            has_col = true;
        if (strcmp(tag, "size") == 0)
            has_sz = true;
    }
    return has_pos && has_col && has_sz;
}


/**
 * Return true when vertex buffers identify a Gaussian splat visual.
 *
 * @param state resource id state
 * @param ids resource ids
 * @param n resource id count
 * @return whether the ids carry position, color, sigma, and angle resources
 */
bool _scene_untyped_compat_is_splat_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n < 4)
        return false;
    bool has_pos = false, has_col = false, has_sigma = false, has_angle = false;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
            has_pos = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)
            has_col = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA)
            has_sigma = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE)
            has_angle = true;
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
            has_pos = true;
        if (strcmp(tag, "color") == 0)
            has_col = true;
        if (strcmp(tag, "sigma") == 0)
            has_sigma = true;
        if (strcmp(tag, "angle") == 0)
            has_angle = true;
    }
    return has_pos && has_col && has_sigma && has_angle;
}



/*
 * Return true when ids carry "position" + "color" with an optional "normal" attribute and
 * a topology hint on the position resource, identifying a DvzPrimitive visual.
 */
bool _scene_untyped_compat_is_primitive_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n < 2 || n > 3)
        return false;
    bool has_pos = false, has_col = false, has_topo = false, has_normal = false;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
        {
            has_pos = true;
            if (_resource_topology(state, ids[i]) != UINT32_MAX)
                has_topo = true;
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)
        {
            has_col = true;
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL)
        {
            has_normal = true;
            continue;
        }
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
        {
            has_pos = true;
            if (_resource_topology(state, ids[i]) != UINT32_MAX)
                has_topo = true;
        }
        if (strcmp(tag, "color") == 0)
            has_col = true;
        if (strcmp(tag, "normal") == 0)
            has_normal = true;
    }
    return has_pos && has_col && has_topo && (n == 2 || has_normal);
}



/*
 * Return true when ids carry exactly "position" + "texcoords" + "texture", identifying
 * a DvzImage visual. Outputs the position id, texcoords id, and texture id.
 */
bool _scene_untyped_compat_is_image_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n, uint64_t* out_pos,
    uint64_t* out_uv, uint64_t* out_tex)
{
    if (n != 3)
        return false;
    uint64_t pos = 0, uv = 0, tex = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
        {
            pos = ids[i];
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS)
        {
            uv = ids[i];
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE)
        {
            tex = ids[i];
            continue;
        }
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
            pos = ids[i];
        else if (strcmp(tag, "texcoords") == 0)
            uv = ids[i];
        else if (strcmp(tag, "texture") == 0)
            tex = ids[i];
    }
    if (pos == 0 || uv == 0 || tex == 0)
        return false;
    if (out_pos)
        *out_pos = pos;
    if (out_uv)
        *out_uv = uv;
    if (out_tex)
        *out_tex = tex;
    return true;
}


/**
 * Return true when ids carry position, color, normal, texcoords, and texture resources.
 *
 * @param state resource id state
 * @param ids resource ids
 * @param n resource id count
 * @param out_pos optional position resource id
 * @param out_color optional color resource id
 * @param out_normal optional normal resource id
 * @param out_uv optional texcoord resource id
 * @param out_tex optional texture resource id
 * @return whether the ids identify a textured mesh visual
 */
bool _scene_untyped_compat_is_textured_mesh_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n, uint64_t* out_pos,
    uint64_t* out_color, uint64_t* out_normal, uint64_t* out_uv, uint64_t* out_tex)
{
    if (n != 5)
        return false;
    uint64_t pos = 0, color = 0, normal = 0, uv = 0, tex = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
            pos = ids[i];
        else if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)
            color = ids[i];
        else if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL)
            normal = ids[i];
        else if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS)
            uv = ids[i];
        else if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE)
            tex = ids[i];
    }
    if (pos == 0 || color == 0 || normal == 0 || uv == 0 || tex == 0)
        return false;
    if (out_pos != NULL)
        *out_pos = pos;
    if (out_color != NULL)
        *out_color = color;
    if (out_normal != NULL)
        *out_normal = normal;
    if (out_uv != NULL)
        *out_uv = uv;
    if (out_tex != NULL)
        *out_tex = tex;
    return true;
}


