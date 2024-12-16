/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Box                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BOX
#define DVZ_HEADER_BOX



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_BOX_NDC                                                                               \
    (DvzBox) { -1, +1, -1, +1, -1, +1 }



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBox DvzBox;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Box flags.
typedef enum
{
    DVZ_BOX_EXTENT_DEFAULT = 0,               // no fixed aspect ratio
    DVZ_BOX_EXTENT_FIXED_ASPECT_EXPAND = 1,   // expand the box to match the aspect ratio
    DVZ_BOX_EXTENT_FIXED_ASPECT_CONTRACT = 2, // contract the box to match the aspect ratio
} DvzBoxExtentStrategy;



// Box merge flags.
typedef enum
{
    DVZ_BOX_MERGE_DEFAULT = 0, // take extrema of input boxes
    DVZ_BOX_MERGE_CENTER = 1,  // merged is centered around 0 and encompasses all input boxes
    DVZ_BOX_MERGE_CORNER = 2,  // merged has (0,0,0) in its lower left corner
} DvzBoxMergeStrategy;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBox
{
    double xmin, xmax, ymin, ymax, zmin, zmax;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  General functions */
/*************************************************************************************************/

/**
 * Create a box.
 *
 * @param xmin minimum x value
 * @param xmax maximum x value
 * @param ymin minimum y value
 * @param ymax maximum y value
 * @param zmin minimum z value
 * @param zmax maximum z value
 * @returns the box
 */
DVZ_EXPORT DvzBox
dvz_box(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);



/**
 * Return the aspect ratio of a box.
 *
 * @param box the box
 * @returns the aspect ratio width/height
 */
DVZ_EXPORT double dvz_box_aspect(DvzBox box);



/**
 * Return the box center.
 *
 * @param box the box
 * @param[out] the box's center
 */
DVZ_EXPORT void dvz_box_center(DvzBox box, dvec3 center);



/**
 * Return the extent of a box, in the same coordinate system, depending on the aspect ratio.
 * This will return the same box if the aspect ratio is unconstrained.
 *
 * @param box the original box
 * @param width the viewport width
 * @param height the viewport height
 * @param strategy indicates how the extent box should be computed
 * @returns the extent box
 */
DVZ_EXPORT
DvzBox dvz_box_extent(DvzBox box, float width, float height, DvzBoxExtentStrategy strategy);



/**
 * Merge a number of boxes into a single box.
 *
 * @param box_count the number of boxes to merge
 * @param boxes the boxes to merge
 * @param strategy the merge strategy
 * @returns the merged box
 */
DVZ_EXPORT DvzBox dvz_box_merge(uint32_t box_count, DvzBox* boxes, DvzBoxMergeStrategy strategy);



/**
 * Normalize 3D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out pointer to an array with the normalized positions to compute (single precision)
 */
DVZ_EXPORT void
dvz_box_normalize(DvzBox source, DvzBox target, uint32_t count, dvec3* pos, vec3* out);



/**
 * Normalize 2D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out pointer to an array with the normalized positions to compute (single precision)
 */
DVZ_EXPORT void
dvz_box_normalize_2D(DvzBox source, DvzBox target, uint32_t count, dvec2* pos, vec3* out);



/**
 * Perform an inverse transformation of a position from a target box to a source box.
 */
DVZ_EXPORT void dvz_box_inverse(DvzBox source, DvzBox target, vec3 pos, dvec3* out);



EXTERN_C_OFF

#endif
