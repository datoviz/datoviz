// #include <visky/triangulation.h>

#ifdef SINGLE
#define REAL float
#else /* not SINGLE */
#define REAL double
#endif /* not SINGLE */
#define TRILIBRARY 1
#define VOID       int

#include <stdio.h>
#include <stdlib.h>

#include <triangle.h>

#include <visky/visky.h>



/*************************************************************************************************/
/*  Earcut polygon triangulation                                                                 */
/*************************************************************************************************/

// NOTE: caller must free tr.indices
VkyPolygonTriangulation vky_triangulate_polygons(
    const uint32_t point_count, const dvec2* points,               // points
    const uint32_t polygon_count, const uint32_t* polygon_lengths) // polygons
{
    uint32_t index_count = 0;
    uint32_t total_index_count = 0;
    uint32_t* indices = NULL;
    uint32_t* index_count_list = (uint32_t*)calloc(polygon_count, sizeof(uint32_t));
    uint32_t** indices_list = (uint32_t**)calloc(polygon_count, sizeof(uint32_t*));
    uint32_t offset = 0;

    // Triangulate all polygons.
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        vky_triangulate_polygon(polygon_lengths[i], points + offset, &index_count, &indices);
        total_index_count += index_count;
        // Save each triangulation's indices in order to concatenate them afterwards.
        index_count_list[i] = index_count;
        indices_list[i] = indices;
        offset += polygon_lengths[i];
    }

    // Concatenate all triangulations.
    uint32_t* total_indices = (uint32_t*)calloc(total_index_count, sizeof(uint32_t));
    offset = 0;
    uint32_t voffset = 0;
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        for (uint32_t j = 0; j < index_count_list[i]; j++)
        {
            total_indices[offset + j] = voffset + indices_list[i][j];
        }
        offset += index_count_list[i];
        voffset += polygon_lengths[i];
        free(indices_list[i]);
    }

    // Make the output structure.
    VkyPolygonTriangulation tr = {0};
    tr.index_count = total_index_count;
    // This field must be freed by the caller.
    tr.indices = total_indices;

    free(index_count_list);
    free(indices_list);
    return tr;
}


void vky_destroy_polygon_triangulation(VkyPolygonTriangulation* tr)
{
    free(tr->indices);
    if (tr->mesh_vertices != NULL)
        free(tr->mesh_vertices);
}



/*************************************************************************************************/
/*  Polygon visual bundle                                                                        */
/*************************************************************************************************/

VkyVisualBundle* vky_bundle_polygon(VkyScene* scene, VkyPolygonParams params)
{
    VkyVisualBundle* vb = vky_create_visual_bundle(scene);

    // Raw mesh visual.
    VkyVisual* visual_poly = vky_visual_mesh_raw(scene);
    vky_add_visual_to_bundle(vb, visual_poly);

    // Polygon outlines.
    if (params.linewidth > 0)
    {
        VkyPathParams vparams = (VkyPathParams){
            params.linewidth, 4., VKY_CAP_ROUND, VKY_JOIN_ROUND, VKY_DEPTH_DISABLE};
        VkyVisual* visual_outline = vky_visual_path(scene, &vparams);
        vky_add_visual_to_bundle(vb, visual_outline);
    }

    // Copy the parameters.
    vb->params = malloc(sizeof(VkyPolygonParams));
    memcpy(vb->params, &params, sizeof(VkyPolygonParams));

    return vb;
}



VkyPolygonTriangulation vky_bundle_polygon_upload(
    VkyVisualBundle* vb,                                     // visual bundle
    const uint32_t point_count, const dvec2* points,         // points
    const uint32_t poly_count, const uint32_t* poly_lengths, // polygons
    const VkyColorBytes* poly_colors                         // polygon colors
)
{
    VkyPolygonParams* params = (VkyPolygonParams*)vb->params;
    ASSERT(params != NULL);

    VkyVisual* visual_poly = vb->visuals[0];
    VkyVisual* visual_outline = NULL;
    if (vb->visual_count == 2)
        visual_outline = vb->visuals[1];

    // Make the triangulation of the polygons.
    VkyPolygonTriangulation tr =
        vky_triangulate_polygons(point_count, points, poly_count, poly_lengths);

    // Upload the polygon visual data.
    VkyVertex* vertices = calloc(point_count, sizeof(VkyVertex));
    uint32_t poly = 0, offset = 0;
    for (uint32_t i = 0; i < point_count; i++)
    {
        vertices[i].pos[0] = points[i][0];
        vertices[i].pos[1] = points[i][1];
        vertices[i].pos[2] = 0;

        vertices[i].color = poly_colors[poly];

        if (i - offset >= poly_lengths[poly] - 1)
        {
            ASSERT(poly < poly_count);
            offset += poly_lengths[poly];
            poly++;
        }
    }
    ASSERT(poly == poly_count);
    ASSERT(offset == point_count);
    vky_visual_upload(
        visual_poly, (VkyData){0, NULL, point_count, vertices, tr.index_count, tr.indices});

    if (visual_outline != NULL)
    {
        // Make the paths data.
        VkyPathData* paths = calloc(poly_count, sizeof(VkyPathData));
        vec3* path_points = calloc(point_count, sizeof(vec3));
        VkyColorBytes* path_colors = calloc(point_count, sizeof(VkyColorBytes));

        for (uint32_t i = 0; i < point_count; i++)
        {
            path_points[i][0] = (float)points[i][0];
            path_points[i][1] = (float)points[i][1];
            path_colors[i] = params->edge_color;
        }
        offset = 0;
        uint32_t n = 0;
        for (uint32_t i = 0; i < poly_count; i++)
        {
            n = poly_lengths[i];
            paths[i].point_count = n;
            paths[i].topology = VKY_PATH_CLOSED;
            paths[i].points = path_points + offset;
            paths[i].colors = path_colors + offset;
            offset += n;
        }
        ASSERT(offset == point_count);

        // Upload the paths data.
        vky_visual_upload(visual_outline, (VkyData){poly_count, paths});
        free(paths);
        free(path_points);
        free(path_colors);
    }

    // Cleanup.
    // vky_destroy_polygon_triangulation(&tr);

    tr.mesh_vertices = vertices; // to be freed by the caller
    return tr;
}



/*************************************************************************************************/
/*  PSLG Delaunay triangulation with Triangle                                                    */
/*************************************************************************************************/

static void*
concatenate_arrays(size_t item_size, size_t size0, void* arr0, size_t size1, void* arr1)
{
    size0 *= item_size;
    size1 *= item_size;
    // Concatenate 2 arrays.
    void* cat = malloc(size0 + size1);
    memcpy(cat, arr0, size0);
    memcpy((void*)((int64_t)cat + (int64_t)size0), arr1, size1);
    free(arr0);
    free(arr1);
    return cat;
}



VkyPSLGTriangulation vky_triangulate_pslg(
    const uint32_t point_count, const dvec2* points,         // PSLG points
    const uint32_t segment_count, const uvec2* segments,     // PSLG segments
    const uint32_t region_count, const dvec2* region_coords, // regions
    const char* triangle_params)
{
    // Triangulation parameters.

    // This should always be true, except for debugging purposes. This parameter enables an
    // additional postprocessing step in the triangulation, where triangles lying on the edges
    // between different regions are duplicated to avoid conflicts between the vertex colors that
    // are shared on the GPU (there is normally only one copy of each vertex, and the indices are
    // used to define the triangles).
    const bool add_duplicate_vertices = true;

    // Triangle parameters string.
    const char* default_triangle_params = VKY_DEFAULT_TRIANGLE_PARAMS;
    if (triangle_params == NULL)
        triangle_params = default_triangle_params;

    VkyPSLGTriangulation tr = {0};

    // Initialize the Triangle structures.
    struct triangulateio in = {0}, out = {0};
    in.numberofpoints = (int)point_count;
    in.numberofpointattributes = 0;
    in.pointattributelist = NULL;
    in.pointmarkerlist = NULL;

    // List of PSLG points.
    in.pointlist = (REAL*)malloc(point_count * sizeof(dvec2));
    // NOTE: make a copy to avoid modifying the output array.
    memcpy(in.pointlist, points, point_count * sizeof(dvec2));

    // Segments.
    in.numberofsegments = (int)segment_count;
    in.segmentlist = (int*)malloc(segment_count * sizeof(uvec2));
    memcpy(in.segmentlist, segments, segment_count * sizeof(uvec2));
    in.numberofholes = 0;

    // Regions.
    in.numberoftriangleattributes = 1;
    in.numberofregions = (int)region_count;
    in.regionlist = NULL;
    // Initialize the regions to match the Triangle format.
    if (region_count > 0)
    {
        in.regionlist = (REAL*)calloc((size_t)in.numberofregions * 4, sizeof(REAL));
        // Each region = 1 point, Triangle will assign region_idx to each vertex in the region.
        for (uint32_t i = 0; i < region_count; i++)
        {
            in.regionlist[4 * i + 0] = region_coords[i][0];
            in.regionlist[4 * i + 1] = region_coords[i][1];
            in.regionlist[4 * i + 2] = 1 + (double)round(i);
            in.regionlist[4 * i + 3] = 0;
        }
    }

    // Initialize the output structure.
    out.pointlist = (REAL*)NULL;             // will contain the triangulation points
    out.trianglelist = (int*)NULL;           // will contain the triangle indices
    out.segmentlist = (int*)NULL;            // will contain the segment point indices (2 ind/seg)
    out.triangleattributelist = (REAL*)NULL; // will contain the triangle region indices (3 ind/tr)

    // Launch the triangulation of the PSLG .
    log_debug("starting triangulation");
    char triangle_params_[64];
    strcpy(triangle_params_, triangle_params);
    triangulate(triangle_params_, &in, &out, NULL);
    log_debug("triangulation succeeded");

    // Check the output variables have been set.
    ASSERT(out.numberofcorners == 3);
    const uint32_t three = (uint32_t)out.numberofcorners;

    ASSERT(out.pointlist != NULL);
    ASSERT(out.trianglelist != NULL);
    ASSERT(out.segmentlist != NULL);
    ASSERT(out.triangleattributelist != NULL);

    ASSERT(out.numberofpoints > 0);
    ASSERT(out.numberoftriangles > 0);

    // Retrieve and store the results.
    tr.vertex_count = (uint32_t)out.numberofpoints;
    uint32_t N = tr.vertex_count; // shortcut for the number of vertices in the triangulation
    tr.index_count = three * (uint32_t)out.numberoftriangles;

    tr.vertices = (dvec2*)out.pointlist;
    tr.indices = (VkyIndex*)out.trianglelist;

    // Find the vertices that belong to the input segments so that we can detect vertex region
    // conflicts.
    bool* onedge = (bool*)calloc(tr.vertex_count, sizeof(bool));
    for (uint32_t i = 0; i < (uint32_t)out.numberofsegments; i++)
    {
        onedge[out.segmentlist[2 * i + 0]] = true;
        onedge[out.segmentlist[2 * i + 1]] = true;
    }

    // We allocate an array giving the region of each vertex. Triangle gives the region of each
    // triangle instead, but the GPU expects one color per vertex and not one color per triangle.
    // However this may introduce conflicts between vertices that are shared between triangles from
    // different regions.
    tr.region_idx = (double*)calloc(tr.vertex_count, sizeof(double)); // need to be freed by caller
    uint32_t i0, i1, i2;
    double region = 0;

    // Extra vertices for complex boundaries.
    // For each triangle, whether we need to duplicate it.
    uint32_t extra_triangle_count = 0;
    bool* extra_triangles = (bool*)calloc((size_t)out.numberoftriangles, sizeof(bool));

    // Loop over all triangles.
    for (uint32_t i = 0; i < (size_t)out.numberoftriangles; i++)
    {
        // Processing trick #1 to fix vertex region conflicts on the boundaries:
        //
        // HACK: when a triangle lies partly on a segment, its (flat) color will be determined by
        // the color of its first vertex among the 3 that makes the triangle (flat qualifier in
        // GLSL). So we want to make sure that, as much as possible, the first vertex of any
        // triangle does *not* lie on a segment, such that its color will be determined by
        // one of its 2 other vertices, which (hopefully) will be inside a region, and not
        // on a segment as well. Obviously this is not always possible as there are triangles
        // for which all 3 vertices may lie on a segment. They will be dealed with another trick
        // below. Here, if we detect that the first vertex of a triangle lies on a segment, we
        // cycle the indices of the 3 vertices in the triangle.
        for (uint32_t k = 0; k < three - 1; k++)
        {
            if (onedge[tr.indices[three * i + 0]]) // the first vertex of the current triangle lies
                                                   // on a segment
            {
                i0 = tr.indices[three * i + 0];
                i1 = tr.indices[three * i + 1];
                i2 = tr.indices[three * i + 2];

                tr.indices[three * i + 0] = i1;
                tr.indices[three * i + 1] = i2;
                tr.indices[three * i + 2] = i0;
            }
            else
            {
                break;
            }
        }

        // Get the region assigned by Triangle to the current triangle.
        region = out.triangleattributelist[i];
        if (region == 0)
        {
            log_trace("region 0 assigned to triangle %d", i);
            continue;
        }

        // The three vertex indices making the current triangle.
        i0 = tr.indices[three * i + 0];
        i1 = tr.indices[three * i + 1];
        i2 = tr.indices[three * i + 2];

        // Processing trick #2 to fix vertex region conflicts on the boundaries:
        //
        // Now we consider the special case where the 3 vertices lie on a segment, AND
        // a different region than the current region has been assigned to at least one of
        // the 3 vertices.
        if (onedge[i0] && onedge[i1] && onedge[i2])
        {
            // log_debug("extra triangle %d region %.3f", i, region);
            // We note that the current triangle will need to be duplicated afterwards.
            extra_triangles[i] = true;
            extra_triangle_count++;
        }


        // NOTE: in case of extra triangle, these indices will be overriden in the subsequent step
        // below
        tr.region_idx[i0] = region;
        tr.region_idx[i1] = region;
        tr.region_idx[i2] = region;
    }

    // Processing trick #2 to fix vertex region conflicts on the boundaries:
    // - vertex duplication step:
    //
    if (add_duplicate_vertices)
    {
        // Allocate the extra vertices.
        uint32_t extra_vertex_count = extra_triangle_count * three;
        dvec2* extra_vertices = (dvec2*)calloc(extra_vertex_count, sizeof(dvec2));
        double* extra_region_idx = (double*)calloc(extra_vertex_count, sizeof(double));

        uint32_t extra_offset = 0; // offset of the extra triangle
        uint32_t i0n = 0, i1n = 0, i2n = 0;
        for (uint32_t i = 0; i < (uint32_t)out.numberoftriangles; i++)
        {
            // Go through the extra triangles.
            if (extra_triangles[i])
            {
                // The three vertex indices making the current triangle.
                i0 = tr.indices[three * i + 0];
                i1 = tr.indices[three * i + 1];
                i2 = tr.indices[three * i + 2];

                // Region currently assigned to the current triangle.
                region = out.triangleattributelist[i];

                // Indices of the new vertices, relative to the start of the new vertex array.
                i0n = three * extra_offset + 0;
                i1n = three * extra_offset + 1;
                i2n = three * extra_offset + 2;

                // Copy the 3 vertices making the triangle.
                extra_vertices[i0n][0] = tr.vertices[i0][0];
                extra_vertices[i0n][1] = tr.vertices[i0][1];
                extra_vertices[i1n][0] = tr.vertices[i1][0];
                extra_vertices[i1n][1] = tr.vertices[i1][1];
                extra_vertices[i2n][0] = tr.vertices[i2][0];
                extra_vertices[i2n][1] = tr.vertices[i2][1];

                // Replace the indices of the wrong triangle to the newly-created on.
                tr.indices[three * i + 0] = N + i0n;
                tr.indices[three * i + 1] = N + i1n;
                tr.indices[three * i + 2] = N + i2n;

                // Assign the region to the new triangle.
                extra_region_idx[three * extra_offset + 0] = region;
                extra_region_idx[three * extra_offset + 1] = region;
                extra_region_idx[three * extra_offset + 2] = region;

                extra_offset++; // triangle offset
            }
        }
        ASSERT(extra_offset == extra_triangle_count);

        // Concatenate tr.vertices with extra_vertices
        tr.vertices = (dvec2*)concatenate_arrays(
            sizeof(dvec2), N, tr.vertices,       // initial vertices
            extra_vertex_count, extra_vertices); // extra vertices

        // Concatenate region_idx with extra_region_idx
        tr.region_idx = (double*)concatenate_arrays(
            sizeof(double), N, tr.region_idx,      // initial regions
            extra_vertex_count, extra_region_idx); // extra regions

        // Update the number of vertices.
        out.numberofpoints += (int)extra_vertex_count;
        tr.vertex_count += extra_vertex_count;
        ASSERT(tr.vertex_count == (uint32_t)out.numberofpoints);

        free(extra_triangles);
    }

    // Copy the triangle struct into the output struct.
    tr.triangle = (struct triangulateio*)calloc(1, sizeof(struct triangulateio));
    memcpy(tr.triangle, &out, sizeof(out));

    // Free the initialized variables.
    free(in.pointlist);
    if (region_count > 0)
        free(in.regionlist);
    free(out.triangleattributelist);
    return tr;
}



void vky_destroy_pslg_triangulation(VkyPSLGTriangulation* tr)
{
    // Allocated by Triangle.
    // free(tr->triangle->pointlist);  // NOTE: already freed by free(tr.vertices)
    free(tr->triangle->trianglelist);
    free(tr->triangle->segmentlist);

    // Allocated by vky_triangulate_pslg.
    free(tr->region_idx);
    if (tr->triangle != NULL)
        free(tr->triangle);
    if (tr->mesh_vertices != NULL)
        free(tr->mesh_vertices);
}



/*************************************************************************************************/
/*  PSLG visual bundle                                                                           */
/*************************************************************************************************/

VkyVisualBundle* vky_bundle_pslg(VkyScene* scene, VkyPSLGParams params)
{
    VkyVisualBundle* vb = vky_create_visual_bundle(scene);

    // Mesh visual.
    VkyVisual* visual_mesh = vky_visual_mesh_flat(scene);
    vky_add_visual_to_bundle(vb, visual_mesh);

    // Segment visual.
    VkyVisual* visual_segments = vky_visual_segment(scene);
    vky_add_visual_to_bundle(vb, visual_segments);

    // Copy the parameters.
    vb->params = malloc(sizeof(VkyPSLGParams));
    memcpy(vb->params, &params, sizeof(VkyPSLGParams));

    return vb;
}



VkyPSLGTriangulation vky_bundle_pslg_upload(
    VkyVisualBundle* vb,                                     //
    const uint32_t point_count, const dvec2* points,         // points
    const uint32_t segment_count, const uvec2* segments,     // segments
    const uint32_t region_count, const dvec2* region_coords, // regions
    const VkyColorBytes* region_colors,                      // region  colors
    const char* triangle_params)                             // triangle params
{
    VkyVisual* visual_mesh = vb->visuals[0];
    VkyVisual* visual_segments = vb->visuals[1];
    VkyPSLGParams* params = (VkyPSLGParams*)vb->params;
    ASSERT(params != NULL);


    // PSLG triangulation.
    VkyPSLGTriangulation tr = vky_triangulate_pslg(
        point_count, points, segment_count, (const uvec2*)segments, region_count, region_coords,
        triangle_params);


    // Mesh visual.
    VkyVertex* vertices = (VkyVertex*)calloc(tr.vertex_count, sizeof(VkyVertex));
    for (uint32_t i = 0; i < tr.vertex_count; i++)
    {
        vertices[i].pos[0] = tr.vertices[i][0];
        vertices[i].pos[1] = tr.vertices[i][1];
        vertices[i].pos[2] = 0; // TODO: float z

        vertices[i].color = region_colors[(uint32_t)round(tr.region_idx[i])];
    }
    vky_visual_upload(
        visual_mesh, (VkyData){0, NULL, tr.vertex_count, vertices, tr.index_count, tr.indices});
    // free(vertices); // NOTE: the caller must free the vertices
    tr.mesh_vertices = vertices;


    // Segments visual.
    VkySegmentVertex* seg_vertices =
        (VkySegmentVertex*)calloc(segment_count, sizeof(VkySegmentVertex));

    uint32_t i0, i1;
    for (uint32_t i = 0; i < segment_count; i++)
    {
        i0 = segments[i][0];
        i1 = segments[i][1];

        seg_vertices[i].P0[0] = points[i0][0];
        seg_vertices[i].P0[1] = points[i0][1];
        seg_vertices[i].P1[0] = points[i1][0];
        seg_vertices[i].P1[1] = points[i1][1];
        seg_vertices[i].linewidth = params->linewidth;
        seg_vertices[i].color = params->edge_color;
        seg_vertices[i].cap0 = VKY_CAP_ROUND;
        seg_vertices[i].cap1 = VKY_CAP_ROUND;
    }

    vky_visual_upload(visual_segments, (VkyData){segment_count, seg_vertices});
    free(seg_vertices);

    return tr;
}



/*************************************************************************************************/
/*  Triangulation visual bundle with triangle segments and markers                               */
/*    Can be used with any raw mesh VkyVertex-based visual                                       */
/*************************************************************************************************/

VkyVisualBundle* vky_bundle_triangulation(VkyScene* scene, VkyTriangulationParams params)
{
    VkyVisualBundle* vb = vky_create_visual_bundle(scene);

    // Segment visual.
    VkyVisual* visual_segments = vky_visual_segment(scene);
    vky_add_visual_to_bundle(vb, visual_segments);

    // Marker visual.
    VkyMarkersRawParams vparams =
        (VkyMarkersRawParams){{params.marker_size[0], params.marker_size[1]}, VKY_SCALING_OFF};
    VkyVisual* visual_markers = vky_visual_marker_raw(scene, &vparams);
    vky_add_visual_to_bundle(vb, visual_markers);

    // Copy the parameters.
    vb->params = malloc(sizeof(VkyTriangulationParams));
    memcpy(vb->params, &params, sizeof(VkyTriangulationParams));

    return vb;
}



void vky_bundle_triangulation_upload(
    VkyVisualBundle* vb,                                        //
    uint32_t vertex_count, size_t stride, const void* vertices, // vertices
    uint32_t index_count, const VkyIndex* indices)              // indices
{
    VkyVisual* visual_segments = vb->visuals[0];
    VkyVisual* visual_markers = vb->visuals[1];
    VkyTriangulationParams* params = (VkyTriangulationParams*)vb->params;

    ASSERT(params != NULL);

    ASSERT(vertices != NULL);
    ASSERT(indices != NULL);
    ASSERT(vertex_count > 0);
    ASSERT(index_count > 0);

    // Segments.
    VkySegmentVertex* seg_vertices =
        (VkySegmentVertex*)calloc(index_count, sizeof(VkySegmentVertex));

    uint32_t i0, i1, i2;
    float *v0, *v1, *v2;
    uint32_t offset = 0;
    float linewidth = params->linewidth;

    // Loop over the triangles.
    for (uint32_t i = 0; i < index_count / 3; i++)
    {
        offset = 3 * i;
        i0 = indices[offset + 0];
        i1 = indices[offset + 1];
        i2 = indices[offset + 2];

        // Pointers to the first vec3 component of each of the 3 vertices making the triangle.
        v0 = (float*)((int64_t)vertices + (int64_t)stride * i0);
        v1 = (float*)((int64_t)vertices + (int64_t)stride * i1);
        v2 = (float*)((int64_t)vertices + (int64_t)stride * i2);

        // Offset in the segment vertices array.
        offset = 3 * i;

        // Segment 0->1.
        glm_vec3_copy(v0, seg_vertices[offset + 0].P0);
        glm_vec3_copy(v1, seg_vertices[offset + 0].P1);
        seg_vertices[offset + 0].linewidth = linewidth;
        seg_vertices[offset + 0].color = params->edge_color;

        // Segment 1->2.
        glm_vec3_copy(v1, seg_vertices[offset + 1].P0);
        glm_vec3_copy(v2, seg_vertices[offset + 1].P1);
        seg_vertices[offset + 1].linewidth = linewidth;
        seg_vertices[offset + 1].color = params->edge_color;

        // Segment 2->0.
        glm_vec3_copy(v2, seg_vertices[offset + 2].P0);
        glm_vec3_copy(v0, seg_vertices[offset + 2].P1);
        seg_vertices[offset + 2].linewidth = linewidth;
        seg_vertices[offset + 2].color = params->edge_color;
    }

    // Upload the segment data.
    vky_visual_upload(visual_segments, (VkyData){index_count, seg_vertices});
    free(seg_vertices);

    // Make the marker data.
    VkyVertex* vertices_markers = (VkyVertex*)calloc(vertex_count, sizeof(VkyVertex));
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        glm_vec3_copy((float*)((int64_t)vertices + (int64_t)stride * i), vertices_markers[i].pos);
        vertices_markers[i].color = params->marker_color;
    }
    // Upload the marker data.
    vky_visual_upload(visual_markers, (VkyData){vertex_count, vertices_markers});
    free(vertices_markers);
}
