/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  SVG tiger prepared-model helpers                                                             */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "svg_tiger_model.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define SVG_TIGER_MAGIC       "DVZSVG1"
#define SVG_TIGER_MAGIC_SIZE  8u
#define SVG_TIGER_VERSION     2u
#define SVG_TIGER_RECORD_SIZE 28u
#define SVG_TIGER_MAX_PATHS   10000u
#define SVG_TIGER_MAX_POINTS  10000000u

#define SVG_TIGER_Z_STEP         0.0025f
#define SVG_TIGER_STROKE_Z_BIAS  0.00075f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SvgTigerFileHeader
{
    char magic[SVG_TIGER_MAGIC_SIZE];
    uint32_t version;
    uint32_t path_count;
    uint32_t point_count;
    uint32_t record_size;
    double width;
    double height;
    double bounds[4];
} SvgTigerFileHeader;


typedef struct SvgTigerFilePath
{
    uint32_t point_offset;
    uint32_t point_count;
    uint8_t closed;
    uint8_t has_fill;
    uint8_t has_stroke;
    uint8_t reserved;
    DvzColor fill;
    DvzColor stroke;
    float stroke_width_px;
    uint32_t paint_order;
} SvgTigerFilePath;


static_assert(sizeof(SvgTigerFileHeader) == 72, "SVG tiger file header layout changed");
static_assert(sizeof(SvgTigerFilePath) == SVG_TIGER_RECORD_SIZE, "SVG tiger path layout changed");



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a file header is valid and bounded.
 *
 * @param header file header
 * @return whether the header may be loaded safely
 */
static bool _header_valid(const SvgTigerFileHeader* header)
{
    if (
        header == NULL ||
        memcmp(header->magic, SVG_TIGER_MAGIC, SVG_TIGER_MAGIC_SIZE) != 0 ||
        header->version != SVG_TIGER_VERSION || header->record_size != SVG_TIGER_RECORD_SIZE ||
        header->path_count == 0 || header->path_count > SVG_TIGER_MAX_PATHS ||
        header->point_count == 0 || header->point_count > SVG_TIGER_MAX_POINTS ||
        !(header->width > 0.0) || !(header->height > 0.0) || !isfinite(header->width) ||
        !isfinite(header->height))
    {
        return false;
    }
    for (uint32_t i = 0; i < 4; i++)
    {
        if (!isfinite(header->bounds[i]))
            return false;
    }
    return header->bounds[0] <= header->bounds[2] && header->bounds[1] <= header->bounds[3];
}



/**
 * Return whether one path record refers to valid points and paint state.
 *
 * @param path path record
 * @param point_count total point count
 * @param path_count total path count
 * @return whether the record is valid
 */
static bool _path_valid(
    const SvgTigerFilePath* path, uint32_t point_count, uint32_t path_count)
{
    if (
        path == NULL || path->closed > 1 || path->has_fill > 1 || path->has_stroke > 1 ||
        path->reserved != 0 || !isfinite(path->stroke_width_px) || path->stroke_width_px < 0.0f ||
        path->paint_order >= path_count)
    {
        return false;
    }
    uint64_t end = 0;
    return path->point_count > 0 &&
           !_dvz_add_u64_overflows(path->point_offset, path->point_count, &end) &&
           end <= point_count;
}



/**
 * Return the visual Z coordinate for one authored path.
 *
 * @param path source path
 * @return Z coordinate preserving document paint order
 */
static float _path_z(const SvgTigerPath* path)
{
    return SVG_TIGER_Z_STEP * (float)path->paint_order;
}



/**
 * Destroy an array of owned geometry parts.
 *
 * @param parts geometry pointer array
 * @param count populated geometry count
 */
static void _geometry_parts_destroy(DvzGeometry** parts, uint32_t count)
{
    if (parts == NULL)
        return;
    for (uint32_t i = 0; i < count; i++)
        dvz_geometry_destroy(parts[i]);
    dvz_free(parts);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Load one prepared SVG tiger bundle.
 *
 * @param path prepared binary path
 * @param out output retained path data
 * @return whether the file was loaded and validated
 */
bool svg_tiger_load(const char* path, SvgTigerData* out)
{
    if (path == NULL || out == NULL)
        return false;
    dvz_memset(out, sizeof(*out), 0, sizeof(*out));

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;

    bool ok = false;
    SvgTigerFileHeader header = {0};
    SvgTigerFilePath* file_paths = NULL;
    if (fread(&header, sizeof(header), 1, fp) != 1 || !_header_valid(&header))
        goto cleanup;

    out->paths = (SvgTigerPath*)dvz_calloc(header.path_count, sizeof(*out->paths));
    out->points = (dvec2*)dvz_calloc(header.point_count, sizeof(*out->points));
    file_paths = (SvgTigerFilePath*)dvz_calloc(header.path_count, sizeof(*file_paths));
    if (out->paths == NULL || out->points == NULL || file_paths == NULL)
        goto cleanup;
    if (fread(file_paths, sizeof(*file_paths), header.path_count, fp) != header.path_count ||
        fread(out->points, sizeof(*out->points), header.point_count, fp) != header.point_count)
    {
        goto cleanup;
    }
    if (fgetc(fp) != EOF)
        goto cleanup;

    for (uint32_t i = 0; i < header.path_count; i++)
    {
        const SvgTigerFilePath* source = &file_paths[i];
        if (
            !_path_valid(source, header.point_count, header.path_count) ||
            source->paint_order != i)
            goto cleanup;
        out->paths[i] = (SvgTigerPath){
            .point_offset = source->point_offset,
            .point_count = source->point_count,
            .closed = source->closed != 0,
            .has_fill = source->has_fill != 0,
            .has_stroke = source->has_stroke != 0,
            .fill = source->fill,
            .stroke = source->stroke,
            .stroke_width_px = source->stroke_width_px,
            .paint_order = source->paint_order,
        };
    }
    for (uint32_t i = 0; i < header.point_count; i++)
    {
        if (!isfinite(out->points[i][0]) || !isfinite(out->points[i][1]))
            goto cleanup;
    }

    out->path_count = header.path_count;
    out->point_count = header.point_count;
    out->width = header.width;
    out->height = header.height;
    for (uint32_t i = 0; i < 4; i++)
        out->bounds[i] = header.bounds[i];
    ok = true;

cleanup:
    dvz_free(file_paths);
    fclose(fp);
    if (!ok)
        svg_tiger_destroy(out);
    return ok;
}



/**
 * Destroy loaded SVG tiger path data.
 *
 * @param data data to reset
 */
void svg_tiger_destroy(SvgTigerData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->paths);
    dvz_free(data->points);
    dvz_memset(data, sizeof(*data), 0, sizeof(*data));
}



/**
 * Triangulate and merge authored fills while preserving paint order in Z.
 *
 * @param data loaded SVG tiger paths
 * @return owned merged geometry, or NULL on failure
 */
DvzGeometry* svg_tiger_fill_geometry(const SvgTigerData* data)
{
    if (data == NULL || data->paths == NULL || data->points == NULL || data->path_count == 0)
        return NULL;

    DvzGeometry** parts = (DvzGeometry**)dvz_calloc(data->path_count, sizeof(*parts));
    if (parts == NULL)
        return NULL;

    uint32_t part_count = 0;
    for (uint32_t i = 0; i < data->path_count; i++)
    {
        const SvgTigerPath* path = &data->paths[i];
        if (!path->has_fill || path->fill.a == 0 || path->point_count < 3)
            continue;

        dvec2* ring = (dvec2*)dvz_calloc(path->point_count, sizeof(*ring));
        if (ring == NULL)
            goto error;
        for (uint32_t j = 0; j < path->point_count; j++)
        {
            const double* point = data->points[path->point_offset + j];
            ring[j][0] = point[0];
            ring[j][1] = data->height - point[1];
        }

        DvzGeometry* geometry = dvz_triangulate_polygon(
            &(DvzPolygonDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                .outer = {.xy = (const dvec2*)ring, .count = path->point_count},
            },
            NULL);
        dvz_free(ring);
        if (geometry == NULL)
        {
            dvz_fprintf(stderr, "svg_tiger: triangulation failed for path %u\n", i);
            goto error;
        }

        const float z = _path_z(path);
        for (uint32_t j = 0; j < geometry->vertex_count; j++)
        {
            geometry->positions[j][2] = z;
            geometry->colors[j] = path->fill;
        }
        parts[part_count++] = geometry;
    }
    if (part_count == 0)
        goto error;

    DvzGeometry* merged = dvz_geometry_merge(part_count, (const DvzGeometry* const*)parts);
    _geometry_parts_destroy(parts, part_count);
    return merged;

error:
    _geometry_parts_destroy(parts, part_count);
    return NULL;
}



/**
 * Build flattened stroke arrays and explicit subpath lengths.
 *
 * @param data loaded SVG tiger paths
 * @param out output stroke arrays
 * @return whether stroke arrays were built
 */
bool svg_tiger_stroke_data(const SvgTigerData* data, SvgTigerStrokeData* out)
{
    if (data == NULL || out == NULL || data->paths == NULL || data->points == NULL)
        return false;
    dvz_memset(out, sizeof(*out), 0, sizeof(*out));

    uint64_t point_count = 0;
    uint64_t subpath_count = 0;
    for (uint32_t i = 0; i < data->path_count; i++)
    {
        const SvgTigerPath* path = &data->paths[i];
        if (!path->has_stroke || path->stroke.a == 0 || path->point_count < 2)
            continue;
        uint64_t next = 0;
        if (_dvz_add_u64_overflows(
                point_count, path->point_count + (path->closed ? 1u : 0u), &next))
            return false;
        point_count = next;
        subpath_count++;
    }
    if (point_count == 0 || point_count > UINT32_MAX || subpath_count > UINT32_MAX)
        return false;

    out->positions = (vec3*)dvz_calloc((uint32_t)point_count, sizeof(*out->positions));
    out->colors = (DvzColor*)dvz_calloc((uint32_t)point_count, sizeof(*out->colors));
    out->widths = (float*)dvz_calloc((uint32_t)point_count, sizeof(*out->widths));
    out->lengths = (uint32_t*)dvz_calloc((uint32_t)subpath_count, sizeof(*out->lengths));
    if (
        out->positions == NULL || out->colors == NULL || out->widths == NULL ||
        out->lengths == NULL)
    {
        svg_tiger_stroke_destroy(out);
        return false;
    }

    uint32_t point_offset = 0;
    uint32_t subpath = 0;
    for (uint32_t i = 0; i < data->path_count; i++)
    {
        const SvgTigerPath* path = &data->paths[i];
        if (!path->has_stroke || path->stroke.a == 0 || path->point_count < 2)
            continue;
        const uint32_t length = path->point_count + (path->closed ? 1u : 0u);
        out->lengths[subpath++] = length;
        const float z = _path_z(path) + SVG_TIGER_STROKE_Z_BIAS;
        for (uint32_t j = 0; j < length; j++)
        {
            const uint32_t source_index = j < path->point_count ? j : 0;
            const double* point = data->points[path->point_offset + source_index];
            const uint32_t target = point_offset + j;
            out->positions[target][0] = (float)point[0];
            out->positions[target][1] = (float)(data->height - point[1]);
            out->positions[target][2] = z;
            out->colors[target] = path->stroke;
            out->widths[target] = path->stroke_width_px;
        }
        point_offset += length;
    }
    out->point_count = point_offset;
    out->subpath_count = subpath;
    if (point_offset != point_count || subpath != subpath_count)
    {
        svg_tiger_stroke_destroy(out);
        return false;
    }
    return true;
}



/**
 * Destroy flattened stroke arrays.
 *
 * @param data stroke arrays to reset
 */
void svg_tiger_stroke_destroy(SvgTigerStrokeData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->positions);
    dvz_free(data->colors);
    dvz_free(data->widths);
    dvz_free(data->lengths);
    dvz_memset(data, sizeof(*data), 0, sizeof(*data));
}
