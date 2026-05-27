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
#include "_controller.h"
#include "_log.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/controller/panzoom.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define FLOAT_MIN +1e-5f
#define FLOAT_MAX +1e+5f
#define BOX_MIN   +1e-12
#define DVZ_PANZOOM_ZOOM_MIN_DEFAULT 1e-3f
#define DVZ_PANZOOM_ZOOM_MAX_DEFAULT 1e+4f

#if defined(__APPLE__)
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .003
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF  12.0
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


/**
 * Clamp one zoom factor to the configured limits.
 *
 * @param pz the panzoom controller
 * @param dim axis dimension
 * @param value requested zoom
 * @return clamped zoom
 */
static float _clamp_zoom_value(const DvzPanzoom* pz, uint32_t dim, float value)
{
    ANN(pz);
    if (!isfinite(value) || value <= 0.0f)
        value = 1.0f;
    if (!pz->has_zoom_limits || dim >= 2)
        return value;
    if (value < pz->zoom_min[dim])
        return pz->zoom_min[dim];
    if (value > pz->zoom_max[dim])
        return pz->zoom_max[dim];
    return value;
}



/**
 * Clamp current zoom and drag baseline to the configured limits.
 *
 * @param pz the panzoom controller
 */
static void _clamp_zoom(DvzPanzoom* pz)
{
    ANN(pz);
    for (uint32_t dim = 0; dim < 2; dim++)
    {
        pz->zoom[dim] = _clamp_zoom_value(pz, dim, pz->zoom[dim]);
        pz->zoom_center[dim] = _clamp_zoom_value(pz, dim, pz->zoom_center[dim]);
    }
}



static inline void _normalize_pos(DvzPanzoom* pz, vec2 in, vec2 out)
{
    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    out[0] = -1.0f + 2.0f * in[0] / w;
    out[1] = +1.0f - 2.0f * in[1] / h;
}



static inline bool _contains_pos(DvzPanzoom* pz, const vec2 pos)
{
    float x = pz->viewport_origin[0];
    float y = pz->viewport_origin[1];
    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    return pos[0] >= x && pos[0] < x + w && pos[1] >= y && pos[1] < y + h;
}



static inline void _local_pos(DvzPanzoom* pz, const vec2 in, vec2 out)
{
    out[0] = in[0] - pz->viewport_origin[0];
    out[1] = in[1] - pz->viewport_origin[1];
}



static bool _event_in_viewport(DvzPanzoom* pz, const DvzPointerEvent* ev)
{
    ANN(pz);
    ANN(ev);
    if (!pz->has_viewport)
        return true;

    switch (ev->type)
    {
    case DVZ_POINTER_EVENT_DRAG:
        if (ev->content.d.is_press_valid)
            return _contains_pos(pz, ev->content.d.press_pos);
        return _contains_pos(pz, ev->pos);

    case DVZ_POINTER_EVENT_DRAG_STOP:
        return pz->interacting;

    default:
        return _contains_pos(pz, ev->pos);
    }
}



static void _local_event(DvzPanzoom* pz, const DvzPointerEvent* ev, DvzPointerEvent* out)
{
    ANN(pz);
    ANN(ev);
    ANN(out);
    *out = *ev;
    if (!pz->has_viewport)
        return;

    _local_pos(pz, ev->pos, out->pos);
    if (ev->type == DVZ_POINTER_EVENT_DRAG || ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
    {
        _local_pos(pz, ev->content.d.press_pos, out->content.d.press_pos);
        _local_pos(pz, ev->content.d.last_pos, out->content.d.last_pos);
    }
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
/*  Input callback (registered with dvz_input_subscribe_event)                                   */
/*                                                                                               */
/*  We listen to union events rather than raw pointer events so we receive gesture-derived       */
/*  events (DRAG, DRAG_STOP, DOUBLE_CLICK) emitted by the per-window gesture handler via         */
/*  dvz_input_emit_event. Raw WHEEL events also reach us here because dvz_input_emit_pointer     */
/*  re-emits them onto the union event stream.                                                   */
/*************************************************************************************************/

static void _panzoom_input_callback(
    DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
{
    DvzPanzoom* pz = (DvzPanzoom*)user_data;
    if (ev->type == DVZ_INPUT_EVENT_POINTER)
    {
        dvz_panzoom_pointer(pz, &ev->content.pointer);
    }
    else if (ev->type == DVZ_INPUT_EVENT_RESIZE)
    {
        /* Track the actual window size so cursor-pixel shifts convert to NDC at
         * the correct rate (window_width is the cursor coordinate space). */
        const DvzInputResizeEvent* r = &ev->content.resize;
        if (!pz->has_viewport && r->window_width > 0 && r->window_height > 0)
            dvz_panzoom_resize(pz, (float)r->window_width, (float)r->window_height);
    }
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

/**
 * Return a default panzoom descriptor.
 *
 * @return the panzoom descriptor
 */
DvzPanzoomDesc dvz_panzoom_desc(void)
{
    return (DvzPanzoomDesc){
        .width = 800.0f,
        .height = 600.0f,
        .flags = 0,
    };
}



DvzPanzoom* _dvz_panzoom(float width, float height, int flags)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzPanzoom* pz = (DvzPanzoom*)dvz_calloc(1, sizeof(DvzPanzoom));
    pz->viewport_size[0] = width;
    pz->viewport_size[1] = height;
    pz->zoom_min[0] = pz->zoom_min[1] = DVZ_PANZOOM_ZOOM_MIN_DEFAULT;
    pz->zoom_max[0] = pz->zoom_max[1] = DVZ_PANZOOM_ZOOM_MAX_DEFAULT;
    pz->has_zoom_limits = true;
    pz->flags = flags;
    dvz_panzoom_reset(pz);
    return pz;
}


/**
 * Create a standalone panzoom controller.
 *
 * @param desc panzoom descriptor, or NULL for defaults
 * @return the panzoom controller, or NULL on allocation failure
 */
DvzPanzoom* dvz_panzoom_create(const DvzPanzoomDesc* desc)
{
    DvzPanzoomDesc default_desc = dvz_panzoom_desc();
    if (desc == NULL)
        desc = &default_desc;
    return _dvz_panzoom(desc->width, desc->height, desc->flags);
}



void dvz_panzoom_reset(DvzPanzoom* pz)
{
    ANN(pz);
    memset(pz->pan, 0, sizeof(pz->pan));
    memset(pz->pan_center, 0, sizeof(pz->pan_center));
    pz->zoom[0] = pz->zoom[1] = 1.0f;
    pz->zoom_center[0] = pz->zoom_center[1] = 1.0f;
    _clamp_zoom(pz);
    memset(pz->pan_locked, 0, sizeof(pz->pan_locked));
    memset(pz->zoom_locked, 0, sizeof(pz->zoom_locked));
}



void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height)
{
    ANN(pz);
    pz->viewport_size[0] = width;
    pz->viewport_size[1] = height;
}



/**
 * Update the viewport rectangle used for input filtering and coordinate normalization.
 *
 * @param pz the panzoom controller
 * @param x viewport x origin in window pixels
 * @param y viewport y origin in window pixels
 * @param width viewport width in window pixels
 * @param height viewport height in window pixels
 */
void dvz_panzoom_viewport(DvzPanzoom* pz, float x, float y, float width, float height)
{
    ANN(pz);
    pz->viewport_origin[0] = x;
    pz->viewport_origin[1] = y;
    pz->viewport_size[0] = width;
    pz->viewport_size[1] = height;
    pz->has_viewport = true;
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
    pz->zoom[0] = _clamp_zoom_value(pz, 0, zoom[0]);
    pz->zoom[1] = _clamp_zoom_value(pz, 1, zoom[1]);
}


/**
 * Set zoom limits.
 *
 * @param pz the panzoom controller
 * @param min_zoom minimum zoom factors
 * @param max_zoom maximum zoom factors
 * @return whether the limits were accepted
 */
bool dvz_panzoom_zoom_limits(DvzPanzoom* pz, vec2 min_zoom, vec2 max_zoom)
{
    ANN(pz);
    for (uint32_t dim = 0; dim < 2; dim++)
    {
        if (!isfinite(min_zoom[dim]) || !isfinite(max_zoom[dim]) || min_zoom[dim] <= 0.0f ||
            max_zoom[dim] < min_zoom[dim])
            return false;
    }
    glm_vec2_copy(min_zoom, pz->zoom_min);
    glm_vec2_copy(max_zoom, pz->zoom_max);
    pz->has_zoom_limits = true;
    _clamp_zoom(pz);
    return true;
}


/**
 * Return the visible VisualSpace extent from the current pan/zoom state.
 *
 * @param pz the panzoom controller
 * @param out extent as xmin, xmax, ymin, ymax
 * @return whether the extent was written
 */
bool dvz_panzoom_extent(const DvzPanzoom* pz, float out[4])
{
    ANN(pz);
    ANN(out);
    if (!isfinite(pz->zoom[0]) || !isfinite(pz->zoom[1]) || pz->zoom[0] <= 0.0f ||
        pz->zoom[1] <= 0.0f)
        return false;
    out[0] = -pz->pan[0] - 1.0f / pz->zoom[0];
    out[1] = -pz->pan[0] + 1.0f / pz->zoom[0];
    out[2] = -pz->pan[1] - 1.0f / pz->zoom[1];
    out[3] = -pz->pan[1] + 1.0f / pz->zoom[1];
    return true;
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
        pz->zoom[0] = _clamp_zoom_value(
            pz, 0, zx0 * expf((float)(DVZ_PANZOOM_ZOOM_DRAG_COEF) * a * shift[0]));
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->zoom[1] = _clamp_zoom_value(
            pz, 1, zy0 * expf((float)(DVZ_PANZOOM_ZOOM_DRAG_COEF) * a * shift[1]));

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
    if (!_event_in_viewport(pz, ev))
        return false;

    DvzPointerEvent local = {0};
    _local_event(pz, ev, &local);
    ev = &local;

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
        pz->interacting = true;
        break;

    case DVZ_POINTER_EVENT_DRAG_STOP:
        dvz_panzoom_end(pz);
        pz->interacting = false;
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
        pz->interacting = false;
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
    /* Adopt the router's current window dimensions if a resize has already fired
     * (the typical case: the GLFW backend emits a resize at window creation,
     * before the controller is connected). */
    DvzInputResizeEvent r;
    if (!pz->has_viewport && dvz_input_router_last_resize(router, &r) && r.window_width > 0 &&
        r.window_height > 0)
    {
        dvz_panzoom_resize(pz, (float)r.window_width, (float)r.window_height);
    }
    dvz_input_subscribe_event(router, _panzoom_input_callback, pz);
}



void dvz_panzoom_disconnect(DvzPanzoom* pz, DvzInputRouter* router)
{
    ANN(pz);
    ANN(router);
    dvz_input_unsubscribe_event(router, _panzoom_input_callback, pz);
}



void dvz_panzoom_destroy(DvzPanzoom* pz)
{
    ANN(pz);
    dvz_free(pz);
}
