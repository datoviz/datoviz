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
    VKY_ALPHA_SCALING_OFF = 0,
    VKY_ALPHA_SCALING_ON = 1,
} VkyAlphaScalingMode;

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
    VKY_DEPTH_DISABLE = 0,
    VKY_DEPTH_ENABLE = 1
} VkyDepthStatus;

typedef enum
{
    VKY_JOIN_SQUARE = false,
    VKY_JOIN_ROUND = true
} VkyJoinType;

// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKY_MARKER_DISC = 0,
    VKY_MARKER_ASTERISK = 1,
    VKY_MARKER_CHEVRON = 2,
    VKY_MARKER_CLOVER = 3,
    VKY_MARKER_CLUB = 4,
    VKY_MARKER_CROSS = 5,
    VKY_MARKER_DIAMOND = 6,
    VKY_MARKER_ARROW = 7,
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

typedef enum
{
    VKY_PATH_OPEN = 0,
    VKY_PATH_CLOSED = 1
} VkyPathTopology;

typedef enum
{
    VKY_SCALING_OFF = 0,
    VKY_SCALING_ON = 1,
} VkyScalingMode;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VkyVertexUV VkyVertexUV;
typedef struct VkyVertexUVW VkyVertexUVW;
typedef struct VkyVertex VkyVertex;
typedef struct VkyVertexUV VkyVertexUV;

typedef struct VkyAreaData VkyAreaData;
typedef struct VkyAreaParams VkyAreaParams;
typedef struct VkyAreaVertex VkyAreaVertex;
typedef struct VkyArrowVertex VkyArrowVertex;
typedef struct VkyColorbarVertex VkyColorbarVertex;
typedef struct VkyFakeSphereParams VkyFakeSphereParams;
typedef struct VkyFakeSphereVertex VkyFakeSphereVertex;
typedef struct VkyGraphEdge VkyGraphEdge;
typedef struct VkyGraphParams VkyGraphParams;
typedef struct VkyImageCmapParams VkyImageCmapParams;
typedef struct VkyImageData VkyImageData;
typedef struct VkyMarkersParams VkyMarkersParams;
typedef struct VkyMarkersRawParams VkyMarkersRawParams;
typedef struct VkyMarkersTransientParams VkyMarkersTransientParams;
typedef struct VkyMarkersTransientVertex VkyMarkersTransientVertex;
typedef struct VkyMarkersVertex VkyMarkersVertex;
typedef VkyMarkersVertex VkyGraphNode;
typedef struct VkyMeshParams VkyMeshParams;
typedef struct VkyMeshVertex VkyMeshVertex;
typedef struct VkyMultiRawPathParams VkyMultiRawPathParams;
typedef struct VkyPathData VkyPathData;
typedef struct VkyPathParams VkyPathParams;
typedef struct VkyPathVertex VkyPathVertex;
typedef struct VkyPolygonParams VkyPolygonParams;
typedef struct VkyPSLGParams VkyPSLGParams;
typedef struct VkyRectangleAxisData VkyRectangleAxisData;
typedef struct VkyRectangleData VkyRectangleData;
typedef struct VkyRectangleParams VkyRectangleParams;
typedef struct VkySegmentVertex VkySegmentVertex;
typedef struct VkyTextData VkyTextData;
typedef struct VkyTextParams VkyTextParams;
typedef struct VkyTextVertex VkyTextVertex;
typedef struct VkyTriangulationParams VkyTriangulationParams;
typedef struct VkyVolumeParams VkyVolumeParams;



/*************************************************************************************************/
/*  Consts                                                                                       */
/*************************************************************************************************/

static const char VKY_TEXT_CHARS[] =
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";



/*************************************************************************************************/
/*  Common vertex structs                                                                        */
/*************************************************************************************************/

// Default vertex with vec3 pos and usvec4 color.
struct VkyVertex
{
    vec3 pos;
    VkyColor color; // 4 bytes for RGBA
};

// Default vertex with vec3 pos and vec2 texture 2D coordinates.
struct VkyVertexUV
{
    vec3 pos;
    vec2 uv;
};

// Default vertex with vec3 pos and vec3 texture 3D coordinates.
struct VkyVertexUVW
{
    vec3 pos;
    vec3 uvw;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VkyRectangleParams
{
    vec3 origin;
    vec3 u;
    vec3 v;
};

struct VkyRectangleData
{
    // position of the lower left corner in the 2D coordinate system defined by (origin, u, v)
    vec2 p;
    vec2 size; // size of the rectangle
    VkyColor color;
};

struct VkyRectangleAxisData
{
    vec2 ab;
    uint8_t span_axis; // 0 or 1
    VkyColor color;
};



struct VkyAreaParams
{
    vec3 origin;
    vec3 u;
    vec3 v;
};

struct VkyAreaData
{
    // position of the lower left corner in the 2D coordinate system defined by (origin, u, v)
    vec2 p;
    float h;
    VkyColor color;
};

struct VkyAreaVertex
{
    vec3 pos;
    VkyColor color;
    uint32_t area_idx;
};



// Struct sent to the GPU as uniform buffer.
struct VkyMeshParams
{
    vec4 light_pos;
    vec4 light_coefs;
    ivec2 tex_size;
    int32_t mode_color; // how to interpret the u16vec2 color attribute
    int32_t mode_shading;
    float wire_linewidth;
};

struct VkyMeshVertex
{
    vec3 pos;
    vec3 normal;
    VkyColor color;
};


struct VkyMarkersVertex
{
    vec3 pos;
    VkyColor color;
    float size;
    uint8_t
        marker; // in fact a VkyMarkerType but we should control the exact data type for the GPU
    uint8_t angle;
};

struct VkyMarkersParams
{
    vec4 edge_color;
    float edge_width;
    int32_t enable_depth;
};

struct VkyMarkersRawParams
{
    vec2 marker_size;
    int32_t scaling_mode;
    int32_t alpha_scaling_mode;
};

struct VkyMarkersTransientVertex
{
    vec3 pos;
    VkyColor color;
    float size;
    float half_life;
    float last_active;
};

struct VkyMarkersTransientParams
{
    float local_time;
};



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



struct VkyArrowVertex
{
    vec3 P0;
    vec3 P1;
    VkyColor color;
    float head;
    float linewidth;
    float arrow_type;
};



struct VkyPathParams
{
    float linewidth;
    float miter_limit;
    int32_t cap_type;
    int32_t round_join;
    int32_t enable_depth;
};

struct VkyPathData
{
    vec3 pos;
    VkyColor color;
    // uint32_t path_idx;
};

struct VkyPathVertex
{
    vec3 p0;
    vec3 p1;
    vec3 p2;
    vec3 p3;
    VkyColor color;
};



struct VkyMultiRawPathParams
{
    vec4 info;                                  // path_count, vertex_count_per_path, scaling
    vec4 y_offsets[VKY_RAW_PATH_MAX_PATHS / 4]; // NOTE: 16 bytes alignment enforced
    vec4 colors[VKY_RAW_PATH_MAX_PATHS];        // 16 bytes per path
};



struct VkyFakeSphereParams
{
    vec4 light_pos;
};

struct VkyFakeSphereVertex
{
    vec3 pos;
    VkyColor color;
    float radius;
};



struct VkyImageData
{
    vec3 p0, p1;
    vec2 uv0, uv1;
};

struct VkyImageCmapParams
{
    uint32_t cmap;
    float scaling;
    float alpha;
    VkyTextureParams* tex_params;
};



struct VkyVolumeParams
{
    mat4 inv_proj_view;
    mat4 normal_mat;
};

struct VkyColorbarVertex
{
    vec3 pos;
    vec2 padding;
    cvec2 uv;
};



struct VkyGraphParams
{
    float marker_edge_width;
    vec4 marker_edge_color;
};

struct VkyGraphEdge
{
    uint32_t source_node;
    uint32_t target_node;
    VkyColor color;
    float linewidth;
    VkyCapType cap0;
    VkyCapType cap1;
};



struct VkyTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
};

struct VkyTextData
{
    vec3 pos;
    vec2 shift;
    VkyColor color;
    float glyph_size;
    vec2 anchor;
    float angle;
    char glyph;
    uint8_t transform_mode;
};

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



struct VkyPolygonParams
{
    float linewidth;
    VkyColor edge_color;
};

struct VkyPSLGParams
{
    // segments
    float linewidth;
    VkyColor edge_color;
};

struct VkyTriangulationParams
{
    // segments
    float linewidth;
    VkyColor edge_color;
    // markers
    vec2 marker_size;
    VkyColor marker_color;
};



/*************************************************************************************************/
/*  Rectangle visual                                                                             */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_rectangle(VkyScene* scene, const VkyRectangleParams* params);



/*************************************************************************************************/
/*  Axis rectangle visual                                                                        */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_rectangle_axis(VkyScene* scene);



/*************************************************************************************************/
/*  Area visual                                                                                  */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_area(VkyScene* scene, const VkyAreaParams* params);



/*************************************************************************************************/
/*  Mesh visual                                                                                  */
/*************************************************************************************************/

VKY_EXPORT VkyMeshParams vky_default_mesh_params(
    VkyMeshColorType color_type, VkyMeshShading shading, ivec2 tex_size, float wire_linewidth);

VKY_EXPORT VkyVisual*
vky_visual_mesh(VkyScene* scene, const VkyMeshParams* params, const VkyTextureParams* tparams);

VKY_EXPORT void vky_visual_mesh_upload(VkyVisual* visual, const void* pixels);

VKY_EXPORT VkyVisual* vky_visual_mesh_raw(VkyScene* scene);

VKY_EXPORT VkyVisual* vky_visual_mesh_flat(VkyScene* scene);



/*************************************************************************************************/
/*  Markers visual                                                                               */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_marker(VkyScene* scene, const VkyMarkersParams* params);



/*************************************************************************************************/
/*  Raw marker                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_marker_raw(VkyScene* scene, const VkyMarkersRawParams* params);

VKY_EXPORT VkyVisual*
vky_visual_marker_transient(VkyScene* scene, const VkyMarkersTransientParams* params);



/*************************************************************************************************/
/*  Segment visual                                                                               */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_segment(VkyScene* scene);



/*************************************************************************************************/
/*  Arrow visual                                                                                 */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_arrow(VkyScene* scene);



/*************************************************************************************************/
/*  Path visual                                                                                  */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_path(VkyScene* scene, const VkyPathParams* params);



/*************************************************************************************************/
/*  Raw path                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_path_raw(VkyScene* scene);



/*************************************************************************************************/
/*  Multi raw path visual                                                                        */
/*************************************************************************************************/

VKY_EXPORT VkyVisual*
vky_visual_path_raw_multi(VkyScene* scene, const VkyMultiRawPathParams* params);



/*************************************************************************************************/
/*  Fake sphere visual                                                                           */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_fake_sphere(VkyScene* scene, const VkyFakeSphereParams* params);



/*************************************************************************************************/
/*  Image visual                                                                                 */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_image(VkyScene* scene, const VkyTextureParams* params);

VKY_EXPORT VkyVisual* vky_visual_image_cmap(VkyScene* scene, const VkyImageCmapParams* params);

VKY_EXPORT void vky_visual_image_upload(VkyVisual* visual, const void* image);



/*************************************************************************************************/
/*  Volume visual                                                                                */
/*************************************************************************************************/

VKY_EXPORT VkyVisual*
vky_visual_volume(VkyScene* scene, const VkyTextureParams* params, const void* volume);



/*************************************************************************************************/
/*  Volume slicer visual                                                                         */
/*************************************************************************************************/

VkyVisual* vky_visual_volume_slicer(VkyScene* scene, VkyTexture* tex);



/*************************************************************************************************/
/*  Colorbar                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_colorbar(VkyScene* scene, VkyColorbarParams params);



/*************************************************************************************************/
/*  Graph                                                                                        */
/*************************************************************************************************/

VKY_EXPORT void vky_graph_upload(
    VkyVisual* visual_root,                    //
    uint32_t node_count, VkyGraphNode* nodes,  // nodes
    uint32_t edge_count, VkyGraphEdge* edges); // edges

VKY_EXPORT VkyVisual* vky_visual_graph(VkyScene* scene, VkyGraphParams params);



/*************************************************************************************************/
/*  Text visual                                                                                  */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_text(VkyScene* scene);

VKY_EXPORT void vky_visual_text_set_size(VkyVisual* visual, uint32_t char_count);
VKY_EXPORT void vky_visual_text_add_string(VkyVisual* visual, const char* text);



/*************************************************************************************************/
/*  Polygon                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_polygon(VkyScene* scene, const VkyPolygonParams* params);



/*************************************************************************************************/
/*  PSLG visual                                                                                  */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_pslg(VkyScene* scene, const VkyPSLGParams* params);



/*************************************************************************************************/
/*  Triangulation visual with triangle segments and markers                                      */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_triangulation(VkyScene*, const VkyTriangulationParams*);

VKY_EXPORT void vky_visual_triangulation_upload(
    VkyVisual*, uint32_t, size_t, const void*, uint32_t, const VkyIndex*);



#endif
