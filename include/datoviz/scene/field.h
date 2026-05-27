/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene sampled fields                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_FIELD_DIM_2D = 0,
    DVZ_FIELD_DIM_3D,
} DvzFieldDim;



typedef enum
{
    DVZ_FIELD_FORMAT_R8_UNORM = 0,
    DVZ_FIELD_FORMAT_R8_UINT,
    DVZ_FIELD_FORMAT_R8_SINT,
    DVZ_FIELD_FORMAT_R8_SNORM,
    DVZ_FIELD_FORMAT_R16_UNORM,
    DVZ_FIELD_FORMAT_R16_UINT,
    DVZ_FIELD_FORMAT_R16_SNORM,
    DVZ_FIELD_FORMAT_R16_SINT,
    DVZ_FIELD_FORMAT_R16_FLOAT,
    DVZ_FIELD_FORMAT_R32_UINT,
    DVZ_FIELD_FORMAT_R32_SINT,
    DVZ_FIELD_FORMAT_R32_FLOAT,
    DVZ_FIELD_FORMAT_RG8_UNORM,
    DVZ_FIELD_FORMAT_RG8_UINT,
    DVZ_FIELD_FORMAT_RG8_SINT,
    DVZ_FIELD_FORMAT_RG16_UNORM,
    DVZ_FIELD_FORMAT_RG16_UINT,
    DVZ_FIELD_FORMAT_RG16_SINT,
    DVZ_FIELD_FORMAT_RG16_FLOAT,
    DVZ_FIELD_FORMAT_RG32_UINT,
    DVZ_FIELD_FORMAT_RG32_SINT,
    DVZ_FIELD_FORMAT_RG32_FLOAT,
    DVZ_FIELD_FORMAT_RGBA8_UNORM,
    DVZ_FIELD_FORMAT_RGBA8_UINT,
    DVZ_FIELD_FORMAT_RGBA8_SINT,
    DVZ_FIELD_FORMAT_RGBA16_UNORM,
    DVZ_FIELD_FORMAT_RGBA16_UINT,
    DVZ_FIELD_FORMAT_RGBA16_SINT,
    DVZ_FIELD_FORMAT_RGBA16_FLOAT,
    DVZ_FIELD_FORMAT_RGBA32_UINT,
    DVZ_FIELD_FORMAT_RGBA32_SINT,
    DVZ_FIELD_FORMAT_RGBA32_FLOAT,
} DvzFieldFormat;



typedef enum
{
    DVZ_FIELD_SEMANTIC_GENERIC = 0,
    DVZ_FIELD_SEMANTIC_SCALAR,
    DVZ_FIELD_SEMANTIC_VECTOR_2,
    DVZ_FIELD_SEMANTIC_VECTOR_3,
    DVZ_FIELD_SEMANTIC_COLOR,
    DVZ_FIELD_SEMANTIC_LABEL,
    DVZ_FIELD_SEMANTIC_NORMAL,
} DvzFieldSemantic;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSampledFieldDesc
{
    DvzFieldDim dim;
    DvzFieldFormat format;
    DvzFieldSemantic semantic;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t flags;
};
typedef struct DvzSampledFieldDesc DvzSampledFieldDesc;



struct DvzFieldGeometry
{
    uint32_t axis_order[3];
    bool axis_flip[3];
    double origin[3];
    double spacing[3];
    char unit[32];
};
typedef struct DvzFieldGeometry DvzFieldGeometry;



struct DvzFieldRegion
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};
typedef struct DvzFieldRegion DvzFieldRegion;



struct DvzFieldDataView
{
    const void* data;
    uint64_t bytes_per_row;
    uint64_t rows_per_image;
};
typedef struct DvzFieldDataView DvzFieldDataView;



/*************************************************************************************************/
/*  Sampled-field lifecycle                                                                      */
/*************************************************************************************************/

/**
 * Create a scene-owned sampled field.
 *
 * @param scene the scene
 * @param desc the field descriptor
 * @return the sampled field, or NULL on error
 */
DVZ_EXPORT DvzSampledField* dvz_sampled_field(
    DvzScene* scene, const DvzSampledFieldDesc* desc);


/**
 * Destroy a sampled field.
 *
 * Any visual bindings to this field are cleared.
 *
 * @param field the sampled field
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_sampled_field_destroy(DvzSampledField* field);


/**
 * Replace the entire field payload.
 *
 * @param field the sampled field
 * @param view the uploaded data view
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_sampled_field_set_data(
    DvzSampledField* field, const DvzFieldDataView* view);


/**
 * Change the field extent and replace the entire payload.
 *
 * The field format, semantic, dimensionality, and visual bindings are preserved. Bound image
 * visuals receive a full dirty mark so the next scene emission reallocates the texture if needed.
 *
 * @param field the sampled field
 * @param width new field width in samples
 * @param height new field height in samples
 * @param depth new field depth in samples (must be 1 for 2D fields)
 * @param view the uploaded data view for the new extent
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_sampled_field_resize(
    DvzSampledField* field, uint32_t width, uint32_t height, uint32_t depth,
    const DvzFieldDataView* view);


/**
 * Update a field subregion in sample coordinates.
 *
 * @param field the sampled field
 * @param region the updated sample-space region
 * @param view the uploaded data view
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_sampled_field_update_region(
    DvzSampledField* field, DvzFieldRegion region, const DvzFieldDataView* view);


/**
 * Update the field geometry metadata.
 *
 * @param field the sampled field
 * @param geometry the geometry descriptor
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_sampled_field_set_geometry(
    DvzSampledField* field, const DvzFieldGeometry* geometry);


/**
 * Return the immutable field descriptor.
 *
 * @param field the sampled field
 * @return the descriptor, or NULL on error
 */
DVZ_EXPORT const DvzSampledFieldDesc* dvz_sampled_field_desc(const DvzSampledField* field);



/*************************************************************************************************/
/*  Visual field bindings                                                                        */
/*************************************************************************************************/

/**
 * Bind a scene-owned sampled field to a named visual slot.
 *
 * Image, glyph, and labels visuals accept the `"field"` slot and require a 2D field. Mesh visuals
 * accept the `"texture"` slot for a first-slice RGBA8 2D texture. Volume visuals accept the
 * `"field"` slot and require a 3D field. Labels visuals additionally require
 * `DVZ_FIELD_SEMANTIC_LABEL`.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param field the field, or NULL to clear the binding
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_visual_set_field(
    DvzVisual* visual, const char* slot_name, DvzSampledField* field);


EXTERN_C_OFF
