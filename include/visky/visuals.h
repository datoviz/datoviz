#ifndef VKY_VISUALS_HEADER
#define VKY_VISUALS_HEADER

#include "agg.h"
#include "fake.h"
#include "mesh.h"
#include "raw.h"
#include "rectangle.h"
#include "volume.h"



/*************************************************************************************************/
/*  Graph                                                                                        */
/*************************************************************************************************/

typedef struct VkyGraphParams VkyGraphParams;
struct VkyGraphParams
{
    float marker_edge_width;
    vec4 marker_edge_color;
};

typedef VkyMarkersVertex VkyGraphNode;

typedef struct VkyGraphEdge VkyGraphEdge;
struct VkyGraphEdge
{
    uint32_t source_node;
    uint32_t target_node;
    VkyColorBytes color;
    float linewidth;
    VkyCapType cap0;
    VkyCapType cap1;
};

VKY_EXPORT void vky_graph_upload(
    VkyVisualBundle* vb,                       //
    uint32_t node_count, VkyGraphNode* nodes,  // nodes
    uint32_t edge_count, VkyGraphEdge* edges); // edges

VKY_EXPORT VkyVisualBundle* vky_bundle_graph(VkyScene* scene, VkyGraphParams params);



/*************************************************************************************************/
/*  Colorbar                                                                                     */
/*************************************************************************************************/

typedef struct VkyColorbarVertex VkyColorbarVertex;
struct VkyColorbarVertex
{
    vec3 pos;
    vec2 padding;
    cvec2 uv;
};

VKY_EXPORT VkyVisualBundle* vky_bundle_colorbar(VkyScene* scene, VkyColorbarParams params);



#endif
