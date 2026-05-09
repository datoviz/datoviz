/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene internal types                                                                         */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_frame_plan.h"
#include "datoviz/drp2/types.h"
#include "datoviz/scene/arcball.h"
#include "datoviz/scene/enums.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/scene/panzoom.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_MAX_FIGURES    16
#define DVZ_SCENE_MAX_PANELS     64
#define DVZ_SCENE_MAX_VISUALS    256
#define DVZ_SCENE_MAX_SCALES     64
#define DVZ_SCENE_MAX_COLORMAPS  64
#define DVZ_SCENE_MAX_COLORBARS  64
#define DVZ_SCENE_MAX_PANEL_COLORBARS 16
#define DVZ_SCENE_MAX_COLOR_STOPS 32
#define DVZ_SCENE_MAX_ITEM_ATTRS 8



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_VISUAL_TYPE_NONE      = 0,
    DVZ_VISUAL_TYPE_POINT     = 1,
    DVZ_VISUAL_TYPE_PIXEL     = 2,
    DVZ_VISUAL_TYPE_MARKER    = 3,
    DVZ_VISUAL_TYPE_SEGMENT   = 4,
    DVZ_VISUAL_TYPE_PATH      = 5,
    DVZ_VISUAL_TYPE_IMAGE     = 6,
    DVZ_VISUAL_TYPE_MESH      = 7,
    DVZ_VISUAL_TYPE_VOLUME    = 8,
    DVZ_VISUAL_TYPE_PRIMITIVE = 9,
} DvzVisualType;



/* Image texture payload (first-slice: one RGBA8 texture per visual). */
typedef struct DvzVisualTexture DvzVisualTexture;
struct DvzVisualTexture
{
    const void* data;   /* borrowed RGBA8, row-major */
    uint32_t    width;  /* pixels */
    uint32_t    height; /* pixels */
    bool        dirty;  /* needs upload on next emit */
};



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzScene   DvzScene;
typedef struct DvzFigure  DvzFigure;
typedef struct DvzPanel   DvzPanel;
typedef struct DvzVisual  DvzVisual;
typedef struct DvzScale   DvzScale;
typedef struct DvzColormap DvzColormap;
typedef struct DvzColorbar DvzColorbar;



/*************************************************************************************************/
/*  Shared retained-object state                                                                 */
/*************************************************************************************************/

typedef struct DvzSceneFormatState DvzSceneFormatState;

struct DvzSceneFormatState
{
    int32_t precision;
    bool scientific;
    bool trim_trailing_zeros;
    bool show_unit;
    char unit[32];
    char prefix[DVZ_SCENE_LABEL_SIZE];
    char suffix[DVZ_SCENE_LABEL_SIZE];
};



/*************************************************************************************************/
/*  Scale / colormap / colorbar                                                                  */
/*************************************************************************************************/

struct DvzScale
{
    DvzScene* scene;
    DvzScaleKind kind;
    char label[DVZ_SCENE_LABEL_SIZE];
    char unit[32];
    DvzSceneFormatState format;
    double domain_min;
    double domain_max;
    double view_min;
    double view_max;
    bool has_domain;
    bool has_view_range;
    DvzColormap* colormap;
};


struct DvzColormap
{
    DvzScene* scene;
    DvzColormapKind kind;
    DvzBuiltinColormap builtin;
    double center;
    bool has_center;
    char label[DVZ_SCENE_LABEL_SIZE];
    uint32_t stop_count;
    DvzColormapStop stops[DVZ_SCENE_MAX_COLOR_STOPS];
};


struct DvzColorbar
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzScale* scale;
    DvzColorbarOrientation orientation;
    DvzSceneAnchor anchor;
    char title[DVZ_SCENE_LABEL_SIZE];
    uint32_t flags;
    bool has_format;
    DvzSceneFormatState format;
};



/*************************************************************************************************/
/*  Visual attribute slot                                                                        */
/*************************************************************************************************/

typedef struct DvzVisualAttr DvzVisualAttr;

struct DvzVisualAttr
{
    char     name[64];
    void*    data;
    uint64_t item_count;
    uint32_t item_size;         /* bytes per item */
    uint64_t dirty_first_item;  /* first dirty item index */
    uint64_t dirty_item_count;  /* number of dirty items (0 = not dirty) */
};



/*************************************************************************************************/
/*  DvzVisual                                                                                   */
/*************************************************************************************************/

struct DvzVisual
{
    DvzScene*    scene;
    DvzVisualType type;
    uint32_t     flags;
    bool         visible;
    int32_t      z_layer;

    DvzPrimitiveTopology topology; /* used by DVZ_VISUAL_TYPE_PRIMITIVE */
    DvzVisualTexture texture;      /* used by DVZ_VISUAL_TYPE_IMAGE */
    DvzScale*     scale;           /* first slice: image colormap scale */
    char          scale_slot[32];  /* semantic binding slot name */

    /* Attribute slots — indexed by attr index (type-specific) */
    uint32_t      attr_count;
    DvzVisualAttr attrs[DVZ_SCENE_MAX_ITEM_ATTRS];
};



/*************************************************************************************************/
/*  DvzPanel                                                                                    */
/*************************************************************************************************/

/* Per-visual attachment state on a panel. Stored alongside the visual pointer in the
 * panel's visuals array so the converter can sort by z_layer and choose APPLY vs FIXED MVP. */
typedef struct DvzPanelAttach
{
    DvzVisual*        visual;          /* weak ref — owned by scene */
    int32_t           z_layer;         /* signed; default 0 */
    DvzControllerMode controller_mode; /* default APPLY */
    uint32_t          insertion_index; /* used as stable tie-breaker when z_layer ties */
} DvzPanelAttach;



struct DvzPanel
{
    DvzFigure*  figure;
    DvzPanelDesc desc; /* normalized position and size */

    uint32_t       visual_count;
    DvzPanelAttach visuals[DVZ_SCENE_MAX_VISUALS];

    DvzPanzoom* panzoom; /* optional pan/zoom controller (owned) */
    DvzArcball* arcball; /* optional arcball controller (owned) */

    /* Optional background visual created by dvz_panel_set_background_*. The visual itself
     * lives in scene->visuals[] (weak ref); this pointer lets repeat calls update the
     * existing visual instead of stacking new ones. */
    DvzVisual* background_visual;

    uint32_t colorbar_count;
    DvzColorbar* colorbars[DVZ_SCENE_MAX_PANEL_COLORBARS];
};



/*************************************************************************************************/
/*  DvzFigure                                                                                   */
/*************************************************************************************************/

struct DvzFigure
{
    DvzScene*  scene;
    uint32_t   width;
    uint32_t   height;
    uint32_t   flags;

    uint32_t   panel_count;
    DvzPanel   panels[DVZ_SCENE_MAX_PANELS];
};



/*************************************************************************************************/
/*  DvzScene                                                                                    */
/*************************************************************************************************/

struct DvzScene
{
    DvzCapabilitySnapshot caps;

    DvzFramePlanEmitter* emitter; /* shared across all figures — owns GPU resource key→ID map */

    uint32_t outstanding_emitted_streams;

    uint32_t  figure_count;
    DvzFigure figures[DVZ_SCENE_MAX_FIGURES];

    uint32_t  visual_count;
    DvzVisual visuals[DVZ_SCENE_MAX_VISUALS]; /* owner of all visual objects */

    uint32_t scale_count;
    DvzScale scales[DVZ_SCENE_MAX_SCALES];

    uint32_t colormap_count;
    DvzColormap colormaps[DVZ_SCENE_MAX_COLORMAPS];

    uint32_t colorbar_count;
    DvzColorbar colorbars[DVZ_SCENE_MAX_COLORBARS];
};
