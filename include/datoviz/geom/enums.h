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

// Geometry type.
typedef enum
{
    DVZ_GEOMETRY_NONE,
    DVZ_GEOMETRY_CUSTOM,
    DVZ_GEOMETRY_CUBE,
    DVZ_GEOMETRY_PLANE,
    DVZ_GEOMETRY_SURFACE_GRID,
    DVZ_GEOMETRY_SPHERE,
    DVZ_GEOMETRY_CYLINDER,
    DVZ_GEOMETRY_CONE,
    DVZ_GEOMETRY_TORUS,
    DVZ_GEOMETRY_ARROW,
} DvzGeometryType;



// Indexing flags.
typedef enum
{
    DVZ_GEOMETRY_INDEXING_NONE = 0x00,          // no indexing provenance
    DVZ_GEOMETRY_INDEXING_TRIANGLES = 0x01,     // triangle-list indexing
    DVZ_GEOMETRY_INDEXING_SURFACE_GRID = 0x02,  // structured surface-grid indexing
    DVZ_GEOMETRY_INDEXING_TRIANGULATION = 0x04, // polygon/PSLG triangulation
} DvzGeometryIndexingFlags;



// Derived mesh-edge flags.
typedef enum
{
    DVZ_GEOMETRY_EDGE_NONE = 0x00,        // no derived edge flags
    DVZ_GEOMETRY_EDGE_BOUNDARY = 0x01,    // edge belongs to one face
    DVZ_GEOMETRY_EDGE_NONMANIFOLD = 0x02, // edge belongs to more than two faces
} DvzGeometryEdgeFlags;
