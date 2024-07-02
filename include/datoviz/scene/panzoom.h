/*************************************************************************************************/
/* Panzoom                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PANZOOM
#define DVZ_HEADER_PANZOOM



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"
#include "mouse.h"
#include "scene/mvp.h"



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
    DVZ_PANZOOM_FLAGS_FIXED_X = 0x10,
    DVZ_PANZOOM_FLAGS_FIXED_Y = 0x20,
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
    vec2 pan_center;
    vec2 zoom;
    vec2 zoom_center;
    // DvzMVP mvp;
};



EXTERN_C_ON



/*************************************************************************************************/
/*  Panzoom event functions                                                                      */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzPanzoom* dvz_panzoom(float width, float height, int flags); // inner viewport size



/**
 *
 */
DVZ_EXPORT bool dvz_panzoom_mouse(DvzPanzoom* pz, DvzMouseEvent ev);



/**
 * Function.
 *
 * @param pz the pz
 */
DVZ_EXPORT void dvz_panzoom_destroy(DvzPanzoom* pz);



EXTERN_C_OFF

#endif
