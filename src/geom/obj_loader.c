/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Wavefront OBJ geometry loader. */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "datoviz/geom.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GEOMETRY_OBJ_DESC_KNOWN_FLAGS 0u
#define OBJ_LINE_MAX 2048
#define OBJ_FACE_MAX 128



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ObjFaceVertex
{
    uint32_t position;
    uint32_t normal;
} ObjFaceVertex;



typedef struct ObjArrays
{
    dvec3* positions;
    uint32_t position_count;
    uint32_t position_capacity;

    dvec3* normals;
    uint32_t normal_count;
    uint32_t normal_capacity;

    ObjFaceVertex* vertices;
    uint32_t vertex_count;
    uint32_t vertex_capacity;
} ObjArrays;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static char* _obj_strtok(char* str, const char* delimiters, char** context)
{
#if defined(_WIN32) || defined(_MSC_VER)
    return strtok_s(str, delimiters, context);
#else
    return strtok_r(str, delimiters, context);
#endif
}



static bool _obj_desc_validate(const DvzGeometryObjDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryObjDesc, DVZ_GEOMETRY_OBJ_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryObjDesc ABI prologue");
        return false;
    }
    return true;
}



static void _obj_color_or_default(const DvzColor color, DvzColor* out)
{
    ANN(out);
    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0)
        *out = dvz_color_rgb(255, 255, 255);
    else
        *out = color;
}



static bool _obj_reserve(void** data, uint32_t* capacity, uint32_t count, DvzSize item_size)
{
    ANN(data);
    ANN(capacity);

    if (count <= *capacity)
        return true;
    uint32_t next = *capacity == 0 ? 16u : *capacity;
    while (next < count)
    {
        if (next > UINT32_MAX / 2u)
            return false;
        next *= 2u;
    }
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows((uint64_t)next, (uint64_t)item_size, &bytes))
        return false;

    void* resized = dvz_realloc(*data, (DvzSize)bytes);
    if (resized == NULL)
        return false;
    *data = resized;
    *capacity = next;
    return true;
}



static bool _obj_append_dvec3(dvec3** data, uint32_t* count, uint32_t* capacity, const dvec3 value)
{
    ANN(data);
    ANN(count);
    ANN(capacity);
    ANN(value);

    if (!_obj_reserve((void**)data, capacity, *count + 1u, sizeof(dvec3)))
        return false;
    (*data)[*count][0] = value[0];
    (*data)[*count][1] = value[1];
    (*data)[*count][2] = value[2];
    (*count)++;
    return true;
}



static bool _obj_append_face_vertex(ObjArrays* arrays, ObjFaceVertex value)
{
    ANN(arrays);
    if (!_obj_reserve(
            (void**)&arrays->vertices, &arrays->vertex_capacity, arrays->vertex_count + 1u,
            sizeof(ObjFaceVertex)))
    {
        return false;
    }
    arrays->vertices[arrays->vertex_count++] = value;
    return true;
}



static bool _obj_index_from_token(long raw, uint32_t count, uint32_t* out)
{
    ANN(out);
    if (raw == 0 || count == 0)
        return false;
    long index = raw > 0 ? raw - 1 : (long)count + raw;
    if (index < 0 || (uint64_t)index >= count)
        return false;
    *out = (uint32_t)index;
    return true;
}



static bool _obj_parse_face_vertex(
    const char* token, uint32_t position_count, uint32_t normal_count, ObjFaceVertex* out)
{
    ANN(token);
    ANN(out);

    errno = 0;
    char* end = NULL;
    const long position = strtol(token, &end, 10);
    if (errno != 0 || end == token || !_obj_index_from_token(position, position_count, &out->position))
        return false;
    out->normal = UINT32_MAX;

    if (*end == '\0')
        return true;
    if (*end != '/')
        return false;
    end++;

    if (*end != '/' && *end != '\0')
    {
        (void)strtol(end, &end, 10);
        if (errno != 0)
            return false;
    }
    if (*end == '/')
    {
        end++;
        if (*end != '\0')
        {
            errno = 0;
            char* normal_end = NULL;
            const long normal = strtol(end, &normal_end, 10);
            if (
                errno != 0 || normal_end == end ||
                !_obj_index_from_token(normal, normal_count, &out->normal))
            {
                return false;
            }
            end = normal_end;
        }
    }
    return *end == '\0';
}



static bool _obj_parse_face(char* line, ObjArrays* arrays)
{
    ANN(line);
    ANN(arrays);

    ObjFaceVertex face[OBJ_FACE_MAX] = {{0}};
    uint32_t face_count = 0;
    char* saveptr = NULL;
    for (char* token = _obj_strtok(line, " \t\r\n", &saveptr); token != NULL;
         token = _obj_strtok(NULL, " \t\r\n", &saveptr))
    {
        if (face_count >= OBJ_FACE_MAX)
            return false;
        if (!_obj_parse_face_vertex(
                token, arrays->position_count, arrays->normal_count, &face[face_count]))
        {
            return false;
        }
        face_count++;
    }
    if (face_count < 3u)
        return false;

    for (uint32_t i = 1; i + 1u < face_count; i++)
    {
        if (!_obj_append_face_vertex(arrays, face[0]) ||
            !_obj_append_face_vertex(arrays, face[i]) ||
            !_obj_append_face_vertex(arrays, face[i + 1u]))
        {
            return false;
        }
    }
    return true;
}



static void _obj_arrays_destroy(ObjArrays* arrays)
{
    if (arrays == NULL)
        return;
    dvz_free(arrays->positions);
    dvz_free(arrays->normals);
    dvz_free(arrays->vertices);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzGeometryObjDesc dvz_geometry_obj_desc(void)
{
    return (DvzGeometryObjDesc){DVZ_STRUCT_INIT_FIELDS(DvzGeometryObjDesc)};
}



DvzGeometry* dvz_geom_obj(const char* filename, const DvzGeometryObjDesc* desc)
{
    if (filename == NULL || !_obj_desc_validate(desc))
        return NULL;

    DvzGeometryObjDesc cfg = dvz_geometry_obj_desc();
    if (desc != NULL)
        cfg = *desc;
    DvzColor color = {0};
    _obj_color_or_default(cfg.color, &color);

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        log_error("could not open OBJ file %s", filename);
        return NULL;
    }

    ObjArrays arrays = {0};
    bool ok = true;
    char line[OBJ_LINE_MAX] = {0};
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == 'v' && line[1] == ' ')
        {
            dvec3 value = {0};
            if (sscanf(line + 2, "%lf %lf %lf", &value[0], &value[1], &value[2]) != 3 ||
                !_obj_append_dvec3(
                    &arrays.positions, &arrays.position_count, &arrays.position_capacity, value))
            {
                ok = false;
                break;
            }
        }
        else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
        {
            dvec3 value = {0};
            if (sscanf(line + 3, "%lf %lf %lf", &value[0], &value[1], &value[2]) != 3 ||
                !_obj_append_dvec3(
                    &arrays.normals, &arrays.normal_count, &arrays.normal_capacity, value))
            {
                ok = false;
                break;
            }
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            if (!_obj_parse_face(line + 2, &arrays))
            {
                ok = false;
                break;
            }
        }
    }
    fclose(fp);

    if (!ok || arrays.position_count == 0 || arrays.vertex_count == 0)
    {
        _obj_arrays_destroy(&arrays);
        return NULL;
    }

    DvzGeometry* geometry = dvz_geometry(arrays.vertex_count, arrays.vertex_count);
    if (geometry == NULL)
    {
        _obj_arrays_destroy(&arrays);
        return NULL;
    }
    geometry->type = DVZ_GEOMETRY_CUSTOM;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    bool has_all_normals = true;
    for (uint32_t i = 0; i < arrays.vertex_count; i++)
    {
        const ObjFaceVertex face = arrays.vertices[i];
        if (face.position >= arrays.position_count)
        {
            dvz_geometry_destroy(geometry);
            _obj_arrays_destroy(&arrays);
            return NULL;
        }
        geometry->positions[i][0] = arrays.positions[face.position][0];
        geometry->positions[i][1] = arrays.positions[face.position][1];
        geometry->positions[i][2] = arrays.positions[face.position][2];
        geometry->colors[i] = color;
        geometry->texcoords[i][0] = 0.0;
        geometry->texcoords[i][1] = 0.0;
        geometry->indices[i] = i;
        if (face.normal != UINT32_MAX && face.normal < arrays.normal_count)
        {
            geometry->normals[i][0] = arrays.normals[face.normal][0];
            geometry->normals[i][1] = arrays.normals[face.normal][1];
            geometry->normals[i][2] = arrays.normals[face.normal][2];
        }
        else
        {
            has_all_normals = false;
        }
    }

    _obj_arrays_destroy(&arrays);
    if (!has_all_normals && dvz_geometry_compute_normals(geometry) != 0)
    {
        dvz_geometry_destroy(geometry);
        return NULL;
    }
    return geometry;
}
