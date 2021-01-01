#ifndef VKL_MESH_HEADER
#define VKL_MESH_HEADER

#include "array.h"
#include "graphics.h"


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

VKY_EXPORT void vkl_mesh_transform_reset(VklMesh* mesh);

VKY_EXPORT void vkl_mesh_transform_add(VklMesh* mesh, mat4 transform);

VKY_EXPORT void vkl_mesh_translate(VklMesh* mesh, vec3 translate);

VKY_EXPORT void vkl_mesh_scale(VklMesh* mesh, vec3 scale);

VKY_EXPORT void vkl_mesh_rotate(VklMesh* mesh, float angle, vec3 axis);



/*************************************************************************************************/
/*  Common shapes                                                                                */
/*************************************************************************************************/

VKY_EXPORT VklMesh vkl_mesh(void);

VKY_EXPORT VklMesh vkl_mesh_grid(uint32_t row_count, uint32_t col_count, const vec3* positions);

VKY_EXPORT VklMesh vkl_mesh_grid_surface(
    uint32_t row_count, uint32_t col_count, vec3 p00, vec3 p01, vec3 p10, const float* heights);

VKY_EXPORT VklMesh vkl_mesh_cube(void);

VKY_EXPORT VklMesh vkl_mesh_sphere(uint32_t rows, uint32_t cols);

VKY_EXPORT VklMesh vkl_mesh_cylinder(uint32_t count);

VKY_EXPORT VklMesh vkl_mesh_cone(uint32_t count);

VKY_EXPORT VklMesh vkl_mesh_square(void);

VKY_EXPORT VklMesh vkl_mesh_disc(uint32_t count);

VKY_EXPORT void vkl_mesh_destroy(VklMesh* mesh);



#endif
