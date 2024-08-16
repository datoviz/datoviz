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
typedef struct DvzImageParams DvzImageParams;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzImageVertex
{
    vec3 pos;    /* position */
    vec2 size;   /* size */
    vec2 anchor; /* anchor */
    vec2 uv;     /* texture coordinates */
};



struct DvzImageParams
{
    float radius;     /* rounded rectangle radius, 0 for sharp corners */
    float edge_width; /* width of the border, 0 for no border */
    vec4 edge_color;  /* color of the border */
};



#endif
