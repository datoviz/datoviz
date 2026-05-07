/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Panzoom controller                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene/panzoom.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define FLOAT_MIN +1e-5f
#define FLOAT_MAX +1e+5f
#define BOX_MIN   +1e-12

#if defined(__APPLE__)
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .003
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF -12.0
#else
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .002
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF 120.0
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static inline bool _too_small(float v) { return fabsf(v) < FLOAT_MIN; }
static inline bool _too_large(float v) { return fabsf(v) > FLOAT_MAX; }
static inline bool _out_of_range(float v) { return _too_small(v) || _too_large(v); }



static inline void _normalize_pos(DvzPanzoom* pz, vec2 in, vec2 out)
{
    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    out[0] = -1.0f + 2.0f * in[0] / w;
    out[1] = +1.0f - 2.0f * in[1] / h;
}



static inline void _normalize_shift(DvzPanzoom* pz, vec2 in, vec2 out)
{
    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    out[0] = +2.0f * in[0] / w;
    out[1] = -2.0f * in[1] / h;
}



static inline void _get_view(DvzPanzoom* pz, mat4 view)
{
    float x = -pz->pan[0];
    float y = -pz->pan[1];
    glm_lookat((vec3){x, y, 2}, (vec3){x, y, 0}, (vec3){0, 1, 0}, view);
}



static inline void _get_proj(DvzPanzoom* pz, mat4 proj)
{
    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, +1.0f / zy, -10.0f, 10.0f, proj);
}



static void _lock_pan(DvzPanzoom* pz, int dim, bool cond)
{
    if (cond)
    {
        if (!pz->pan_locked[dim])
        {
            pz->pan_lock[dim] = pz->pan[dim];
            pz->pan_locked[dim] = true;
        }
        else
        {
            pz->pan[dim] = pz->pan_lock[dim];
            pz->pan_center[dim] = pz->pan_lock[dim];
        }
    }
    else
    {
        pz->pan_locked[dim] = false;
    }
}



static void _lock_zoom(DvzPanzoom* pz, int dim, bool cond)
{
    if (cond)
    {
        if (!pz->zoom_locked[dim])
        {
            pz->zoom_lock[dim] = pz->zoom[dim];
            pz->zoom_locked[dim] = true;
        }
        else
        {
            pz->zoom[dim] = pz->zoom_lock[dim];
            pz->zoom_center[dim] = pz->zoom_lock[dim];
        }
    }
    else
    {
        pz->zoom_locked[dim] = false;
    }
}



static void _lock(DvzPanzoom* pz)
{
    mat4 view = GLM_MAT4_IDENTITY_INIT;
    mat4 proj = GLM_MAT4_IDENTITY_INIT;
    mat4 proj_view = GLM_MAT4_IDENTITY_INIT;

    _get_view(pz, view);
    _get_proj(pz, proj);
    glm_mat4_mul(proj, view, proj_view);

    bool cond_pan_x  = _too_large(view[0][3]) || _too_large(view[3][0]);
    bool cond_pan_y  = _too_large(view[1][3]) || _too_large(view[3][1]);
    bool cond_zoom_x = _out_of_range(proj[0][0]);
    bool cond_zoom_y = _out_of_range(proj[1][1]);

    bool cond_panzoom_x =
        _out_of_range(proj_view[0][0]) || _too_large(proj_view[0][3]) ||
        _too_large(proj_view[3][0]);
    bool cond_panzoom_y =
        _out_of_range(proj_view[1][1]) || _too_large(proj_view[1][3]) ||
        _too_large(proj_view[3][1]);

    /* Clamp if the visible range collapses below floating-point precision. */
    float xrange = 2.0f / pz->zoom[0];
    float yrange = 2.0f / pz->zoom[1];
    bool cond_box_x = xrange < (float)BOX_MIN;
    bool cond_box_y = yrange < (float)BOX_MIN;

    cond_zoom_x |= cond_panzoom_x | cond_box_x;
    cond_zoom_y |= cond_panzoom_y | cond_box_y;

    _lock_pan(pz, 0, cond_pan_x);
    _lock_pan(pz, 1, cond_pan_y);
    _lock_zoom(pz, 0, cond_zoom_x);
    _lock_zoom(pz, 1, cond_zoom_y);
}



/*************************************************************************************************/
/*  Pointer callback (registered with dvz_input_subscribe_pointer)                              */
/*************************************************************************************************/

static void _panzoom_pointer_callback(
    DvzInputRouter* router, const DvzPointerEvent* ev, void* user_data)
{
    DvzPanzoom* pz = (DvzPanzoom*)user_data;
    dvz_panzoom_pointer(pz, ev);
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

DvzPanzoom* dvz_panzoom(float width, float height, int flags)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzPanzoom* pz = (DvzPanzoom*)calloc(1, sizeof(DvzPanzoom));
    pz->viewport_size[0] = width;
    pz->viewport_size[1] = height;
    pz->flags = flags;
    dvz_panzoom_reset(pz);
    return pz;
}



void dvz_panzoom_reset(DvzPanzoom* pz)
{
    ANN(pz);
    memset(pz->pan, 0, sizeof(pz->pan));
    memset(pz->pan_center, 0, sizeof(pz->pan_center));
    pz->zoom[0] = pz->zoom[1] = 1.0f;
    pz->zoom_center[0] = pz->zoom_center[1] = 1.0f;
    memset(pz->pan_locked, 0, sizeof(pz->pan_locked));
    memset(pz->zoom_locked, 0, sizeof(pz->zoom_locked));
}



void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height)
{
    ANN(pz);
    pz->viewport_size[0] = width;
    pz->viewport_size[1] = height;
}



void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan)
{
    ANN(pz);
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->pan[0] = pan[0];
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->pan[1] = pan[1];
}



void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom)
{
    ANN(pz);
    glm_vec2_copy(zoom, pz->zoom);
}



void dvz_panzoom_pan_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px)
{
    ANN(pz);
    (void)center_px;

    vec2 shift = {0};
    _normalize_shift(pz, shift_px, shift);

    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    ASSERT(zx > 0);
    ASSERT(zy > 0);

    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->pan[0] = pz->pan_center[0] + shift[0] / zx;
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->pan[1] = pz->pan_center[1] + shift[1] / zy;
}



void dvz_panzoom_zoom_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px)
{
    ANN(pz);

    vec2 shift = {0};
    _normalize_shift(pz, shift_px, shift);

    vec2 center = {0};
    _normalize_pos(pz, center_px, center);

    float zx0 = pz->zoom_center[0];
    float zy0 = pz->zoom_center[1];

    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    float a = .5f * (w + h);

    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->zoom[0] = zx0 * expf((float)(DVZ_PANZOOM_ZOOM_DRAG_COEF) * a * shift[0]);
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->zoom[1] = zy0 * expf((float)(DVZ_PANZOOM_ZOOM_DRAG_COEF) * a * shift[1]);

    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    ASSERT(zx > 0);
    ASSERT(zy > 0);

    float px = center[0] * (1.0f / zx0 - 1.0f / zx) * zx;
    float py = center[1] * (1.0f / zy0 - 1.0f / zy) * zy;

    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->pan[0] = pz->pan_center[0] - px / zx;
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->pan[1] = pz->pan_center[1] - py / zy;
}



void dvz_panzoom_zoom_wheel(DvzPanzoom* pz, vec2 dir, vec2 center_px)
{
    ANN(pz);

    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    ASSERT(w > 0);
    ASSERT(h > 0);
    float a = h / w;

    float d = dir[1];
    if (d != 0.0f)
    {
        d /= 4.0f;
        vec2 shift = {0};
        shift[0] = (float)DVZ_PANZOOM_ZOOM_WHEEL_COEF * d;
        shift[1] = -a * shift[0];
        dvz_panzoom_zoom_shift(pz, shift, center_px);
        dvz_panzoom_end(pz);
    }
}



void dvz_panzoom_end(DvzPanzoom* pz)
{
    ANN(pz);
    glm_vec2_copy(pz->pan, pz->pan_center);
    glm_vec2_copy(pz->zoom, pz->zoom_center);
}



void dvz_panzoom_mvp(DvzPanzoom* pz, DvzMVP* mvp)
{
    ANN(pz);
    ANN(mvp);
    _lock(pz);
    _get_view(pz, mvp->view);
    _get_proj(pz, mvp->proj);
}



bool dvz_panzoom_pointer(DvzPanzoom* pz, const DvzPointerEvent* ev)
{
    ANN(pz);
    ANN(ev);

    switch (ev->type)
    {
    case DVZ_POINTER_EVENT_DRAG:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT && ev->content.d.is_press_valid)
        {
            vec2 shift = {ev->content.d.shift[0], ev->content.d.shift[1]};
            dvz_panzoom_pan_shift(pz, shift, (vec2){0, 0});
        }
        else if (ev->button == DVZ_POINTER_BUTTON_RIGHT && ev->content.d.is_press_valid)
        {
            vec2 shift = {ev->content.d.shift[0], ev->content.d.shift[1]};
            vec2 press = {ev->content.d.press_pos[0], ev->content.d.press_pos[1]};
            if (pz->flags & DVZ_PANZOOM_FLAGS_KEEP_ASPECT)
                shift[1] = -shift[0];
            dvz_panzoom_zoom_shift(pz, shift, press);
        }
        break;

    case DVZ_POINTER_EVENT_DRAG_STOP:
        dvz_panzoom_end(pz);
        break;

    case DVZ_POINTER_EVENT_WHEEL:
    {
        vec2 dir = {ev->content.w.dir[0], ev->content.w.dir[1]};
        vec2 pos = {ev->pos[0], ev->pos[1]};
        dvz_panzoom_zoom_wheel(pz, dir, pos);
        break;
    }

    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        dvz_panzoom_reset(pz);
        break;

    default:
        return false;
    }

    return true;
}



void dvz_panzoom_connect(DvzPanzoom* pz, DvzInputRouter* router)
{
    ANN(pz);
    ANN(router);
    dvz_input_subscribe_pointer(router, _panzoom_pointer_callback, pz);
}



void dvz_panzoom_disconnect(DvzPanzoom* pz, DvzInputRouter* router)
{
    ANN(pz);
    ANN(router);
    dvz_input_unsubscribe_pointer(router, _panzoom_pointer_callback, pz);
}



void dvz_panzoom_destroy(DvzPanzoom* pz)
{
    ANN(pz);
    dvz_free(pz);
}
