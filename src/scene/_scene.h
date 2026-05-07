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
#include "datoviz/scene/enums.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_MAX_FIGURES    16
#define DVZ_SCENE_MAX_PANELS     64
#define DVZ_SCENE_MAX_VISUALS    256
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



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzScene   DvzScene;
typedef struct DvzFigure  DvzFigure;
typedef struct DvzPanel   DvzPanel;
typedef struct DvzVisual  DvzVisual;



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

    /* Attribute slots — indexed by attr index (type-specific) */
    uint32_t      attr_count;
    DvzVisualAttr attrs[DVZ_SCENE_MAX_ITEM_ATTRS];
};



/*************************************************************************************************/
/*  DvzPanel                                                                                    */
/*************************************************************************************************/

struct DvzPanel
{
    DvzFigure*  figure;
    DvzPanelDesc desc; /* normalized position and size */

    uint32_t    visual_count;
    DvzVisual*  visuals[DVZ_SCENE_MAX_VISUALS]; /* weak refs — owned by scene */
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
};
