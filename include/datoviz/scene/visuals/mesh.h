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
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_MESH_FLAGS_NONE = 0x00,
    DVZ_MESH_FLAGS_TEXTURED = 0x01,
    DVZ_MESH_FLAGS_LIGHTING = 0x02,
} DvzMeshFlags;



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
    vec3 pos;    /* position */
    vec3 normal; /* normal vector */
    vec4 uv_a;   /* uv*a */
};



struct DvzMeshParams
{
    vec4 light_pos;    /* light position */
    vec4 light_params; /* ambient, diffuse, specular coefs */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzBatch* batch, int flags);



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
DVZ_EXPORT DvzId dvz_mesh_texture(
    DvzVisual* visual, uvec3 shape, DvzFormat format, DvzFilter filter, DvzSize size, void* data);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_index(DvzVisual* mesh, uint32_t first, uint32_t count, DvzIndex* values);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* mesh, uint32_t vertex_count, uint32_t index_count);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_light_pos(DvzVisual* mesh, vec4 pos);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_light_params(DvzVisual* mesh, vec4 params);



/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape);



EXTERN_C_OFF

#endif
