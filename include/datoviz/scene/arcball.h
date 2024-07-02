/*************************************************************************************************/
/* Arcball                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ARCBALL
#define DVZ_HEADER_ARCBALL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "datoviz_math.h"
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
    vec3 init;       // initial Euler angles
    versor rotation; // current rotation (while dragging), to be applied to mat after dragging
    vec3 constrain;  // constrain axis, null if no constraint
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzArcball* dvz_arcball(float width, float height, int flags); // inner viewport size



/**
 *
 */
DVZ_EXPORT bool dvz_arcball_mouse(DvzArcball* arcball, DvzMouseEvent ev);



/**
 * Destroy an arcball.
 *
 * @param arcball the arcball
 */
DVZ_EXPORT void dvz_arcball_destroy(DvzArcball* arcball);



#endif
