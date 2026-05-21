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

    DvzBox bounds = dvz_geometry_bounds(cube);
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

    DvzBox bounds = dvz_geometry_bounds(plane);
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

    return 0;
}
