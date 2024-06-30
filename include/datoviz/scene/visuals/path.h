/*************************************************************************************************/
/* Path                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PATH
#define DVZ_HEADER_PATH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../graphics.h"
#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPathVertex DvzPathVertex;
typedef struct DvzPathParams DvzPathParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPathVertex
{
    vec3 p0;     /* previous position */
    vec3 p1;     /* current position */
    vec3 p2;     /* next position */
    vec3 p3;     /* next next position */
    cvec4 color; /* point color */
};



struct DvzPathParams
{
    float linewidth;    /* line width in pixels */
    float miter_limit;  /* miter limit for joins */
    int32_t cap_type;   /* type of the ends of the path */
    int32_t round_join; /* whether to use round joins */
};



#endif
