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
 * Destroy a retained annotation object.
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
