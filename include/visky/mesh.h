#ifndef VKY_MESH_HEADER
#define VKY_MESH_HEADER

#include "scene.h"



/*************************************************************************************************/
/*  Mesh visual                                                                                  */
/*************************************************************************************************/

typedef enum
{
    VKY_MESH_COLOR_RGBA = 1,
    VKY_MESH_COLOR_UV = 2,
} VkyMeshColorType;

typedef enum
{
    VKY_MESH_SHADING_NONE = 0,
    VKY_MESH_SHADING_BLINN_PHONG = 1,
} VkyMeshShading;

// Struct sent to the GPU as uniform buffer.
typedef struct VkyMeshParams VkyMeshParams;
struct VkyMeshParams
{
    vec4 light_pos;
    vec4 light_coefs;
    ivec2 tex_size;
    int32_t mode_color; // how to interpret the u16vec2 color attribute
    int32_t mode_shading;
    float wire_linewidth;
};

typedef struct VkyMeshVertex VkyMeshVertex;
struct VkyMeshVertex
{
    vec3 pos;
    vec3 normal;
    VkyColorBytes color;
};

VKY_EXPORT VkyVisual*
vky_visual_mesh(VkyScene* scene, VkyMeshColorType, VkyMeshShading, float, VkyTexture*);

VKY_EXPORT VkyVisual* vky_visual_mesh_raw(VkyScene* scene);

VKY_EXPORT VkyVisual* vky_visual_mesh_flat(VkyScene* scene);



/*************************************************************************************************/
/*  Mesh creation                                                                                */
/*************************************************************************************************/

typedef struct VkyMesh VkyMesh;
struct VkyMesh
{
    uint32_t max_vertex_count;
    VkyMeshVertex* vertices;

    uint32_t max_index_count;
    VkyIndex* indices;

    uint32_t object_count;
    VkDeviceSize vertex_size;
    uint32_t* vertex_offsets;
    uint32_t* index_offsets;

    mat4 current_transform;

    VkyVisual* visual;
    VkyTexture* texture;
};

VKY_EXPORT VkyMesh vky_create_mesh(uint32_t total_vertex_count, uint32_t total_index_count);

VKY_EXPORT void vky_mesh_set_transform(VkyMesh*, mat4 transform);

VKY_EXPORT VkyData vky_mesh_data(VkyMesh*);

VKY_EXPORT void vky_mesh_upload(VkyMesh*, VkyVisual*);

VKY_EXPORT void vky_mesh_destroy(VkyMesh*);

VKY_EXPORT void vky_normalize_mesh(uint32_t vertex_count, VkyMeshVertex* vertices);



/*************************************************************************************************/
/*  Mesh transformation                                                                          */
/*************************************************************************************************/

VKY_EXPORT void vky_mesh_transform_reset(VkyMesh* mesh);

VKY_EXPORT void vky_mesh_transform_add(VkyMesh* mesh, mat4 transform);

VKY_EXPORT void vky_mesh_translate(VkyMesh* mesh, vec3 translate);

VKY_EXPORT void vky_mesh_scale(VkyMesh* mesh, vec3 scale);

VKY_EXPORT void vky_mesh_rotate(VkyMesh* mesh, float angle, vec3 axis);



/*************************************************************************************************/
/*  Common shapes                                                                                */
/*************************************************************************************************/

VKY_EXPORT void vky_mesh_begin(
    VkyMesh* mesh, uint32_t* first_vertex, VkyMeshVertex** vertices, VkyIndex** indices);

VKY_EXPORT void vky_mesh_end(VkyMesh* mesh, uint32_t vertex_count, uint32_t index_count);

VKY_EXPORT void vky_mesh_grid(
    VkyMesh*, uint32_t row_count, uint32_t col_count, const vec3* positions, const void* color);

VKY_EXPORT void vky_mesh_grid_surface(
    VkyMesh* mesh, uint32_t row_count, uint32_t col_count, vec3 p00, vec3 p01, vec3 p10,
    const float* heights, const void* color);

VKY_EXPORT void vky_mesh_cube(VkyMesh*, const void*);

VKY_EXPORT void vky_mesh_sphere(VkyMesh*, uint32_t, uint32_t, const void*);

VKY_EXPORT void vky_mesh_cylinder(VkyMesh*, uint32_t, const void*);

VKY_EXPORT void vky_mesh_cone(VkyMesh*, uint32_t, const void*);

VKY_EXPORT void vky_mesh_square(VkyMesh*, const void*);

VKY_EXPORT void vky_mesh_disc(VkyMesh*, uint32_t count, const void* color);



#endif
