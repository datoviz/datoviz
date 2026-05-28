/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text preparation                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scale_ticks.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Update or create the internal glyph visual for one retained annotation label.
 *
 * @param figure the figure being emitted
 * @param annotation the annotation object
 * @return whether preparation succeeded
 */
static bool _annotation_prepare_visual(DvzFigure* figure, DvzAnnotation* annotation)
{
    ANN(figure);
    ANN(annotation);
    if (annotation->scene == NULL || annotation->panel == NULL ||
        annotation->panel->figure != figure)
        return true;
    if (annotation->kind == DVZ_ANNOTATION_SCALEBAR)
        return _scalebar_prepare_visual(figure, annotation);
    if (annotation->kind != DVZ_ANNOTATION_LABEL)
    {
        if (annotation->visual != NULL)
            dvz_visual_set_visible(annotation->visual, false);
        if (annotation->scalebar_visual != NULL)
            dvz_visual_set_visible(annotation->scalebar_visual, false);
        return true;
    }

    DvzText proxy = {0};
    proxy.scene = annotation->scene;
    proxy.panel = annotation->panel;
    dvz_strlcpy(proxy.string, annotation->text, sizeof(proxy.string));
    proxy.style = annotation->style;
    proxy.placement = annotation->placement;
    proxy.flags = annotation->flags;
    proxy.dirty_flags = annotation->dirty_flags;
    proxy.version = annotation->version;
    proxy.metrics = annotation->metrics;
    proxy.visual = annotation->visual;
    proxy.visual_version = annotation->visual_version;
    proxy.visual_figure_width = annotation->visual_figure_width;
    proxy.visual_figure_height = annotation->visual_figure_height;

    bool ok = _text_prepare_visual(figure, &proxy);
    annotation->metrics = proxy.metrics;
    annotation->visual = proxy.visual;
    annotation->visual_version = proxy.visual_version;
    annotation->visual_figure_width = proxy.visual_figure_width;
    annotation->visual_figure_height = proxy.visual_figure_height;
    if (ok)
        annotation->dirty_flags = proxy.dirty_flags;
    return ok;
}



/*************************************************************************************************/
/*  Internal text realization                                                                    */
/*************************************************************************************************/

/**
 * Prepare image-backed visuals for retained text attached to one figure.
 *
 * @param figure the figure being emitted
 */
void _scene_prepare_text_visuals(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    DvzScene* scene = figure->scene;
    for (uint32_t pass = 0; pass < 3; pass++)
    {
        uint64_t font_version_before = _text_scene_font_version_sum(scene);
        _scene_prepare_pinned_readout_cards(figure);
        _scene_prepare_selection_cards(figure);
        _scene_prepare_overlay_cards(figure);
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            uint32_t visual_count = panel->visual_count;
            for (uint32_t vi = 0; vi < visual_count; vi++)
            {
                DvzPanelAttach* attach = &panel->visuals[vi];
                DvzVisual* visual = attach->visual;
                if (visual != NULL && visual->type == DVZ_VISUAL_TYPE_TEXT &&
                    !_text_visual_prepare(figure, panel, attach, visual))
                {
                    log_error("failed to prepare batched text visual %u", vi);
                }
            }
        }
        for (uint32_t i = 0; i < scene->annotation_count; i++)
        {
            if (!_annotation_prepare_visual(figure, &scene->annotations[i]))
                log_error("failed to prepare retained annotation visual %u", i);
        }
        for (uint32_t i = 0; i < scene->text_count; i++)
        {
            if (!_text_prepare_visual(figure, &scene->texts[i]))
                log_error("failed to prepare retained text visual %u", i);
        }
        if (_text_scene_font_version_sum(scene) == font_version_before)
            break;
    }
}



