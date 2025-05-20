/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**************************************************************************************************

 * DATOVIZ PUBLIC API HEADER FILE
 * ==============================
 * 2024-07-01
 * Cyrille Rossant
 * cyrille dot rossant at gmail com

This file exposes the public API of Datoviz, a C/C++ library for high-performance GPU scientific
visualization.

Datoviz is still an early stage library and the API may change at any time.

**************************************************************************************************/



/*************************************************************************************************/
/*  Public visuals API                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_VISUALS
#define DVZ_HEADER_PUBLIC_VISUALS



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "datoviz_app.h"
#include "datoviz_keycodes.h"
#include "datoviz_macros.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "datoviz_version.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzBatch DvzBatch;
typedef struct DvzShape DvzShape;
typedef struct DvzTex DvzTex;
typedef struct DvzTexture DvzTexture;
typedef struct DvzVisual DvzVisual;



EXTERN_C_ON


/*************************************************************************************************/
/*  Basic visual                                                                                 */
/*************************************************************************************************/

/**
 * Create a basic visual using the few GPU visual primitives (point, line, triangles).
 *
 * @param batch the batch
 * @param topology the primitive topology
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_basic(DvzBatch* batch, DvzPrimitiveTopology topology, int flags);



/**
 * Set the vertex positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the vertex colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the vertex group index.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the group index of each vertex
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_group(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the point size (for POINT_LIST topology only).
 *
 * @param visual the visual
 * @param size the point size in pixels
 */
DVZ_EXPORT void dvz_basic_size(DvzVisual* visual, float size);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_basic_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/

/**
 * Create a pixel visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_pixel(DvzBatch* batch, int flags);



/**
 * Set the pixel positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_pixel_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the pixel colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_pixel_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the pixel size.
 *
 * @param visual the visual
 * @param size the point size in pixels
 */
DVZ_EXPORT void dvz_pixel_size(DvzVisual* visual, float size);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_pixel_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Point                                                                                        */
/*************************************************************************************************/

/**
 * Create a point visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzBatch* batch, int flags);



/**
 * Set the point positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the point sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_size(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the point colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_point_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Marker                                                                                       */
/*************************************************************************************************/

/**
 * Create a marker visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_marker(DvzBatch* batch, int flags);



/**
 * Set the marker mode.
 *
 * @param visual the visual
 * @param mode the marker mode, one of DVZ_MARKER_MODE_CODE, DVZ_MARKER_MODE_BITMAP,
 * DVZ_MARKER_MODE_SDF, DVZ_MARKER_MODE_MSDF, DVZ_MARKER_MODE_MTSDF.
 */
DVZ_EXPORT void dvz_marker_mode(DvzVisual* visual, DvzMarkerMode mode);



/**
 * Set the marker aspect.
 *
 * @param visual the visual
 * @param aspect the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE,
 * DVZ_MARKER_ASPECT_OUTLINE.
 */
DVZ_EXPORT void dvz_marker_aspect(DvzVisual* visual, DvzMarkerAspect aspect);



/**
 * Set the marker shape.
 *
 * @param visual the visual
 * @param shape the marker shape
 */
DVZ_EXPORT void dvz_marker_shape(DvzVisual* visual, DvzMarkerShape shape);



/**
 * Set the marker positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the marker sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_size(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the marker angles.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the angles of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the marker colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the marker edge color.
 *
 * @param visual the visual
 * @param color the edge color
 */
DVZ_EXPORT void dvz_marker_edgecolor(DvzVisual* visual, DvzColor color);



/**
 * Set the marker edge width.
 *
 * @param visual the visual
 * @param width the edge width
 */
DVZ_EXPORT void dvz_marker_linewidth(DvzVisual* visual, float width);



/**
 * Set the marker texture.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_marker_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the texture scale.
 *
 * @param visual the visual
 * @param scale the texture scale
 */
DVZ_EXPORT void dvz_marker_tex_scale(DvzVisual* visual, float scale);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_marker_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Segment                                                                                      */
/*************************************************************************************************/

/**
 * Create a segment visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_segment(DvzBatch* batch, int flags);



/**
 * Set the segment positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param initial the initial 3D positions of the segments
 * @param terminal the terminal 3D positions of the segments
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_segment_position(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* initial, vec3* terminal, int flags);



/**
 * Set the segment shift.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Set the segment colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the segment line widths.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the segment line widths
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_linewidth(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the segment cap types.
 *
 * @param visual the visual
 * @param initial the initial segment cap type
 * @param terminal the terminal segment cap type
 */
DVZ_EXPORT void dvz_segment_cap(DvzVisual* visual, DvzCapType initial, DvzCapType terminal);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_segment_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Path                                                                                         */
/*************************************************************************************************/

/**
 * Create a path visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_path(DvzBatch* batch, int flags);



/**
 * Set the path positions. Note: all path point positions must be updated at once for now.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param point_count the total number of points across all paths
 * @param positions the path point positions
 * @param path_count the number of different paths
 * @param path_lengths the number of points in each path
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_path_position(
    DvzVisual* visual, uint32_t first, uint32_t point_count, vec3* positions, //
    uint32_t path_count, uint32_t* path_lengths, int flags);



/**
 * Set the path colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_path_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the path line width (may be variable along a path).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the line width of the vertex, in pixels
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_path_linewidth(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the path cap.
 *
 * @param visual the visual
 * @param cap the cap
 */
DVZ_EXPORT
void dvz_path_cap(DvzVisual* visual, DvzCapType cap);



/**
 * Set the path join.
 *
 * @param visual the visual
 * @param join the join
 */
DVZ_EXPORT void dvz_path_join(DvzVisual* visual, DvzJoinType join);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param total_point_count the total number of points to allocate for this visual
 */
DVZ_EXPORT void dvz_path_alloc(DvzVisual* visual, uint32_t total_point_count);



/*************************************************************************************************/
/*  Glyph                                                                                        */
/*************************************************************************************************/

/**
 * Create a glyph visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_glyph(DvzBatch* batch, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_glyph_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Set the glyph positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the glyph axes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D axis vectors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_axis(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the glyph sizes, in pixels.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes (width and height) of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph anchors.
 *
 * The anchor should be the same for each glyph in a given string. In addition, it is important to
 * set dvz_glyph_group_size() (the size of each string in pixels) for the anchor computation to be
 * correct.
 *
 * The anchor determines the relationship between the glyph 3D position, and the position of the
 * string bounding box. Each string comes with a local coordinate system extending from (-1, -1)
 * (bottom-left corner) to (+1, +1) (top-right corner), and (0, 0) refers to the center of the
 * string. The anchor is the point, in this local coordinate system, that matches the glyph 3D
 * position. For example, to center a string around the glyph 3D position, use (0, 0) as anchor.
 * To align the string to the right of the glyph 3D position, use (-1, -1) for example.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the anchors (x and y) of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph shifts.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the shifts (x and y) of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param coords the x,y,w,h texture coordinates
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags);



/**
 * Set the glyph group size, in pixels (size of each string).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the glyph group shapes (width and height, in pixels)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_group_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph scaling applied to the size of all individual glyphs.
 *
 * We assume that the scaling is the same within each string (group of glyphs).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the scaling of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_scale(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the glyph angles.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the angles of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the glyph colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the glyph background color.
 *
 * @param visual the visual
 * @param bgcolor the background color
 */
DVZ_EXPORT void dvz_glyph_bgcolor(DvzVisual* visual, DvzColor bgcolor);



/**
 * Assign a texture to a glyph visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_glyph_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Associate an atlas and font with a glyph visual.
 *
 * @param visual the visual
 * @param af the atlas font
 */
DVZ_EXPORT void dvz_glyph_atlas_font(DvzVisual* visual, DvzAtlasFont* af);



/**
 * Set the glyph unicode code points.
 *
 * @param visual the visual
 * @param count the number of glyphs
 * @param codepoints the unicode codepoints
 */
DVZ_EXPORT void dvz_glyph_unicode(DvzVisual* visual, uint32_t count, uint32_t* codepoints);



/**
 * Set the glyph ascii characters.
 *
 * @param visual the visual
 * @param string the characters
 */
DVZ_EXPORT void dvz_glyph_ascii(DvzVisual* visual, const char* string);



/**
 * Set the xywh parameters of each glyph.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the xywh values of each glyph
 * @param offset the xy offsets of each glyph
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_glyph_xywh(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, vec2 offset, int flags);



/**
 * Helper function to easily set multiple strings of the same size and color on a glyph visual.
 *
 * @param visual the visual
 * @param string_count the number of strings
 * @param strings the strings
 * @param positions the positions of each string
 * @param scales the scaling of each string
 * @param color the same color for all strings
 * @param offset the same offset for all strings
 * @param anchor the same anchor for all strings
 */
DVZ_EXPORT
void dvz_glyph_strings(
    DvzVisual* visual, uint32_t string_count, char** strings, vec3* positions, float* scales,
    DvzColor color, vec2 offset, vec2 anchor);



/*************************************************************************************************/
/*  Monoglyph visual                                                                             */
/*************************************************************************************************/

/**
 * Create a monoglyph visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_monoglyph(DvzBatch* batch, int flags);



/**
 * Set the glyph positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_monoglyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the glyph offsets.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the glyph offsets (ivec2 integers: row,column)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_monoglyph_offset(DvzVisual* visual, uint32_t first, uint32_t count, ivec2* values, int flags);



/**
 * Set the glyph colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_monoglyph_color(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the text.
 *
 * @param visual the visual
 * @param first the first item
 * @param text the ASCII test (string length without the null terminal byte = number of glyphs)
 * @param flags the upload flags
 */
DVZ_EXPORT void
dvz_monoglyph_glyph(DvzVisual* visual, uint32_t first, const char* text, int flags);



/**
 * Set the glyph anchor (relative to the glyph size).
 *
 * @param visual the visual
 * @param anchor the anchor
 */
DVZ_EXPORT void dvz_monoglyph_anchor(DvzVisual* visual, vec2 anchor);



/**
 * Set the glyph size (relative to the initial glyph size).
 *
 * @param visual the visual
 * @param size the glyph size
 */
DVZ_EXPORT void dvz_monoglyph_size(DvzVisual* visual, float size);



/**
 * All-in-one function for multiline text.
 *
 * @param visual the visual
 * @param pos the text position
 * @param color the text color
 * @param size the glyph size
 * @param text the text, can contain `\n` new lines
 */
DVZ_EXPORT void
dvz_monoglyph_textarea(DvzVisual* visual, vec3 pos, DvzColor color, float size, const char* text);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_monoglyph_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

/**
 * Create an image visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzBatch* batch, int flags);



/**
 * Set the image positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the top left corner
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the image sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes of each image, in pixels
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the image anchors.
 *
 * The anchor determines the relationship between the image 3D position, and the position of the
 * image on the screen. Each images comes with a local coordinate system extending from (-1, -1)
 * (bottom-left corner) to (+1, +1) (top-right corner), and (0, 0) refers to the center of the
 * image. The anchor is the point, in this local coordinate system, that matches the image 3D
 * position. For example, to center an image around the image 3D position, use (0, 0) as anchor.
 * To align the image to the right and bottom of the image 3D position, so that this position
 * refers to the top-left corner, use (-1, +1) as anchor.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the relative anchors of each image, (0,0 = position pertains to top left corner)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the image texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param tl_br the tex coordinates of the top left and bottom right corners (vec4 u0,v0,u1,v1)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* tl_br, int flags);



/**
 * Set the image colors (only when using DVZ_IMAGE_FLAGS_FILL).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the image colors
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_image_facecolor(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Assign a texture to an image visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_image_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the edge color.
 *
 * @param visual the visual
 * @param color the edge color
 */
DVZ_EXPORT void dvz_image_edgecolor(DvzVisual* visual, DvzColor color);



/**
 * Set the texture coordinates index permutation.
 *
 * @param visual the visual
 * @param ij index permutation
 */
DVZ_EXPORT void dvz_image_permutation(DvzVisual* visual, ivec2 ij);



/**
 * Set the edge width.
 *
 * @param visual the visual
 * @param width the edge width
 */
DVZ_EXPORT void dvz_image_linewidth(DvzVisual* visual, float width);



/**
 * Use a rounded rectangle for images, with a given radius in pixels.
 *
 * @param visual the visual
 * @param radius the rounded corner radius, in pixel
 */
DVZ_EXPORT void dvz_image_radius(DvzVisual* visual, float radius);



/**
 * Specify the colormap when using DVZ_IMAGE_FLAGS_MODE_COLORMAP.
 *
 * Only the following colormaps are available on the GPU at the moment:
 *
 * `CMAP_BINARY`
 * `CMAP_HSV`
 * `CMAP_CIVIDIS`
 * `CMAP_INFERNO`
 * `CMAP_MAGMA`
 * `CMAP_PLASMA`
 * `CMAP_VIRIDIS`
 * `CMAP_AUTUMN`
 * `CMAP_BONE`
 * `CMAP_COOL`
 * `CMAP_COPPER`
 * `CMAP_HOT`
 * `CMAP_SPRING`
 * `CMAP_SUMMER`
 * `CMAP_WINTER`
 * `CMAP_JET`
 *
 * @param visual the visual
 * @param cmap the colormap
 */
DVZ_EXPORT void dvz_image_colormap(DvzVisual* visual, DvzColormap cmap);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of images to allocate for this visual
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/

/**
 * Create a mesh visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzBatch* batch, int flags);



/**
 * Set the mesh vertex positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D vertex positions
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the mesh colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the vertex colors
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the mesh texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the vertex texture coordinates (vec4 u,v,*,alpha)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Set the mesh normals.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the vertex normal vectors
 * @param flags the data update flags
 */
DVZ_EXPORT
void dvz_mesh_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the isolines values.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the scalar field for which to draw isolines
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_isoline(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the distance between the current vertex to the left edge at corner A, B, or C in triangle
 * ABC.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the distance to the left edge adjacent to each triangle vertex
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_left(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the distance between the current vertex to the right edge at corner A, B, or C in triangle
 * ABC.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the distance to the right edge adjacent to each triangle vertex
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_right(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the contour information for polygon contours.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values for vertex A, B, C, the least significant bit is 1 if the opposite edge is a
 * contour, and the second least significant bit is 1 if the corner is a contour
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_contour(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Assign a 2D texture to a mesh visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_mesh_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the mesh indices.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the face indices (three vertex indices per triangle)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param vertex_count the number of vertices
 * @param index_count the number of indices
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* visual, uint32_t vertex_count, uint32_t index_count);



/**
 * Set the light direction.
 *
 * @param visual the mesh
 * @param idx the light index (0, 1, 2, or 3)
 * @param dir the light direction
 */
DVZ_EXPORT void dvz_mesh_light_dir(DvzVisual* visual, uint32_t idx, vec3 dir);



/**
 * Set the light color.
 *
 * @param visual the mesh
 * @param idx the light index (0, 1, 2, or 3)
 * @param rgba the light color (rgba, but the a component is ignored)
 */
DVZ_EXPORT void dvz_mesh_light_color(DvzVisual* visual, uint32_t idx, DvzColor rgba);



/**
 * Set the light parameters.
 *
 * @param visual the mesh
 * @param idx the light index (0, 1, 2, or 3)
 * @param params the light parameters (vec4 ambient, diffuse, specular, exponent)
 */
DVZ_EXPORT void dvz_mesh_light_params(DvzVisual* visual, uint32_t idx, vec4 params);



/**
 * Set the marker edge color.
 *
 * Note: the alpha component is currently unused.
 *
 * @param visual the mesh
 * @param rgba the rgba components
 */
DVZ_EXPORT void dvz_mesh_edgecolor(DvzVisual* visual, DvzColor rgba);



/**
 * Set the mesh contour linewidth (wireframe or isoline).
 *
 * @param visual the mesh
 * @param linewidth the line width
 */
DVZ_EXPORT void dvz_mesh_linewidth(DvzVisual* visual, float linewidth);



/**
 * Set the number of isolines
 *
 * @param visual the mesh
 * @param count the number of isolines
 */
DVZ_EXPORT void dvz_mesh_density(DvzVisual* visual, uint32_t count);



/**
 * Create a mesh out of a shape.
 *
 * @param batch the batch
 * @param shape the shape
 * @param flags the visual creation flags
 * @returns the mesh
 */
DVZ_EXPORT DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape, int flags);



/**
 * Update a mesh once a shape has been updated.
 *
 * @param visual the mesh
 * @param shape the shape
 */
DVZ_EXPORT void dvz_mesh_reshape(DvzVisual* visual, DvzShape* shape);



/*************************************************************************************************/
/*  Sphere                                                                                  */
/*************************************************************************************************/

/**
 * Create a sphere visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_sphere(DvzBatch* batch, int flags);



/**
 * Set the sphere positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param pos the 3D positions of the sphere centers
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* pos, int flags);



/**
 * Set the sphere colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param color the sphere colors
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* color, int flags);



/**
 * Set the sphere sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param size the radius of the spheres
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* size, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of spheres to allocate for this visual
 */
DVZ_EXPORT void dvz_sphere_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Set the sphere light position.
 *
 * @param visual the visual
 * @param pos the light position
 */
DVZ_EXPORT void dvz_sphere_light_pos(DvzVisual* visual, vec3 pos);



/**
 * Set the sphere light parameters.
 *
 * @param visual the visual
 * @param params the light parameters (vec4 ambient, diffuse, specular, exponent)
 */
DVZ_EXPORT void dvz_sphere_light_params(DvzVisual* visual, vec4 params);



/*************************************************************************************************/
/*  Volume                                                                                       */
/*************************************************************************************************/

/**
 * Create a volume visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_volume(DvzBatch* batch, int flags);



/**
 * Assign a 3D texture to a volume visual.
 *
 * @param visual the visual
 * @param texture the 3D texture
 */
DVZ_EXPORT void dvz_volume_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the volume bounds.
 *
 * @param visual the visual
 * @param xlim xmin and xmax
 * @param ylim ymin and ymax
 * @param zlim zmin and zmax
 */
DVZ_EXPORT void dvz_volume_bounds(DvzVisual* visual, vec2 xlim, vec2 ylim, vec2 zlim);



/**
 * Set the texture coordinates of two corner points.
 *
 * @param visual the visual
 * @param uvw0 coordinates of one of the corner points
 * @param uvw1 coordinates of one of the corner points
 */
DVZ_EXPORT void dvz_volume_texcoords(DvzVisual* visual, vec3 uvw0, vec3 uvw1);



/**
 * Set the texture coordinates index permutation.
 *
 * @param visual the visual
 * @param ijk index permutation
 */
DVZ_EXPORT void dvz_volume_permutation(DvzVisual* visual, ivec3 ijk);



/**
 * Set the bounding box face index on which to slice (showing the texture itself).
 *
 * @param visual the visual
 * @param face_index -1 to disable, or the face index between 0 and 5 included
 */
DVZ_EXPORT void dvz_volume_slice(DvzVisual* visual, int32_t face_index);



/**
 * Set the volume size.
 *
 * @param visual the visual
 * @param transfer transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor
 */
DVZ_EXPORT void dvz_volume_transfer(DvzVisual* visual, vec4 transfer);



/*************************************************************************************************/
/*  Slice                                                                                        */
/*************************************************************************************************/

/**
 * Create a slice visual (multiple 2D images with slices of a 3D texture).
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_slice(DvzBatch* batch, int flags);



/**
 * Set the slice positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param p0 the 3D positions of the top left corner
 * @param p1 the 3D positions of the top right corner
 * @param p2 the 3D positions of the bottom left corner
 * @param p3 the 3D positions of the bottom right corner
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_slice_position(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    vec3* p0, vec3* p1, vec3* p2, vec3* p3, int flags);



/**
 * Set the slice texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param uvw0 the 3D texture coordinates of the top left corner
 * @param uvw1 the 3D texture coordinates of the top right corner
 * @param uvw2 the 3D texture coordinates of the bottom left corner
 * @param uvw3 the 3D texture coordinates of the bottom right corner
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_slice_texcoords(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags);



/**
 * Assign a texture to a slice visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_slice_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of slices to allocate for this visual
 */
DVZ_EXPORT void dvz_slice_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Create a 3D texture to be used in a slice visual.
 *
 * @param batch the batch
 * @param format the texture format
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth
 * @param data the texture data to upload
 * @returns the texture ID
 */
DVZ_EXPORT DvzId dvz_tex_slice(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth,
    void* data);



/**
 * Set the slice transparency alpha value.
 *
 * @param visual the visual
 * @param alpha the alpha value
 */
DVZ_EXPORT void dvz_slice_alpha(DvzVisual* visual, float alpha);



EXTERN_C_OFF

#endif
