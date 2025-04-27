/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Viewset                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VIEWSET
#define DVZ_HEADER_VIEWSET



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_atomic.h"
#include "../_enums.h"
#include "../_obj.h"
#include "datoviz_types.h"
#include "dual.h"
#include "mvp.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Mouse reference.
typedef enum
{
    DVZ_MOUSE_REFERENCE_GLOBAL, // global coordinate system, (0,0) -> (w,h) in screen coords
    DVZ_MOUSE_REFERENCE_LOCAL,  // (x0, y0) -> (vw, vh) in screen coords, relative to the View
    DVZ_MOUSE_REFERENCE_SCALED, // like local but rescaled in (-1,1)
} DvzMouseReference;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzView DvzView;
typedef struct DvzViewset DvzViewset;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzList DvzList;
typedef struct DvzTransform DvzTransform;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzView
{
    DvzViewset* viewset; // reference to the parent viewset
    DvzDual dual;
    vec2 offset, shape; // in framebuffer pixels
    vec4 margins;
    float scale;      // scale (multiplied by the window's content scale)
    DvzList* visuals; // list of visuals in the view
    bool is_visible;
};



struct DvzViewset
{
    DvzBatch* batch;
    DvzAtomic status;
    DvzId canvas_id;
    float scale;
    DvzList* views;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Viewset                                                                                      */
/*************************************************************************************************/

/**
 *
 */
DvzViewset* dvz_viewset(DvzBatch* batch, DvzId canvas_id);



/**
 *
 */
void dvz_viewset_clear(DvzViewset* viewset);



/**
 *
 */
void dvz_viewset_build(DvzViewset* viewset);



/**
 *
 */
void dvz_viewset_destroy(DvzViewset* viewset);



/*************************************************************************************************/
/*  View                                                                                         */
/*************************************************************************************************/

/**
 *
 */
DvzView* dvz_view(DvzViewset* viewset, vec2 offset, vec2 shape);



/**
 *
 */
void dvz_view_add(
    DvzView* view, DvzVisual* visual,                 //
    uint32_t first, uint32_t count,                   // items
    uint32_t first_instance, uint32_t instance_count, // instances
    DvzTransform* transform, int viewport_flags);     // transform and viewport flags



/**
 *
 */
DvzMouseEvent
dvz_view_mouse(DvzView* view, DvzMouseEvent ev, float content_scale, DvzMouseReference ref);



/**
 *
 */
void dvz_view_clear(DvzView* view);



/**
 *
 */
void dvz_view_resize(DvzView* view, vec2 offset, vec2 shape);



/**
 *
 */
void dvz_view_margins(DvzView* view, vec4 margins);



/**
 *
 */
void dvz_view_destroy(DvzView* view);



EXTERN_C_OFF

#endif
