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
    vec3 init;       // initial Euler angles
    versor rotation; // current rotation (while dragging), to be applied to mat after dragging
    vec3 constrain;  // constrain axis, null if no constraint
};



DVZ_EXPORT bool dvz_arcball_mouse(DvzArcball* arcball, DvzMouseEvent ev);



#endif
