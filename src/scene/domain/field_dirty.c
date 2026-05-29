/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field dirty propagation                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "field_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Advance a retained visual texture version.
 *
 * @param visual the visual whose texture changed
 */
static void _visual_texture_bump_version(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->texture.version =
        _visual_family_state(visual)->texture.version == UINT64_MAX ? 1 : _visual_family_state(visual)->texture.version + 1;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Mark one visual texture upload state as clean after a successful emit.
 *
 * @param visual the visual
 */
void _scene_visual_texture_mark_clean(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->texture.dirty = false;
    _visual_family_state(visual)->texture.field_dirty = false;
    _visual_family_state(visual)->texture.field_dirty_full = false;
    dvz_memset(
        &_visual_family_state(visual)->texture.field_dirty_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));
}



/**
 * Mark one visual texture state as requiring an upload refresh.
 *
 * @param visual the visual
 */
void _scene_visual_texture_mark_dirty(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->texture.dirty = true;
    _visual_texture_bump_version(visual);
}



/**
 * Mark one visual texture state as requiring a full field upload.
 *
 * @param visual the visual
 * @param desc the sampled field descriptor
 */
void _scene_visual_texture_mark_full_dirty(
    DvzVisual* visual, const DvzSampledFieldDesc* desc)
{
    ANN(visual);
    ANN(desc);
    _visual_family_state(visual)->texture.dirty = true;
    _visual_family_state(visual)->texture.field_dirty = true;
    _visual_family_state(visual)->texture.field_dirty_full = true;
    _visual_family_state(visual)->texture.field_dirty_region = _field_full_region(desc);
    _visual_texture_bump_version(visual);
}



/**
 * Mark one visual texture state as dirty for one field subregion.
 *
 * @param visual the visual
 * @param desc the sampled field descriptor
 * @param region the dirty field region
 */
void _scene_visual_texture_mark_region_dirty(
    DvzVisual* visual, const DvzSampledFieldDesc* desc, DvzFieldRegion region)
{
    ANN(visual);
    ANN(desc);
    _visual_family_state(visual)->texture.dirty = true;
    _visual_texture_bump_version(visual);
    if (!_visual_family_state(visual)->texture.field_dirty)
    {
        _visual_family_state(visual)->texture.field_dirty = true;
        _visual_family_state(visual)->texture.field_dirty_full = false;
        _visual_family_state(visual)->texture.field_dirty_region = region;
        return;
    }
    if (_visual_family_state(visual)->texture.field_dirty_full)
    {
        _visual_family_state(visual)->texture.field_dirty_region = _field_full_region(desc);
        return;
    }
    if (!_field_regions_union(
            &_visual_family_state(visual)->texture.field_dirty_region, &region, &_visual_family_state(visual)->texture.field_dirty_region))
    {
        _visual_family_state(visual)->texture.field_dirty_full = true;
        _visual_family_state(visual)->texture.field_dirty_region = _field_full_region(desc);
    }
}



/**
 * Mark one sampled-field region and all bound visuals as dirty.
 *
 * @param field the sampled field
 * @param region dirty region
 * @param full whether the full field must be marked dirty
 */
void _scene_mark_field_region_dirty(DvzSampledField* field, DvzFieldRegion region, bool full)
{
    if (field == NULL || field->scene == NULL)
        return;
    if (full)
    {
        region = _field_full_region(&field->desc);
        field->dirty_region = region;
        field->dirty = true;
        field->dirty_full = true;
    }
    else if (!field->dirty)
    {
        field->dirty_region = region;
        field->dirty = true;
        field->dirty_full = false;
    }
    else if (field->dirty_full)
    {
        field->dirty_region = _field_full_region(&field->desc);
    }
    else if (!_field_regions_union(&field->dirty_region, &region, &field->dirty_region))
    {
        field->dirty_region = region;
        field->dirty_full = true;
    }
    else
    {
        field->dirty_full = false;
    }
    field->dirty = true;
    DvzScene* scene = field->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        if (visual->scene == scene && _visual_family_state(visual)->field == field)
        {
            if (full)
            {
                _scene_visual_texture_mark_full_dirty(visual, &field->desc);
            }
            else
                _scene_visual_texture_mark_region_dirty(visual, &field->desc, region);
            _scene_notify_visual_changed(visual);
        }
    }
}



/**
 * Refresh the sampled-field dirty region from all visuals bound to the field.
 *
 * @param scene the owning scene
 * @param field the sampled field
 */
void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field)
{
    if (scene == NULL || field == NULL || field->scene != scene)
        return;
    bool any_pending = false;
    bool any_full = false;
    DvzFieldRegion merged = {0};
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        const DvzVisual* visual = &scene->visuals[i];
        if (visual->scene != scene || _visual_family_state(visual)->field != field || !_visual_family_state(visual)->texture.field_dirty)
            continue;
        any_pending = true;
        if (_visual_family_state(visual)->texture.field_dirty_full)
        {
            any_full = true;
            merged = _field_full_region(&field->desc);
            break;
        }
        if (merged.width == 0 && merged.height == 0 && merged.depth == 0)
            merged = _visual_family_state(visual)->texture.field_dirty_region;
        else if (!_field_regions_union(&merged, &_visual_family_state(visual)->texture.field_dirty_region, &merged))
        {
            any_full = true;
            merged = _field_full_region(&field->desc);
            break;
        }
    }
    field->dirty = any_pending;
    field->dirty_full = any_full;
    if (any_pending)
        field->dirty_region = merged;
    else
        dvz_memset(&field->dirty_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));
}
