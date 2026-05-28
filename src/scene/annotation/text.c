/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text / annotation                                                                       */
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



/*************************************************************************************************/
/*  Text                                                                                         */
/*************************************************************************************************/

/**
 * Return the default retained text style.
 *
 * @param scene the scene
 * @return default text style
 */
static DvzTextStyle _text_default_style(const DvzScene* scene)
{
    float size_px = dvz_font_defaults().text_size_px;
    if (scene != NULL && scene->font_defaults.text_size_px > 0.0f)
        size_px = scene->font_defaults.text_size_px;
    DvzTextStyle style = {0};
    style.size_px = size_px;
    style.renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    style.color[0] = 255;
    style.color[1] = 255;
    style.color[2] = 255;
    style.color[3] = 255;
    return style;
}



/**
 * Return the default retained text placement.
 *
 * @return default text placement
 */
static DvzTextPlacement _text_default_placement(void)
{
    DvzTextPlacement placement = {0};
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    return placement;
}



/**
 * Return whether a renderer enum is implemented by the retained text path.
 *
 * @param renderer the renderer
 * @return whether the renderer is supported
 */
static bool _text_renderer_supported(DvzTextRenderer renderer)
{
    return renderer == DVZ_TEXT_RENDERER_AUTO ||
           renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS;
}



/**
 * Mark a retained text object dirty and request a frame.
 *
 * @param text the text object
 * @param flags dirty flags
 */
static void _text_mark_dirty(DvzText* text, uint32_t flags)
{
    ANN(text);
    text->dirty_flags |= flags;
    text->version++;
    _scene_notify_request_frame(text->panel != NULL ? text->panel->figure : NULL);
}



/**
 * Create a retained text object attached to a panel.
 *
 * @param panel the panel
 * @param flags creation flags
 * @return the text object, or NULL on allocation failure
 */
DvzText* dvz_text(DvzPanel* panel, uint32_t flags)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->text_count >= DVZ_SCENE_MAX_TEXTS)
    {
        log_error("maximum text count reached");
        return NULL;
    }
    DvzText* text = &scene->texts[scene->text_count++];
    dvz_memset(text, sizeof(DvzText), 0, sizeof(DvzText));
    text->scene = scene;
    text->panel = panel;
    text->style = _text_default_style(scene);
    text->placement = _text_default_placement();
    text->flags = flags;
    text->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    text->version = 1;
    _scene_notify_request_frame(panel->figure);
    return text;
}



/**
 * Destroy a retained text object.
 *
 * @param text the text object
 */
void dvz_text_destroy(DvzText* text)
{
    if (text == NULL)
        return;
    if (text->visual != NULL)
        dvz_visual_set_visible(text->visual, false);
    _scene_notify_request_frame(text->panel != NULL ? text->panel->figure : NULL);
    text->scene = NULL;
    text->panel = NULL;
    text->string[0] = '\0';
    text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
}



/**
 * Set the UTF-8 content of a retained text object.
 *
 * @param text the text object
 * @param string the string, or NULL to clear
 */
void dvz_text_set_string(DvzText* text, const char* string)
{
    ANN(text);
    const char* src = string != NULL ? string : "";
    if (strcmp(text->string, src) == 0)
        return;
    dvz_strlcpy(text->string, src, sizeof(text->string));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
}



/**
 * Set the style of a retained text object.
 *
 * @param text the text object
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_text_set_style(DvzText* text, const DvzTextStyle* style)
{
    ANN(text);
    DvzTextStyle resolved = style != NULL ? *style : _text_default_style(text->scene);
    if (resolved.font != NULL && resolved.font->scene != text->scene)
    {
        log_error("cannot bind a font from a different scene");
        return -1;
    }
    if (!_text_renderer_supported(resolved.renderer))
    {
        log_error("text renderer %d is not implemented for retained text yet", resolved.renderer);
        return -1;
    }
    text->style = resolved;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}



/**
 * Set the placement of a retained text object.
 *
 * @param text the text object
 * @param placement the placement descriptor, or NULL for defaults
 */
void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement)
{
    ANN(text);
    text->placement = placement != NULL ? *placement : _text_default_placement();
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT);
}



/**
 * Select the renderer used by a retained text object.
 *
 * @param text the text object
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
int dvz_text_set_renderer(DvzText* text, DvzTextRenderer renderer)
{
    ANN(text);
    if (!_text_renderer_supported(renderer))
    {
        log_error("text renderer %d is not implemented for retained text yet", renderer);
        return -1;
    }
    if (text->style.renderer == renderer)
        return 0;
    text->style.renderer = renderer;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}



/*************************************************************************************************/
/*  Annotations                                                                                  */
/*************************************************************************************************/

/**
 * Create a retained annotation object attached to a panel.
 *
 * @param panel the panel
 * @param desc the annotation descriptor
 * @return the annotation, or NULL on allocation failure
 */
DvzAnnotation* dvz_annotation(DvzPanel* panel, const DvzAnnotationDesc* desc)
{
    ANN(panel);
    ANN(desc);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->annotation_count >= DVZ_SCENE_MAX_ANNOTATIONS)
    {
        log_error("maximum annotation count reached");
        return NULL;
    }
    if (desc->style.font != NULL && desc->style.font->scene != scene)
    {
        log_error("cannot bind a font from a different scene");
        return NULL;
    }
    DvzAnnotation* annotation = &scene->annotations[scene->annotation_count++];
    dvz_memset(annotation, sizeof(DvzAnnotation), 0, sizeof(DvzAnnotation));
    annotation->scene = scene;
    annotation->panel = panel;
    annotation->kind = desc->kind;
    annotation->style = desc->style;
    annotation->placement = desc->placement;
    annotation->flags = desc->flags;
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version = 1;
    if (desc->text != NULL)
        dvz_strlcpy(annotation->text, desc->text, sizeof(annotation->text));
    _scene_notify_request_frame(panel->figure);
    return annotation;
}



/**
 * Create a retained label annotation.
 *
 * @param panel the panel
 * @param desc the label descriptor
 * @return the annotation, or NULL on allocation failure
 */
DvzAnnotation* dvz_annotation_label(DvzPanel* panel, const DvzLabelDesc* desc)
{
    ANN(desc);
    return dvz_annotation(
        panel, &(DvzAnnotationDesc){
                   .kind = DVZ_ANNOTATION_LABEL,
                   .text = desc->text,
                   .style = desc->style,
                   .placement = desc->placement,
                   .flags = desc->flags});
}



/**
 * Destroy a retained annotation object.
 *
 * @param annotation the annotation
 */
void dvz_annotation_destroy(DvzAnnotation* annotation)
{
    if (annotation == NULL)
        return;
    if (annotation->visual != NULL)
        dvz_visual_set_visible(annotation->visual, false);
    if (annotation->scalebar_visual != NULL)
        dvz_visual_set_visible(annotation->scalebar_visual, false);
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    annotation->scene = NULL;
    annotation->panel = NULL;
    annotation->has_format = false;
}



/**
 * Override formatting policy on an annotation.
 *
 * @param annotation the annotation
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_annotation_set_format(DvzAnnotation* annotation, const DvzFormatDesc* format)
{
    ANN(annotation);
    annotation->has_format = format != NULL;
    _scene_format_state_copy(&annotation->format, format);
    if (annotation->kind == DVZ_ANNOTATION_SCALEBAR)
    {
        if (format != NULL)
            annotation->scalebar.format = *format;
        else
            dvz_memset(
                &annotation->scalebar.format, sizeof(DvzFormatDesc), 0, sizeof(DvzFormatDesc));
    }
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
}
