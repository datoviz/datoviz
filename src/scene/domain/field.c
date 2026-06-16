/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field helpers                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "field_internal.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static DvzSampledField* _scene_alloc_field_slot(DvzScene* scene);

/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_SAMPLED_FIELD_DESC_KNOWN_FLAGS 0u
#define DVZ_FIELD_GEOMETRY_KNOWN_FLAGS 0u



static bool _sampled_field_desc_validate(const DvzSampledFieldDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzSampledFieldDesc, DVZ_SAMPLED_FIELD_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzSampledFieldDesc ABI prologue");
        return false;
    }
    return true;
}


static DvzColorRole _sampled_field_default_color_role(
    DvzFieldFormat format, DvzFieldSemantic semantic)
{
    switch (semantic)
    {
    case DVZ_FIELD_SEMANTIC_SCALAR:
    case DVZ_FIELD_SEMANTIC_VECTOR_2:
    case DVZ_FIELD_SEMANTIC_VECTOR_3:
    case DVZ_FIELD_SEMANTIC_LABEL:
    case DVZ_FIELD_SEMANTIC_NORMAL:
        return DVZ_COLOR_ROLE_DATA;
    case DVZ_FIELD_SEMANTIC_COLOR:
        if (format == DVZ_FIELD_FORMAT_RGBA16_FLOAT || format == DVZ_FIELD_FORMAT_RGBA32_FLOAT)
            return DVZ_COLOR_ROLE_LINEAR_COLOR;
        return DVZ_COLOR_ROLE_SRGB_COLOR;
    case DVZ_FIELD_SEMANTIC_GENERIC:
    default:
        return _field_format_is_scalar(format) ? DVZ_COLOR_ROLE_DATA : DVZ_COLOR_ROLE_SRGB_COLOR;
    }
}


static DvzSampledFieldDesc _sampled_field_desc_resolve(const DvzSampledFieldDesc* desc)
{
    ANN(desc);
    DvzSampledFieldDesc resolved = *desc;
    if (resolved.color_role == DVZ_COLOR_ROLE_NONE)
        resolved.color_role =
            _sampled_field_default_color_role(resolved.format, resolved.semantic);
    return resolved;
}


/**
 * Validate that a sampled-field color role matches the field semantic.
 *
 * @param desc resolved sampled-field descriptor
 * @return whether the color-role/semantic pair is valid
 */
static bool _sampled_field_color_role_validate(const DvzSampledFieldDesc* desc)
{
    ANN(desc);
    switch (desc->color_role)
    {
    case DVZ_COLOR_ROLE_SRGB_COLOR:
    case DVZ_COLOR_ROLE_LINEAR_COLOR:
    case DVZ_COLOR_ROLE_DATA:
        break;
    case DVZ_COLOR_ROLE_NONE:
    default:
        log_error("invalid sampled field color role %d", (int)desc->color_role);
        return false;
    }

    switch (desc->semantic)
    {
    case DVZ_FIELD_SEMANTIC_SCALAR:
    case DVZ_FIELD_SEMANTIC_VECTOR_2:
    case DVZ_FIELD_SEMANTIC_VECTOR_3:
    case DVZ_FIELD_SEMANTIC_LABEL:
    case DVZ_FIELD_SEMANTIC_NORMAL:
        if (desc->color_role != DVZ_COLOR_ROLE_DATA)
        {
            log_error(
                "sampled field semantic %d requires data color role, got %d",
                (int)desc->semantic, (int)desc->color_role);
            return false;
        }
        return true;
    case DVZ_FIELD_SEMANTIC_COLOR:
        if (desc->color_role == DVZ_COLOR_ROLE_DATA)
        {
            log_error("color sampled fields require srgb_color or linear_color role");
            return false;
        }
        return true;
    case DVZ_FIELD_SEMANTIC_GENERIC:
    default:
        return true;
    }
}



static bool _field_geometry_validate(const DvzFieldGeometry* geometry)
{
    if (geometry == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(geometry, DvzFieldGeometry, DVZ_FIELD_GEOMETRY_KNOWN_FLAGS))
    {
        log_error("invalid DvzFieldGeometry ABI prologue");
        return false;
    }
    return true;
}



DvzSampledFieldDesc dvz_sampled_field_desc(void)
{
    DvzSampledFieldDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc)};
    desc.dim = DVZ_FIELD_DIM_2D;
    desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
    desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    desc.color_role = DVZ_COLOR_ROLE_SRGB_COLOR;
    desc.depth = 1;
    return desc;
}



DvzFieldGeometry dvz_field_geometry(void)
{
    DvzFieldGeometry geometry = {DVZ_STRUCT_INIT_FIELDS(DvzFieldGeometry)};
    geometry.axis_order[0] = 0;
    geometry.axis_order[1] = 1;
    geometry.axis_order[2] = 2;
    geometry.spacing[0] = 1.0;
    geometry.spacing[1] = 1.0;
    geometry.spacing[2] = 1.0;
    return geometry;
}



uint32_t _scene_field_index(const DvzScene* scene, const DvzSampledField* field)
{
    if (scene == NULL || field == NULL)
        return UINT32_MAX;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
    {
        if (&scene->fields[i] == field && field->scene == scene)
            return i;
    }
    return UINT32_MAX;
}


/**
 * Reset one sampled-field slot to its empty state.
 *
 * @param field the field slot
 */
void _scene_field_reset(DvzSampledField* field)
{
    if (field == NULL)
        return;
    if (field->data != NULL)
    {
        dvz_free(field->data);
        field->data = NULL;
    }
    if (field->upload != NULL)
    {
        dvz_free(field->upload);
        field->upload = NULL;
        field->upload_size = 0;
    }
    dvz_memset(field, sizeof(DvzSampledField), 0, sizeof(DvzSampledField));
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Sampled fields                                                                               */
/*************************************************************************************************/

/**
 * Create a scene-owned sampled field.
 *
 * @param scene the scene
 * @param desc the field descriptor
 * @return the sampled field, or NULL on error
 */
DvzSampledField* dvz_sampled_field(DvzScene* scene, const DvzSampledFieldDesc* desc)
{
    ANN(scene);
    if (!_sampled_field_desc_validate(desc))
        return NULL;
    DvzSampledFieldDesc resolved = _sampled_field_desc_resolve(desc);
    if (!_sampled_field_color_role_validate(&resolved))
        return NULL;
    if (!_field_format_supported(resolved.format))
    {
        log_error("unsupported sampled field format %d", (int)resolved.format);
        return NULL;
    }
    if (resolved.dim != DVZ_FIELD_DIM_2D && resolved.dim != DVZ_FIELD_DIM_3D)
    {
        log_error("unsupported sampled field dimensionality %d", (int)resolved.dim);
        return NULL;
    }
    if (resolved.width == 0 || resolved.height == 0 || resolved.depth == 0)
    {
        log_error("sampled field dimensions must be non-zero");
        return NULL;
    }
    if (resolved.dim == DVZ_FIELD_DIM_2D && resolved.depth != 1)
    {
        log_error("2D sampled fields must use depth=1");
        return NULL;
    }
    uint64_t data_size = 0;
    if (!_field_expected_data_size(&resolved, &data_size))
    {
        log_error("sampled field size overflow");
        return NULL;
    }

    DvzSampledField* field = _scene_alloc_field_slot(scene);
    if (field == NULL)
    {
        log_error("maximum sampled field count reached");
        return NULL;
    }
    field->desc = resolved;
    field->data_size = data_size;
    field->geometry = dvz_field_geometry();
    field->dirty = false;
    field->dirty_full = false;
    return field;

}


/**
 * Destroy a sampled field.
 *
 * @param field the sampled field
 * @return true on success, false on error
 */
bool dvz_sampled_field_destroy(DvzSampledField* field)
{
    if (field == NULL)
        return false;
    if (!_scene_visual_mutation_allowed(field->scene, "destroy sampled field"))
        return false;
    _scene_release_field_bindings(field);
    _scene_field_reset(field);
    return true;
}



/**
 * Update the field geometry metadata.
 *
 * @param field the sampled field
 * @param geometry the geometry descriptor
 * @return true on success, false on error
 */
bool dvz_sampled_field_set_geometry(
    DvzSampledField* field, const DvzFieldGeometry* geometry)
{
    ANN(field);
    ANN(geometry);
    if (!_field_geometry_validate(geometry))
        return false;
    if (!_scene_visual_mutation_allowed(field->scene, "update sampled field geometry"))
        return false;
    field->geometry = *geometry;
    return true;
}



/**
 * Return the immutable field descriptor.
 *
 * @param field the sampled field
 * @return the descriptor, or NULL on error
 */
const DvzSampledFieldDesc* dvz_sampled_field_get_desc(const DvzSampledField* field)
{
    return field != NULL ? &field->desc : NULL;
}



/**
 * Allocate one free sampled-field slot from a scene.
 *
 * @param scene the scene
 * @return the zero-initialized slot, or NULL when full
 */
static DvzSampledField* _scene_alloc_field_slot(DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
    {
        DvzSampledField* field = &scene->fields[i];
        if (field->scene != NULL)
            continue;
        dvz_memset(field, sizeof(DvzSampledField), 0, sizeof(DvzSampledField));
        field->scene = scene;
        if (i + 1 > scene->field_count)
            scene->field_count = i + 1;
        return field;
    }
    return NULL;
}
