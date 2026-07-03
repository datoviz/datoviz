/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Sphere visual API                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Mirror the retained sphere mode into the material payload spare slot.
 *
 * @param visual the visual
 */
void _sphere_params_sync_mode(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_SPHERE)
        return;
    _visual_family_state(visual)->material_params.depth_cue_extra[3] = (float)_visual_family_state(visual)->sphere_mode;
}



/**
 * Create a sphere impostor visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_sphere(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_SPHERE, flags);
    if (visual == NULL)
        return NULL;
    _visual_family_state(visual)->topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
    _visual_family_state(visual)->sphere_mode = DVZ_SPHERE_MODE_FAST_IMPOSTOR;
    _sphere_params_sync_mode(visual);
    _visual_family_state(visual)->material_params_dirty = true;
    return visual;
}



/**
 * Set the sphere impostor rendering mode.
 *
 * @param visual the sphere visual
 * @param mode the rendering mode
 * @return 0 on success, -1 on error
 */
DvzResult dvz_sphere_set_mode(DvzVisual* visual, DvzSphereMode mode)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_SPHERE)
    {
        log_error("dvz_sphere_set_mode requires a sphere visual");
        return -1;
    }
    if (mode != DVZ_SPHERE_MODE_FAST_IMPOSTOR && mode != DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR)
    {
        log_error("invalid sphere rendering mode");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update sphere mode"))
        return -1;

    if (_visual_family_state(visual)->sphere_mode == mode)
        return 0;
    _visual_family_state(visual)->sphere_mode = mode;
    _sphere_params_sync_mode(visual);
    _visual_bump_version(&visual->material.version);
    _visual_family_state(visual)->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}
