/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/panzoom.h"



/*************************************************************************************************/
/*  Panzoom functions                                                                            */
/*************************************************************************************************/

DvzPanzoom dvz_panzoom(float width, float height, int flags)
{
    // inner viewport size
}

void dvz_panzoom_reset(DvzPanzoom* pz) {}

void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height) {}

void dvz_panzoom_flags(DvzPanzoom* pz, int flags) {}

void dvz_panzoom_xlim(DvzPanzoom* pz, vec2 xlim)
{
    // FLOAT_MIN/MAX=no lim
}

void dvz_panzoom_ylim(DvzPanzoom* pz, vec2 ylim) {}

void dvz_panzoom_zlim(DvzPanzoom* pz, vec2 zlim) {}

void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan)
{
    // in NDC, gets or sets
}

void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom)
{
    // in NDC
}

void dvz_panzoom_shift_pan(DvzPanzoom* pz, vec2 pan_shift) {}

void dvz_panzoom_shift_zoom(DvzPanzoom* pz, vec2 zoom_shift, vec2 center_px) {}

void dvz_panzoom_xrange(DvzPanzoom* pz, vec2 xrange)
{
    // if (0, 0), gets the xrange, otherwise sets it
}

void dvz_panzoom_yrange(DvzPanzoom* pz, vec2 yrange) {}

DvzMVP* dvz_panzoom_mvp(DvzPanzoom* pz) {}

void dvz_panzoom_destroy(DvzPanzoom* pz) {}
