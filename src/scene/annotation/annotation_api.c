/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene annotation API                                                                       */
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
#include "core/scene_notify_internal.h"
#include "core/format_state_internal.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Annotations                                                                                  */
/*************************************************************************************************/

#define DVZ_ANNOTATION_DESC_KNOWN_FLAGS 0u
#define DVZ_LABEL_DESC_KNOWN_FLAGS      0u


static bool _annotation_desc_validate(const DvzAnnotationDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzAnnotationDesc, DVZ_ANNOTATION_DESC_KNOWN_FLAGS))
    {
        log_error("invalid annotation descriptor ABI");
        return false;
    }
    if (!_text_style_is_zero(&desc->style) && !_text_style_validate(&desc->style))
        return false;
    if (!_text_placement_is_zero(&desc->placement) &&
        !_text_placement_validate(&desc->placement))
        return false;
    return true;
}


static bool _label_desc_validate(const DvzLabelDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzLabelDesc, DVZ_LABEL_DESC_KNOWN_FLAGS))
    {
        log_error("invalid label descriptor ABI");
        return false;
    }
    if (!_text_style_is_zero(&desc->style) && !_text_style_validate(&desc->style))
        return false;
    if (!_text_placement_is_zero(&desc->placement) &&
        !_text_placement_validate(&desc->placement))
        return false;
    return true;
}


/**
 * Return the default annotation descriptor.
 *
 * @return default annotation descriptor
 */
DvzAnnotationDesc dvz_annotation_desc(void)
{
    return (DvzAnnotationDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzAnnotationDesc),
        .kind = DVZ_ANNOTATION_LABEL,
        .style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle)},
        .placement = {DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement)},
    };
}


/**
 * Return the default label annotation descriptor.
 *
 * @return default label descriptor
 */
DvzLabelDesc dvz_label_desc(void)
{
    return (DvzLabelDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzLabelDesc),
        .style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle)},
        .placement = {DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement)},
    };
}


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
    if (!_annotation_desc_validate(desc))
        return NULL;
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
    annotation->flags = desc->annotation_flags;
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
    if (!_label_desc_validate(desc))
        return NULL;
    return dvz_annotation(
        panel, &(DvzAnnotationDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzAnnotationDesc),
                   .kind = DVZ_ANNOTATION_LABEL,
                   .text = desc->text,
                   .style = desc->style,
                   .placement = desc->placement,
                   .annotation_flags = desc->label_flags});
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
    if (!_scene_format_desc_validate(format))
        return;
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
