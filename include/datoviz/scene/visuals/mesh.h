/*************************************************************************************************/
/* Mesh                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MESH
#define DVZ_HEADER_MESH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMeshColorVertex DvzMeshColorVertex;
typedef struct DvzMeshTexturedVertex DvzMeshTexturedVertex;
typedef struct DvzMeshParams DvzMeshParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzShape DvzShape;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMeshColorVertex
{
    // HACK: use vec4 for alignment when accessing from compute shader (need std140 on GPU)
    vec3 pos;    /* position */
    vec3 normal; /* normal vector */
    cvec4 color; /* color */
};

struct DvzMeshTexturedVertex
{
    // HACK: use vec4 for alignment when accessing from compute shader (need std140 on GPU)
    vec3 pos;       /* position */
    vec3 normal;    /* normal vector */
    vec4 texcoords; /* u, v, *, a */
};



struct DvzMeshParams
{
    vec4 light_pos;    /* light position */
    vec4 light_params; /* ambient, diffuse, specular coefs */
};



#endif
