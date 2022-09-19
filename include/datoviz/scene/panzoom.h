/*************************************************************************************************/
/* Panzoom                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PANZOOM
#define DVZ_HEADER_PANZOOM



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_math.h"
#include "mvp.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPanzoom DvzPanzoom;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_PANZOOM_FLAGS_NONE = 0x00,
    DVZ_PANZOOM_FLAGS_KEEP_ASPECT = 0x01,
} DvzPanzoomFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPanzoom
{
    vec2 viewport_size;
    vec2 xlim;
    vec2 ylim;
    vec2 zlim;
    int flags;

    vec2 pan;
    vec2 zoom;
    DvzMVP mvp;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzPanzoom dvz_panzoom(float width, float height, int flags); // inner viewport size

DVZ_EXPORT void dvz_panzoom_reset(DvzPanzoom* pz);

DVZ_EXPORT void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height);

DVZ_EXPORT void dvz_panzoom_flags(DvzPanzoom* pz, int flags);

DVZ_EXPORT void dvz_panzoom_xlim(DvzPanzoom* pz, vec2 xlim); // FLOAT_MIN/MAX=no lim

DVZ_EXPORT void dvz_panzoom_ylim(DvzPanzoom* pz, vec2 ylim);

DVZ_EXPORT void dvz_panzoom_zlim(DvzPanzoom* pz, vec2 zlim);

DVZ_EXPORT void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan); // in NDC, gets or sets

DVZ_EXPORT void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom); // in NDC

DVZ_EXPORT void dvz_panzoom_shift_pan(DvzPanzoom* pz, vec2 pan_shift);

DVZ_EXPORT void dvz_panzoom_shift_zoom(DvzPanzoom* pz, vec2 zoom_shift, vec2 center_px);

DVZ_EXPORT void dvz_panzoom_xrange(DvzPanzoom* pz, vec2 xrange);
// if (0, 0), gets the xrange, otherwise sets it

DVZ_EXPORT void dvz_panzoom_yrange(DvzPanzoom* pz, vec2 yrange);

DVZ_EXPORT DvzMVP* dvz_panzoom_mvp(DvzPanzoom* pz);

DVZ_EXPORT void dvz_panzoom_destroy(DvzPanzoom* pz);



EXTERN_C_OFF

#endif
