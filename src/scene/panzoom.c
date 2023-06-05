/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/panzoom.h"
#include "mouse.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/


// NOTE: the wheel behavior depends on both parameters, so first finetune DRAG_COEF with right-drag
// before tweaking WHEEL_COEFF.
#if OS_MACOS
// HACK: touchpad wheel too sensitive on macOS
// TODO
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .001
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF -8.0
#else
// TODO: test on linux/Windows.
#define DVZ_PANZOOM_ZOOM_DRAG_COEF  .002
#define DVZ_PANZOOM_ZOOM_WHEEL_COEF 60.0
#endif



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/

static inline bool _is_vec2_null(vec2 v) { return memcmp(v, (vec2){0, 0}, sizeof(vec2)); }



static inline void _normalize_pos(DvzPanzoom* pz, vec2 in, vec2 out)
{
    // From pixel coordinates (origin is upper left corner) to NDC (origin is center of
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

    // pz->mvp = dvz_mvp_default();
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



void dvz_panzoom_xlim(DvzPanzoom* pz, vec2 xlim)
{
    ANN(pz);
    if (_is_vec2_null(xlim))
    {
        // TODO: compute xlim from pan and zoom
        // FLOAT_MIN/MAX=no lim
    }
    else
    {
        _vec2_copy(xlim, pz->xlim);
    }
}



void dvz_panzoom_ylim(DvzPanzoom* pz, vec2 ylim)
{
    ANN(pz);
    if (_is_vec2_null(ylim))
    {
        // TODO: compute ylim from pan and zoom
        // FLOAT_MIN/MAX=no lim
    }
    else
    {
        _vec2_copy(ylim, pz->ylim);
    }
}



void dvz_panzoom_zlim(DvzPanzoom* pz, vec2 zlim)
{
    ANN(pz);
    _vec2_copy(zlim, pz->zlim);
}



void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan)
{
    ANN(pz);
    _vec2_copy(pan, pz->pan);
}



void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom)
{
    ANN(pz);
    _vec2_copy(zoom, pz->zoom);
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

    pz->pan[0] = x0 + shift[0] / zx;
    pz->pan[1] = y0 + shift[1] / zy;
}



void dvz_panzoom_zoom_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px)
{
    // NOTE: center_px is the center of the zoom, in pixel coordinates (origin at the upper left
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

    pz->zoom[0] = zx0 * exp(DVZ_PANZOOM_ZOOM_DRAG_COEF * a * shift[0]);
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

    pz->pan[0] = x0 - px / zx;
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
        d /= (float)fabs((double)d);
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



void dvz_panzoom_xrange(DvzPanzoom* pz, vec2 xrange)
{
    // if (0, 0), gets the xrange, otherwise sets it
    ANN(pz);
    if (_is_vec2_null(xrange))
    {
        // TODO: compute xrange from pan and zoom
    }
    else
    {
        // TODO: compute from pan and zoom from xrange
    }
}



void dvz_panzoom_yrange(DvzPanzoom* pz, vec2 yrange)
{
    ANN(pz);
    // if (0, 0), gets the yrange, otherwise sets it
    ANN(pz);
    if (_is_vec2_null(yrange))
    {
        // TODO: compute yrange from pan and zoom
    }
    else
    {
        // TODO: compute from pan and zoom from yrange
    }
}



void dvz_panzoom_mvp(DvzPanzoom* pz, DvzMVP* mvp)
{
    ANN(pz);

    // DvzMVP* mvp = &pz->mvp;

    // View matrix (depends on the pan).
    {
        float x = -pz->pan[0];
        float y = -pz->pan[1];
        glm_lookat((vec3){x, y, 2}, (vec3){x, y, 0}, (vec3){0, 1, 0}, mvp->view);
    }

    // Proj matrix (depends on the zoom).
    {
        float zx = pz->zoom[0];
        float zy = pz->zoom[1];
        glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f, mvp->proj);
    }

    // return mvp;
}



void dvz_panzoom_destroy(DvzPanzoom* pz)
{
    ANN(pz);
    FREE(pz);
}



/*************************************************************************************************/
/*  Panzoom event functions                                                                      */
/*************************************************************************************************/

bool dvz_panzoom_mouse(DvzPanzoom* pz, DvzMouseEvent ev)
{
    ANN(pz);

    switch (ev.type)
    {
    // Dragging: pan.
    case DVZ_MOUSE_EVENT_DRAG:
        if (ev.content.d.button == DVZ_MOUSE_BUTTON_LEFT)
        {
            dvz_panzoom_pan_shift(pz, ev.content.d.shift, (vec2){0});
        }
        else if (ev.content.d.button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            dvz_panzoom_zoom_shift(pz, ev.content.d.shift, ev.pos);
        }
        break;

    // Stop dragging.
    case DVZ_MOUSE_EVENT_DRAG_STOP:
        dvz_panzoom_end(pz);
        break;

    // Mouse wheel.
    case DVZ_MOUSE_EVENT_WHEEL:
        dvz_panzoom_zoom_wheel(pz, ev.content.w.dir, ev.pos);
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
