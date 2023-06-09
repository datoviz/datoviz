/*************************************************************************************************/
/* Segment                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SEGMENT
#define DVZ_HEADER_SEGMENT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSegmentVertex DvzSegmentVertex;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSegmentVertex
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
DVZ_EXPORT DvzVisual* dvz_segment(DvzRequester* rqr, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_segment_position(DvzVisual* segment, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_alloc(DvzVisual* segment, uint32_t item_count);



EXTERN_C_OFF

#endif
