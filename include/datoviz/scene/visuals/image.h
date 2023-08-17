/*************************************************************************************************/
/* Image                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_IMAGE
#define DVZ_HEADER_IMAGE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzImageVertex DvzImageVertex;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzImageVertex
{
    vec2 pos; /* position */
    vec2 uv;  /* texture coordinates */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzRequester* rqr, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_image_position(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_image_texcoords(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* image, uint32_t item_count);



EXTERN_C_OFF

#endif
