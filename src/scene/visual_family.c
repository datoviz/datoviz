/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family descriptors                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "_visual_family.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define SRC_PER_ITEM (1u << DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
#define SRC_CONSTANT (1u << DVZ_VISUAL_ATTR_SOURCE_CONSTANT)
#define SRC_PER_SPAN (1u << DVZ_VISUAL_ATTR_SOURCE_PER_SPAN)
#define SRC_PER_GROUP (1u << DVZ_VISUAL_ATTR_SOURCE_PER_GROUP)

#define SRC_ITEM_ONLY SRC_PER_ITEM
#define SRC_COLOR_GROUPED (SRC_PER_ITEM | SRC_CONSTANT | SRC_PER_GROUP)
#define SRC_COLOR_SEGMENT (SRC_PER_ITEM | SRC_CONSTANT)
#define SRC_COLOR_PATH (SRC_PER_ITEM | SRC_CONSTANT | SRC_PER_SPAN | SRC_PER_GROUP)
#define SRC_SIZE_GROUPED (SRC_PER_ITEM | SRC_CONSTANT | SRC_PER_GROUP)
#define SRC_LINE_WIDTH (SRC_PER_ITEM | SRC_CONSTANT)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VisualAttrAlias VisualAttrAlias;
struct VisualAttrAlias
{
    DvzVisualType type;
    const char* public_name;
    const char* storage_name;
};


typedef struct VisualFamilyAttrs VisualFamilyAttrs;
struct VisualFamilyAttrs
{
    DvzVisualType type;
    const DvzVisualFamilyAttrDesc* attrs;
    uint32_t attr_count;
    const char* expected;
};



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const VisualAttrAlias ATTR_ALIASES[] = {
    {DVZ_VISUAL_TYPE_POINT, "diameter", "size"},
    {DVZ_VISUAL_TYPE_MARKER, "diameter", "size"},
    {DVZ_VISUAL_TYPE_PIXEL, "pixel_size", "size"},
    {DVZ_VISUAL_TYPE_SPHERE, "radius", "size"},
    {DVZ_VISUAL_TYPE_SEGMENT, "stroke_width", "line_width"},
    {DVZ_VISUAL_TYPE_PATH, "stroke_width", "line_width"},
    {DVZ_VISUAL_TYPE_VECTOR, "stroke_width", "line_width"},
};


static const DvzVisualFamilyAttrDesc SPLAT_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"sigma", 2 * sizeof(float), SRC_SIZE_GROUPED, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc POINT_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"selection", sizeof(uint8_t), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc PIXEL_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
};


static const DvzVisualFamilyAttrDesc MARKER_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"selection", sizeof(uint8_t), SRC_ITEM_ONLY, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
    {"shape", sizeof(uint32_t), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc SPHERE_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
};


static const DvzVisualFamilyAttrDesc PRIMITIVE_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"normal", 3 * sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc MESH_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"normal", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"instance_transform", 16 * sizeof(float), SRC_ITEM_ONLY, true},
};


static const DvzVisualFamilyAttrDesc PATH_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_PATH, false},
    {"line_width", sizeof(float), SRC_LINE_WIDTH, false},
};


static const DvzVisualFamilyAttrDesc SEGMENT_ATTRS[] = {
    {"position_start", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"position_end", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_SEGMENT, false},
    {"line_width", sizeof(float), SRC_LINE_WIDTH, false},
};


static const DvzVisualFamilyAttrDesc VECTOR_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"vector", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_SEGMENT, false},
    {"line_width", sizeof(float), SRC_LINE_WIDTH, false},
};


static const DvzVisualFamilyAttrDesc IMAGE_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"extent", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"position_px", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"extent_px", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"anchor", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"tex_rect", 4 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 2 * sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc TEXT_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"anchor", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc GLYPH_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"bounds", 4 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 4 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc VOLUME_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 3 * sizeof(float), SRC_ITEM_ONLY, false},
};


static const VisualFamilyAttrs FAMILY_ATTRS[] = {
    {DVZ_VISUAL_TYPE_SPLAT, SPLAT_ATTRS, DVZ_ARRAY_COUNT(SPLAT_ATTRS),
     "position, color, sigma, angle"},
    {DVZ_VISUAL_TYPE_POINT, POINT_ATTRS, DVZ_ARRAY_COUNT(POINT_ATTRS),
     "position, color, diameter, selection"},
    {DVZ_VISUAL_TYPE_PIXEL, PIXEL_ATTRS, DVZ_ARRAY_COUNT(PIXEL_ATTRS),
     "position, color, pixel_size"},
    {DVZ_VISUAL_TYPE_MARKER, MARKER_ATTRS, DVZ_ARRAY_COUNT(MARKER_ATTRS),
     "position, color, diameter, selection, angle, shape"},
    {DVZ_VISUAL_TYPE_SEGMENT, SEGMENT_ATTRS, DVZ_ARRAY_COUNT(SEGMENT_ATTRS),
     "position_start, position_end, color, stroke_width"},
    {DVZ_VISUAL_TYPE_VECTOR, VECTOR_ATTRS, DVZ_ARRAY_COUNT(VECTOR_ATTRS),
     "position, vector, color, stroke_width"},
    {DVZ_VISUAL_TYPE_PATH, PATH_ATTRS, DVZ_ARRAY_COUNT(PATH_ATTRS),
     "position, color, stroke_width"},
    {DVZ_VISUAL_TYPE_IMAGE, IMAGE_ATTRS, DVZ_ARRAY_COUNT(IMAGE_ATTRS),
     "position, extent, position_px, extent_px, anchor, tex_rect, texcoords"},
    {DVZ_VISUAL_TYPE_LABELS, IMAGE_ATTRS, DVZ_ARRAY_COUNT(IMAGE_ATTRS),
     "position, extent, position_px, extent_px, anchor, tex_rect, texcoords"},
    {DVZ_VISUAL_TYPE_MESH, MESH_ATTRS, DVZ_ARRAY_COUNT(MESH_ATTRS),
     "position, color, normal, texcoords, instance_transform"},
    {DVZ_VISUAL_TYPE_VOLUME, VOLUME_ATTRS, DVZ_ARRAY_COUNT(VOLUME_ATTRS),
     "position, texcoords, plus a bound 3D field"},
    {DVZ_VISUAL_TYPE_PRIMITIVE, PRIMITIVE_ATTRS, DVZ_ARRAY_COUNT(PRIMITIVE_ATTRS),
     "position, color, normal"},
    {DVZ_VISUAL_TYPE_SPHERE, SPHERE_ATTRS, DVZ_ARRAY_COUNT(SPHERE_ATTRS),
     "position, color, radius"},
    {DVZ_VISUAL_TYPE_GLYPH, GLYPH_ATTRS, DVZ_ARRAY_COUNT(GLYPH_ATTRS),
     "position, bounds, texcoords, color, angle, plus a bound 2D field"},
    {DVZ_VISUAL_TYPE_TEXT, TEXT_ATTRS, DVZ_ARRAY_COUNT(TEXT_ATTRS),
     "text strings plus position, anchor, size, color, angle"},
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the descriptor table for one visual family.
 *
 * @param type the visual type
 * @return the family descriptor table, or NULL when unknown
 */
static const VisualFamilyAttrs* _family_attrs(DvzVisualType type)
{
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(FAMILY_ATTRS); i++)
    {
        if (FAMILY_ATTRS[i].type == type)
            return &FAMILY_ATTRS[i];
    }
    return NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the retained storage name for a public visual attribute name.
 *
 * @param type the visual type
 * @param name the public attribute name
 * @return the retained storage name
 */
const char* _visual_family_attr_storage_name(DvzVisualType type, const char* name)
{
    ANN(name);
    if (strcmp(name, "size") == 0)
        return "size";
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(ATTR_ALIASES); i++)
    {
        if (ATTR_ALIASES[i].type == type && strcmp(ATTR_ALIASES[i].public_name, name) == 0)
            return ATTR_ALIASES[i].storage_name;
    }
    return name;
}



/**
 * Return one visual-family attribute descriptor.
 *
 * @param type the visual type
 * @param name the public or retained attribute name
 * @return the attribute descriptor, or NULL when unsupported
 */
const DvzVisualFamilyAttrDesc*
_visual_family_attr_desc(DvzVisualType type, const char* name)
{
    ANN(name);
    const VisualFamilyAttrs* family = _family_attrs(type);
    if (family == NULL)
        return NULL;
    name = _visual_family_attr_storage_name(type, name);
    for (uint32_t i = 0; i < family->attr_count; i++)
    {
        if (strcmp(family->attrs[i].name, name) == 0)
            return &family->attrs[i];
    }
    return NULL;
}



/**
 * Return a human-readable list of expected attributes for one visual family.
 *
 * @param type the visual type
 * @return expected attribute list
 */
const char* _visual_family_attr_expected(DvzVisualType type)
{
    const VisualFamilyAttrs* family = _family_attrs(type);
    return family != NULL ? family->expected : "position, color, diameter, selection";
}



/**
 * Return whether a semantic source is accepted by a visual-family attribute.
 *
 * @param type the visual type
 * @param name the public or retained attribute name
 * @param source the semantic source
 * @return whether the source is supported
 */
bool _visual_family_attr_source_supported(
    DvzVisualType type, const char* name, DvzVisualAttrSource source)
{
    const DvzVisualFamilyAttrDesc* desc = _visual_family_attr_desc(type, name);
    if (desc == NULL)
        return false;
    if (source < DVZ_VISUAL_ATTR_SOURCE_PER_ITEM || source > DVZ_VISUAL_ATTR_SOURCE_PER_GROUP)
        return false;
    return (desc->source_mask & (1u << source)) != 0;
}
