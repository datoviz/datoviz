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

// typedef struct DvzImage DvzImage;
typedef struct DvzImageVertex DvzImageVertex;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;
typedef struct DvzView DvzView;
typedef struct DvzInstance DvzInstance;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_PROP_NONE = 0x00,

    // use instance vertex rate so that the same value is used for all vertices
    DVZ_PROP_CONSTANT = 0x01,

    DVZ_PROP_DYNAMIC = 0x02,
} DvzPropFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzImageVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
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
dvz_image_position(DvzVisual* image, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_image_color(DvzVisual* image, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* image, uint32_t item_count);



EXTERN_C_OFF

#endif
