/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Box                                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzBox dvz_box(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
    return (DvzBox){
        .xmin = xmin,
        .xmax = xmax,
        .ymin = ymin,
        .ymax = ymax,
        .zmin = zmin,
        .zmax = zmax,
    };
}



double dvz_box_aspect(DvzBox box)
{
    return box.ymin != box.ymax ? (box.xmax - box.xmin) / (box.ymax - box.ymin) : 0;
}



void dvz_box_center(DvzBox box, dvec3 center)
{
    center[0] = .5 * (box.xmin + box.xmax);
    center[1] = .5 * (box.ymin + box.ymax);
    center[2] = .5 * (box.zmin + box.zmax);
}



DvzBox dvz_box_extent(DvzBox box, float width, float height, DvzBoxExtentStrategy strategy)
{
    if (height <= 0)
        return box;
    ASSERT(height > 0);

    // return the extent of a box, especially if the input box aspect ratio should be fixed. There
    // may be multiple strategies to get the box extent (in output) if the aspect ratio does not
    // match. We may want the extent to contain the input box so that its aspect ratio is fixed, or
    // to contract it

    // Calculate the aspect ratio of the specified dimensions
    double target_aspect_ratio = width / height;

    // Calculate the current dimensions of the box
    double box_width = box.xmax - box.xmin;
    double box_height = box.ymax - box.ymin;

    // Calculate the center of the box
    double center_x = (box.xmin + box.xmax) / 2.0;
    double center_y = (box.ymin + box.ymax) / 2.0;

    // Calculate the current aspect ratio of the box
    double box_aspect_ratio = box_width / box_height;

    // Determine whether to extend the width or height
    double new_width, new_height;
    if (box_aspect_ratio > target_aspect_ratio)
    {
        // Extend height to match the target aspect ratio
        new_width = box_width;
        new_height = box_width / target_aspect_ratio;
    }
    else
    {
        // Extend width to match the target aspect ratio
        new_height = box_height;
        new_width = box_height * target_aspect_ratio;
    }

    // Calculate the new boundaries while keeping the center unchanged
    double new_xmin = center_x - new_width / 2.0;
    double new_xmax = center_x + new_width / 2.0;
    double new_ymin = center_y - new_height / 2.0;
    double new_ymax = center_y + new_height / 2.0;

    // Return the new box
    return (DvzBox){
        .xmin = new_xmin,
        .xmax = new_xmax,
        .ymin = new_ymin,
        .ymax = new_ymax,
        .zmin = box.zmin,
        .zmax = box.zmax,
    };
}



DvzBox dvz_box_merge(uint32_t box_count, DvzBox* boxes, DvzBoxMergeStrategy strategy)
{
    if (box_count == 0)
    {
        return DVZ_BOX_NDC;
    };

    ASSERT(box_count > 0);
    ANN(boxes);

    DvzBox merged = {
        .xmin = +INFINITY,
        .xmax = -INFINITY,
        .ymin = +INFINITY,
        .ymax = -INFINITY,
        .zmin = +INFINITY,
        .zmax = -INFINITY,
    };

    // Merged box.
    for (uint32_t i = 0; i < box_count; i++)
    {
        merged.xmin = MIN(merged.xmin, boxes[i].xmin);
        merged.xmax = MAX(merged.xmax, boxes[i].xmax);
        merged.ymin = MIN(merged.ymin, boxes[i].ymin);
        merged.ymax = MAX(merged.ymax, boxes[i].ymax);
        merged.zmin = MIN(merged.zmin, boxes[i].zmin);
        merged.zmax = MAX(merged.zmax, boxes[i].zmax);
    }

    // Center merge strategy.
    if (strategy == DVZ_BOX_MERGE_CENTER)
    {
        merged.xmax = MAX(fabs(merged.xmin), fabs(merged.xmax));
        merged.ymax = MAX(fabs(merged.ymin), fabs(merged.ymax));
        merged.zmax = MAX(fabs(merged.zmin), fabs(merged.zmax));
        merged.xmin = -merged.xmax;
        merged.ymin = -merged.ymax;
        merged.zmin = -merged.zmax;
    }

    // Corner merge strategy.
    if (strategy == DVZ_BOX_MERGE_CORNER)
    {
        merged.xmin = 0;
        merged.ymin = 0;
        merged.zmin = 0;
    }

    return merged;
}



void dvz_box_normalize_1D(
    DvzBox source, DvzBox target, DvzDim dim, uint32_t count, double* pos, vec3* out)
{
    ANN(pos);
    ANN(out);
    ASSERT(dim < DVZ_DIM_COUNT);

    double scale = 0;
    double source_min = 0;
    double target_min = 0;
    if (dim == DVZ_DIM_X)
    {
        scale = source.xmax != source.xmin
                    ? (target.xmax - target.xmin) / (source.xmax - source.xmin)
                    : 1;
        source_min = source.xmin;
        target_min = target.xmin;
    }
    else if (dim == DVZ_DIM_Y)
    {
        scale = source.ymax != source.ymin
                    ? (target.ymax - target.ymin) / (source.ymax - source.ymin)
                    : 1;
        source_min = source.ymin;
        target_min = target.ymin;
    }
    else if (dim == DVZ_DIM_Z)
    {
        scale = source.zmax != source.zmin
                    ? (target.zmax - target.zmin) / (source.zmax - source.zmin)
                    : 1;
        source_min = source.zmin;
        target_min = target.zmin;
    }

#if HAS_OPENMP
#pragma omp parallel for
#endif
    for (uint32_t i = 0; i < count; i++)
    {
        out[i][dim] = (float)((pos[i] - source_min) * scale + target_min);
    }
}



void dvz_box_normalize2D(DvzBox source, DvzBox target, uint32_t count, dvec2* pos, vec3* out)
{
    ANN(pos);
    ANN(out);

    double scale_x =
        source.xmax != source.xmin ? (target.xmax - target.xmin) / (source.xmax - source.xmin) : 1;
    double scale_y =
        source.ymax != source.ymin ? (target.ymax - target.ymin) / (source.ymax - source.ymin) : 1;

#if HAS_OPENMP
#pragma omp parallel for
#endif
    for (uint32_t i = 0; i < count; i++)
    {
        out[i][0] = (float)((pos[i][0] - source.xmin) * scale_x + target.xmin);
        out[i][1] = (float)((pos[i][1] - source.ymin) * scale_y + target.ymin);
    }
}



void dvz_box_normalize_3D(DvzBox source, DvzBox target, uint32_t count, dvec3* pos, vec3* out)
{
    ANN(pos);
    ANN(out);

    double scale_x =
        source.xmax != source.xmin ? (target.xmax - target.xmin) / (source.xmax - source.xmin) : 1;
    double scale_y =
        source.ymax != source.ymin ? (target.ymax - target.ymin) / (source.ymax - source.ymin) : 1;
    double scale_z =
        source.zmax != source.zmin ? (target.zmax - target.zmin) / (source.zmax - source.zmin) : 1;

#if HAS_OPENMP
#pragma omp parallel for
#endif
    for (uint32_t i = 0; i < count; i++)
    {
        out[i][0] = (float)((pos[i][0] - source.xmin) * scale_x + target.xmin);
        out[i][1] = (float)((pos[i][1] - source.ymin) * scale_y + target.ymin);
        out[i][2] = (float)((pos[i][2] - source.zmin) * scale_z + target.zmin);
    }
}



void dvz_box_inverse(DvzBox source, DvzBox target, vec3 pos, dvec3* out)
{
    ANN(pos);

    double scale_x =
        source.xmax != source.xmin ? (target.xmax - target.xmin) / (source.xmax - source.xmin) : 1;
    double scale_y =
        source.ymax != source.ymin ? (target.ymax - target.ymin) / (source.ymax - source.ymin) : 1;
    double scale_z =
        source.zmax != source.zmin ? (target.zmax - target.zmin) / (source.zmax - source.zmin) : 1;

    (*out)[0] = (pos[0] - target.xmin) / scale_x + source.xmin;
    (*out)[1] = (pos[1] - target.ymin) / scale_y + source.ymin;
    (*out)[2] = (pos[2] - target.zmin) / scale_z + source.zmin;
}



void dvz_box_print(DvzBox box)
{
    printf(
        "Box: x:%.3f..%.3f, y:%.3f..%.3f, z:%.3f..%.3f\n", //
        box.xmin, box.xmax, box.ymin, box.ymax, box.zmin, box.zmax);
}
