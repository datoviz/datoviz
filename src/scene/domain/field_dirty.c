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
#include "field_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
        if (visual->scene != scene || visual->field != field || !visual->texture.field_dirty)
            continue;
        any_pending = true;
        if (visual->texture.field_dirty_full)
        {
            any_full = true;
            merged = _field_full_region(&field->desc);
            break;
        }
        if (merged.width == 0 && merged.height == 0 && merged.depth == 0)
            merged = visual->texture.field_dirty_region;
        else if (!_field_regions_union(&merged, &visual->texture.field_dirty_region, &merged))
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
