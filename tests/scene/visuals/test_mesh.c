/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing mesh                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_mesh.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/arcball.h"
#include "scene/dual.h"
#include "scene/scene.h"
#include "scene/scene_testing_utils.h"
#include "scene/transform.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/mesh.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Mesh tests                                                                                   */
/*************************************************************************************************/

int test_mesh_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_1", VISUAL_TEST_ARCBALL, 0);

    // Shape.
    DvzShape* shape = dvz_shape();
    dvz_shape_cube(
        shape, (DvzColor[]){
                   {RED},
                   {GREEN},
                   {BLUE},
                   {CYAN},
                   {PURPLE},
                   {YELLOW},
               });

    // for (uint32_t i = 0; i < shape->vertex_count; i++)
    // {
    //     shape->pos[i][0] *= 3;
    //     shape->pos[i][1] *= 3;
    //     shape->pos[i][2] *= 3;
    //     shape->color[i][3] = 128;
    // }

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_TEXTURED | DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, shape, flags);
    // dvz_visual_depth(visual, DVZ_DEPTH_TEST_DISABLE);

    // Create and upload the texture.
    if (flags & DVZ_MESH_FLAGS_TEXTURED)
    {
        uvec3 tex_shape = {0};
        DvzTexture* texture = load_crate_texture(vt.batch, tex_shape);
        dvz_mesh_texture(visual, texture);
    }


    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(shape);

    return 0;
}


int test_mesh_2(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_2", VISUAL_TEST_ARCBALL, 0);

    const float spacing = 1;
    uint32_t shape_count = 5;
    const float y_offset = 0;

    DvzShape* shapes[5] = {0};

    uint8_t alpha = 255;

    // Sphere
    shapes[0] = dvz_shape();
    dvz_shape_sphere(shapes[0], 32, 32, (DvzColor){RED});

    // Cylinder
    shapes[1] = dvz_shape();
    dvz_shape_cylinder(shapes[1], 32, (DvzColor){GREEN});

    // Cone
    shapes[2] = dvz_shape();
    dvz_shape_cone(shapes[2], 32, (DvzColor){BLUE});

    // Arrow
    shapes[3] = dvz_shape();
    dvz_shape_arrow(shapes[3], 32, 0.3f, 0.2f, 0.05f, (DvzColor){CYAN});

    // Torus
    shapes[4] = dvz_shape();
    dvz_shape_torus(shapes[4], 64, 16, 0.1f, (DvzColor){PURPLE});

    // Translate each shape to a position in the XZ plane
    for (uint32_t i = 0; i < shape_count; i++)
    {
        dvz_shape_begin(shapes[i], 0, shapes[i]->vertex_count);
        dvz_shape_scale(shapes[i], (vec3){.75, .75, .75});
        vec3 t = {spacing * ((float)i - (shape_count - 1) / 2.0f), y_offset, 0};
        dvz_shape_translate(shapes[i], t);
        dvz_shape_end(shapes[i]);
    }

    // Merge all shapes into one visual
    DvzShape* merged = dvz_shape();
    dvz_shape_merge(merged, shape_count, shapes);

    dvz_shape_unindex(merged, DVZ_CONTOUR_FULL);

    DvzVisual* visual =
        dvz_mesh_shape(vt.batch, merged, DVZ_MESH_FLAGS_LIGHTING | DVZ_MESH_FLAGS_CONTOUR);
    dvz_mesh_edgecolor(visual, (DvzColor){WHITE_ALPHA(64)});
    dvz_mesh_linewidth(visual, .1);
    // dvz_visual_depth(visual, DVZ_DEPTH_TEST_DISABLE);
    dvz_panel_visual(vt.panel, visual, 0);

    // Cleanup
    for (uint32_t i = 0; i < shape_count; i++)
        dvz_shape_destroy(shapes[i]);
    dvz_shape_destroy(merged);

    visual_test_end(vt);
    return 0;
}



int test_mesh_polygon(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_polygon", VISUAL_TEST_ORTHO, 0);

    // Polygon.
    uint32_t n = 24;
    dvec2* points = (dvec2*)calloc(n, sizeof(dvec2));
    double r = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        // r = .75 + .25 * 2 * (dvz_rand_double() - 1.0);
        r = .5 + .1 * (+1 - 2 * ((int32_t)i % 2));
        // NOTE: (float) is required otherwise -i overflows as i is unsigned...
        points[i][0] = r * cos(-(float)i * M_2PI / n);
        points[i][1] = r * sin(-(float)i * M_2PI / n);
    }
    DvzShape* shape = dvz_shape();
    dvz_shape_polygon(shape, n, (const dvec2*)points, (DvzColor){BLUE});
    FREE(points);

    // // Display the indices.
    // for (uint32_t i = 0; i < shape->index_count; i++)
    // {
    //     if (i % 3 == 0)
    //         printf("\n");
    //     printf("%d ", shape->index[i]);
    // }

    dvz_shape_unindex(shape, DVZ_INDEXING_EARCUT | DVZ_CONTOUR_JOINTS);

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_CONTOUR;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, shape, flags);

    // Set up the wireframe contour parameters.
    dvz_mesh_edgecolor(visual, (DvzColor){WHITE});
    dvz_mesh_linewidth(visual, 10.0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(shape);

    return 0;
}



// static float cross2D(vec2 v1, vec2 v2) { return v1[0] * v2[1] - v1[1] * v2[0]; }

// static void computeBarycentric(vec3 P0, vec3 P1, vec3 P2, vec2 u, vec2 ubary)
// {
//     // Vectors from P0 to P1 and P0 to P2
//     float v0[2] = {P1[0] - P0[0], P1[1] - P0[1]};
//     float v1[2] = {P2[0] - P0[0], P2[1] - P0[1]};
//     float v2[2] = {u[0] - P0[0], u[1] - P0[1]};

//     // Compute the area of the triangle ABC using the cross product
//     float denom = cross2D(v0, v1);

//     // Calculate barycentric coordinates
//     ubary[0] = cross2D(v2, v1) / denom; // Barycentric coordinate corresponding to P1
//     ubary[1] = cross2D(v0, v2) / denom; // Barycentric coordinate corresponding to P2
//     // ubary[0] = 1.0f - ubary[1] - ubary[2]; // Barycentric coordinate corresponding to P0
// }

#define POS(x) {x[0], x[1], x[2]}

#define COUNT (3 * 3)

#define R {ALPHA_MAX, 0, 0, ALPHA_MAX}
#define G {0, ALPHA_MAX, 0, ALPHA_MAX}
#define B {0, 0, ALPHA_MAX, ALPHA_MAX}

static inline float dot_ortho(vec3 p, vec3 q, vec3 a, vec3 b)
{
    vec2 u = {0};
    u[0] = b[0] - a[0];
    u[1] = b[1] - a[1];
    glm_vec2_normalize(u);
    return -(q[0] - p[0]) * u[1] + (q[1] - p[1]) * u[0];
}

static inline void dist_opposite(vec3 p0, vec3 p1, vec3 p2, vec3* d_opposite)
{
    d_opposite[0][0] = dot_ortho(p1, p0, p1, p2);
    d_opposite[1][1] = dot_ortho(p2, p1, p2, p0);
    d_opposite[2][2] = dot_ortho(p0, p2, p0, p1);
}

static inline void _update_angle(DvzVisual* visual, vec2 angle)
{
    float a = angle[0];
    float b = angle[1];
    vec3 P0 = {0, +.5, 0};
    vec3 P1 = {-.75, -1, 0};
    vec3 P2 = {+.75, -1, 0};
    vec3 Q0 = {-.85, a, 0};
    vec3 R0 = {+.85, b, 0};

    vec3 position[] = {
        POS(Q0), POS(P1), POS(P0), //
        POS(P0), POS(P1), POS(P2), //
        POS(P0), POS(P2), POS(R0), //
    };
    dvz_mesh_position(visual, 0, COUNT, position, 0);

    // Direction vectors.
    vec2 u, v;
    u[0] = Q0[0] - P0[0];
    u[1] = Q0[1] - P0[1];
    glm_vec2_normalize(u);
    v[0] = R0[0] - P0[0];
    v[1] = R0[1] - P0[1];
    glm_vec2_normalize(v);

    // NOTE: distance from P to Au is dot(AP, u_ortho)
    // d_left[i][j] is the distance from Pi to left edge adjacent to Pj
    vec3 d_left[9] = {

        // Q0-P1-P0
        {0, dot_ortho(P1, Q0, P1, P2), 0},                         // Q0
        {0, 0, dot_ortho(P0, P1, P0, Q0)},                         // P1
        {dot_ortho(Q0, P0, Q0, P1), dot_ortho(P1, P0, P1, P2), 0}, // P0

        // P0-P1-P2
        {0, dot_ortho(P1, P0, P1, P2), dot_ortho(P2, P0, P2, R0)}, // P0
        {dot_ortho(P0, P1, P0, Q0), 0, dot_ortho(P2, P1, P2, R0)}, // P1
        {dot_ortho(P0, P2, P0, Q0), 0, 0},                         // P2

        // P0-P2-R0
        {0, dot_ortho(P2, P0, P2, R0), 0},                         // P0
        {dot_ortho(P0, P2, P0, Q0), 0, dot_ortho(R0, P2, R0, P0)}, // P2
        {dot_ortho(P0, R0, P0, Q0), 0, 0},                         // R0

    };
    vec3 d_right[9] = {

        // Q0-P1-P0
        {0, 0, -dot_ortho(P0, Q0, P0, R0)},                          // Q0
        {-dot_ortho(Q0, P1, P0, Q0), 0, -dot_ortho(P0, P1, P0, R0)}, // P1
        {0, -dot_ortho(P1, P0, P1, Q0), 0},                          // P0

        // P0-P1-P2
        {0, -dot_ortho(P1, P0, P1, Q0), -dot_ortho(P2, P0, P2, P1)}, // P0
        {-dot_ortho(P0, P1, P0, R0), 0, 0},                          // P1
        {-dot_ortho(P0, P2, P0, R0), -dot_ortho(P1, P2, P1, Q0), 0}, // P2

        // P0-P2-R0
        {0, -dot_ortho(P2, P0, P2, P1), -dot_ortho(R0, P0, R0, P2)}, // P0
        {-dot_ortho(P0, P2, P0, R0), 0, 0},                          // P2
        {0, -dot_ortho(P2, R0, P2, P1), 0},                          // R0

    };

    dvz_mesh_left(visual, 0, 9, (void*)d_left, 0);
    dvz_mesh_right(visual, 0, 9, (void*)d_right, 0);

    // NOTE: orientation
    cvec4 contour[] = {
        {0, 2, 2, 0}, // Q0
        {0, 2, 2, 0}, // P1
        {0, 2, 2, 0}, // P0

        // NOTE: will be overriden by the GUI
        {2, 2, 2, 0}, // P0
        {2, 2, 2, 0}, // P1
        {2, 2, 2, 0}, // P2

        {2, 2, 0, 0}, // P0
        {2, 2, 0, 0}, // P2
        {2, 2, 0, 0}, // R0
    };
    if (glm_vec2_cross(u, v) < 0)
    {
        contour[0][2] |= 4;
        contour[1][2] |= 4;
        contour[2][2] |= 4;

        contour[3][0] |= 4;
        contour[4][0] |= 4;
        contour[5][0] |= 4;

        contour[6][0] |= 4;
        contour[7][0] |= 4;
        contour[8][0] |= 4;
    }
    dvz_mesh_contour(visual, 0, 9, (void*)contour, 0);

    vec3 normal[] = {
        {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
        {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    };
    dvz_mesh_normal(visual, 0, 9, (void*)normal, 0);
}

static inline void _edgecolor_callback(DvzApp* app, DvzId canvas_id, DvzGuiEvent* ev)
{
    VisualTest* vt = ev->user_data;
    ANN(vt);

    float* angle = (float*)vt->user_data;
    ANN(angle);

    dvz_gui_pos((vec2){0, 0}, (vec2){0, 0});
    dvz_gui_size((vec2){200, 0});
    dvz_gui_begin("Contour", DVZ_DIALOG_FLAGS_OVERLAY);
    bool u_changed = dvz_gui_slider("u", -1 + .01, +5.0, &angle[0]);
    bool v_changed = dvz_gui_slider("v", -1 + .01, +5.0, &angle[1]);
    dvz_gui_end();

    if (u_changed || v_changed)
    {
        _update_angle(vt->visual, (vec2){angle[0], angle[1]});
    }
}

int test_mesh_edgecolor(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_edgecolor", VISUAL_TEST_ORTHO, DVZ_CANVAS_FLAGS_IMGUI);

    // Create the visual.
    DvzVisual* visual = dvz_mesh(vt.batch, DVZ_MESH_FLAGS_CONTOUR);
    dvz_mesh_alloc(visual, COUNT, 0);

    // Mesh color.
    DvzColor color[] = {{BLUE}, {BLUE}, {BLUE}, {RED}, {GREEN}, {BLUE}, {RED}, {RED}, {RED}};
    dvz_mesh_color(visual, 0, COUNT, color, 0);

    // Stroke.
    dvz_mesh_edgecolor(visual, (DvzColor){WHITE});
    dvz_mesh_linewidth(visual, 50);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Angle GUI.
    vt.visual = visual;
    float angle[2] = {0.75, 0.75};
    _update_angle(visual, angle);
    vt.user_data = &angle[0];
    dvz_app_gui(vt.app, vt.figure->canvas_id, _edgecolor_callback, &vt);

    // Run the test.
    visual_test_end(vt);

    return 0;
}



static inline float dot_ortho_u(vec3 p, vec3 q, vec2 u)
{
    return -(q[0] - p[0]) * u[1] + (q[1] - p[1]) * u[0];
}

int test_mesh_contour(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_contour", VISUAL_TEST_ORTHO, 0);

    // Create the visual.
    DvzVisual* visual = dvz_mesh(vt.batch, DVZ_MESH_FLAGS_CONTOUR);
    dvz_mesh_alloc(visual, 3, 0);

    float r = 1.5;
    float w = .707;
    float c = 1;

    vec3 P0 = {r * w, -c * r * w, 0};
    vec3 P1 = {0, c * r, 0};
    vec3 P2 = {-r * w, -c * r * w, 0};

    vec3 position[] = {
        POS(P0), POS(P1), POS(P2), //
    };
    dvz_mesh_position(visual, 0, 3, position, 0);

    // Direction vectors.
    vec2 u = {-w, -w}, v = {+w, -w};

    // d_left[i][j] is the distance from Pi to left edge adjacent to Pj
    vec3 d_left[] = {
        {0, 0, dot_ortho_u(P2, P0, u)},    // P0
        {0, 0, dot_ortho_u(P2, P1, u)},    // P1
        {dot_ortho(P0, P2, P0, P1), 0, 0}, // P2
    };
    vec3 d_right[] = {
        {0, 0, -dot_ortho(P2, P0, P2, P1)}, // P0
        {-dot_ortho_u(P0, P1, v), 0, 0},    // P1
        {-dot_ortho_u(P0, P2, v), 0, 0},    // P2
    };

    dvz_mesh_left(visual, 0, 3, (void*)d_left, 0);
    dvz_mesh_right(visual, 0, 3, (void*)d_right, 0);

    // NOTE: orientation
    cvec4 contour[] = {
        {2 | 4, 1, 2 | 4, 0}, // P0
        {2 | 4, 1, 2 | 4, 0}, // P1
        {2 | 4, 1, 2 | 4, 0}, // P2
    };
    dvz_mesh_contour(visual, 0, 3, (void*)contour, 0);

    // Mesh color.
    DvzColor color[] = {{RED}, {GREEN}, {BLUE}};
    dvz_mesh_color(visual, 0, 3, color, 0);

    // Stroke.
    dvz_mesh_edgecolor(visual, (DvzColor){WHITE});
    dvz_mesh_linewidth(visual, 20);

    // Normal.
    vec3 normal[] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    dvz_mesh_normal(visual, 0, 3, (void*)normal, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    return 0;
}



int test_mesh_surface(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_surface", VISUAL_TEST_ARCBALL, 0);

    // Grid size.
    uint32_t row_count = 150;
    uint32_t col_count = row_count;

    // Grid parameters.
    vec3 o = {-1, 0, -1};                   // upper left corner of the matrix
    vec3 u = {0, 0, 2.0 / (col_count - 1)}; // along the i axis (vertical)
    vec3 v = {2.0 / (row_count - 1), 0, 0}; // along the j axis (horizontal)

    // Allocate heights and colors arrays.
    float* heights = (float*)calloc(row_count * col_count, sizeof(float));
    DvzColor* colors = (DvzColor*)calloc(row_count * col_count, sizeof(DvzColor));

    // Set heights and colors.
    uint32_t idx = 0;
    float a = 4 * M_2PI / row_count, b = 3 * M_2PI / col_count, c = .5, d = 0, h = 0;
    float hmin = -.5;
    float hmax = +.5;
    for (uint32_t i = 0; i < row_count; i++)
    {
        for (uint32_t j = 0; j < col_count; j++)
        {
            // Vertex height.
            d = pow((i - row_count / 2.0) / row_count, 2) + //
                pow((j - col_count / 2.0) / col_count, 2);
            d = exp(-10.0 * d);
            h = c * d * sin(a * i) * cos(b * j);
            heights[idx] = h;

            // Vertex color.
            dvz_colormap_scale(DVZ_CMAP_PLASMA, -h, -hmax, -hmin, colors[idx]);
            // colors[idx][3] = 128;

            idx++;
        }
    }

    // Create the surface shape->
    DvzShape* shape = dvz_shape();
    dvz_shape_surface(shape, row_count, col_count, heights, colors, o, u, v, 0);
    dvz_shape_unindex(shape, DVZ_INDEXING_SURFACE | DVZ_CONTOUR_FULL);

    // NOTE: we need to use non-indexed meshes for mesh wireframe.
    // Create the visual.
    int flags = DVZ_MESH_FLAGS_LIGHTING; // | DVZ_MESH_FLAGS_CONTOUR;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, shape, flags);

    // Wireframe.
    // dvz_mesh_edgecolor(visual, (cvec4){100, 100, 100, 255});
    // dvz_mesh_linewidth(visual, .5);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    dvz_arcball_initial(vt.arcball, (vec3){0.42339, -0.39686, -0.00554});
    dvz_panel_update(vt.panel);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(heights);
    FREE(colors);
    dvz_shape_destroy(shape);

    return 0;
}



static inline void _gui_callback(DvzApp* app, DvzId canvas_id, DvzGuiEvent* ev)
{
    VisualTest* vt = ev->user_data;
    ANN(vt);

    vec4* color = (vec4*)vt->user_data;

    dvz_gui_pos((vec2){0, 0}, (vec2){0, 0});
    dvz_gui_size((vec2){200, 300});
    dvz_gui_begin("Wireframe", DVZ_DIALOG_FLAGS_OVERLAY);
    bool width_changed = dvz_gui_slider("Width", 0, 10.0, &color[0][3]);
    bool color_changed = dvz_gui_colorpicker("Color", (float*)*color, 0);
    dvz_gui_end();

    if (color_changed)
        dvz_mesh_edgecolor(vt->visual, (DvzColor){COLOR_F2D(color[0])});
    if (width_changed)
        dvz_mesh_linewidth(vt->visual, color[0][3]);
}

int test_mesh_obj(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("mesh_obj", VISUAL_TEST_ARCBALL, 0);

    // Load obj shape->
    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);
    DvzShape* shape = dvz_shape();
    dvz_shape_obj(shape, path);
    if (!shape->vertex_count)
    {
        dvz_shape_destroy(shape);
        return 0;
    }

    // Set the color of every vertex (the shape comes with an already allocated color array).
    // for (uint32_t i = 0; i < shape->vertex_count; i++)
    // {
    //     // Generate colors using the "bwr" colormap, in reverse (blue -> red).
    //     dvz_colormap_scale(
    //         DVZ_CMAP_COOLWARM, shape->vertex_count - 1 - i, 0, shape->vertex_count,
    //         shape->color[i]);
    //     shape->color[i][3] = 64;
    // }

    bool isoline = false;

    // Set up isoline values in the shape->
    if (isoline)
    {
        shape->isoline = (float*)calloc(shape->vertex_count, sizeof(float));
        for (uint32_t i = 0; i < shape->vertex_count; i++)
        {
            shape->isoline[i] =
                .5 * (1 + shape->pos[i][1]) + .1 * sin(1 * M_2PI * shape->pos[i][0]);
        }
    }

    // NOTE: we need to use non-indexed meshes for mesh wireframe.
    dvz_shape_unindex(shape, DVZ_INDEXING_EARCUT | DVZ_CONTOUR_FULL);

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_LIGHTING;
    if (isoline)
        flags |= DVZ_MESH_FLAGS_ISOLINE;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, shape, flags);
    // dvz_visual_depth(visual, DVZ_DEPTH_TEST_DISABLE);
    // dvz_visual_cull(visual, DVZ_CULL_MODE_BACK);

    // Lighting.
    {
        // Two lights.
        // dvz_mesh_light_pos(visual, 0, (vec3){+1, -0.25, -.5, 0.0});
        // dvz_mesh_light_params(visual, 1, (vec4){0.1, .5, .5, .9});
        // dvz_mesh_light_params(visual, 0, (vec4){.75, .25, .25, 16});

        dvz_mesh_light_pos(visual, 1, (vec4){-1, -0.25, -.5, 0.0});
        dvz_mesh_material_params(visual, 1, (vec4){0.1, .5, .5, .7});

        dvz_mesh_light_color(visual, 0, (DvzColor){RED});
        dvz_mesh_light_color(visual, 1, (DvzColor){GREEN});
    }

    vec4 color = {.25, .25, .25, .5f};
    // dvz_mesh_edgecolor(visual, (cvec4){100, 100, 100, 255});
    dvz_mesh_linewidth(visual, 1);
    dvz_mesh_density(visual, 10);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    dvz_arcball_initial(vt.arcball, (vec3){-2.7, -.7, -.1});
    dvz_panel_update(vt.panel);

    vt.visual = visual;
    vt.user_data = &color[0];
    // dvz_app_gui(vt.app, vt.figure->canvas_id, _gui_callback, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(shape);

    return 0;
}



static inline dvec2* copy_polygon(uint32_t length, const dvec2* pos)
{
    ANN(pos);
    dvec2* copied = (dvec2*)calloc(length, sizeof(dvec2));
    uint32_t j = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        j = i;
        // j = length - 1 - i;
        copied[i][0] = pos[j][0];
        copied[i][1] = pos[j][1];
    }
    return copied;
}

int test_mesh_geo(TstSuite* suite, TstItem* tstitem)
{
    DvzColor color = {RED};

    // Load positions.
    char pos_path[1024] = {0};
    snprintf(pos_path, sizeof(pos_path), "%s/misc/poly-pos.bin", DATA_DIR);

    DvzSize pos_size = 0;
    uint64_t* pos_bytes = (uint64_t*)dvz_read_file(pos_path, &pos_size);
    log_info("loaded %s (%s)", pos_path, pretty_size(pos_size));


    // Load lengths.
    char length_path[1024] = {0};
    snprintf(length_path, sizeof(length_path), "%s/misc/poly-length.bin", DATA_DIR);

    DvzSize length_size = 0;
    uint32_t* length_bytes = (uint32_t*)dvz_read_file(length_path, &length_size);
    log_info("loaded %s (%s)", length_path, pretty_size(length_size));

    ASSERT(length_size % sizeof(uint32_t) == 0);
    uint32_t poly_count = length_size / sizeof(uint32_t);

    // DEBUG
    // poly_count = 1;

    log_info("loaded %d polygons", poly_count);

    uint32_t* poly_lengths = (uint32_t*)length_bytes;
    const dvec2* poly_pos = (const dvec2*)pos_bytes;

    // Triangulate and merge the polygons into a single shape.
    uint32_t vertex_offset = 0, poly_length = 0;
    DvzShape** shapes = (DvzShape**)calloc(poly_count, sizeof(DvzShape*));
    for (uint32_t i = 0; i < poly_count; i++)
    {
        poly_length = poly_lengths[i];

        // DEBUG
        // poly_length = 10;

        log_debug("polygon #%d length is %d", i, poly_length);

        // Color
        dvz_colormap_scale(DVZ_CMAP_VIRIDIS, i, 0, poly_count - 1, color);

        // Polygon triangulation.
        dvec2* polygon = copy_polygon(poly_length, &poly_pos[vertex_offset]);
        shapes[i] = dvz_shape();
        dvz_shape_polygon(shapes[i], poly_length, (const dvec2*)polygon, color);
        dvz_shape_unindex(shapes[i], DVZ_INDEXING_EARCUT | DVZ_CONTOUR_JOINTS);
        FREE(polygon);

        vertex_offset += poly_length;
    }
    DvzShape* shape = dvz_shape();
    dvz_shape_merge(shape, poly_count, shapes);
    for (uint32_t i = 0; i < poly_count; i++)
    {
        dvz_shape_destroy(shapes[i]);
    }
    FREE(shapes);


    VisualTest vt = visual_test_start("mesh_geo", VISUAL_TEST_ORTHO, 0);

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_CONTOUR;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, shape, flags);

    // Set up the wireframe contour parameters.
    dvz_mesh_linewidth(visual, 1);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(shape);
    FREE(pos_bytes);
    FREE(length_bytes);

    return 0;
}
