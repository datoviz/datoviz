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
    ANN(desc);
    if (!_field_format_supported(desc->format))
    {
        log_error("unsupported sampled field format %d", (int)desc->format);
        return NULL;
    }
    if (desc->dim != DVZ_FIELD_DIM_2D && desc->dim != DVZ_FIELD_DIM_3D)
    {
        log_error("unsupported sampled field dimensionality %d", (int)desc->dim);
        return NULL;
    }
    if (desc->width == 0 || desc->height == 0 || desc->depth == 0)
    {
        log_error("sampled field dimensions must be non-zero");
        return NULL;
    }
    if (desc->dim == DVZ_FIELD_DIM_2D && desc->depth != 1)
    {
        log_error("2D sampled fields must use depth=1");
        return NULL;
    }
    uint64_t data_size = 0;
    if (!_field_expected_data_size(desc, &data_size))
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
    field->desc = *desc;
    field->data_size = data_size;
    field->geometry.axis_order[0] = 0;
    field->geometry.axis_order[1] = 1;
    field->geometry.axis_order[2] = 2;
    field->geometry.spacing[0] = 1.0;
    field->geometry.spacing[1] = 1.0;
    field->geometry.spacing[2] = 1.0;
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
const DvzSampledFieldDesc* dvz_sampled_field_desc(const DvzSampledField* field)
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
