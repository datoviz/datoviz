/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Panzoom controller                                                                           */
/*************************************************************************************************/
/* Advanced/unstable standalone controller internals. Prefer scene-owned controllers for ordinary
 * v0.4 scene/app use. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_PANZOOM_FLAGS_NONE        = 0x00,
    DVZ_PANZOOM_FLAGS_FIXED_X     = 0x01,
    DVZ_PANZOOM_FLAGS_FIXED_Y     = 0x02,
    DVZ_PANZOOM_FLAGS_KEEP_ASPECT = 0x04,
} DvzPanzoomFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzMVP DvzMVP;
typedef struct DvzPanzoomEval DvzPanzoomEval;
typedef struct DvzPanzoomResolved DvzPanzoomResolved;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMVP
{
    mat4     model;
    mat4     view;
    mat4     proj;
    float    time;
    uint32_t flags;
};


typedef enum
{
    DVZ_MVP_FLAGS_NONE = 0x00u,
    DVZ_MVP_FLAGS_ISOTROPIC_LOCAL = 0x01u,
} DvzMVPFlags;



struct DvzPanzoom
{
    vec2 viewport_origin;
    vec2 viewport_size;
    int  flags;
    bool has_viewport;
    bool interacting;

    vec2 pan;
    vec2 pan_center;
    vec2 zoom;
    vec2 zoom_center;
    vec2 zoom_min;
    vec2 zoom_max;
    bool has_zoom_limits;

    vec2 pan_lock;
    vec2 zoom_lock;
    bool pan_locked[2];
    bool zoom_locked[2];
};


struct DvzPanzoomEval
{
    float base_extent[4];
    float viewport_width;
    float viewport_height;
};


struct DvzPanzoomResolved
{
    DvzMVP mvp;
    float visible_extent[4];
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

typedef struct DvzPanzoomDesc DvzPanzoomDesc;



struct DvzPanzoomDesc
{
    uint32_t struct_size;
    uint32_t flags;
    float width;
    float height;
    uint32_t controller_flags;
};



DVZ_EXPORT DvzPanzoomDesc dvz_panzoom_desc(void);



/**
 * Create a standalone panzoom controller.
 *
 * @param desc panzoom descriptor, or NULL for defaults
 * @return the controller, or NULL on allocation failure
 */
DVZ_EXPORT DvzPanzoom* dvz_panzoom_create(const DvzPanzoomDesc* desc);



/**
 * Reset to the identity transform.
 */
DVZ_EXPORT void dvz_panzoom_reset(DvzPanzoom* pz);



/**
 * Update the viewport size (call on window resize).
 */
DVZ_EXPORT void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height);


/**
 * Update the viewport rectangle in window coordinates.
 */
DVZ_EXPORT void
dvz_panzoom_viewport(DvzPanzoom* pz, float x, float y, float width, float height);



/**
 * Set the pan offset in NDC.
 */
DVZ_EXPORT void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan);



/**
 * Set the zoom factors.
 */
DVZ_EXPORT void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom);


/**
 * Set zoom limits.
 *
 * @param pz the panzoom controller
 * @param min_zoom minimum zoom factors
 * @param max_zoom maximum zoom factors
 * @return whether the limits were accepted
 */
DVZ_EXPORT bool dvz_panzoom_zoom_limits(DvzPanzoom* pz, vec2 min_zoom, vec2 max_zoom);


/**
 * Return the visible extent in visual coordinates.
 *
 * @param pz the panzoom controller
 * @param out extent as xmin, xmax, ymin, ymax
 * @return whether the extent was written
 */
DVZ_EXPORT bool dvz_panzoom_extent(const DvzPanzoom* pz, float out[4]);



/**
 * Apply a pan shift (pixel delta).
 */
DVZ_EXPORT void dvz_panzoom_pan_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px);



/**
 * Apply a zoom shift driven by right-drag (pixel delta + anchor).
 */
DVZ_EXPORT void dvz_panzoom_zoom_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px);



/**
 * Apply a wheel zoom.
 */
DVZ_EXPORT void dvz_panzoom_zoom_wheel(DvzPanzoom* pz, vec2 dir, vec2 center_px);



/**
 * Commit the current pan/zoom as the new drag baseline (call at drag stop).
 */
DVZ_EXPORT void dvz_panzoom_end(DvzPanzoom* pz);



/**
 * Fill the view and proj matrices of an MVP struct from the current panzoom state.
 * The model matrix is left untouched.
 */
DVZ_EXPORT void dvz_panzoom_mvp(DvzPanzoom* pz, DvzMVP* mvp);

DVZ_EXPORT bool dvz_panzoom_resolve(
    const DvzPanzoom* panzoom, const DvzPanzoomEval* eval, DvzPanzoomResolved* out);



/**
 * Process a pointer event and update panzoom state.
 *
 * @returns true if the event was consumed
 */
DVZ_EXPORT bool dvz_panzoom_pointer(DvzPanzoom* pz, const DvzPointerEvent* ev);



/**
 * Subscribe the panzoom to an input router.
 * The panzoom pointer callback will be registered; call dvz_panzoom_disconnect() to remove it.
 */
DVZ_EXPORT void dvz_panzoom_connect(DvzPanzoom* pz, DvzInputRouter* router);



/**
 * Unsubscribe the panzoom from a router.
 */
DVZ_EXPORT void dvz_panzoom_disconnect(DvzPanzoom* pz, DvzInputRouter* router);



/**
 * Destroy the panzoom.
 */
DVZ_EXPORT void dvz_panzoom_destroy(DvzPanzoom* pz);



EXTERN_C_OFF
