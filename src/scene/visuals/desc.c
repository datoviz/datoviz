/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual descriptor lowering                                                             */
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
#include "registry/registry.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve a visual descriptor resource id from a metadata key.
 *
 * @param emitter the persistent emitter
 * @param key the resource key
 * @return resource id, or zero when absent
 */
uint64_t _scene_visual_desc_resource(DvzFramePlanEmitter* emitter, const char* key)
{
    ANN(emitter);
    return _scene_visual_resource_lookup_label(&emitter->resources, key);
}


/**
 * Return color-role metadata for one descriptor resource.
 *
 * @param emitter the persistent emitter
 * @param resource_id the resource id
 * @return resource color role, or NONE when absent
 */
DvzColorRole _scene_visual_desc_resource_color_role(
    DvzFramePlanEmitter* emitter, uint64_t resource_id)
{
    ANN(emitter);
    if (resource_id == 0)
        return DVZ_COLOR_ROLE_NONE;
    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        const ResourceId* resource = &emitter->resources.resources[i];
        if (resource->id == resource_id)
            return resource->color_role;
    }
    return DVZ_COLOR_ROLE_NONE;
}



/**
 * Append one descriptor vertex-buffer resource.
 *
 * @param emitter the persistent emitter
 * @param out the output visual descriptor
 * @param key the resource key
 * @param missing_error optional error when the resource is required
 * @param error optional diagnostic output
 * @return whether the resource was appended or skipped when optional
 */
bool _scene_visual_desc_append_resource(
    DvzFramePlanEmitter* emitter, DvzSceneVisualDesc* out, const char* key,
    const char* missing_error, const char** error)
{
    ANN(emitter);
    ANN(out);
    uint64_t id = _scene_visual_desc_resource(emitter, key);
    if (id == 0)
    {
        if (missing_error != NULL && error != NULL)
            *error = missing_error;
        return missing_error == NULL;
    }
    if (out->vbuf_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
    {
        if (error != NULL)
            *error = "typed visual metadata has too many vertex buffers";
        return false;
    }
    out->vbuf_ids[out->vbuf_count++] = id;
    return true;
}



/**
 * Append and count the primary position resource for a descriptor.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param key the primary position resource key
 * @param missing_error error when the resource is absent
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether the primary position resource was resolved
 */
bool _scene_visual_desc_set_primary_position(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, const char* key,
    const char* missing_error, DvzSceneVisualDesc* out, const char** error)
{
    ANN(emitter);
    ANN(meta);
    ANN(out);
    uint64_t pos_buf = _scene_visual_desc_resource(emitter, key);
    if (pos_buf == 0)
    {
        if (error != NULL)
            *error = missing_error;
        return false;
    }
    if (!_scene_visual_desc_append_resource(emitter, out, key, missing_error, error))
        return false;

    uint64_t pos_size = _resource_byte_size(&emitter->resources, pos_buf);
    uint64_t vertex_count = (pos_size > 0) ? pos_size / (3 * sizeof(float)) : 3;
    if (vertex_count > UINT32_MAX)
    {
        if (error != NULL)
            *error = "typed visual metadata vertex count exceeds uint32";
        return false;
    }
    out->vertex_count = (uint32_t)vertex_count;
    if (meta->vertex_count > 0)
        out->vertex_count = meta->vertex_count;
    return true;
}



/**
 * Return resource topology metadata for one descriptor resource.
 *
 * @param emitter the persistent emitter
 * @param resource_id the resource id
 * @return topology value, or UINT32_MAX when absent
 */
uint32_t _scene_visual_desc_resource_topology(DvzFramePlanEmitter* emitter, uint64_t resource_id)
{
    ANN(emitter);
    return _resource_topology(&emitter->resources, resource_id);
}



/**
 * Return resource byte size metadata for one descriptor resource.
 *
 * @param emitter the persistent emitter
 * @param resource_id the resource id
 * @return resource byte size
 */
uint64_t _scene_visual_desc_resource_size(DvzFramePlanEmitter* emitter, uint64_t resource_id)
{
    ANN(emitter);
    return _resource_byte_size(&emitter->resources, resource_id);
}



/**
 * Resolve index count and format for a descriptor index buffer.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param overflow_error error when the index count exceeds uint32
 * @param error optional diagnostic output
 * @return whether index metadata was valid
 */
bool _scene_visual_desc_finish_index(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char* overflow_error, const char** error)
{
    ANN(emitter);
    ANN(meta);
    ANN(out);
    uint64_t index_id = out->index_buffer_id;
    uint64_t stride = _resource_item_stride(&emitter->resources, index_id);
    if (index_id != 0 && stride != 0)
    {
        uint64_t index_count = _resource_byte_size(&emitter->resources, index_id) / stride;
        if (index_count > UINT32_MAX)
        {
            if (error != NULL)
                *error = overflow_error;
            return false;
        }
        out->index_count = (uint32_t)index_count;
    }
    if (meta->index_count > 0)
        out->index_count = meta->index_count;
    out->index_format = stride == sizeof(uint16_t) ? "uint16" : "uint32";
    return true;
}



/**
 * Resolve draw-relevant state from typed FramePlan visual metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @return whether a supported visual descriptor was resolved
 */
static bool _scene_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(emitter);
    ANN(meta);
    ANN(out);

    if (error != NULL)
        *error = NULL;

    out->visual_type = meta->visual_type;
    out->depth_test_enabled = meta->depth_test_enabled;
    out->depth_compare_op =
        meta->depth_compare_op != 0 ? meta->depth_compare_op : VK_COMPARE_OP_LESS_OR_EQUAL;
    out->depth_cue_enabled = meta->depth_cue_enabled;
    out->point_style_enabled = meta->point_style_enabled;
    out->scene_occluded = meta->scene_occluded;
    out->scene_occlusion = meta->scene_occlusion;
    out->has_item_range = meta->has_item_range;
    out->item_range_first = meta->item_range_first;
    out->item_range_count = meta->item_range_count;
    out->instance_count = 1;

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops((DvzVisualType)meta->visual_type);
    if (ops == NULL || ops->resolve_desc == NULL)
    {
        if (error != NULL)
            *error = "unsupported typed visual metadata";
        return false;
    }
    return ops->resolve_desc(emitter, meta, out, error);
}



/**
 * Resolve draw-relevant state for one encoded render visual id.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @param visual_index the visual index within the render node
 * @param out the output visual descriptor
 * @return whether a supported visual descriptor was resolved
 */
bool _scene_visual_desc_from_render(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint32_t visual_index,
    DvzSceneVisualDesc* out, const char** error)
{
    ANN(emitter);
    ANN(render);
    ANN(out);
    if (error != NULL)
        *error = NULL;
    dvz_memset(out, sizeof(DvzSceneVisualDesc), 0, sizeof(DvzSceneVisualDesc));
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER ||
        visual_index >= render->u.render.visual_count)
        return false;

    out->depth_test_enabled = true;
    out->depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    out->instance_count = 1;
    const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[visual_index];
    if (meta->has_metadata)
        return _scene_visual_desc_from_metadata(emitter, meta, out, error);
    if (error != NULL)
        *error = "render visual missing typed metadata";
    return false;
}
