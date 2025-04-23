/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Ortho                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/ortho.h"
#include "datoviz.h"
#include "mouse.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/


// NOTE: the wheel behavior depends on both parameters, so first finetune DRAG_COEF with right-drag
// before tweaking WHEEL_COEFF.
#if OS_MACOS
// HACK: touchpad wheel too sensitive on macOS
#define DVZ_ORTHO_ZOOM_DRAG_COEF  .001
#define DVZ_ORTHO_ZOOM_WHEEL_COEF -2.0
#else
#define DVZ_ORTHO_ZOOM_DRAG_COEF  .001
#define DVZ_ORTHO_ZOOM_WHEEL_COEF 30.0
#endif



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/

// TODO: refactor in .h common with panzoom.c
static inline bool _is_vec2_null(vec2 v) { return memcmp(v, (vec2){0, 0}, sizeof(vec2)); }



static inline void _normalize_pos(DvzOrtho* ortho, vec2 in, vec2 out)
{
    // From pixel coordinates (origin is top left corner) to NDC (origin is center of
    // viewport)
    float x = in[0];
    float y = in[1];

    float w = ortho->viewport_size[0];
    float h = ortho->viewport_size[1];
    ASSERT(h > 0);

    float a = w / h;

    out[0] = -1 + 2 * x / w;
    out[1] = +1 - 2 * y / h;

    if (w > h)
    {
        out[0] *= a;
    }

    if (w < h)
    {
        out[1] /= a;
    }
}



static inline void _normalize_shift(DvzOrtho* ortho, vec2 in, vec2 out)
{
    float x = in[0];
    float y = in[1];

    float w = ortho->viewport_size[0];
    float h = ortho->viewport_size[1];

    float a = fmin(w, h);
    ASSERT(a > 0);

    out[0] = +2 * x / a;
    out[1] = -2 * y / a;
}



/*************************************************************************************************/
/*  Ortho functions                                                                            */
/*************************************************************************************************/

DvzOrtho* dvz_ortho(float width, float height, int flags)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_debug("create ortho of size %.0fx%.0f", width, height);
    // width, height are the inner viewport size
    DvzOrtho* ortho = (DvzOrtho*)calloc(1, sizeof(DvzOrtho));

    ortho->viewport_size[0] = width;
    ortho->viewport_size[1] = height;
    ortho->flags = flags;

    dvz_ortho_reset(ortho);

    return ortho;
}



void dvz_ortho_reset(DvzOrtho* ortho)
{
    ANN(ortho);

    ortho->pan[0] = 0;
    ortho->pan[1] = 0;

    ortho->pan_center[0] = 0;
    ortho->pan_center[1] = 0;

    ortho->zoom = 1;

    ortho->zoom_center[0] = 1;
    ortho->zoom_center[1] = 1;
}



void dvz_ortho_resize(DvzOrtho* ortho, float width, float height)
{
    ANN(ortho);
    ortho->viewport_size[0] = width;
    ortho->viewport_size[1] = height;
}



void dvz_ortho_flags(DvzOrtho* ortho, int flags)
{
    ANN(ortho);
    ortho->flags = flags;
}



void dvz_ortho_pan(DvzOrtho* ortho, vec2 pan)
{
    ANN(ortho);
    _vec2_copy(pan, ortho->pan);
}



void dvz_ortho_zoom(DvzOrtho* ortho, float zoom)
{
    ANN(ortho);
    ortho->zoom = zoom;
}



void dvz_ortho_pan_shift(DvzOrtho* ortho, vec2 shift_px, vec2 center_px)
{
    // NOTE: center_px is unused here
    ANN(ortho);

    vec2 shift = {0};
    _normalize_shift(ortho, shift_px, shift);

    float z = ortho->zoom;
    ASSERT(z > 0);

    float x0 = ortho->pan_center[0];
    float y0 = ortho->pan_center[1];

    ortho->pan[0] = x0 + shift[0] / z;
    ortho->pan[1] = y0 + shift[1] / z;
}



void dvz_ortho_zoom_shift(DvzOrtho* ortho, vec2 shift_px, vec2 center_px)
{
    // NOTE: center_px is the center of the zoom, in pixel coordinates (origin at the top left
    // corner of the viewport)
    ANN(ortho);

    vec2 shift = {0};
    // From pixel coordinates to NDC coordinates.
    _normalize_shift(ortho, shift_px, shift);

    vec2 center = {0};
    _normalize_pos(ortho, center_px, center);

    float zx0 = ortho->zoom_center[0];
    float zy0 = ortho->zoom_center[1];

    // HACK: coefficient depends onthe viewport size.
    float w = ortho->viewport_size[0];
    float h = ortho->viewport_size[1];
    float a = .5 * (w + h);

    float s = shift[0] + shift[1];
    ortho->zoom = zx0 * exp(DVZ_ORTHO_ZOOM_DRAG_COEF * a * s);

    float z = ortho->zoom;
    ASSERT(z > 0);

    // Update pan.
    float px = center[0] * (1.0f / zx0 - 1.0f / z) * z;
    float py = center[1] * (1.0f / zy0 - 1.0f / z) * z;

    float x0 = ortho->pan_center[0];
    float y0 = ortho->pan_center[1];

    ortho->pan[0] = x0 - px / z;
    ortho->pan[1] = y0 - py / z;
}



void dvz_ortho_zoom_wheel(DvzOrtho* ortho, vec2 dir, vec2 center_px)
{
    ANN(ortho);

    float w = ortho->viewport_size[0];
    float h = ortho->viewport_size[1];
    ASSERT(w > 0);
    ASSERT(h > 0);
    // Aspect ratio.
    float a = h / w;

    float d = dir[1];
    if (d != 0)
    {
        d /= (float)fabs((double)d);
        vec2 shift = {0};
        shift[0] = DVZ_ORTHO_ZOOM_WHEEL_COEF * d;
        shift[1] = -a * shift[0];
        dvz_ortho_zoom_shift(ortho, shift, center_px);
        dvz_ortho_end(ortho);
    }
}



void dvz_ortho_end(DvzOrtho* ortho)
{
    ANN(ortho);

    ortho->pan_center[0] = ortho->pan[0];
    ortho->pan_center[1] = ortho->pan[1];

    ortho->zoom_center[0] = ortho->zoom;
    ortho->zoom_center[1] = ortho->zoom;
}



void dvz_ortho_mvp(DvzOrtho* ortho, DvzMVP* mvp)
{
    ANN(ortho);

    // WARNING: this does not affect the model matrix, so ensure it is properly initialized to the
    // identity (not all zeros).

    // View matrix (depends on the pan).
    {
        float x = -ortho->pan[0];
        float y = -ortho->pan[1];
        glm_lookat((vec3){x, y, 2}, (vec3){x, y, 0}, (vec3){0, 1, 0}, mvp->view);
    }

    // Proj matrix (depends on the zoom).
    {
        float z = ortho->zoom;
        float w = ortho->viewport_size[0];
        float h = ortho->viewport_size[1];
        float aspect = w / h;
        glm_ortho_default_s(aspect, 1.0 / z, mvp->proj);
        // glm_ortho(-1.0f / z, +1.0f / z, -1.0f / z, 1.0f / z, -10.0f, 10.0f, mvp->proj);
    }
}



void dvz_ortho_destroy(DvzOrtho* ortho)
{
    ANN(ortho);
    FREE(ortho);
}



/*************************************************************************************************/
/*  Ortho event functions                                                                      */
/*************************************************************************************************/

bool dvz_ortho_mouse(DvzOrtho* ortho, DvzMouseEvent ev)
{
    ANN(ortho);

    switch (ev.type)
    {
    // Dragging: pan.
    case DVZ_MOUSE_EVENT_DRAG:
        if (ev.button == DVZ_MOUSE_BUTTON_LEFT)
        {
            dvz_ortho_pan_shift(ortho, ev.content.d.shift, (vec2){0});
        }
        else if (ev.button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            vec2 shift = {0};
            glm_vec2_copy(ev.content.d.shift, shift);
            dvz_ortho_zoom_shift(ortho, shift, ev.content.d.press_pos);
        }
        break;

    // Stop dragging.
    case DVZ_MOUSE_EVENT_DRAG_STOP:
        dvz_ortho_end(ortho);
        break;

    // Mouse wheel.
    case DVZ_MOUSE_EVENT_WHEEL:
        dvz_ortho_zoom_wheel(ortho, ev.content.w.dir, ev.pos);
        break;

    // Double-click
    case DVZ_MOUSE_EVENT_DOUBLE_CLICK:
        dvz_ortho_reset(ortho);
        break;

    default:
        return false;
    }

    return true;
}
