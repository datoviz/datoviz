/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visuals     enums                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Marker shape.
// NOTE: the numbers need to correspond to graphics_markers.frag.
typedef enum
{
    DVZ_MARKER_SHAPE_DISC = 0,
    DVZ_MARKER_SHAPE_ASTERISK = 1,
    DVZ_MARKER_SHAPE_CHEVRON = 2,
    DVZ_MARKER_SHAPE_CLOVER = 3,
    DVZ_MARKER_SHAPE_CLUB = 4,
    DVZ_MARKER_SHAPE_CROSS = 5,
    DVZ_MARKER_SHAPE_DIAMOND = 6,
    DVZ_MARKER_SHAPE_ARROW = 7,
    DVZ_MARKER_SHAPE_ELLIPSE = 8,
    DVZ_MARKER_SHAPE_HBAR = 9,
    DVZ_MARKER_SHAPE_HEART = 10,
    DVZ_MARKER_SHAPE_INFINITY = 11,
    DVZ_MARKER_SHAPE_PIN = 12,
    DVZ_MARKER_SHAPE_RING = 13,
    DVZ_MARKER_SHAPE_SPADE = 14,
    DVZ_MARKER_SHAPE_SQUARE = 15,
    DVZ_MARKER_SHAPE_TAG = 16,
    DVZ_MARKER_SHAPE_TRIANGLE = 17,
    DVZ_MARKER_SHAPE_VBAR = 18,
    DVZ_MARKER_SHAPE_ROUNDED_RECT = 19,
    DVZ_MARKER_SHAPE_COUNT,
} DvzMarkerShape;



// Marker mode.
typedef enum
{
    DVZ_MARKER_MODE_NONE = 0,
    DVZ_MARKER_MODE_CODE = 1,
    DVZ_MARKER_MODE_BITMAP = 2,
    DVZ_MARKER_MODE_SDF = 3,
    DVZ_MARKER_MODE_MSDF = 4,
    DVZ_MARKER_MODE_MTSDF = 5,
} DvzMarkerMode;



// Marker aspect.
typedef enum
{
    DVZ_MARKER_ASPECT_FILLED = 0,
    DVZ_MARKER_ASPECT_STROKE = 1,
    DVZ_MARKER_ASPECT_OUTLINE = 2,
} DvzMarkerAspect;



// Cap type.
typedef enum
{
    DVZ_CAP_NONE = 0,
    DVZ_CAP_ROUND = 1,
    DVZ_CAP_TRIANGLE_IN = 2,
    DVZ_CAP_TRIANGLE_OUT = 3,
    DVZ_CAP_SQUARE = 4,
    DVZ_CAP_BUTT = 5,
    DVZ_CAP_COUNT,
} DvzCapType;



// Joint type.
typedef enum
{
    DVZ_JOIN_SQUARE = 0,
    DVZ_JOIN_ROUND = 1,
} DvzJoinType;



// Path flags.
typedef enum
{
    DVZ_PATH_FLAGS_OPEN,
    DVZ_PATH_FLAGS_CLOSED,
} DvzPathFlags;



// Image flags.
// NOTE: these flags are also passed as VisualFlags and then BakerFlags
typedef enum
{
    DVZ_IMAGE_FLAGS_SIZE_PIXELS = 0x0000, // image size is specified in pixels
    DVZ_IMAGE_FLAGS_SIZE_NDC = 0x0001, // image size is specified in normalized device coordinates
    DVZ_IMAGE_FLAGS_RESCALE_KEEP_RATIO = 0x0004, // image size ~ to total zoom level
    DVZ_IMAGE_FLAGS_RESCALE = 0x0008,            // image size ~ to axis zoom level
    DVZ_IMAGE_FLAGS_MODE_RGBA = 0x0000,          // image mode: RGBA texture
    DVZ_IMAGE_FLAGS_MODE_COLORMAP = 0x0010, // image mode: single-channel texture with colormap
    DVZ_IMAGE_FLAGS_MODE_FILL = 0x0020,     // image mode: fill color
    DVZ_IMAGE_FLAGS_BORDER = 0x0080,        // square or rounded border around the image
} DvzImageFlags;



// Contour flags.
typedef enum
{
    DVZ_CONTOUR_NONE = 0x00,   // no contours
    DVZ_CONTOUR_EDGES = 0x01,  // set edge on some vertices (those on the contour)
    DVZ_CONTOUR_JOINTS = 0x02, // set joints on some vertices (those with 1 exterior adjacent edge)
    DVZ_CONTOUR_FULL = 0x04,   // set edge on all vertices
} DvzContourFlags;


// Sphere flags.
// NOTE: these flags are also passed as VisualFlags and then BakerFlags
typedef enum
{
    DVZ_SPHERE_FLAGS_NONE = 0x0000,
    DVZ_SPHERE_FLAGS_TEXTURED = 0x0001,
    DVZ_SPHERE_FLAGS_LIGHTING = 0x0002,
    DVZ_SPHERE_FLAGS_SIZE_PIXELS = 0x0004,
    DVZ_SPHERE_FLAGS_EQUAL_RECTANGULAR = 0x0008,
} DvzSphereFlags;



// Mesh flags.
// NOTE: these flags are also passed as VisualFlags and then BakerFlags
typedef enum
{
    DVZ_MESH_FLAGS_NONE = 0x0000,
    DVZ_MESH_FLAGS_TEXTURED = 0x0001,
    DVZ_MESH_FLAGS_LIGHTING = 0x0002,
    DVZ_MESH_FLAGS_CONTOUR = 0x0004,
    DVZ_MESH_FLAGS_ISOLINE = 0x0008,
} DvzMeshFlags;



// Volume flags.
typedef enum
{
    DVZ_VOLUME_FLAGS_NONE = 0x0000,
    DVZ_VOLUME_FLAGS_RGBA = 0x0001,
    DVZ_VOLUME_FLAGS_COLORMAP = 0x0002,
    DVZ_VOLUME_FLAGS_BACK_FRONT = 0x0004,
} DvzVolumeFlags;
