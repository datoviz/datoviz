/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Curve tessellation                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/geom.h"

#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_log.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_BEZIER_TESSELLATION_DESC_KNOWN_FLAGS 0u
#define DVZ_BEZIER_DEFAULT_SEGMENT_COUNT         32u
#define DVZ_BEZIER_MAX_SEGMENT_COUNT             65535u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _bezier_desc_validate(const DvzBezierTessellationDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzBezierTessellationDesc, DVZ_BEZIER_TESSELLATION_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzBezierTessellationDesc ABI prologue");
        return false;
    }
    if (desc->segment_count > DVZ_BEZIER_MAX_SEGMENT_COUNT)
    {
        log_error("Bezier tessellation segment_count exceeds the supported maximum");
        return false;
    }
    if (desc->tolerance < 0 || !isfinite(desc->tolerance))
    {
        log_error("Bezier tessellation tolerance must be finite and non-negative");
        return false;
    }
    return true;
}



static uint32_t _bezier_segment_count(const DvzBezierTessellationDesc* desc)
{
    if (desc == NULL || desc->segment_count == 0)
        return DVZ_BEZIER_DEFAULT_SEGMENT_COUNT;
    return desc->segment_count;
}



static DvzTessellatedPath* _tessellated_path(uint32_t point_count)
{
    if (point_count < 2)
        return NULL;
    DvzTessellatedPath* path = (DvzTessellatedPath*)dvz_calloc(1, sizeof(DvzTessellatedPath));
    if (path == NULL)
        return NULL;
    path->points = (dvec3*)dvz_calloc(point_count, sizeof(dvec3));
    if (path->points == NULL)
    {
        dvz_free(path);
        return NULL;
    }
    path->point_count = point_count;
    return path;
}



static bool _dvec3_finite(const dvec3 v)
{
    return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}



static void _quadratic_eval(const dvec3 p0, const dvec3 p1, const dvec3 p2, double t, dvec3 out)
{
    const double u = 1.0 - t;
    const double b0 = u * u;
    const double b1 = 2.0 * u * t;
    const double b2 = t * t;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        out[dim] = b0 * p0[dim] + b1 * p1[dim] + b2 * p2[dim];
    }
}



static void
_cubic_eval(const dvec3 p0, const dvec3 p1, const dvec3 p2, const dvec3 p3, double t, dvec3 out)
{
    const double u = 1.0 - t;
    const double b0 = u * u * u;
    const double b1 = 3.0 * u * u * t;
    const double b2 = 3.0 * u * t * t;
    const double b3 = t * t * t;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        out[dim] = b0 * p0[dim] + b1 * p1[dim] + b2 * p2[dim] + b3 * p3[dim];
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default Bezier tessellation descriptor.
 *
 * @return initialized Bezier tessellation descriptor
 */
DvzBezierTessellationDesc dvz_bezier_tessellation_desc(void)
{
    return (DvzBezierTessellationDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzBezierTessellationDesc),
        .segment_count = DVZ_BEZIER_DEFAULT_SEGMENT_COUNT,
        .tolerance = 0,
    };
}



/**
 * Tessellate a quadratic Bezier curve into an owned point path.
 *
 * @param p0 first endpoint
 * @param p1 control point
 * @param p2 second endpoint
 * @param desc optional tessellation descriptor
 * @return the tessellated path, or NULL on invalid input or allocation failure
 */
DvzTessellatedPath* dvz_tessellate_quadratic_bezier(
    const dvec3 p0, const dvec3 p1, const dvec3 p2, const DvzBezierTessellationDesc* desc)
{
    if (!_bezier_desc_validate(desc) || !_dvec3_finite(p0) || !_dvec3_finite(p1) ||
        !_dvec3_finite(p2))
        return NULL;

    const uint32_t segment_count = _bezier_segment_count(desc);
    DvzTessellatedPath* path = _tessellated_path(segment_count + 1);
    if (path == NULL)
        return NULL;

    for (uint32_t i = 0; i < path->point_count; i++)
    {
        const double t = segment_count == 0 ? 0 : (double)i / (double)segment_count;
        _quadratic_eval(p0, p1, p2, t, path->points[i]);
    }
    return path;
}



/**
 * Tessellate a cubic Bezier curve into an owned point path.
 *
 * @param p0 first endpoint
 * @param p1 first control point
 * @param p2 second control point
 * @param p3 second endpoint
 * @param desc optional tessellation descriptor
 * @return the tessellated path, or NULL on invalid input or allocation failure
 */
DvzTessellatedPath* dvz_tessellate_cubic_bezier(
    const dvec3 p0, const dvec3 p1, const dvec3 p2, const dvec3 p3,
    const DvzBezierTessellationDesc* desc)
{
    if (!_bezier_desc_validate(desc) || !_dvec3_finite(p0) || !_dvec3_finite(p1) ||
        !_dvec3_finite(p2) || !_dvec3_finite(p3))
        return NULL;

    const uint32_t segment_count = _bezier_segment_count(desc);
    DvzTessellatedPath* path = _tessellated_path(segment_count + 1);
    if (path == NULL)
        return NULL;

    for (uint32_t i = 0; i < path->point_count; i++)
    {
        const double t = segment_count == 0 ? 0 : (double)i / (double)segment_count;
        _cubic_eval(p0, p1, p2, p3, t, path->points[i]);
    }
    return path;
}



/**
 * Destroy a tessellated path.
 *
 * @param path tessellated path
 */
void dvz_tessellated_path_destroy(DvzTessellatedPath* path)
{
    if (path == NULL)
        return;
    dvz_free(path->points);
    dvz_free(path);
}
