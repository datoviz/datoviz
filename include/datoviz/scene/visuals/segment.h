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



#endif
