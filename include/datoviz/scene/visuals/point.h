/*************************************************************************************************/
/* Point                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_POINT
#define DVZ_HEADER_POINT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPointVertex DvzPointVertex;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPointVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* size */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzRequester* rqr, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_point_position(DvzVisual* point, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_point_color(DvzVisual* point, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_point_size(DvzVisual* point, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_point_alloc(DvzVisual* point, uint32_t item_count);



EXTERN_C_OFF

#endif
