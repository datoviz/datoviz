/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Geometry enums                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Shape type.
typedef enum
{
    DVZ_SHAPE_NONE,
    DVZ_SHAPE_SQUARE,
    DVZ_SHAPE_DISC,
    DVZ_SHAPE_SECTOR,
    DVZ_SHAPE_POLYGON,
    DVZ_SHAPE_HISTOGRAM,
    DVZ_SHAPE_CUBE,
    DVZ_SHAPE_SPHERE,
    DVZ_SHAPE_CYLINDER,
    DVZ_SHAPE_CONE,
    DVZ_SHAPE_TORUS,
    DVZ_SHAPE_ARROW,
    DVZ_SHAPE_TETRAHEDRON,
    DVZ_SHAPE_HEXAHEDRON,
    DVZ_SHAPE_OCTAHEDRON,
    DVZ_SHAPE_DODECAHEDRON,
    DVZ_SHAPE_ICOSAHEDRON,
    DVZ_SHAPE_SURFACE,
    DVZ_SHAPE_OBJ,
    DVZ_SHAPE_OTHER,
} DvzShapeType;



// Indexing flags.
// This indicates how a mesh is being triangulated. This is used to specify predefine contours
// in the mesh visual, when using a DvzShape.
typedef enum
{
    DVZ_INDEXING_NONE = 0x00,    // no indexing
    DVZ_INDEXING_EARCUT = 0x10,  // polygon contour = consecutive indices i..(i+1)
    DVZ_INDEXING_SURFACE = 0x20, // indexing of mesh grid for surface
} DvzShapeIndexingFlags;
