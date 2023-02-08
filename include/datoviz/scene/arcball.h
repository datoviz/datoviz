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
#include "scene/mvp.h"
#include "scene/panzoom.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArcball DvzArcball;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_ARCBALL_FLAGS_NONE,
    DVZ_ARCBALL_FLAGS_CONSTRAIN,
} DvzArcballFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArcball
{
    vec2 viewport_size;
    int flags;

    mat4 mat;        // current model
    versor rotation; // current rotation (while dragging), to be applied to mat after dragging
    vec3 constrain;  // constrain axis, null if no constraint
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzArcball* dvz_arcball(float width, float height, int flags); // inner viewport size

DVZ_EXPORT void dvz_arcball_reset(DvzArcball* pz);

DVZ_EXPORT void dvz_arcball_resize(DvzArcball* pz, float width, float height);

DVZ_EXPORT void dvz_arcball_flags(DvzArcball* pz, int flags);

DVZ_EXPORT void dvz_arcball_constrain(DvzArcball* pz, vec3 constrain);

DVZ_EXPORT void dvz_arcball_angles(DvzArcball* arcball, vec3 angles);

DVZ_EXPORT void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);

// DVZ_EXPORT void dvz_arcball_pan(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);

// DVZ_EXPORT void dvz_arcball_lock(DvzArcball* arcball, vec3 dir);

DVZ_EXPORT void dvz_arcball_model(DvzArcball* arcball, mat4 model);

DVZ_EXPORT void dvz_arcball_end(DvzArcball* arcball);

DVZ_EXPORT void dvz_arcball_mvp(DvzArcball* pz, DvzMVP* mvp);

DVZ_EXPORT void dvz_arcball_print(DvzArcball* arcball);

DVZ_EXPORT void dvz_arcball_destroy(DvzArcball* pz);



EXTERN_C_OFF

#endif
