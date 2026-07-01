/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual query geometry helpers                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_visual_internal.h"
#include "query_geometry.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one source vertex index from direct or indexed visual geometry.
 *
 * @param visual visual carrying an optional index buffer binding
 * @param draw_index draw-order vertex index
 * @param vertex_count source vertex count
 * @param out_source_index output source vertex index
 * @return true when the source index is valid
 */
static bool _query_geometry_source_vertex_index(
    const DvzVisual* visual, uint64_t draw_index, uint64_t vertex_count,
    uint64_t* out_source_index)
{
    ANN(visual);
    ANN(out_source_index);
    uint64_t source_index = draw_index;
    if (_visual_family_state(visual)->buffer != NULL && _visual_family_state(visual)->buffer->data != NULL)
    {
        uint32_t stride = _visual_family_state(visual)->buffer->desc.stride;
        uint64_t offset = 0;
        if (
            stride == 0 ||
            _dvz_mul_u64_overflows(draw_index, stride, &offset) ||
            offset > _visual_family_state(visual)->buffer->desc.byte_size ||
            stride > _visual_family_state(visual)->buffer->desc.byte_size - offset)
        {
            return false;
        }
        const uint8_t* data = (const uint8_t*)_visual_family_state(visual)->buffer->data + offset;
        if (stride == sizeof(uint16_t))
        {
            uint16_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else if (stride == sizeof(uint32_t))
        {
            uint32_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else
            return false;
    }
    if (source_index >= vertex_count)
        return false;
    *out_source_index = source_index;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one retained visual attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
bool _dvz_scene_query_dense_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    ANN(out_attr);
    int attr_idx = _attr_index(visual, attr_name);
    if (attr_idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[attr_idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    *out_attr = attr;
    return true;
}



/**
 * Build temporary query buffers for a primitive-topology visual.
 *
 * @param label diagnostic query family label
 * @param visual retained visual with dense position and optional index buffer
 * @param scratch output scratch storage
 * @param out_vertex_count output derived vertex count
 * @param out_topology output Vulkan draw topology
 * @return true when derived query buffers were created
 */
bool _scene_query_indexed_primitive_geometry(
    const char* label, const DvzVisual* visual, DvzSceneQueryScratch* scratch,
    uint64_t* out_vertex_count, uint32_t* out_topology)
{
    ANN(label);
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);
    ANN(out_topology);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_dvz_scene_query_dense_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;
    uint64_t vertex_count = pos_attr->item_count;

    uint64_t source_index_count = vertex_count;
    if (_visual_family_state(visual)->buffer != NULL && _visual_family_state(visual)->buffer->data != NULL &&
        _visual_family_state(visual)->buffer->desc.byte_size > 0 && _visual_family_state(visual)->buffer->desc.stride > 0)
    {
        uint32_t stride = _visual_family_state(visual)->buffer->desc.stride;
        if (stride != sizeof(uint16_t) && stride != sizeof(uint32_t))
        {
            log_error("%s query request index stride must be 16-bit or 32-bit", label);
            return false;
        }
        if (_visual_family_state(visual)->buffer->desc.byte_size % stride != 0)
        {
            log_error("%s query request index buffer size is not stride-aligned", label);
            return false;
        }
        source_index_count = _visual_family_state(visual)->buffer->desc.byte_size / stride;
    }

    uint64_t primitive_count = 0;
    uint64_t draw_vertex_count = 0;
    uint32_t draw_topology = (uint32_t)_visual_family_state(visual)->topology;
    switch (_visual_family_state(visual)->topology)
    {
    case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
        primitive_count = source_index_count;
        draw_vertex_count = primitive_count;
        draw_topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
        primitive_count = source_index_count / 2;
        if (_dvz_mul_u64_overflows(primitive_count, 2, &draw_vertex_count))
            return false;
        draw_topology = DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        if (source_index_count < 2)
            return false;
        primitive_count = source_index_count - 1;
        if (_dvz_mul_u64_overflows(primitive_count, 2, &draw_vertex_count))
            return false;
        draw_topology = DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        primitive_count = source_index_count / 3;
        if (_dvz_mul_u64_overflows(primitive_count, 3, &draw_vertex_count))
            return false;
        draw_topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        if (source_index_count < 3)
            return false;
        primitive_count = source_index_count - 2;
        if (_dvz_mul_u64_overflows(primitive_count, 3, &draw_vertex_count))
            return false;
        draw_topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    default:
        return false;
    }
    if (primitive_count == 0 || draw_vertex_count == 0 || draw_vertex_count > UINT32_MAX)
        return false;

    if (!_dvz_scene_query_alloc(
            label, (void**)&scratch->query_positions, draw_vertex_count, sizeof(vec3)) ||
        !_dvz_scene_query_alloc(
            label, (void**)&scratch->query_ids, draw_vertex_count, sizeof(uint32_t)))
    {
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    for (uint64_t prim = 0; prim < primitive_count; prim++)
    {
        uint64_t draw_indices[3] = {0, 0, 0};
        uint32_t prim_vertex_count = 1;
        switch (_visual_family_state(visual)->topology)
        {
        case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
            draw_indices[0] = prim;
            prim_vertex_count = 1;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
            draw_indices[0] = 2 * prim + 0;
            draw_indices[1] = 2 * prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            draw_indices[0] = 3 * prim + 0;
            draw_indices[1] = 3 * prim + 1;
            draw_indices[2] = 3 * prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            draw_indices[0] = 0;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        default:
            _scene_query_scratch_destroy(scratch);
            return false;
        }

        for (uint32_t j = 0; j < prim_vertex_count; j++)
        {
            uint64_t source_index = 0;
            if (!_query_geometry_source_vertex_index(
                    visual, draw_indices[j], vertex_count, &source_index))
            {
                _scene_query_scratch_destroy(scratch);
                return false;
            }
            uint64_t dst = prim * prim_vertex_count + j;
            dvz_memcpy(
                scratch->query_positions[dst], sizeof(vec3), &position[3 * source_index],
                sizeof(vec3));
            scratch->query_ids[dst] = (uint32_t)prim + 1u;
        }
    }

    *out_vertex_count = draw_vertex_count;
    *out_topology = draw_topology;
    return true;
}



/**
 * Build temporary query buffers for mesh object/instance item selection.
 *
 * @param label diagnostic query family label
 * @param visual retained mesh visual
 * @param scratch output scratch storage
 * @param out_vertex_count output derived vertex count
 * @param out_topology output Vulkan draw topology
 * @return true when derived query buffers were created
 */
bool _scene_query_mesh_item_geometry(
    const char* label, const DvzVisual* visual, DvzSceneQueryScratch* scratch,
    uint64_t* out_vertex_count, uint32_t* out_topology)
{
    ANN(label);
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);
    ANN(out_topology);

    if (!_scene_query_indexed_primitive_geometry(
            label, visual, scratch, out_vertex_count, out_topology))
    {
        return false;
    }

    const DvzVisualAttr* transforms = NULL;
    if (!_dvz_scene_query_dense_attr(visual, "instance_transform", 16 * sizeof(float), &transforms))
    {
        for (uint64_t i = 0; i < *out_vertex_count; i++)
            scratch->query_ids[i] = 1u;
        return true;
    }

    uint64_t base_vertex_count = *out_vertex_count;
    uint64_t instance_count = transforms->item_count;
    uint64_t vertex_count = 0;
    if (
        instance_count == 0 || instance_count > UINT32_MAX ||
        _dvz_mul_u64_overflows(base_vertex_count, instance_count, &vertex_count) ||
        vertex_count > UINT32_MAX)
    {
        log_error("%s query request instance-expanded vertex count is invalid", label);
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    vec3* positions = NULL;
    uint32_t* ids = NULL;
    if (!_dvz_scene_query_alloc(label, (void**)&positions, vertex_count, sizeof(vec3)) ||
        !_dvz_scene_query_alloc(label, (void**)&ids, vertex_count, sizeof(uint32_t)))
    {
        dvz_free(positions);
        dvz_free(ids);
        _scene_query_scratch_destroy(scratch);
        return false;
    }

    const float* transform = (const float*)transforms->data;
    for (uint64_t inst = 0; inst < instance_count; inst++)
    {
        const float* mat = &transform[16 * inst];
        for (uint64_t v = 0; v < base_vertex_count; v++)
        {
            const float x = scratch->query_positions[v][0];
            const float y = scratch->query_positions[v][1];
            const float z = scratch->query_positions[v][2];
            float tx = mat[0] * x + mat[4] * y + mat[8] * z + mat[12];
            float ty = mat[1] * x + mat[5] * y + mat[9] * z + mat[13];
            float tz = mat[2] * x + mat[6] * y + mat[10] * z + mat[14];
            float tw = mat[3] * x + mat[7] * y + mat[11] * z + mat[15];
            if (tw != 0.0f)
            {
                tx /= tw;
                ty /= tw;
                tz /= tw;
            }
            uint64_t dst = inst * base_vertex_count + v;
            positions[dst][0] = tx;
            positions[dst][1] = ty;
            positions[dst][2] = tz;
            ids[dst] = (uint32_t)inst + 1u;
        }
    }

    dvz_free(scratch->query_positions);
    dvz_free(scratch->query_ids);
    scratch->query_positions = positions;
    scratch->query_ids = ids;
    *out_vertex_count = vertex_count;
    return true;
}
