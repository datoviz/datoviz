/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Text visual API                                                                              */
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
#include "text/internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an internal batched text visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_TEXT, flags);
    if (visual != NULL)
    {
        visual->alpha_mode = DVZ_ALPHA_BLENDED;
        visual->depth_test_enabled = false;
        _visual_family_state(visual)->text.renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    }
    return visual;
}



/**
 * Select the renderer used by an internal batched text visual.
 *
 * @param visual text visual
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
int _scene_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_TEXT)
    {
        log_error("dvz_text_set_renderer requires a text visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate text renderer"))
        return -1;
    if (renderer != DVZ_TEXT_RENDERER_AUTO && renderer != DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS &&
        renderer != DVZ_TEXT_RENDERER_BITMAP_ATLAS && renderer != DVZ_TEXT_RENDERER_MSDF_ATLAS)
    {
        log_error("text renderer %d is not implemented for batched text visuals yet", renderer);
        return -1;
    }
    if (_visual_family_state(visual)->text.renderer != renderer)
    {
        _visual_family_state(visual)->text.renderer = renderer;
        _visual_family_state(visual)->text.renderer_version++;
        _scene_notify_visual_changed(visual);
    }
    return 0;
}



/**
 * Resolve a generated adornment text renderer to a supported internal renderer.
 *
 * @param renderer requested renderer
 * @return supported renderer for generated labels
 */
DvzTextRenderer _scene_adornment_text_renderer(DvzTextRenderer renderer)
{
    switch (renderer)
    {
    case DVZ_TEXT_RENDERER_AUTO:
    case DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS:
    case DVZ_TEXT_RENDERER_BITMAP_ATLAS:
    case DVZ_TEXT_RENDERER_MSDF_ATLAS:
        return renderer;
    default:
        return DVZ_TEXT_RENDERER_MSDF_ATLAS;
    }
}



/**
 * Create an internal text visual for scene-generated labels.
 *
 * @param scene the scene
 * @param renderer requested renderer
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* _scene_adornment_text_visual(DvzScene* scene, DvzTextRenderer renderer)
{
    ANN(scene);
    DvzVisual* visual = _scene_text_visual(scene, 0);
    if (visual == NULL)
        return NULL;
    if (_scene_adornment_text_visual_set_renderer(visual, renderer) != 0)
        return NULL;
    return visual;
}



/**
 * Select the renderer for an internal scene-generated label text visual.
 *
 * @param visual text visual
 * @param renderer requested renderer
 * @return 0 on success, -1 on error
 */
int _scene_adornment_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer)
{
    ANN(visual);
    return _scene_text_visual_set_renderer(visual, _scene_adornment_text_renderer(renderer));
}
