/* Copyright (c) 2021 Cyrille Rossant and contributors. MIT license. */

#include "_alloc.h"
#include "_compat.h"
#include "_overflow.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "_visual_pipeline_internal.h"
#include "core/scene_notify_internal.h"
#include "mesh/internal.h"
#include <math.h>
#include <string.h>

/** Validate clip ownership, supported profiles, indices and triangle boundary flags.
 * @param visual the mesh
 * @param clip copied or proposed clip data
 * @param count vertex count
 * @return NULL when valid, otherwise a diagnostic
 */
static const char* _clip_validate(const DvzVisual* visual, const vec2* clip, uint32_t count)
{
    const DvzVisualFamilyState* state = _visual_family_state(visual);
    int pos = _attr_index(visual, "position");
    if (pos < 0 || visual->attrs[pos].item_count != count ||
        visual->attrs[pos].source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
        return "fragment clip requires the exact dense position count";
    if (state->field != NULL || state->texture.rgba != NULL ||
        state->topology != DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ||
        _scene_visual_has_dense_attr(visual, "item_state") ||
        _scene_visual_has_dense_attr(visual, "instance_transform") || visual->scene_occluder ||
        visual->scene_occluded ||
        (visual->alpha_mode != DVZ_ALPHA_OPAQUE && visual->alpha_mode != DVZ_ALPHA_BLENDED &&
         visual->alpha_mode != DVZ_ALPHA_WBOIT))
        return "fragment clip requires ordinary untextured noninstanced triangles in OPAQUE, "
               "BLENDED or WBOIT";
    const DvzSceneBuffer* index = state->buffer;
    uint64_t indices = count;
    if (index != NULL)
    {
        if (index->data == NULL || index->scene != visual->scene ||
            (index->desc.usage & DVZ_SCENE_BUFFER_USAGE_STORAGE) != 0 ||
            (index->desc.stride != 2 && index->desc.stride != 4) ||
            index->desc.byte_size % index->desc.stride != 0)
            return "fragment clip requires retained CPU-backed non-storage u16/u32 indices";
        indices = index->desc.byte_size / index->desc.stride;
    }
    if (indices == 0 || indices % 3 != 0)
        return "fragment clip requires complete triangle indices";
    for (uint64_t i = 0; i < indices; i += 3)
    {
        float boundary = -1;
        for (uint32_t j = 0; j < 3; j++)
        {
            uint32_t v = (uint32_t)(i + j);
            if (index != NULL)
            {
                if (index->desc.stride == 2)
                {
                    uint16_t value = 0;
                    memcpy(&value, (const uint8_t*)index->data + 2 * (i + j), 2);
                    v = value;
                }
                else
                    memcpy(&v, (const uint8_t*)index->data + 4 * (i + j), 4);
            }
            if (v >= count)
                return "fragment clip triangle index exceeds vertex count";
            if (j > 0 && clip[v][1] != boundary)
                return "fragment clip include-zero flags must be constant within each triangle";
            boundary = clip[v][1];
        }
    }
    return NULL;
}

/** Resolve and cache authored clip validation without scanning triangles on state updates.
 * @param visual the mesh
 * @return NULL when valid, otherwise a diagnostic
 */
const char* _scene_mesh_fragment_clip_error(const DvzVisual* visual)
{
    int idx = _attr_index(visual, "fragment_clip");
    if (idx < 0 || visual->attrs[idx].data == NULL)
        return NULL;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    DvzVisualFamilyState* state = _visual_family_state(visual);
    const DvzSceneBuffer* index = state->buffer;
    int pos = _attr_index(visual, "position");
    uint64_t key[6] = {
        attr->version,
        index != NULL ? index->id : 0,
        index != NULL ? index->content_revision : 0,
        index != NULL ? index->extent_revision : 0,
        index != NULL ? index->lifecycle_revision : 0,
        pos >= 0 ? visual->attrs[pos].item_count : 0};
    // Profile checks remain live even when geometry has not changed.
    if (pos < 0 || visual->attrs[pos].source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM ||
        (index != NULL && (index->data == NULL || index->scene != visual->scene ||
                           (index->desc.usage & DVZ_SCENE_BUFFER_USAGE_STORAGE) != 0)))
        return "fragment clip requires retained dense positions and CPU-backed non-storage "
               "indices";
    if (state->field != NULL || state->texture.rgba != NULL ||
        state->topology != DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ||
        _scene_visual_has_dense_attr(visual, "item_state") ||
        _scene_visual_has_dense_attr(visual, "instance_transform") || visual->scene_occluder ||
        visual->scene_occluded ||
        (visual->alpha_mode != DVZ_ALPHA_OPAQUE && visual->alpha_mode != DVZ_ALPHA_BLENDED &&
         visual->alpha_mode != DVZ_ALPHA_WBOIT))
        return "fragment clip profile is unsupported";
    if (memcmp(key, state->fragment_clip_validation, sizeof(key)) == 0)
        return NULL;
    const char* error = _clip_validate(visual, attr->data, attr->item_count);
    if (error == NULL)
        memcpy(state->fragment_clip_validation, key, sizeof(key));
    return error;
}

/** Set copied generic per-vertex signed fragment coverage.
 * @param mesh the mesh
 * @param clip distance and exact zero-inclusion flag, or NULL to clear
 * @param vertex_count exact position count, or zero to clear
 * @return DVZ_OK on success
 */
DvzResult dvz_mesh_set_fragment_clip(DvzVisual* mesh, const vec2* clip, uint32_t vertex_count)
{
    uint64_t bytes = 0;
    if (mesh == NULL || mesh->type != DVZ_VISUAL_TYPE_MESH || mesh->scene == NULL ||
        !_scene_visual_mutation_allowed(mesh->scene, "set fragment clip") ||
        (clip == NULL) != (vertex_count == 0) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec2), &bytes) || bytes > SIZE_MAX ||
        (mesh->scene->caps.max_buffer_size > 0 && bytes > mesh->scene->caps.max_buffer_size))
        return DVZ_ERROR;
    if (clip != NULL)
    {
        for (uint32_t i = 0; i < vertex_count; i++)
            if (!isfinite(clip[i][0]) || !isfinite(clip[i][1]) ||
                (clip[i][1] != 0 && clip[i][1] != 1))
                return DVZ_ERROR;
        if (_clip_validate(mesh, clip, vertex_count) != NULL)
            return DVZ_ERROR;
    }
    void* copy = bytes > 0 ? dvz_malloc(bytes) : NULL;
    if (bytes > 0 && copy == NULL)
        return DVZ_ERROR;
    if (bytes > 0)
        memcpy(copy, clip, bytes);
    DvzVisualAttr* attr = _attr_get_or_create(mesh, "fragment_clip", sizeof(vec2));
    if (attr == NULL)
    {
        dvz_free(copy);
        return DVZ_ERROR;
    }
    dvz_free(attr->data);
    attr->data = copy;
    attr->buffer = NULL;
    attr->source = DVZ_VISUAL_ATTR_SOURCE_PER_ITEM;
    attr->buffer_byte_offset = 0;
    attr->item_count = vertex_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = vertex_count;
    _visual_bump_version(&attr->version);
    memset(
        _visual_family_state(mesh)->fragment_clip_validation, 0,
        sizeof(_visual_family_state(mesh)->fragment_clip_validation));
    _scene_notify_visual_changed(mesh);
    return DVZ_OK;
}
