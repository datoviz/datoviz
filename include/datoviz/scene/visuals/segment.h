/*************************************************************************************************/
/* Segment                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SEGMENT
#define DVZ_HEADER_SEGMENT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../graphics.h"
#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSegmentVertex DvzSegmentVertex;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// TODO: comment out once graphics.h has been removed and this enum has been declared in _enums.h
// Cap type.
// typedef enum
// {
//     DVZ_CAP_TYPE_NONE = 0,
//     DVZ_CAP_ROUND = 1,
//     DVZ_CAP_TRIANGLE_IN = 2,
//     DVZ_CAP_TRIANGLE_OUT = 3,
//     DVZ_CAP_SQUARE = 4,
//     DVZ_CAP_BUTT = 5,
//     DVZ_CAP_COUNT,
// } DvzCapType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSegmentVertex
{
    vec3 P0;         /* start position */
    vec3 P1;         /* end position */
    vec4 shift;      /* shift of start (xy) and end (zw) positions, in pixels */
    cvec4 color;     /* color */
    float linewidth; /* line width, in pixels */
    DvzCapType cap0; /* start cap enum */
    DvzCapType cap1; /* end cap enum */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_segment(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_position(
    DvzVisual* segment, uint32_t first, uint32_t count, vec3* initial, vec3* terminal, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_segment_shift(DvzVisual* segment, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_linewidth(
    DvzVisual* segment, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_cap(
    DvzVisual* segment, uint32_t first, uint32_t count, DvzCapType* initial, DvzCapType* terminal,
    int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_alloc(DvzVisual* segment, uint32_t item_count);



EXTERN_C_OFF

#endif
