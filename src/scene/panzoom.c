/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/panzoom.h"



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/

static inline bool _is_vec2_null(vec2 v) { return memcmp(v, (vec2){0, 0}, sizeof(vec2)); }



// static inline void _normalize_pos(DvzPanzoom* pz, vec2 in, vec2 out)
// {
//     // From pixel coordinates (origin is upper left corner) to NDC (origin is center of
//     viewport) float x = in[0]; float y = in[1];

//     float w = pz->viewport_size[0];
//     float h = pz->viewport_size[1];

//     out[0] = -1 + 2 * x / w;
//     out[1] = +1 - 2 * y / h;
// }



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

DvzPanzoom dvz_panzoom(float width, float height, int flags)
{
    // width, width are the inner viewport size
    DvzPanzoom pz = {0};
    pz.flags = flags;
    pz.viewport_size[0] = width;
    pz.viewport_size[1] = height;

    dvz_panzoom_reset(&pz);

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
    // NOTE: center_px is in pixel center coordinates (origin at the center of the viewport)
    ANN(pz);

    vec2 shift = {0};
    _normalize_shift(pz, shift_px, shift);
    shift[1] *= -1;

    float zx0 = pz->zoom_center[0];
    float zy0 = pz->zoom_center[1];

    pz->zoom[0] = zx0 * exp(shift[0]);
    pz->zoom[1] = zy0 * exp(shift[1]);

    float zx = pz->zoom[0];
    float zy = pz->zoom[1];
    ASSERT(zx > 0);
    ASSERT(zy > 0);

    // Update pan.
    float px = -center_px[0] * (1.0f / zx0 - 1.0f / zx) * zx;
    float py = -center_px[1] * (1.0f / zy0 - 1.0f / zy) * zy;

    float x0 = pz->pan_center[0];
    float y0 = pz->pan_center[1];

    // dvz_panzoom_pan_shift(pz, pan_shift_px, center_px);
    pz->pan[0] = x0 - px / zx;
    pz->pan[1] = y0 - py / zy;
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



DvzMVP* dvz_panzoom_mvp(DvzPanzoom* pz)
{
    ANN(pz);

    DvzMVP* mvp = &pz->mvp;

    // View matrix (depends on the pan).
    {
        float x = pz->pan[0];
        float y = pz->pan[1];
        glm_lookat(pz->pan, (vec3){x, y, 0}, (vec3){0, 1, 0}, mvp->view);
    }

    // Proj matrix (depends on the zoom).
    {
        float zx = pz->zoom[0];
        float zy = pz->zoom[1];
        glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f, mvp->proj);
    }

    return mvp;
}



void dvz_panzoom_destroy(DvzPanzoom* pz) { ANN(pz); }
