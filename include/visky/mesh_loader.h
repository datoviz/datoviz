#ifndef VKY_MESH_LOADER_HEADER
#define VKY_MESH_LOADER_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include "mesh.h"


VKY_EXPORT void vky_mesh_assimp(VkyMesh* mesh, const char* file_path);

VKY_EXPORT void vky_mesh_obj(VkyMesh* mesh, const char* file_path);


#ifdef __cplusplus
}
#endif

#endif
