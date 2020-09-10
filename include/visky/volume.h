#ifndef VKY_VOLUME_HEADER
#define VKY_VOLUME_HEADER

#include "mesh.h"
#include "scene.h"



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



#endif
