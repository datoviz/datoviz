/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mesh geometry upload                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/scene.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a count-sized allocation is representable.
 *
 * @param count number of items
 * @param item_size item size in bytes
 * @return whether the allocation size is valid
 */
static bool _mesh_geometry_allocation_valid(uint32_t count, DvzSize item_size)
{
    uint64_t total = 0;
    return !_dvz_mul_u64_overflows((uint64_t)count, (uint64_t)item_size, &total);
}



/**
 * Return whether a double value can be represented as a finite float.
 *
 * @param value value to check
 * @return whether the value is finite and in float range
 */
static bool _mesh_geometry_f32_valid(double value)
{
    return isfinite(value) && value >= -(double)FLT_MAX && value <= (double)FLT_MAX;
}



/**
 * Return whether one geometry position array is valid for upload.
 *
 * @param values flattened F64 3-vector array
 * @param count number of items
 * @return whether all values are valid
 */
static bool _mesh_geometry_dvec3_valid(const double* values, uint32_t count)
{
    if (values == NULL)
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            if (!_mesh_geometry_f32_valid(values[(size_t)3 * i + j]))
                return false;
        }
    }
    return true;
}



/**
 * Return whether one optional geometry texcoord array is valid for upload.
 *
 * @param values flattened F64 2-vector array
 * @param count number of items
 * @return whether all values are valid
 */
static bool _mesh_geometry_dvec2_valid(const double* values, uint32_t count)
{
    if (values == NULL)
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = 0; j < 2; j++)
        {
            if (!_mesh_geometry_f32_valid(values[(size_t)2 * i + j]))
                return false;
        }
    }
    return true;
}



/**
 * Copy a F64 3-vector array into a F32 3-vector array.
 *
 * @param out output F32 vectors
 * @param in flattened input F64 vectors
 * @param count number of items
 */
static void _mesh_geometry_copy_dvec3(vec3* out, const double* in, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        out[i][0] = (float)in[(size_t)3 * i + 0];
        out[i][1] = (float)in[(size_t)3 * i + 1];
        out[i][2] = (float)in[(size_t)3 * i + 2];
    }
}



/**
 * Copy a F64 2-vector array into a F32 2-vector array.
 *
 * @param out output F32 vectors
 * @param in flattened input F64 vectors
 * @param count number of items
 */
static void _mesh_geometry_copy_dvec2(vec2* out, const double* in, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        out[i][0] = (float)in[(size_t)2 * i + 0];
        out[i][1] = (float)in[(size_t)2 * i + 1];
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Upload a CPU geometry object into a mesh visual.
 *
 * @param visual the mesh visual
 * @param geometry the CPU geometry object
 * @return 0 on success, -1 on invalid input or upload failure
 */
int dvz_mesh_set_geometry(DvzVisual* visual, const DvzGeometry* geometry)
{
    if (
        visual == NULL || visual->type != DVZ_VISUAL_TYPE_MESH || geometry == NULL ||
        geometry->vertex_count == 0 || geometry->positions == NULL)
    {
        return -1;
    }

    const uint32_t vertex_count = geometry->vertex_count;
    if (!_mesh_geometry_allocation_valid(vertex_count, sizeof(vec3)) ||
        !_mesh_geometry_allocation_valid(vertex_count, sizeof(vec2)))
    {
        return -1;
    }

    if (!_mesh_geometry_dvec3_valid(&geometry->positions[0][0], vertex_count))
        return -1;
    if (
        geometry->normals != NULL &&
        !_mesh_geometry_dvec3_valid(&geometry->normals[0][0], vertex_count))
        return -1;
    if (
        geometry->texcoords != NULL &&
        !_mesh_geometry_dvec2_valid(&geometry->texcoords[0][0], vertex_count))
        return -1;

    if (geometry->index_count > 0)
    {
        if (
            geometry->indices == NULL || geometry->index_count % 3 != 0 ||
            !_mesh_geometry_allocation_valid(geometry->index_count, sizeof(DvzIndex)))
        {
            return -1;
        }
        for (uint32_t i = 0; i < geometry->index_count; i++)
        {
            if (geometry->indices[i] >= vertex_count)
                return -1;
        }
    }

    vec3* positions = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
    vec3* normals = NULL;
    vec2* texcoords = NULL;
    if (positions == NULL)
        return -1;

    int out = -1;
    _mesh_geometry_copy_dvec3(positions, &geometry->positions[0][0], vertex_count);

    if (geometry->normals != NULL)
    {
        normals = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
        if (normals == NULL)
            goto cleanup;
        _mesh_geometry_copy_dvec3(normals, &geometry->normals[0][0], vertex_count);
    }

    if (geometry->texcoords != NULL)
    {
        texcoords = (vec2*)dvz_calloc(vertex_count, sizeof(vec2));
        if (texcoords == NULL)
            goto cleanup;
        _mesh_geometry_copy_dvec2(texcoords, &geometry->texcoords[0][0], vertex_count);
    }

    DvzVisualDataUpdate updates[4] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
    };
    uint32_t update_count = 1;

    if (geometry->colors != NULL)
    {
        updates[update_count++] = (DvzVisualDataUpdate){
            .attr_name = "color", .data = geometry->colors, .item_count = vertex_count};
    }
    if (normals != NULL)
    {
        updates[update_count++] = (DvzVisualDataUpdate){
            .attr_name = "normal", .data = normals, .item_count = vertex_count};
    }
    if (texcoords != NULL)
    {
        updates[update_count++] = (DvzVisualDataUpdate){
            .attr_name = "texcoords", .data = texcoords, .item_count = vertex_count};
    }

    if (dvz_visual_set_data_many(visual, updates, update_count) != 0)
        goto cleanup;
    if (
        geometry->index_count > 0 &&
        dvz_visual_set_index_data(visual, geometry->indices, geometry->index_count) != 0)
    {
        goto cleanup;
    }

    out = 0;

cleanup:
    dvz_free(positions);
    dvz_free(normals);
    dvz_free(texcoords);
    return out;
}
