/*************************************************************************************************/
/* Pixel                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PIXEL
#define DVZ_HEADER_PIXEL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_obj.h"
#include "scene/viewport.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPixel DvzPixel;
typedef struct DvzPixelVertex DvzPixelVertex;

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

struct DvzPixelVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
};



struct DvzPixel
{
    DvzObject obj;
    DvzRequester* rqr;
    int flags;
    DvzVisual* visual;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzPixel* dvz_pixel(DvzRequester* rqr, uint32_t item_count, int flags);



DVZ_EXPORT void dvz_pixel_viewport(DvzPixel* pixel, DvzViewport viewport);



DVZ_EXPORT void
dvz_pixel_position(DvzPixel* pixel, uint32_t first, uint32_t count, vec3* values, int flags);



DVZ_EXPORT void
dvz_pixel_color(DvzPixel* pixel, uint32_t first, uint32_t count, cvec4* values, int flags);



DVZ_EXPORT void
dvz_pixel_draw(DvzPixel* pixel, DvzId canvas, uint32_t first, uint32_t count, int flags);



DVZ_EXPORT DvzInstance* dvz_pixel_instance( //
    DvzPixel* pixel, DvzView* view, uint32_t first, uint32_t count);



DVZ_EXPORT void dvz_pixel_create(DvzPixel* pixel);



DVZ_EXPORT void dvz_pixel_update(DvzPixel* pixel);



DVZ_EXPORT void dvz_pixel_destroy(DvzPixel* pixel);



EXTERN_C_OFF

#endif
