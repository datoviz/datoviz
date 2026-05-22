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



int test_geometry_cube(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometryCubeDesc desc = {.center = {1.0, 2.0, 3.0}, .size = 2.0};
    desc.color[0] = 10;
    desc.color[1] = 20;
    desc.color[2] = 30;
    desc.color[3] = 255;

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
    AT(cube->colors[0][0] == 10);
    AT(cube->colors[0][1] == 20);
    AT(cube->colors[0][2] == 30);
    AT(cube->colors[0][3] == 255);
    AC(cube->normals[0][0], 1.0, EPS);

    dvz_geometry_destroy(cube);
    return 0;
}



int test_geometry_plane(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometryPlaneDesc desc = {.center = {2.0, 3.0, 4.0}, .width = 4.0, .height = 2.0};
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
        AT(plane->colors[i][3] == 255);
    }

    dvz_geometry_destroy(plane);
    return 0;
}



int test_geometry_f32(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzGeometry* cube = dvz_geom_cube(NULL);
    AT(cube != NULL);

    vec3 positions[24] = {0};
    vec3 normals[24] = {0};
    AT(dvz_geometry_positions_f32(cube, positions, 24) == 0);
    AT(dvz_geometry_normals_f32(cube, normals, 24) == 0);
    AT(dvz_geometry_positions_f32(cube, positions, 23) == -1);
    AC(positions[0][0], 0.5f, EPS);
    AC(normals[0][0], 1.0f, EPS);

    dvz_geometry_destroy(cube);
    return 0;
}



int test_geometry_surface_grid(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    double heights[9] = {
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 0.0,
    };
    DvzColor colors[9] = {0};
    for (uint32_t i = 0; i < 9; i++)
    {
        colors[i][0] = (uint8_t)i;
        colors[i][1] = 20;
        colors[i][2] = 30;
        colors[i][3] = 255;
    }

    DvzGeometrySurfaceGridDesc desc = {
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
    AT(grid->colors[8][0] == 8);
    AT(grid->colors[8][3] == 255);

    AT(grid->indices[0] == 0);
    AT(grid->indices[1] == 1);
    AT(grid->indices[2] == 4);
    for (uint32_t i = 0; i < grid->index_count; i++)
        AT(grid->indices[i] < grid->vertex_count);

    DvzGeometrySurfaceGridDesc invalid = {.rows = 1, .cols = 3};
    AT(dvz_geom_surface_grid(&invalid) == NULL);

    dvz_geometry_destroy(grid);
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

    DvzGeometryPlaneDesc desc0 = {.center = {0.0, 0.0, 0.0}, .width = 1.0, .height = 1.0};
    DvzGeometryPlaneDesc desc1 = {.center = {2.0, 0.0, 0.0}, .width = 1.0, .height = 1.0};
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
    TST_CASE(test_geometry_cube);
    TST_CASE(test_geometry_plane);
    TST_CASE(test_geometry_f32);
    TST_CASE(test_geometry_surface_grid);
    TST_CASE(test_geometry_transform);
    TST_CASE(test_geometry_merge);

    return 0;
}
