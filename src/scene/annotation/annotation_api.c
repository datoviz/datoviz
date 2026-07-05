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
    DvzAnnotation* annotation = &scene->annotations[scene->annotation_count++];
    dvz_memset(annotation, sizeof(DvzAnnotation), 0, sizeof(DvzAnnotation));
    annotation->scene = scene;
    annotation->id = _scene_next_id(scene);
    annotation->panel = panel;
    annotation->kind = desc->kind;
    annotation->style = (DvzTextStyle){DVZ_STRUCT_INIT_FIELDS(DvzTextStyle)};
    annotation->placement = (DvzTextPlacement){DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement)};
    annotation->flags = desc->annotation_flags;
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version = 1;
    if (desc->text != NULL)
        dvz_strlcpy(annotation->text, desc->text, sizeof(annotation->text));
    _scene_notify_request_frame(panel->figure);
    return annotation;
}


DvzId dvz_annotation_id(const DvzAnnotation* annotation)
{
    return annotation != NULL && annotation->scene != NULL ? annotation->id : DVZ_ID_NONE;
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
                   .annotation_flags = desc->label_flags});
}


DvzResult dvz_annotation_set_style(DvzAnnotation* annotation, const DvzTextStyle* style)
{
    if (annotation == NULL || annotation->scene == NULL)
        return -1;
    if (style != NULL && !_text_style_validate(style))
        return -1;
    DvzTextStyle resolved = style != NULL ?
                                *style :
                                (DvzTextStyle){DVZ_STRUCT_INIT_FIELDS(DvzTextStyle)};
    if (resolved.font != NULL && resolved.font->scene != annotation->scene)
        return -1;
    annotation->style = resolved;
    annotation->dirty_flags |= DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}


DvzResult dvz_annotation_set_placement(
    DvzAnnotation* annotation, const DvzTextPlacement* placement)
{
    if (annotation == NULL || annotation->scene == NULL)
        return -1;
    if (placement != NULL && !_text_placement_validate(placement))
        return -1;
    annotation->placement = placement != NULL ?
                                *placement :
                                (DvzTextPlacement){DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement)};
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
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
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
}
