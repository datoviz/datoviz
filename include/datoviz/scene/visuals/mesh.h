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

// typedef struct DvzMesh DvzMesh;
typedef struct DvzMeshVertex DvzMeshVertex;

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

struct DvzMeshVertex
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
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* mesh, uint32_t item_count);



EXTERN_C_OFF

#endif
