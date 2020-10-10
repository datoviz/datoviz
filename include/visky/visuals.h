#ifndef VKY_VIS_HEADER
#define VKY_VIS_HEADER

#include "scene.h"

#include "mesh.h"
#include "triangulation.h"


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_CAP_TYPE_NONE = 0,
    VKY_CAP_ROUND = 1,
    VKY_CAP_TRIANGLE_IN = 2,
    VKY_CAP_TRIANGLE_OUT = 3,
    VKY_CAP_SQUARE = 4,
    VKY_CAP_BUTT = 5
} VkyCapType;

typedef enum
{
    VKY_JOIN_SQUARE = false,
    VKY_JOIN_ROUND = true
} VkyJoinType;

// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKY_MARKER_ARROW = 0,
    VKY_MARKER_ASTERISK = 1,
    VKY_MARKER_CHEVRON = 2,
    VKY_MARKER_CLOVER = 3,
    VKY_MARKER_CLUB = 4,
    VKY_MARKER_CROSS = 5,
    VKY_MARKER_DIAMOND = 6,
    VKY_MARKER_DISC = 7,
    VKY_MARKER_ELLIPSE = 8,
    VKY_MARKER_HBAR = 9,
    VKY_MARKER_HEART = 10,
    VKY_MARKER_INFINITY = 11,
    VKY_MARKER_PIN = 12,
    VKY_MARKER_RING = 13,
    VKY_MARKER_SPADE = 14,
    VKY_MARKER_SQUARE = 15,
    VKY_MARKER_TAG = 16,
    VKY_MARKER_TRIANGLE = 17,
    VKY_MARKER_VBAR = 18,
} VkyMarkerType;

// NOTE: the numbers need to correspond to arrows.glsl at the bottom.
typedef enum
{
    VKY_ARROW_CURVED = 0,
    VKY_ARROW_STEALTH = 1,
    VKY_ARROW_ANGLE_30 = 2,
    VKY_ARROW_ANGLE_60 = 3,
    VKY_ARROW_ANGLE_90 = 4,
    VKY_ARROW_TRIANGLE_30 = 5,
    VKY_ARROW_TRIANGLE_60 = 6,
    VKY_ARROW_TRIANGLE_90 = 7,
} VkyArrowType;

typedef uint8_t VkyMarkerSize;



/*************************************************************************************************/
/*  Multi raw path visual                                                                        */
/*************************************************************************************************/

typedef struct VkyMultiRawPathParams VkyMultiRawPathParams;
struct VkyMultiRawPathParams
{
    vec4 info;                                  // path_count, vertex_count_per_path, scaling
    vec4 y_offsets[VKY_RAW_PATH_MAX_PATHS / 4]; // NOTE: 16 bytes alignment enforced
    vec4 colors[VKY_RAW_PATH_MAX_PATHS];        // 16 bytes per path
};


VKY_EXPORT VkyVisual*
vky_visual_path_raw_multi(VkyScene* scene, const VkyMultiRawPathParams* params);



/*************************************************************************************************/
/*  Image visual                                                                                 */
/*************************************************************************************************/
typedef struct VkyImageData VkyImageData;
struct VkyImageData
{
    vec3 p0, p1;
    vec2 uv0, uv1;
};

VKY_EXPORT VkyVisual* vky_visual_image(VkyScene* scene, const VkyTextureParams* params);

VKY_EXPORT void vky_visual_image_upload(VkyVisual*, const void*);


/*************************************************************************************************/
/*  Raw marker                                                                                   */
/*************************************************************************************************/

typedef enum
{
    VKY_SCALING_OFF,
    VKY_SCALING_ON
} VkyScalingMode;

typedef enum
{
    VKY_ALPHA_SCALING_OFF,
    VKY_ALPHA_SCALING_ON
} VkyAlphaScalingMode;

typedef struct VkyMarkersRawParams VkyMarkersRawParams;
struct VkyMarkersRawParams
{
    vec2 marker_size;
    int32_t scaling_mode;
    int32_t alpha_scaling_mode;
};

VKY_EXPORT VkyVisual* vky_visual_marker_raw(VkyScene* scene, const VkyMarkersRawParams* params);



/*************************************************************************************************/
/*  Raw path                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_path_raw(VkyScene* scene);



/*************************************************************************************************/
/*  Path visual                                                                                  */
/*************************************************************************************************/

typedef enum
{
    VKY_PATH_OPEN = 0,
    VKY_PATH_CLOSED = 1
} VkyPathTopology;

typedef enum
{
    VKY_DEPTH_DISABLE = 0,
    VKY_DEPTH_ENABLE = 1
} VkyDepthStatus;

typedef struct VkyPathParams VkyPathParams;
struct VkyPathParams
{
    float linewidth;
    float miter_limit;
    int32_t cap_type;
    int32_t round_join;
    int32_t enable_depth;
};

typedef struct VkyPathData VkyPathData;
struct VkyPathData
{
    uint32_t point_count;
    vec3* points;     // size: point_count
    VkyColor* colors; // size: point_count
    VkyPathTopology topology;
};

typedef struct VkyPathVertex VkyPathVertex;
struct VkyPathVertex
{
    vec3 p0;
    vec3 p1;
    vec3 p2;
    vec3 p3;
    VkyColor color;
};

VKY_EXPORT VkyVisual* vky_visual_path(VkyScene* scene, const VkyPathParams* params);



/*************************************************************************************************/
/*  Segment visual                                                                               */
/*************************************************************************************************/

typedef struct VkySegmentVertex VkySegmentVertex;
struct VkySegmentVertex
{
    vec3 P0;
    vec3 P1;
    vec4 shift;
    VkyColor color;
    float linewidth;
    int32_t cap0;
    int32_t cap1;
    uint8_t transform_mode;
};

VKY_EXPORT VkyVisual* vky_visual_segment(VkyScene* scene);



/*************************************************************************************************/
/*  Markers visual                                                                               */
/*************************************************************************************************/

typedef struct VkyMarkersVertex VkyMarkersVertex;
struct VkyMarkersVertex
{
    vec3 pos;
    VkyColor color;
    VkyMarkerSize size;
    uint8_t
        marker; // in fact a VkyMarkerType but we should control the exact data type for the GPU
    uint8_t angle;
};

typedef struct VkyMarkersParams VkyMarkersParams;
struct VkyMarkersParams
{
    vec4 edge_color;
    float edge_width;
    int32_t enable_depth;
};

VKY_EXPORT VkyVisual* vky_visual_marker(VkyScene* scene, const VkyMarkersParams* params);



/*************************************************************************************************/
/*  Text visual                                                                                  */
/*************************************************************************************************/

static const char VKY_TEXT_CHARS[] =
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";

typedef struct VkyTextData VkyTextData;
struct VkyTextData
{
    vec3 pos;
    vec2 shift;
    VkyColor color;
    float glyph_size;
    vec2 anchor;
    float angle;
    uint32_t string_len;
    char* string;
    uint8_t transform_mode;
};

typedef struct VkyTextVertex VkyTextVertex;
struct VkyTextVertex
{
    vec3 pos;
    vec2 shift;
    VkyColor color;
    vec2 glyph_size;
    vec2 anchor;
    float angle;
    usvec4 glyph; // char, char_index, str_len, str_index
    uint8_t transform_mode;
};

typedef struct VkyTextParams VkyTextParams;
struct VkyTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
};

VKY_EXPORT VkyVisual* vky_visual_text(VkyScene* scene);



/*************************************************************************************************/
/*  Arrow visual                                                                                 */
/*************************************************************************************************/

typedef struct VkyArrowVertex VkyArrowVertex;
struct VkyArrowVertex
{
    vec3 P0;
    vec3 P1;
    VkyColor color;
    float head;
    float linewidth;
    float arrow_type;
};

VKY_EXPORT VkyVisual* vky_visual_arrow(VkyScene* scene);



/*************************************************************************************************/
/*  Fake sphere visual                                                                           */
/*************************************************************************************************/

typedef struct VkyFakeSphereParams VkyFakeSphereParams;
struct VkyFakeSphereParams
{
    vec4 light_pos;
};

typedef struct VkyFakeSphereVertex VkyFakeSphereVertex;
struct VkyFakeSphereVertex
{
    vec3 pos;
    VkyColor color;
    float radius;
};

VKY_EXPORT VkyVisual* vky_visual_fake_sphere(VkyScene* scene, const VkyFakeSphereParams* params);



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

// typedef struct VkyMeshVertex VkyMeshVertex;
// struct VkyMeshVertex
// {
//     vec3 pos;
//     vec3 normal;
//     VkyColor color;
// };

VKY_EXPORT VkyMeshParams vky_default_mesh_params(
    VkyMeshColorType color_type, VkyMeshShading shading, ivec2 tex_size, float wire_linewidth);

VKY_EXPORT VkyVisual*
vky_visual_mesh(VkyScene* scene, const VkyMeshParams* params, const VkyTextureParams* tparams);

VKY_EXPORT void vky_visual_mesh_upload(VkyVisual* visual, const void* pixels);

VKY_EXPORT VkyVisual* vky_visual_mesh_raw(VkyScene* scene);

VKY_EXPORT VkyVisual* vky_visual_mesh_flat(VkyScene* scene);



/*************************************************************************************************/
/*  Rectangle visual                                                                             */
/*************************************************************************************************/

typedef struct VkyRectangleParams VkyRectangleParams;
struct VkyRectangleParams
{
    vec3 origin;
    vec3 u;
    vec3 v;
};

typedef struct VkyRectangleData VkyRectangleData;
struct VkyRectangleData
{
    // position of the lower left corner in the 2D coordinate system defined by (origin, u, v)
    vec2 p;
    vec2 size; // size of the rectangle
    VkyColor color;
};

VKY_EXPORT VkyVisual* vky_visual_rectangle(VkyScene* scene, const VkyRectangleParams* params);



/*************************************************************************************************/
/*  Area visual                                                                                  */
/*************************************************************************************************/

typedef struct VkyAreaParams VkyAreaParams;
struct VkyAreaParams
{
    vec3 origin;
    vec3 u;
    vec3 v;
};

typedef struct VkyAreaVertex VkyAreaVertex;
struct VkyAreaVertex
{
    vec3 pos;
    VkyColor color;
    uint32_t area_idx;
};

typedef struct VkyAreaData VkyAreaData;
struct VkyAreaData
{
    // position of the lower left corner in the 2D coordinate system defined by (origin, u, v)
    vec2 p;
    float h;
    VkyColor color;
    uint32_t area_idx;
};

VKY_EXPORT VkyVisual* vky_visual_area(VkyScene* scene, const VkyAreaParams* params);



/*************************************************************************************************/
/*  Axis rectangle visual                                                                        */
/*************************************************************************************************/

typedef struct VkyRectangleAxisData VkyRectangleAxisData;
struct VkyRectangleAxisData
{
    vec2 ab;
    uint8_t span_axis; // 0 or 1
    VkyColor color;
};

VKY_EXPORT VkyVisual* vky_visual_rectangle_axis(VkyScene* scene);



/*************************************************************************************************/
/*  Polygon                                                                                      */
/*************************************************************************************************/

typedef struct VkyPolygonParams VkyPolygonParams;
struct VkyPolygonParams
{
    float linewidth;
    VkyColor edge_color;
};

VKY_EXPORT VkyVisual* vky_visual_polygon(VkyScene* scene, const VkyPolygonParams* params);


/*************************************************************************************************/
/*  PSLG visual bundle                                                                           */
/*************************************************************************************************/

typedef struct VkyPSLGParams VkyPSLGParams;
struct VkyPSLGParams
{
    // segments
    float linewidth;
    VkyColor edge_color;
};



VKY_EXPORT VkyVisual* vky_visual_pslg(VkyScene* scene, const VkyPSLGParams* params);


/*************************************************************************************************/
/*  Triangulation visual bundle with triangle segments and markers                               */
/*************************************************************************************************/

typedef struct VkyTriangulationParams VkyTriangulationParams;
struct VkyTriangulationParams
{
    // segments
    float linewidth;
    VkyColor edge_color;
    // markers
    vec2 marker_size;
    VkyColor marker_color;
};

VKY_EXPORT VkyVisualBundle* vky_bundle_triangulation(VkyScene*, const VkyTriangulationParams*);

VKY_EXPORT void vky_bundle_triangulation_upload(
    VkyVisualBundle*, uint32_t, size_t, const void*, uint32_t, const VkyIndex*);



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

VkyVisual* vky_visual_volume_slicer(VkyScene* scene, VkyTexture* tex);



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
    VkyColor color;
    float linewidth;
    VkyCapType cap0;
    VkyCapType cap1;
};

VKY_EXPORT void vky_graph_upload(
    VkyVisual* vb,                             //
    uint32_t node_count, VkyGraphNode* nodes,  // nodes
    uint32_t edge_count, VkyGraphEdge* edges); // edges

VKY_EXPORT VkyVisual* vky_visual_graph(VkyScene* scene, VkyGraphParams params);



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

VKY_EXPORT VkyVisual* vky_visual_colorbar(VkyScene* scene, VkyColorbarParams params);



#endif
