#ifndef VKY_EARCUT_HEADER
#define VKY_EARCUT_HEADER

#include <visky/visky.h>

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Structures                                                                                   */
/*************************************************************************************************/

typedef struct triangulateio triangulateio; // forward declaration

typedef struct VkyPSLGTriangulation VkyPSLGTriangulation;
struct VkyPSLGTriangulation
{
    uint32_t vertex_count;          // vertices in the triangulation
    dvec2* vertices;                // TODO: rename into points?
    uint32_t index_count;           // indices making the triangles, 3 indices = 1 triangle
    VkyIndex* indices;              //
    double* region_idx;             // region index for each vertex
    struct triangulateio* triangle; // Triangle library struct
    VkyVertex* mesh_vertices;
};

typedef struct VkyPolygonTriangulation VkyPolygonTriangulation;
struct VkyPolygonTriangulation
{
    uint32_t index_count; // indices making the triangles, 3 indices = 1 triangle
    VkyIndex* indices;    //
    VkyVertex* mesh_vertices;
};



/*************************************************************************************************/
/*  Earcut polygon triangulation                                                                 */
/*************************************************************************************************/

// Ear-clip algorithm
VKY_EXPORT void vky_triangulate_polygon(
    const uint32_t, const dvec2*, // points
    uint32_t*, uint32_t**);       // triangulation

// Triangulation each polygon with the ear-clip algorithm.
VKY_EXPORT VkyPolygonTriangulation vky_triangulate_polygons(
    const uint32_t, const dvec2*,     // points
    const uint32_t, const uint32_t*); // polygons

VKY_EXPORT void vky_destroy_polygon_triangulation(VkyPolygonTriangulation*);



/*************************************************************************************************/
/*  Polygon                                                                                      */
/*************************************************************************************************/

typedef struct VkyPolygonParams VkyPolygonParams;
struct VkyPolygonParams
{
    float linewidth;
    VkyColorBytes edge_color;
};

VKY_EXPORT VkyVisualBundle* vky_bundle_polygon(VkyScene* scene, const VkyPolygonParams* params);

VKY_EXPORT VkyPolygonTriangulation vky_bundle_polygon_upload(
    VkyVisualBundle* vb,                                     // visual bundle
    const uint32_t point_count, const dvec2* points,         // points
    const uint32_t poly_count, const uint32_t* poly_lengths, // polygons
    const VkyColorBytes* poly_colors                         // polygon colors
);



/*************************************************************************************************/
/*  PSLG Delaunay triangulation with Triangle                                                    */
/*************************************************************************************************/

// Delaunay triangulation with the Triangle C library
VKY_EXPORT VkyPSLGTriangulation vky_triangulate_pslg(
    const uint32_t, const dvec2*, // points
    const uint32_t, const uvec2*, // segments
    const uint32_t, const dvec2*, // regions
    const char* triangle_params);

VKY_EXPORT void vky_destroy_pslg_triangulation(VkyPSLGTriangulation*);



/*************************************************************************************************/
/*  PSLG visual bundle                                                                           */
/*************************************************************************************************/

typedef struct VkyPSLGParams VkyPSLGParams;
struct VkyPSLGParams
{
    // segments
    float linewidth;
    VkyColorBytes edge_color;
};



VKY_EXPORT VkyVisualBundle* vky_bundle_pslg(VkyScene* scene, const VkyPSLGParams* params);

VKY_EXPORT VkyPSLGTriangulation vky_bundle_pslg_upload(
    VkyVisualBundle* vb,          //
    const uint32_t, const dvec2*, // points
    const uint32_t, const uvec2*, // segments
    const uint32_t, const dvec2*, // regions
    const VkyColorBytes*,         // region colors
    const char*);                 // triangle params



/*************************************************************************************************/
/*  Triangulation visual bundle with triangle segments and markers                               */
/*************************************************************************************************/

typedef struct VkyTriangulationParams VkyTriangulationParams;
struct VkyTriangulationParams
{
    // segments
    float linewidth;
    VkyColorBytes edge_color;
    // markers
    vec2 marker_size;
    VkyColorBytes marker_color;
};

VKY_EXPORT VkyVisualBundle* vky_bundle_triangulation(VkyScene*, const VkyTriangulationParams*);

VKY_EXPORT void vky_bundle_triangulation_upload(
    VkyVisualBundle*, uint32_t, size_t, const void*, uint32_t, const VkyIndex*);



#ifdef __cplusplus
}
#endif

#endif
