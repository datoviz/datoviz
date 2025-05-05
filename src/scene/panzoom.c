/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/panzoom.h"
#include "datoviz.h"
#include "mouse.h"
#include "scene/box.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BOX_MIN   +1e-12
#define FLOAT_MIN +1e-5
#define FLOAT_MAX +1e+5

// NOTE: the wheel behavior depends on both parameters, so first finetune DRAG_COEF with right-drag
// before tweaking WHEEL_COEFF.
#if OS_MACOS
// HACK: touchpad wheel too sensitive on macOS
// TODO
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .003
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF -12.0
#else
// TODO: test on linux/Windows.
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .002
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF 120.0
#endif



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/

static inline bool _is_vec2_null(vec2 v) { return memcmp(v, (vec2){0, 0}, sizeof(vec2)); }



static inline bool _is_dvec2_null(dvec2 v) { return fabs(v[0]) < 1e-12 && fabs(v[1]) < 1e-12; }



static inline bool _too_small(float v) { return fabsf(v) < FLOAT_MIN; }



static inline bool _too_large(float v) { return fabsf(v) > FLOAT_MAX; }



static inline bool _out_of_range(float v) { return _too_small(v) || _too_large(v); }



static inline void _normalize_pos(DvzPanzoom* pz, vec2 in, vec2 out)
{
    // From pixel coordinates (origin is top left corner) to NDC (origin is center of
    // viewport)
    float x = in[0];
    float y = in[1];

    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];

    out[0] = -1 + 2 * x / w;
    out[1] = +1 - 2 * y / h;
}



static inline void _normalize_shift(DvzPanzoom* pz, vec2 in, vec2 out)
{
    float x = in[0];
    float y = in[1];

    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];

    out[0] = +2 * x / w;
    out[1] = -2 * y / h;
}



/*************************************************************************************************/
/*  Panzoom functions                                                                            */
/*************************************************************************************************/

DvzPanzoom* dvz_panzoom(float width, float height, int flags)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_debug("create panzoom of size %.0fx%.0f", width, height);
    // width, height are the inner viewport size
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

    pz->pan[0] = 0;
    pz->pan[1] = 0;

    pz->pan_center[0] = 0;
    pz->pan_center[1] = 0;

    pz->zoom[0] = 1;
    pz->zoom[1] = 1;

    pz->zoom_center[0] = 1;
    pz->zoom_center[1] = 1;
}



void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height)
{
    ANN(pz);
    pz->viewport_size[0] = width;
    pz->viewport_size[1] = height;
}



void dvz_panzoom_flags(DvzPanzoom* pz, int flags)
{
    ANN(pz);
    pz->flags = flags;
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
    _vec2_copy(zoom, pz->zoom);
}



float dvz_panzoom_level(DvzPanzoom* pz, DvzDim dim)
{
    ANN(pz);
    ASSERT(dim < 2);
    return pz->zoom[dim];
}



void dvz_panzoom_pan_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px)
{
    // NOTE: center_px is unused here
    ANN(pz);

    vec2 shift = {0};
    _normalize_shift(pz, shift_px, shift);

    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    ASSERT(zx > 0);
    ASSERT(zy > 0);

    float x0 = pz->pan_center[0];
    float y0 = pz->pan_center[1];

    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->pan[0] = x0 + shift[0] / zx;
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->pan[1] = y0 + shift[1] / zy;
}



void dvz_panzoom_zoom_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px)
{
    // NOTE: center_px is the center of the zoom, in pixel coordinates (origin at the top left
    // corner of the viewport)
    ANN(pz);

    vec2 shift = {0};
    // From pixel coordinates to NDC coordinates.
    _normalize_shift(pz, shift_px, shift);

    vec2 center = {0};
    _normalize_pos(pz, center_px, center);

    float zx0 = pz->zoom_center[0];
    float zy0 = pz->zoom_center[1];

    // HACK: coefficient depends onthe viewport size.
    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    float a = .5 * (w + h);

    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->zoom[0] = zx0 * exp(DVZ_PANZOOM_ZOOM_DRAG_COEF * a * shift[0]);
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->zoom[1] = zy0 * exp(DVZ_PANZOOM_ZOOM_DRAG_COEF * a * shift[1]);

    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    ASSERT(zx > 0);
    ASSERT(zy > 0);

    // Update pan.
    float px = center[0] * (1.0f / zx0 - 1.0f / zx) * zx;
    float py = center[1] * (1.0f / zy0 - 1.0f / zy) * zy;

    float x0 = pz->pan_center[0];
    float y0 = pz->pan_center[1];

    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_X))
        pz->pan[0] = x0 - px / zx;
    if (!(pz->flags & DVZ_PANZOOM_FLAGS_FIXED_Y))
        pz->pan[1] = y0 - py / zy;
}



void dvz_panzoom_zoom_wheel(DvzPanzoom* pz, vec2 dir, vec2 center_px)
{
    ANN(pz);

    float w = pz->viewport_size[0];
    float h = pz->viewport_size[1];
    ASSERT(w > 0);
    ASSERT(h > 0);
    // Aspect ratio.
    float a = h / w;

    float d = dir[1];
    if (d != 0)
    {
        // d /= (float)fabs((double)d);
        d /= 4.0;
        vec2 shift = {0};
        shift[0] = DVZ_PANZOOM_ZOOM_WHEEL_COEF * (d);
        shift[1] = -a * shift[0];
        dvz_panzoom_zoom_shift(pz, shift, center_px);
        dvz_panzoom_end(pz);
    }
}



void dvz_panzoom_end(DvzPanzoom* pz)
{
    ANN(pz);

    pz->pan_center[0] = pz->pan[0];
    pz->pan_center[1] = pz->pan[1];

    pz->zoom_center[0] = pz->zoom[0];
    pz->zoom_center[1] = pz->zoom[1];
}



void dvz_panzoom_extent(DvzPanzoom* pz, DvzBox* box)
{
    ANN(pz);

    float xmin = -pz->pan[0] - 1.0f / pz->zoom[0];
    float xmax = -pz->pan[0] + 1.0f / pz->zoom[0];
    float ymin = -pz->pan[1] - 1.0f / pz->zoom[1];
    float ymax = -pz->pan[1] + 1.0f / pz->zoom[1];

    // log_error("BOX %g %g %g %g", xmin, xmax, ymin, ymax);

    *box = dvz_box(xmin, xmax, ymin, ymax, -1, +1);
}



void dvz_panzoom_set(DvzPanzoom* pz, DvzBox* extent)
{
    ANN(pz);

    float width = extent->xmax - extent->xmin;
    float height = extent->ymax - extent->ymin;

    pz->zoom[0] = 2.0f / width;
    pz->zoom[1] = 2.0f / height;

    pz->pan[0] = -(extent->xmin + extent->xmax) / 2.0f;
    pz->pan[1] = -(extent->ymin + extent->ymax) / 2.0f;

    pz->pan_center[0] = pz->pan[0];
    pz->pan_center[1] = pz->pan[1];
}



static inline void _lock_pan(DvzPanzoom* pz, DvzDim dim, bool cond)
{
    ANN(pz);
    ASSERT(0 <= dim && dim <= 1);
    if (cond)
    {
        log_trace("lock pan on axis %d", dim);
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

static inline void _lock_zoom(DvzPanzoom* pz, DvzDim dim, bool cond)
{
    ANN(pz);
    ASSERT(0 <= dim && dim <= 1);
    if (cond)
    {
        log_trace("lock zoom on axis %d", dim);
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

static inline void _get_view(DvzPanzoom* pz, mat4 view)
{
    ANN(pz);
    float x = -pz->pan[0];
    float y = -pz->pan[1];
    glm_lookat((vec3){x, y, 2}, (vec3){x, y, 0}, (vec3){0, 1, 0}, view);
}

static inline void _get_proj(DvzPanzoom* pz, mat4 proj)
{
    ANN(pz);
    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f, proj);
}

static void _lock(DvzPanzoom* pz)
{
    ANN(pz);

    mat4 view = {0};
    mat4 proj = {0};
    mat4 proj_view = {0};
    cvec4 out = {0};

    _get_view(pz, view);
    _get_proj(pz, proj);
    glm_mat4_mul(proj, view, proj_view);

    bool cond_pan_x = _too_large(view[0][3]) || _too_large(view[3][0]);
    bool cond_pan_y = _too_large(view[1][3]) || _too_large(view[3][1]);

    bool cond_zoom_x = _out_of_range(proj[0][0]);
    bool cond_zoom_y = _out_of_range(proj[1][1]);

    bool cond_panzoom_x = //
        _out_of_range(proj_view[0][0]) || _too_large(proj_view[0][3]) ||
        _too_large(proj_view[3][0]);
    bool cond_panzoom_y = //
        _out_of_range(proj_view[1][1]) || _too_large(proj_view[1][3]) ||
        _too_large(proj_view[3][1]);

    DvzBox box = {0};
    dvz_panzoom_extent(pz, &box);

    bool cond_box_x = box.xmax - box.xmin < BOX_MIN;
    bool cond_box_y = box.ymax - box.ymin < BOX_MIN;

    // log_error("%d %d %d %d", cond_pan_x, cond_zoom_x, cond_panzoom_x, cond_box_x);

    // cond_pan_x |= cond_panzoom_x;
    cond_zoom_x |= cond_panzoom_x | cond_box_x;

    // cond_pan_y |= cond_panzoom_y;
    cond_zoom_y |= cond_panzoom_y | cond_box_y;

    _lock_pan(pz, DVZ_DIM_X, cond_pan_x);
    _lock_pan(pz, DVZ_DIM_Y, cond_pan_y);
    _lock_zoom(pz, DVZ_DIM_X, cond_zoom_x);
    _lock_zoom(pz, DVZ_DIM_Y, cond_zoom_y);

    // log_error(
    //     "DIMX box: %g, zoom: %g, zoom_center: %g, pan: %g, pan_center: %g", //
    //     box.xmax - box.xmin, pz->zoom[0], pz->zoom_center[0], pz->pan[0], pz->pan_center[0]);
    // log_error(
    //     "DIMY box: %g, zoom: %g, zoom_center: %g, pan: %g, pan_center: %g", //
    //     box.ymax - box.ymin, pz->zoom[1], pz->zoom_center[1], pz->pan[1], pz->pan_center[1]);
}

void dvz_panzoom_mvp(DvzPanzoom* pz, DvzMVP* mvp)
{
    ANN(pz);

    // NOTE: hard limits due to floating point precision.
    _lock(pz);

    // WARNING: this does not affect the model matrix, so ensure it is properly initialized to the
    // identity (not all zeros).

    // View matrix (depends on the pan).
    _get_view(pz, mvp->view);

    // Proj matrix (depends on the zoom).
    _get_proj(pz, mvp->proj);
}



void dvz_panzoom_bounds(
    DvzPanzoom* pz, DvzRef* ref, double* xmin, double* xmax, double* ymin, double* ymax)
{
    ANN(pz);
    ANN(ref);

    // Find the extent.
    DvzBox box = {0};
    dvz_panzoom_extent(pz, &box);
    dvec3 pos = {0};

    dvz_ref_inverse(ref, (vec3){box.xmin, box.ymin, 0}, &pos);
    // log_error("%f %f %f", pos[0], pos[1], pos[2]);
    *xmin = pos[0];
    *ymin = pos[1];

    dvz_ref_inverse(ref, (vec3){box.xmax, box.ymax, 0}, &pos);
    *xmax = pos[0];
    *ymax = pos[1];
}



void dvz_panzoom_xlim(DvzPanzoom* pz, DvzRef* ref, double xmin, double xmax)
{
    ANN(pz);
    ANN(ref);

    // Convert the passed limits to NDC so that we can appropriately set the panzoom extent.
    vec3 xlim_ndc[2];
    dvz_ref_normalize_1D(ref, DVZ_DIM_X, 2, (double[]){xmin, xmax}, xlim_ndc);
    float xmin_ndc = xlim_ndc[0][DVZ_DIM_X];
    float xmax_ndc = xlim_ndc[1][DVZ_DIM_X];

    DvzBox extent = {0};
    dvz_panzoom_extent(pz, &extent);
    extent.xmin = xmin_ndc;
    extent.xmax = xmax_ndc;
    dvz_panzoom_set(pz, &extent);
}



void dvz_panzoom_ylim(DvzPanzoom* pz, DvzRef* ref, double ymin, double ymax)
{
    ANN(pz);
    ANN(ref);

    // Convert the passed limits to NDC so that we can appropriately set the panzoom extent.
    vec3 ylim_ndc[2];
    dvz_ref_normalize_1D(ref, DVZ_DIM_Y, 2, (double[]){ymin, ymax}, ylim_ndc);
    float ymin_ndc = ylim_ndc[0][DVZ_DIM_Y];
    float ymax_ndc = ylim_ndc[1][DVZ_DIM_Y];

    DvzBox extent = {0};
    dvz_panzoom_extent(pz, &extent);
    extent.ymin = ymin_ndc;
    extent.ymax = ymax_ndc;
    dvz_panzoom_set(pz, &extent);
}



void dvz_panzoom_destroy(DvzPanzoom* pz)
{
    ANN(pz);
    FREE(pz);
}



/*************************************************************************************************/
/*  Panzoom event functions                                                                      */
/*************************************************************************************************/

bool dvz_panzoom_mouse(DvzPanzoom* pz, DvzMouseEvent* ev)
{
    ANN(pz);

    switch (ev->type)
    {
    // Dragging: pan.
    case DVZ_MOUSE_EVENT_DRAG:
        if (ev->button == DVZ_MOUSE_BUTTON_LEFT && ev->content.d.is_press_valid)
        {
            dvz_panzoom_pan_shift(pz, ev->content.d.shift, (vec2){0});
        }
        else if (ev->button == DVZ_MOUSE_BUTTON_RIGHT && ev->content.d.is_press_valid)
        {
            vec2 shift = {0};
            glm_vec2_copy(ev->content.d.shift, shift);

            // Flag: keep aspect ratio.
            if ((pz->flags & DVZ_PANZOOM_FLAGS_KEEP_ASPECT) != 0)
                shift[1] = -shift[0];

            dvz_panzoom_zoom_shift(pz, shift, ev->content.d.press_pos);
        }
        break;

    // Stop dragging.
    case DVZ_MOUSE_EVENT_DRAG_STOP:
        dvz_panzoom_end(pz);
        break;

    // Mouse wheel.
    case DVZ_MOUSE_EVENT_WHEEL:
        dvz_panzoom_zoom_wheel(pz, ev->content.w.dir, ev->pos);
        break;

    // Double-click
    case DVZ_MOUSE_EVENT_DOUBLE_CLICK:
        dvz_panzoom_reset(pz);
        break;

    default:
        return false;
    }

    return true;
}
