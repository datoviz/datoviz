/*************************************************************************************************/
/* Arcball                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ARCBALL
#define DVZ_HEADER_ARCBALL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"
#include "mvp.h"
#include "scene/panzoom.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArcball DvzArcball;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArcball
{
    vec2 viewport_size;
    int flags;

    versor rotation;
    // mat4 mat;

    // mat4 translate;
    // mat4 inv_model;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzArcball dvz_arcball(float width, float height, int flags); // inner viewport size

DVZ_EXPORT void dvz_arcball_reset(DvzArcball* pz);

DVZ_EXPORT void dvz_arcball_resize(DvzArcball* pz, float width, float height);

DVZ_EXPORT void dvz_arcball_flags(DvzArcball* pz, int flags);

DVZ_EXPORT void dvz_arcball_angles(DvzArcball* arcball, vec3 angles);

DVZ_EXPORT void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);

// DVZ_EXPORT void dvz_arcball_pan(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);

// DVZ_EXPORT void dvz_arcball_lock(DvzArcball* arcball, vec3 dir);

DVZ_EXPORT void dvz_arcball_model(DvzArcball* arcball, mat4 model);

DVZ_EXPORT void dvz_arcball_mvp(DvzArcball* pz, DvzMVP* mvp);

DVZ_EXPORT void dvz_arcball_print(DvzArcball* arcball);

DVZ_EXPORT void dvz_arcball_destroy(DvzArcball* pz);



EXTERN_C_OFF

#endif
