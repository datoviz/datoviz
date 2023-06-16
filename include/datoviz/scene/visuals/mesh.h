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

typedef struct DvzMeshVertex DvzMeshVertex;
typedef struct DvzMeshParams DvzMeshParams;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;
typedef struct DvzShape DvzShape;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// // NOTE: unused for now
struct DvzMeshVertex
{
    // HACK: use vec4 for alignment when accessing from compute shader (need std140 on GPU)
    vec3 pos;    /* position */
    vec3 normal; /* normal vector */
    cvec4 color; /* color */

    // float alpha;
    // TODO FIX
    // uint8_t alpha; /* transparency value */
};



struct DvzMeshParams
{
    mat4 lights_pos_0;    /* positions of each of the maximum four lights */
    mat4 lights_params_0; /* ambient, diffuse, specular coefs for each light */
    vec4 tex_coefs;       /* blending coefficients for the four textures */
    vec4 clip_coefs;      /* clip coefficients */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzRequester* rqr, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_mesh_position(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_mesh_color(DvzVisual* mesh, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT
void dvz_mesh_normal(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_index(DvzVisual* mesh, uint32_t first, uint32_t count, DvzIndex* values);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* mesh, uint32_t vertex_count, uint32_t index_count);



DVZ_EXPORT DvzVisual* dvz_mesh_shape(DvzRequester* rqr, DvzShape* shape);



EXTERN_C_OFF

#endif
