/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene annotation                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Annotations                                                                                  */
/*************************************************************************************************/

/**
 * Return the default annotation descriptor.
 *
 * @return default annotation descriptor
 */
DVZ_EXPORT DvzAnnotationDesc dvz_annotation_desc(void);


/**
 * Return the default label annotation descriptor.
 *
 * @return default label descriptor
 */
DVZ_EXPORT DvzLabelDesc dvz_label_desc(void);


/**
 * Return the default scale-bar annotation descriptor.
 *
 * @return default scale-bar descriptor
 */
DVZ_EXPORT DvzScaleBarDesc dvz_scalebar_desc(void);


/**
 * Create a retained annotation object attached to a panel.
 *
 * @param panel the panel
 * @param desc the annotation descriptor
 * @return the annotation
 */
DVZ_EXPORT DvzAnnotation* dvz_annotation(DvzPanel* panel, const DvzAnnotationDesc* desc);


/**
 * Return the scene-local identity of an annotation.
 *
 * @param annotation the annotation
 * @return the scene-local identity, or DVZ_ID_NONE when annotation is NULL or destroyed
 */
DVZ_EXPORT DvzId dvz_annotation_id(const DvzAnnotation* annotation);


/**
 * Create a retained label annotation attached to a panel.
 *
 * @param panel the panel
 * @param desc the label descriptor
 * @return the annotation
 */
DVZ_EXPORT DvzAnnotation* dvz_annotation_label(DvzPanel* panel, const DvzLabelDesc* desc);


/**
 * Create a retained 2D scale-bar annotation attached to a panel.
 *
 * @param panel the panel
 * @param desc the scale-bar descriptor
 * @return the annotation
 */
DVZ_EXPORT DvzAnnotation* dvz_annotation_scalebar(
    DvzPanel* panel, const DvzScaleBarDesc* desc);


/**
 * Create a retained scale bar attached to a panel.
 *
 * `DvzScaleBar` is a typed alias for the retained annotation object returned here. Destroy it with
 * `dvz_annotation_destroy((DvzAnnotation*)scalebar)`.
 *
 * @param panel the panel
 * @return the scale bar
 */
DVZ_EXPORT DvzScaleBar* dvz_scalebar(DvzPanel* panel);


/**
 * Set the data dimension measured by a retained scale bar.
 *
 * @param scalebar the scale bar
 * @param dim dimension
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_scalebar_dimension(DvzScaleBar* scalebar, DvzDim dim);


/**
 * Set the panel anchor of a retained scale bar.
 *
 * @param scalebar the scale bar
 * @param anchor panel anchor
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_scalebar_anchor(DvzScaleBar* scalebar, DvzSceneAnchor anchor);


/**
 * Attach numeric units to a retained scale bar.
 *
 * @param scalebar the scale bar
 * @param units units object
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_scalebar_set_units(DvzScaleBar* scalebar, DvzUnits* units);


/**
 * Attach duration units to a retained scale bar.
 *
 * This is an alias for dvz_scalebar_set_units() in the first retained scale-bar slice.
 *
 * @param scalebar the scale bar
 * @param duration_units duration units object
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_scalebar_set_duration(DvzScaleBar* scalebar, DvzUnits* duration_units);


/**
 * Destroy a retained annotation object.
 *
 * Also destroys typed annotation aliases such as `DvzScaleBar` after casting to `DvzAnnotation*`.
 *
 * @param annotation the annotation
 */
DVZ_EXPORT void dvz_annotation_destroy(DvzAnnotation* annotation);


/**
 * Override formatting policy on an annotation.
 *
 * @param annotation the annotation
 * @param format the format descriptor, or NULL to clear the override
 */
DVZ_EXPORT void dvz_annotation_set_format(
    DvzAnnotation* annotation, const DvzFormatDesc* format);


EXTERN_C_OFF
