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
