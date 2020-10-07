#ifndef VKY_VOLUME_HEADER
#define VKY_VOLUME_HEADER

#include "mesh.h"
#include "scene.h"



/*************************************************************************************************/
/*  Volume visual                                                                                */
/*************************************************************************************************/

typedef struct VkyVertexUV VkyVertexUV;
struct VkyVertexUV
{
    vec3 pos;
    vec2 uv;
};

typedef struct VkyVolumeParams VkyVolumeParams;
struct VkyVolumeParams
{
    mat4 inv_proj_view;
    mat4 normal_mat;
};

VKY_EXPORT VkyVisual* vky_visual_volume(VkyScene*, const VkyTextureParams*, const void*);



/*************************************************************************************************/
/*  Volume slicer visual                                                                         */
/*************************************************************************************************/

typedef struct VkyTexturedVertex3D VkyTexturedVertex3D;
struct VkyTexturedVertex3D
{
    vec3 pos;
    vec3 coords;
};

VkyVisual*
vky_visual_volume_slicer(VkyScene* scene, const VkyTextureParams* tex_params, const void* pixels);


#endif
