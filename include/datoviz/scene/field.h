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



typedef enum
{
    DVZ_COLOR_ROLE_NONE = 0,
    DVZ_COLOR_ROLE_SRGB_COLOR,
    DVZ_COLOR_ROLE_LINEAR_COLOR,
    DVZ_COLOR_ROLE_DATA,
} DvzColorRole;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSampledFieldDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzFieldDim dim;
    DvzFieldFormat format;
    DvzFieldSemantic semantic;
    DvzColorRole color_role;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};
typedef struct DvzSampledFieldDesc DvzSampledFieldDesc;



struct DvzFieldGeometry
{
    uint32_t struct_size;
    uint32_t flags;
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
    uint32_t struct_size;
    uint32_t flags;
    const void* data;        /* Required payload pointer for non-empty uploads. */
    uint64_t bytes_per_row;  /* 0 means tightly packed rows for the field format. */
    uint64_t rows_per_image; /* 0 means tightly packed 2D slices for the field format. */
};
typedef struct DvzFieldDataView DvzFieldDataView;



/*************************************************************************************************/
/*  Sampled-field lifecycle                                                                      */
/*************************************************************************************************/

/**
 * Return the default sampled-field descriptor.
 *
 * @return default sampled-field descriptor
 */
DVZ_EXPORT DvzSampledFieldDesc dvz_sampled_field_desc(void);


/**
 * Return the default sampled-field geometry descriptor.
 *
 * @return default sampled-field geometry descriptor
 */
DVZ_EXPORT DvzFieldGeometry dvz_field_geometry(void);


/**
 * Return the default sampled-field data view descriptor.
 *
 * @return default sampled-field data view descriptor
 */
DVZ_EXPORT DvzFieldDataView dvz_field_data_view(void);


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
 * Return the scene-local identity of a sampled field.
 *
 * @param field the sampled field
 * @return the scene-local identity, or DVZ_ID_NONE when field is NULL or destroyed
 */
DVZ_EXPORT DvzId dvz_sampled_field_id(const DvzSampledField* field);


/**
 * Destroy a sampled field.
 *
 * Any visual bindings to this field are cleared.
 *
 * @param field the sampled field
 */
DVZ_EXPORT void dvz_sampled_field_destroy(DvzSampledField* field);


/**
 * Replace the entire field payload.
 *
 * `view->data` must cover the full field extent. Payload bytes are copied into scene-owned storage
 * before return and may be released by the caller after this function returns. Passing NULL or an
 * empty view is rejected.
 *
 * @param field the sampled field
 * @param view the uploaded data view
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DVZ_EXPORT DvzResult dvz_sampled_field_set_data(
    DvzSampledField* field, const DvzFieldDataView* view);


/**
 * Change the field extent and replace the entire payload.
 *
 * The field format, semantic, dimensionality, and visual bindings are preserved. Bound image
 * visuals receive a full dirty mark so the next scene emission reallocates the texture if needed.
 * `width`, `height`, and `depth` must describe a non-empty extent. `view->data` must cover that
 * extent and is copied before return.
 *
 * @param field the sampled field
 * @param width new field width in samples
 * @param height new field height in samples
 * @param depth new field depth in samples (must be 1 for 2D fields)
 * @param view the uploaded data view for the new extent
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DVZ_EXPORT DvzResult dvz_sampled_field_resize(
    DvzSampledField* field, uint32_t width, uint32_t height, uint32_t depth,
    const DvzFieldDataView* view);


/**
 * Update a field subregion in sample coordinates.
 *
 * `region` must be non-empty and fully inside the current field extent. `view->data` must cover the
 * subregion and is copied before return.
 *
 * @param field the sampled field
 * @param region the updated sample-space region
 * @param view the uploaded data view
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DVZ_EXPORT DvzResult dvz_sampled_field_update_region(
    DvzSampledField* field, DvzFieldRegion region, const DvzFieldDataView* view);


/**
 * Update the field geometry metadata.
 *
 * @param field the sampled field
 * @param geometry the geometry descriptor
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DVZ_EXPORT DvzResult dvz_sampled_field_set_geometry(
    DvzSampledField* field, const DvzFieldGeometry* geometry);


/**
 * Copy immutable field descriptor information.
 *
 * The descriptor is copied into caller-owned storage and remains valid after return.
 *
 * @param field the sampled field
 * @param out output field descriptor
 * @return whether the descriptor was copied
 */
DVZ_EXPORT bool dvz_sampled_field_info(const DvzSampledField* field, DvzSampledFieldDesc* out);



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
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DVZ_EXPORT DvzResult dvz_visual_set_field(
    DvzVisual* visual, const char* slot_name, const DvzSampledField* field);


EXTERN_C_OFF
