/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Scene                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SCENE
#define DVZ_HEADER_SCENE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_enums.h"
#include "../_log.h"
#include "_obj.h"
#include "datoviz_defaults.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "mvp.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFigure DvzFigure;
typedef struct DvzPanel DvzPanel;
typedef struct DvzPanelLink DvzPanelLink;
typedef struct DvzScene DvzScene;

// Forward declarations.
typedef struct DvzApp DvzApp;
typedef struct DvzArcball DvzArcball;
typedef struct DvzAxes DvzAxes;
typedef struct DvzBatch DvzBatch;
typedef struct DvzCamera DvzCamera;
typedef struct DvzColorbar DvzColorbar;
typedef struct DvzFly DvzFly;
typedef struct DvzLegend DvzLegend;
typedef struct DvzList DvzList;
typedef struct DvzOrtho DvzOrtho;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzRef DvzRef;
typedef struct DvzTitle DvzTitle;
typedef struct DvzTransform DvzTransform;
typedef struct DvzView DvzView;
typedef struct DvzViewset DvzViewset;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Default object IDs used for empty textures bound to unused texture descriptors.
#define DVZ_SCENE_DEFAULT_TEX_ID     1
#define DVZ_SCENE_DEFAULT_SAMPLER_ID 2
#define DVZ_PANEL_GUI_MARGIN         20.0


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzScene
{
    DvzObject obj;
    DvzApp* app;
    DvzBatch* batch;
    DvzList* figures;
};



struct DvzPanelLink
{
    DvzPanel* target;
    DvzPanel* source;
    int flags;
};



struct DvzFigure
{
    DvzScene* scene;
    DvzList* panels;
    DvzList* links;
    vec2 shape, shape_init; // NOTE: in screen coordinates
    float scale;
    int flags;

    DvzViewset* viewset;
    DvzId canvas_id;
};



struct DvzPanel
{
    DvzFigure* figure;
    DvzView* view;                // has a list of visuals
    vec2 offset_init, shape_init; // initial viewport size
    DvzTransform* transform;
    DvzTransform* static_transform;

    DvzCamera* camera;
    DvzPanzoom* panzoom;
    DvzOrtho* ortho;
    DvzArcball* arcball;
    DvzFly* fly;

    // Panel axes and decorations.
    DvzRef* ref;
    DvzAxes* axes;
    DvzLegend* legend;
    DvzTitle* title;
    DvzColorbar* colorbar;
    DvzVisual* background;

    bool transform_to_destroy; // HACK: avoid double destruction with transform sharing
    bool is_press_valid;
    char* gui_title; // used for GUI panels
    int flags;
};



#endif
