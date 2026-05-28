/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Volume visual bounds                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "bounds_internal.h"
#include "volume/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Compute bounds for a volume visual from its retained volume state.
 *
 * @param visual the volume visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _volume_bounds_from_state(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return false;
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(visual->volume.bounds_min[i]) || !isfinite(visual->volume.bounds_max[i]))
            return false;
    }
    _bounds_include_point(
        out, visual->volume.bounds_min[0], visual->volume.bounds_min[1],
        visual->volume.bounds_min[2]);
    _bounds_include_point(
        out, visual->volume.bounds_max[0], visual->volume.bounds_max[1],
        visual->volume.bounds_max[2]);
    return out->valid;
}



/**
 * Resolve bounds for a volume visual through the visual-family registry.
 *
 * @param visual the volume visual
 * @param out output bounds
 * @param out_force_3d output flag indicating whether flat bounds should still be treated as 3D
 * @return whether bounds were produced
 */
bool _scene_volume_visual_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d)
{
    ANN(visual);
    ANN(out);
    ANN(out_force_3d);
    *out_force_3d = true;
    return _volume_bounds_from_state(visual, out);
}
