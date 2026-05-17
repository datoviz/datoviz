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

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Fonts                                                                                        */
/*************************************************************************************************/

/**
 * Create a scene-owned font resource.
 *
 * @param scene the scene
 * @param desc the font descriptor
 * @return the font, or NULL on allocation failure
 */
DvzFont* dvz_font(DvzScene* scene, const DvzFontDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (scene->font_count >= DVZ_SCENE_MAX_FONTS)
    {
        log_error("maximum font count reached");
        return NULL;
    }
    DvzFont* font = &scene->fonts[scene->font_count++];
    dvz_memset(font, sizeof(DvzFont), 0, sizeof(DvzFont));
    font->scene = scene;
    font->size_pts = desc->size_pts;
    font->face_index = desc->face_index;
    font->flags = desc->flags;
    font->version = 1;
    if (desc->path != NULL)
        dvz_strlcpy(font->path, desc->path, sizeof(font->path));
    if (desc->family != NULL)
        dvz_strlcpy(font->family, desc->family, sizeof(font->family));
    if (desc->style != NULL)
        dvz_strlcpy(font->style, desc->style, sizeof(font->style));
    return font;
}



/**
 * Destroy a scene-owned font resource.
 *
 * @param font the font
 */
void dvz_font_destroy(DvzFont* font)
{
    if (font == NULL)
        return;
    font->scene = NULL;
}



/*************************************************************************************************/
/*  Text                                                                                         */
/*************************************************************************************************/

/**
 * Create a retained text object attached to a panel.
 *
 * @param panel the panel
 * @param desc the text descriptor
 * @return the text object, or NULL on allocation failure
 */
DvzText* dvz_text(DvzPanel* panel, const DvzTextDesc* desc)
{
    ANN(panel);
    ANN(desc);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->text_count >= DVZ_SCENE_MAX_TEXTS)
    {
        log_error("maximum text count reached");
        return NULL;
    }
    if (desc->style.font != NULL && desc->style.font->scene != scene)
    {
        log_error("cannot bind a font from a different scene");
        return NULL;
    }
    DvzText* text = &scene->texts[scene->text_count++];
    dvz_memset(text, sizeof(DvzText), 0, sizeof(DvzText));
    text->scene = scene;
    text->panel = panel;
    text->style = desc->style;
    text->placement = desc->placement;
    text->flags = desc->flags;
    text->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    text->version = 1;
    if (desc->string != NULL)
        dvz_strlcpy(text->string, desc->string, sizeof(text->string));
    return text;
}



/**
 * Destroy a retained text object.
 *
 * @param text the text
 */
void dvz_text_destroy(DvzText* text)
{
    if (text == NULL)
        return;
    text->scene = NULL;
    text->panel = NULL;
}



/**
 * Update the content string on a retained text object.
 *
 * @param text the text
 * @param string the new string
 */
void dvz_text_set_string(DvzText* text, const char* string)
{
    ANN(text);
    text->string[0] = '\0';
    if (string != NULL)
        dvz_strlcpy(text->string, string, sizeof(text->string));
    text->dirty_flags |= DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    text->version++;
}



/**
 * Update the style on a retained text object.
 *
 * @param text the text
 * @param style the new style
 */
void dvz_text_set_style(DvzText* text, const DvzTextStyle* style)
{
    ANN(text);
    ANN(style);
    if (style->font != NULL && (text->scene == NULL || style->font->scene != text->scene))
    {
        log_error("cannot bind a font from a different scene");
        return;
    }
    text->style = *style;
    text->dirty_flags |= DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    text->version++;
}



/**
 * Update the placement on a retained text object.
 *
 * @param text the text
 * @param placement the new placement
 */
void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement)
{
    ANN(text);
    ANN(placement);
    text->placement = *placement;
    text->dirty_flags |= DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    text->version++;
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
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
}
