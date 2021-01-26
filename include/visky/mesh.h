/*************************************************************************************************/
/*  Simple 3D mesh creation and manipulation                                                     */
/*************************************************************************************************/

#ifndef VKL_MESH_HEADER
#define VKL_MESH_HEADER

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
    VKL_MESH_CUSTOM,
    VKL_MESH_CUBE,
    VKL_MESH_SPHERE,
    VKL_MESH_SURFACE,
    VKL_MESH_CYLINDER,
    VKL_MESH_CONE,
    VKL_MESH_SQUARE,
    VKL_MESH_DISC,
    VKL_MESH_OBJ,
    VKL_MESH_COUNT,
} VklMeshType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklMesh VklMesh;



/*************************************************************************************************/
/*  Structs                                                                                */
/*************************************************************************************************/

struct VklMesh
{
    VklArray vertices;
    VklArray indices;
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
VKY_EXPORT void vkl_mesh_transform_reset(VklMesh* mesh);

/**
 * Append a mesh transformation.
 *
 * @param mesh the mesh
 * @param transform the transform matrix
 */
VKY_EXPORT void vkl_mesh_transform_add(VklMesh* mesh, mat4 transform);

/**
 * Append a translation transformation.
 *
 * @param mesh the mesh
 * @param translate the translation vector
 */
VKY_EXPORT void vkl_mesh_translate(VklMesh* mesh, vec3 translate);

/**
 * Append a scaling transformation.
 *
 * @param mesh the mesh
 * @param scale the scaling coefficients
 */
VKY_EXPORT void vkl_mesh_scale(VklMesh* mesh, vec3 scale);

/**
 * Append a rotation transformation.
 *
 * @param mesh the mesh
 * @param angle the rotation angle
 * @param axis the rotation axis
 */
VKY_EXPORT void vkl_mesh_rotate(VklMesh* mesh, float angle, vec3 axis);



/*************************************************************************************************/
/*  Common shapes                                                                                */
/*************************************************************************************************/

/**
 * Create a new mesh.
 *
 * A mesh is represented by an array of vertices of type `VklGraphicsMeshVertex` and indices, where
 * every triplet of vertex indices represents a triangular face of the mesh.
 *
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh(void);

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
VKY_EXPORT VklMesh vkl_mesh_grid(uint32_t row_count, uint32_t col_count, const vec3* positions);

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
VKY_EXPORT VklMesh vkl_mesh_surface(uint32_t row_count, uint32_t col_count, const float* heights);

/**
 * Create a unit cube mesh (ranging [-0.5, +0.5]).
 *
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh_cube(void);

/**
 * Create a sphere mesh.
 *
 * @param row_count number of rows
 * @param col_count number of columns
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh_sphere(uint32_t row_count, uint32_t col_count);

/**
 * Create a cylinder mesh.
 *
 * @param count number of sides
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh_cylinder(uint32_t count);

/**
 * Create a cone mesh.
 *
 * @param count number of sides
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh_cone(uint32_t count);

/**
 * Create a square mesh.
 *
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh_square(void);

/**
 * Create a disc mesh.
 *
 * @param count number of sides
 * @returns a mesh object
 */
VKY_EXPORT VklMesh vkl_mesh_disc(uint32_t count);

/**
 * Normalize a mesh.
 *
 * @param mesh the mesh
 */
VKY_EXPORT void vkl_mesh_normalize(VklMesh* mesh);

/**
 * Destroy a mesh.
 *
 * @param mesh the mesh
 */
VKY_EXPORT void vkl_mesh_destroy(VklMesh* mesh);



#ifdef __cplusplus
}
#endif

#endif
