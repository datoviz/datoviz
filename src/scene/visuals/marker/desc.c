/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual descriptor lowering                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"

#include "point/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_marker_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    return _scene_point_like_visual_desc_from_metadata(
        emitter, meta, DVZ_SCENE_VISUAL_DESC_MARKER, DVZ_SCENE_POINT_LIKE_MARKER, true, out,
        error);
}
