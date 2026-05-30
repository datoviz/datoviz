/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Vector visual descriptor lowering                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "vector/internal.h"

#include "_assertions.h"
#include "_visual_pipeline_internal.h"
#include "path/internal.h"
#include "segment/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve vector descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_vector_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(emitter);
    ANN(meta);
    if (_scene_visual_meta_is_stroked_path(&emitter->resources, meta))
        return _scene_path_stroke_visual_desc_from_metadata(emitter, meta, out, error);
    return _scene_stroke_quad_visual_desc_from_metadata(emitter, meta, out, error);
}
