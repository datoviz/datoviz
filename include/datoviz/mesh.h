/*************************************************************************************************/
/*  Simple 3D mesh creation and manipulation                                                     */
/*************************************************************************************************/

#ifndef DVZ_MESH_HEADER
#define DVZ_MESH_HEADER

#include "array.h"
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Enums                                                                                     */
/*************************************************************************************************/

typedef enum
{
    DVZ_MESH_CUSTOM,
    DVZ_MESH_CUBE,
    DVZ_MESH_SPHERE,
    DVZ_MESH_SURFACE,
    DVZ_MESH_CYLINDER,
    DVZ_MESH_CONE,
    DVZ_MESH_SQUARE,
    DVZ_MESH_DISC,
    DVZ_MESH_OBJ,
    DVZ_MESH_COUNT,
} DvzMeshType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMesh DvzMesh;



/*************************************************************************************************/
/*  Structs                                                                                */
/*************************************************************************************************/

struct DvzMesh
{
    DvzArray vertices;
    DvzArray indices;
    mat4 transform;
};



/*************************************************************************************************/
/*  Mesh transformation                                                                          */
/*************************************************************************************************/

/**
 * Reset the mesh transformation.
 *
 * @param mesh the mesh
 */
DVZ_EXPORT void dvz_mesh_transform_reset(DvzMesh* mesh);

/**
 * Append a mesh transformation.
 *
 * @param mesh the mesh
 * @param transform the transform matrix
 */
DVZ_EXPORT void dvz_mesh_transform_add(DvzMesh* mesh, mat4 transform);

/**
 * Append a translation transformation.
 *
 * @param mesh the mesh
 * @param translate the translation vector
 */
DVZ_EXPORT void dvz_mesh_translate(DvzMesh* mesh, vec3 translate);

/**
 * Append a scaling transformation.
 *
 * @param mesh the mesh
 * @param scale the scaling coefficients
 */
DVZ_EXPORT void dvz_mesh_scale(DvzMesh* mesh, vec3 scale);

/**
 * Append a rotation transformation.
 *
 * @param mesh the mesh
 * @param angle the rotation angle
 * @param axis the rotation axis
 */
DVZ_EXPORT void dvz_mesh_rotate(DvzMesh* mesh, float angle, vec3 axis);

/**
 * Apply the transformation matrix to a mesh.
 *
 * @param mesh the mesh
 */
DVZ_EXPORT void dvz_mesh_transform(DvzMesh* mesh);



/*************************************************************************************************/
/*  Common shapes                                                                                */
/*************************************************************************************************/

/**
 * Create a new mesh.
 *
 * A mesh is represented by an array of vertices of type `DvzGraphicsMeshVertex` and indices, where
 * every triplet of vertex indices represents a triangular face of the mesh.
 *
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh(void);

/**
 * Create a grid mesh.
 *
 * The `positions` buffer should contain row_count * col_count vec3 positions (C order).
 *
 * @param row_count number of rows
 * @param col_count number of columns
 * @param positions the 3D position of each vertex in the grid
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_grid(uint32_t row_count, uint32_t col_count, const vec3* positions);

/**
 * Create a surface mesh.
 *
 * The `heights` buffer should contain row_count * col_count float positions (C order).
 *
 * @param row_count number of rows
 * @param col_count number of columns
 * @param heights the height of each vertex in the grid
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_surface(uint32_t row_count, uint32_t col_count, const float* heights);

/**
 * Create a unit cube mesh (ranging [-0.5, +0.5]).
 *
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_cube(void);

/**
 * Create a sphere mesh.
 *
 * @param row_count number of rows
 * @param col_count number of columns
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_sphere(uint32_t row_count, uint32_t col_count);

/**
 * Create a cylinder mesh.
 *
 * @param count number of sides
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_cylinder(uint32_t count);

/**
 * Create a cone mesh.
 *
 * @param count number of sides
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_cone(uint32_t count);

/**
 * Create a square mesh.
 *
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_square(void);

/**
 * Create a disc mesh.
 *
 * @param count number of sides
 * @returns a mesh object
 */
DVZ_EXPORT DvzMesh dvz_mesh_disc(uint32_t count);

/**
 * Normalize a mesh.
 *
 * @param mesh the mesh
 */
DVZ_EXPORT void dvz_mesh_normalize(DvzMesh* mesh);

/**
 * Destroy a mesh.
 *
 * @param mesh the mesh
 */
DVZ_EXPORT void dvz_mesh_destroy(DvzMesh* mesh);



#ifdef __cplusplus
}
#endif

#endif
