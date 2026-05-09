/*
 * Draft public scene API companion for the next interaction/annotation pass.
 * This file records the intended ownership boundaries while the installed
 * headers are being drafted.
 */

/*************************************************************************************************/
/*  Scene public API draft                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/scene/annotation.h"
#include "datoviz/scene/interaction.h"
#include "datoviz/scene/text.h"



/*************************************************************************************************/
/*  Ownership notes                                                                              */
/*************************************************************************************************/

/*
 * Retained scene-owned opaque handles:
 * - DvzInteractionPolicy
 * - DvzSelection
 * - DvzLinkChannel
 * - DvzPinnedReadout
 * - DvzScale
 * - DvzColormap
 * - DvzColorbar
 * - DvzFont
 * - DvzText
 * - DvzAnnotation
 *
 * Public value-like structs:
 * - DvzPickRequest / DvzPickResult
 * - DvzProbeRequest / DvzProbeResult
 * - DvzSelectionDesc / DvzSelectionItem
 * - DvzFormatDesc
 * - DvzScaleDesc / DvzColormapDesc / DvzColorbarDesc / DvzColormapStop
 * - DvzFontDesc / DvzTextStyle / DvzTextPlacement / DvzTextDesc
 * - DvzAnnotationDesc / DvzLabelDesc
 *
 * Public result structs may reference opaque scene-owned handles where that
 * preserves semantic identity, for example DvzProbeResult.scale.
 */
