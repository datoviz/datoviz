/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing geometry                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "datoviz/geom.h"
#include "test_geom.h"
#include "testing.h"

#include <math.h>



/*************************************************************************************************/
/*  Geometry tests                                                                               */
/*************************************************************************************************/

int test_geometry_alloc(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometry* geometry = dvz_geometry(3, 3);
    AT(geometry != NULL);
    AT(geometry->vertex_count == 3);
    AT(geometry->index_count == 3);
    AT(geometry->positions != NULL);
    AT(geometry->normals != NULL);
    AT(geometry->colors != NULL);
    AT(geometry->texcoords != NULL);
    AT(geometry->indices != NULL);

    dvz_geometry_reset(geometry);
    AT(geometry->vertex_count == 0);
    AT(geometry->positions == NULL);
    AT(geometry->indices == NULL);

    dvz_geometry_destroy(geometry);
    return 0;
}



int test_geometry_descriptor_abi(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    (void)tstitem;

    DvzGeometryCubeDesc cube = dvz_geometry_cube_desc();
    cube.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_geom_cube(&cube) == NULL);

    DvzGeometryPlaneDesc plane = dvz_geometry_plane_desc();
    plane.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_geom_plane(&plane) == NULL);

    DvzGeometrySphereDesc sphere = dvz_geometry_sphere_desc();
    sphere.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_geom_sphere(&sphere) == NULL);

    DvzGeometrySurfaceGridDesc grid = dvz_geometry_surface_grid_desc();
    grid.rows = 2;
    grid.cols = 2;
    grid.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_geom_surface_grid(&grid) == NULL);

    const dvec2 xy[3] = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}};
    DvzPolygonDesc polygon = dvz_polygon_desc();
    polygon.outer.xy = xy;
    polygon.outer.count = 3;
    polygon.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_triangulate_polygon(&polygon, NULL) == NULL);

    polygon = dvz_polygon_desc();
    polygon.outer.xy = xy;
    polygon.outer.count = 3;
    DvzTriangulationDesc triangulation = dvz_triangulation_desc();
    triangulation.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_triangulate_polygon(&polygon, &triangulation) == NULL);

    return 0;
}



int test_geometry_cube(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometryCubeDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc), .center = {1.0, 2.0, 3.0}, .size = 2.0};
    desc.color = dvz_color_rgba(10, 20, 30, 255);

    DvzGeometry* cube = dvz_geom_cube(&desc);
    AT(cube != NULL);
    AT(cube->type == DVZ_GEOMETRY_CUBE);
    AT(cube->vertex_count == 24);
    AT(cube->index_count == 36);

    DvzGeometryBounds bounds = dvz_geometry_bounds(cube);
    AC(bounds.xmin, 0.0, EPS);
    AC(bounds.xmax, 2.0, EPS);
    AC(bounds.ymin, 1.0, EPS);
    AC(bounds.ymax, 3.0, EPS);
    AC(bounds.zmin, 2.0, EPS);
    AC(bounds.zmax, 4.0, EPS);

    for (uint32_t i = 0; i < cube->index_count; i++)
        AT(cube->indices[i] < cube->vertex_count);
    AT(cube->colors[0].r == 10);
    AT(cube->colors[0].g == 20);
    AT(cube->colors[0].b == 30);
    AT(cube->colors[0].a == 255);
    AC(cube->normals[0][0], 1.0, EPS);

    dvz_geometry_destroy(cube);

    DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        {239, 83, 80, 255},  {66, 165, 245, 255}, {102, 187, 106, 255},
        {255, 202, 40, 255}, {171, 71, 188, 255}, {255, 112, 67, 255},
    };
    DvzGeometry* colored_cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = 1.0,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    AT(colored_cube != NULL);
    for (uint32_t face = 0; face < DVZ_GEOM_CUBE_FACE_COUNT; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            AT(colored_cube->colors[vertex].r == face_colors[face].r);
            AT(colored_cube->colors[vertex].g == face_colors[face].g);
            AT(colored_cube->colors[vertex].b == face_colors[face].b);
            AT(colored_cube->colors[vertex].a == face_colors[face].a);
        }
    }
    dvz_geometry_destroy(colored_cube);

    AT(dvz_geom_cube(&(DvzGeometryCubeDesc){
           DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
           .size = 1.0,
           .face_colors = face_colors,
           .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT - 1,
       }) == NULL);

    return 0;
}



int test_geometry_plane(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometryPlaneDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryPlaneDesc), .center = {2.0, 3.0, 4.0}, .width = 4.0,
        .height = 2.0};
    DvzGeometry* plane = dvz_geom_plane(&desc);
    AT(plane != NULL);
    AT(plane->type == DVZ_GEOMETRY_PLANE);
    AT(plane->vertex_count == 4);
    AT(plane->index_count == 6);

    DvzGeometryBounds bounds = dvz_geometry_bounds(plane);
    AC(bounds.xmin, 0.0, EPS);
    AC(bounds.xmax, 4.0, EPS);
    AC(bounds.ymin, 2.0, EPS);
    AC(bounds.ymax, 4.0, EPS);
    AC(bounds.zmin, 4.0, EPS);
    AC(bounds.zmax, 4.0, EPS);

    for (uint32_t i = 0; i < plane->vertex_count; i++)
    {
        AC(plane->normals[i][0], 0.0, EPS);
        AC(plane->normals[i][1], 0.0, EPS);
        AC(plane->normals[i][2], 1.0, EPS);
        AT(plane->colors[i].a == 255);
    }

    dvz_geometry_destroy(plane);
    return 0;
}



int test_geometry_surface_grid(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    double heights[9] = {
        0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    };
    DvzColor colors[9] = {0};
    for (uint32_t i = 0; i < 9; i++)
    {
        colors[i] = dvz_color_rgba((uint8_t)i, 20, 30, 255);
    }

    DvzGeometrySurfaceGridDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .rows = 3,
        .cols = 3,
        .heights = heights,
        .colors = colors,
    };
    DvzGeometry* grid = dvz_geom_surface_grid(&desc);
    AT(grid != NULL);
    AT(grid->type == DVZ_GEOMETRY_SURFACE_GRID);
    AT(grid->flags & DVZ_GEOMETRY_INDEXING_SURFACE_GRID);
    AT(grid->grid_rows == 3);
    AT(grid->grid_cols == 3);
    AT(grid->vertex_count == 9);
    AT(grid->index_count == 24);

    AC(grid->positions[4][0], 1.0, EPS);
    AC(grid->positions[4][1], 1.0, EPS);
    AC(grid->positions[4][2], 1.0, EPS);
    AC(grid->texcoords[8][0], 1.0, EPS);
    AC(grid->texcoords[8][1], 1.0, EPS);
    AT(grid->colors[8].r == 8);
    AT(grid->colors[8].a == 255);

    AT(grid->indices[0] == 0);
    AT(grid->indices[1] == 1);
    AT(grid->indices[2] == 4);
    for (uint32_t i = 0; i < grid->index_count; i++)
        AT(grid->indices[i] < grid->vertex_count);

    DvzGeometrySurfaceGridDesc invalid = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc), .rows = 1, .cols = 3};
    AT(dvz_geom_surface_grid(&invalid) == NULL);

    dvz_geometry_destroy(grid);
    return 0;
}



int test_geometry_surface_grid_update(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    double heights[4] = {0.0, 0.0, 0.0, 0.0};
    DvzGeometrySurfaceGridDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .rows = 2,
        .cols = 2,
        .heights = heights,
        .origin = {10.0, 20.0, 30.0},
        .col_basis = {2.0, 0.0, 0.0},
        .row_basis = {0.0, 3.0, 0.0},
        .height_axis = {0.0, 0.0, 1.0},
        .height_scale = 2.0,
    };
    DvzGeometry* grid = dvz_geom_surface_grid(&desc);
    AT(grid != NULL);
    AT(grid->grid_rows == 2);
    AT(grid->grid_cols == 2);
    AC(grid->grid_origin[0], 10.0, EPS);
    AC(grid->grid_row_basis[1], 3.0, EPS);
    AC(grid->grid_col_basis[0], 2.0, EPS);
    AC(grid->grid_height_axis[2], 1.0, EPS);
    AC(grid->grid_height_scale, 2.0, EPS);

    double updated[4] = {0.0, 1.0, 2.0, 3.0};
    AT(dvz_geom_surface_grid_update_heights(grid, updated, 4) == 0);
    AC(grid->positions[0][0], 10.0, EPS);
    AC(grid->positions[0][1], 20.0, EPS);
    AC(grid->positions[0][2], 30.0, EPS);
    AC(grid->positions[1][0], 12.0, EPS);
    AC(grid->positions[1][1], 20.0, EPS);
    AC(grid->positions[1][2], 32.0, EPS);
    AC(grid->positions[3][0], 12.0, EPS);
    AC(grid->positions[3][1], 23.0, EPS);
    AC(grid->positions[3][2], 36.0, EPS);

    AT(dvz_geom_surface_grid_update_heights(grid, updated, 3) == -1);
    DvzGeometry* plane = dvz_geom_plane(NULL);
    AT(plane != NULL);
    AT(dvz_geom_surface_grid_update_heights(plane, updated, 4) == -1);

    dvz_geometry_destroy(plane);
    dvz_geometry_destroy(grid);
    return 0;
}



int test_geometry_sphere(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometrySphereDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .center = {1.0, 2.0, 3.0},
        .radius = 2.0,
        .rings = 4,
        .sectors = 8,
    };
    DvzGeometry* sphere = dvz_geom_sphere(&desc);
    AT(sphere != NULL);
    AT(sphere->type == DVZ_GEOMETRY_SPHERE);
    AT(sphere->vertex_count == 45);
    AT(sphere->index_count == 192);

    DvzGeometryBounds bounds = dvz_geometry_bounds(sphere);
    AC(bounds.xmin, -1.0, EPS);
    AC(bounds.xmax, 3.0, EPS);
    AC(bounds.ymin, 0.0, EPS);
    AC(bounds.ymax, 4.0, EPS);
    AC(bounds.zmin, 1.0, EPS);
    AC(bounds.zmax, 5.0, EPS);

    for (uint32_t i = 0; i < sphere->index_count; i++)
        AT(sphere->indices[i] < sphere->vertex_count);
    for (uint32_t i = 0; i < sphere->vertex_count; i++)
    {
        const double norm = sqrt(
            sphere->normals[i][0] * sphere->normals[i][0] +
            sphere->normals[i][1] * sphere->normals[i][1] +
            sphere->normals[i][2] * sphere->normals[i][2]);
        AC(norm, 1.0, EPS);
    }
    AC(sphere->texcoords[0][0], 0.0, EPS);
    AC(sphere->texcoords[8][0], 1.0, EPS);
    AT(dvz_geom_sphere(&(DvzGeometrySphereDesc){
           DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc), .radius = -1.0}) == NULL);

    dvz_geometry_destroy(sphere);
    return 0;
}



int test_geometry_transform(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometry* plane = dvz_geom_plane(NULL);
    AT(plane != NULL);

    dmat4 transform = _DMAT4_IDENTITY_INIT;
    transform[3][0] = 2.0;
    transform[3][1] = -1.0;
    transform[3][2] = 3.0;

    AT(dvz_geometry_transform(plane, transform) == 0);
    DvzGeometryBounds bounds = dvz_geometry_bounds(plane);
    AC(bounds.xmin, 1.5, EPS);
    AC(bounds.xmax, 2.5, EPS);
    AC(bounds.ymin, -1.5, EPS);
    AC(bounds.ymax, -0.5, EPS);
    AC(bounds.zmin, 3.0, EPS);
    AC(bounds.zmax, 3.0, EPS);

    for (uint32_t i = 0; i < plane->vertex_count; i++)
    {
        AC(plane->normals[i][0], 0.0, EPS);
        AC(plane->normals[i][1], 0.0, EPS);
        AC(plane->normals[i][2], 1.0, EPS);
    }

    dvz_geometry_destroy(plane);
    return 0;
}



int test_geometry_merge(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometryPlaneDesc desc0 = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryPlaneDesc), .center = {0.0, 0.0, 0.0}, .width = 1.0,
        .height = 1.0};
    DvzGeometryPlaneDesc desc1 = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryPlaneDesc), .center = {2.0, 0.0, 0.0}, .width = 1.0,
        .height = 1.0};
    DvzGeometry* plane0 = dvz_geom_plane(&desc0);
    DvzGeometry* plane1 = dvz_geom_plane(&desc1);
    AT(plane0 != NULL);
    AT(plane1 != NULL);

    const DvzGeometry* parts[2] = {plane0, plane1};
    DvzGeometry* merged = dvz_geometry_merge(2, parts);
    AT(merged != NULL);
    AT(merged->type == DVZ_GEOMETRY_CUSTOM);
    AT(merged->vertex_count == 8);
    AT(merged->index_count == 12);
    AT(merged->indices[0] == 0);
    AT(merged->indices[5] == 3);
    AT(merged->indices[6] == 4);
    AT(merged->indices[11] == 7);

    DvzGeometryBounds bounds = dvz_geometry_bounds(merged);
    AC(bounds.xmin, -0.5, EPS);
    AC(bounds.xmax, 2.5, EPS);
    AC(bounds.ymin, -0.5, EPS);
    AC(bounds.ymax, 0.5, EPS);

    dvz_geometry_destroy(merged);
    dvz_geometry_destroy(plane0);
    dvz_geometry_destroy(plane1);
    return 0;
}



int test_geometry_edges(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometry* plane = dvz_geom_plane(NULL);
    AT(plane != NULL);

    DvzGeometryEdges* edges = dvz_geometry_edges(plane);
    AT(edges != NULL);
    AT(edges->edge_count == 5);

    uint32_t boundary_count = 0;
    uint32_t interior_count = 0;
    bool found_diagonal = false;
    for (uint32_t i = 0; i < edges->edge_count; i++)
    {
        const DvzGeometryEdge* edge = &edges->edges[i];
        AT(edge->v0 < plane->vertex_count);
        AT(edge->v1 < plane->vertex_count);
        AT(edge->v0 <= edge->v1);
        if (edge->flags & DVZ_GEOMETRY_EDGE_BOUNDARY)
            boundary_count++;
        if (edge->adjacent_count == 2)
            interior_count++;
        if (edge->v0 == 0 && edge->v1 == 2)
            found_diagonal = true;
    }
    AT(boundary_count == 4);
    AT(interior_count == 1);
    AT(found_diagonal);

    dvz_geometry_edges_destroy(edges);
    dvz_geometry_destroy(plane);
    return 0;
}



int test_geometry_contours(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometry* plane = dvz_geom_plane(NULL);
    AT(plane != NULL);

    double values[4] = {0};
    for (uint32_t i = 0; i < plane->vertex_count; i++)
        values[i] = plane->positions[i][0];
    double levels[1] = {0.0};

    DvzGeometryContours* contours =
        dvz_geometry_contours(plane, values, plane->vertex_count, levels, 1);
    AT(contours != NULL);
    AT(contours->segment_count == 2);

    for (uint32_t i = 0; i < contours->segment_count; i++)
    {
        const DvzGeometryContourSegment* segment = &contours->segments[i];
        AC(segment->p0[0], 0.0, EPS);
        AC(segment->p1[0], 0.0, EPS);
        AC(segment->level, 0.0, EPS);
        AT(segment->level_index == 0);
        AT(segment->face_index < 2);
    }

    AT(dvz_geometry_contours(plane, values, plane->vertex_count - 1, levels, 1) == NULL);

    dvz_geometry_contours_destroy(contours);
    dvz_geometry_destroy(plane);
    return 0;
}



int test_geometry_polygon_triangulation(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const dvec2 triangle_xy[3] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 1.0},
    };
    DvzGeometry* triangle = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = triangle_xy, .count = 3}},
        NULL);
    AT(triangle != NULL);
    AT(triangle->type == DVZ_GEOMETRY_CUSTOM);
    AT(triangle->flags & DVZ_GEOMETRY_INDEXING_TRIANGLES);
    AT(triangle->flags & DVZ_GEOMETRY_INDEXING_TRIANGULATION);
    AT(triangle->vertex_count == 3);
    AT(triangle->index_count == 3);
    for (uint32_t i = 0; i < triangle->index_count; i++)
        AT(triangle->indices[i] < triangle->vertex_count);
    for (uint32_t i = 0; i < triangle->vertex_count; i++)
    {
        AC(triangle->positions[i][2], 0.0, EPS);
        AC(triangle->normals[i][0], 0.0, EPS);
        AC(triangle->normals[i][1], 0.0, EPS);
        AC(triangle->normals[i][2], 1.0, EPS);
        AC(triangle->texcoords[i][0], 0.0, EPS);
        AC(triangle->texcoords[i][1], 0.0, EPS);
        AT(triangle->colors[i].r == 255);
        AT(triangle->colors[i].g == 255);
        AT(triangle->colors[i].b == 255);
        AT(triangle->colors[i].a == 255);
    }
    dvz_geometry_destroy(triangle);

    const dvec2 square_xy[4] = {
        {0.0, 0.0},
        {2.0, 0.0},
        {2.0, 2.0},
        {0.0, 2.0},
    };
    DvzGeometry* square = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = square_xy, .count = 4}},
        NULL);
    AT(square != NULL);
    AT(square->vertex_count == 4);
    AT(square->index_count == 6);
    for (uint32_t i = 0; i < square->index_count; i++)
        AT(square->indices[i] < square->vertex_count);
    dvz_geometry_destroy(square);

    const dvec2 closed_xy[5] = {
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}, {0.0, 0.0},
    };
    DvzGeometry* closed = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = closed_xy, .count = 5}},
        NULL);
    AT(closed != NULL);
    AT(closed->vertex_count == 4);
    AT(closed->index_count == 6);
    dvz_geometry_destroy(closed);

    const dvec2 hole_xy[4] = {
        {0.5, 0.5},
        {1.5, 0.5},
        {1.5, 1.5},
        {0.5, 1.5},
    };
    const DvzPolygonRing holes[1] = {{.xy = hole_xy, .count = 4}};
    DvzGeometry* holed = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = square_xy, .count = 4},
            .holes = holes,
            .hole_count = 1,
        },
        &(DvzTriangulationDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzTriangulationDesc),
            .backend = DVZ_TRIANGULATION_BACKEND_EARCUT});
    AT(holed != NULL);
    AT(holed->vertex_count == 8);
    AT(holed->index_count > 0);
    AT(holed->index_count % 3 == 0);
    for (uint32_t i = 0; i < holed->index_count; i++)
        AT(holed->indices[i] < holed->vertex_count);
    dvz_geometry_destroy(holed);

    return 0;
}



int test_geometry_polygon_triangulation_invalid(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const dvec2 two_xy[2] = {
        {0.0, 0.0},
        {1.0, 0.0},
    };
    AT(dvz_triangulate_polygon(
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = two_xy, .count = 2}},
           NULL) == NULL);

    const dvec2 square_xy[4] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},
    };
    AT(dvz_triangulate_polygon(
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = square_xy, .count = 4},
               .hole_count = 1},
           NULL) == NULL);

    const DvzPolygonRing short_hole[1] = {{.xy = two_xy, .count = 2}};
    AT(dvz_triangulate_polygon(
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
               .outer = {.xy = square_xy, .count = 4},
               .holes = short_hole,
               .hole_count = 1,
           },
           NULL) == NULL);

    const dvec2 nan_xy[3] = {
        {0.0, 0.0},
        {NAN, 0.0},
        {0.0, 1.0},
    };
    AT(dvz_triangulate_polygon(
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = nan_xy, .count = 3}},
           NULL) == NULL);

    const dvec2 zero_area_xy[3] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {2.0, 0.0},
    };
    AT(dvz_triangulate_polygon(
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = zero_area_xy, .count = 3}},
           NULL) == NULL);

    AT(dvz_triangulate_polygon(
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc), .outer = {.xy = square_xy, .count = 4}},
           &(DvzTriangulationDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzTriangulationDesc),
               .backend = (DvzTriangulationBackend)UINT32_MAX}) == NULL);

    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_geom(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "geom";

    TST_MODULE(suite, "geom");
    TST_GROUP("geometry");
    TST_CASE(test_geometry_alloc);
    TST_CASE(test_geometry_descriptor_abi);
    TST_CASE(test_geometry_cube);
    TST_CASE(test_geometry_plane);
    TST_CASE(test_geometry_surface_grid);
    TST_CASE(test_geometry_surface_grid_update);
    TST_CASE(test_geometry_sphere);
    TST_CASE(test_geometry_transform);
    TST_CASE(test_geometry_merge);
    TST_CASE(test_geometry_edges);
    TST_CASE(test_geometry_contours);
    TST_CASE(test_geometry_polygon_triangulation);
    TST_CASE(test_geometry_polygon_triangulation_invalid);

    return 0;
}
