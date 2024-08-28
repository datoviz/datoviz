/*************************************************************************************************/
/*  Testing mesh                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_mesh.h"
#include "datoviz.h"
#include "renderer.h"
#include "request.h"
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

int test_mesh_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh", VISUAL_TEST_ARCBALL, 0);

    // Shape.
    DvzShape shape = dvz_shape_cube((cvec4[]){
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {0, 255, 255, 255},
        {255, 0, 255, 255},
        {255, 255, 0, 255},
    });

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_TEXTURED | DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);


    // Lighting.
    if (flags & DVZ_MESH_FLAGS_LIGHTING)
    {
        dvz_mesh_light_pos(visual, (vec3){-1, +1, +10});

        // Light parameters: ambient, diffuse, specular, exponent.
        dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16});
    }

    // Create and upload the texture.
    if (flags & DVZ_MESH_FLAGS_TEXTURED)
    {
        uvec3 tex_shape = {0};
        DvzId tex = load_crate_texture(vt.batch, tex_shape);
        dvz_mesh_texture(visual, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);
    }


    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);

    return 0;
}



int test_mesh_polygon(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh_polygon", VISUAL_TEST_PANZOOM, 0);

    // Polygon.
    uint32_t n = 6;
    dvec2* points = (dvec2*)calloc(n, sizeof(dvec2));
    double r = .75;
    for (uint32_t i = 0; i < n; i++)
    {
        points[i][0] = r * cos(i * M_2PI / n);
        points[i][1] = r * sin(i * M_2PI / n) * WIDTH / (float)HEIGHT;
    }
    cvec4 color = {255, 128, 0, 255};
    DvzShape shape = dvz_shape_polygon(n, (const dvec2*)points, color);
    FREE(points);

    // Make the shape a non-indexed shape so that each vertex gets its own barycentric coordinates.
    dvz_shape_unindex(&shape, DVZ_CONTOUR_JOINTS);

    // Create the visual.
    int flags = 0;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);

    // Set up the wireframe stroke parameters.
    dvz_mesh_stroke(visual, (vec4){.75, .75, .75, 50.0});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);

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

#define R {255, 0, 0, 255}
#define G {0, 255, 0, 255}
#define B {0, 0, 255, 255}

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
    cvec3 contour[] = {
        {0, 2, 2}, // Q0
        {0, 2, 2}, // P1
        {0, 2, 2}, // P0

        // NOTE: will be overriden by the GUI
        {2, 2, 2}, // P0
        {2, 2, 2}, // P1
        {2, 2, 2}, // P2

        {2, 2, 0}, // P0
        {2, 2, 0}, // P2
        {2, 2, 0}, // R0
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
}

static inline void _stroke_callback(DvzApp* app, DvzId canvas_id, DvzGuiEvent ev)
{
    VisualTest* vt = ev.user_data;
    ANN(vt);

    float* angle = (float*)vt->user_data;
    ANN(angle);

    dvz_gui_pos((vec2){0, 0}, (vec2){0, 0});
    dvz_gui_size((vec2){200, 0});
    dvz_gui_begin("Contour", dvz_gui_flags(DVZ_DIALOG_FLAGS_OVERLAY));
    bool u_changed = dvz_gui_slider("u", -1 + .01, +5.0, &angle[0]);
    bool v_changed = dvz_gui_slider("v", -1 + .01, +5.0, &angle[1]);
    dvz_gui_end();

    if (u_changed || v_changed)
    {
        _update_angle(vt->visual, (vec2){angle[0], angle[1]});
    }
}

int test_mesh_stroke(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh_stroke", VISUAL_TEST_NONE, DVZ_CANVAS_FLAGS_IMGUI);

    // Create the visual.
    DvzVisual* visual = dvz_mesh(vt.batch, 0);
    dvz_mesh_alloc(visual, COUNT, 0);

    // Mesh color.
    cvec4 color[] = {B, B, B, R, G, B, R, R, R};
    dvz_mesh_color(visual, 0, COUNT, color, 0);

    // Stroke.
    dvz_mesh_stroke(visual, (vec4){1, 1, 1, 50.0});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    vt.panel->camera = dvz_panel_camera(vt.panel, DVZ_CAMERA_FLAGS_ORTHO);
    DvzMVP* mvp = dvz_transform_mvp(vt.panel->transform);
    dvz_camera_mvp(vt.panel->camera, mvp); // set the view and proj matrices
    dvz_transform_update(vt.panel->transform);

    // Angle GUI.
    vt.visual = visual;
    float angle[2] = {0.75, 0.75};
    _update_angle(visual, angle);
    vt.user_data = &angle[0];
    dvz_app_gui(vt.app, vt.figure->canvas_id, _stroke_callback, &vt);

    // Run the test.
    visual_test_end(vt);

    return 0;
}



int test_mesh_surface(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh_surface", VISUAL_TEST_ARCBALL, 0);

    // Grid size.
    uint32_t row_count = 250;
    uint32_t col_count = row_count;

    // Grid parameters.
    vec3 o = {-1, 0, -1};
    vec3 u = {2.0 / (row_count - 1), 0, 0};
    vec3 v = {0, 0, 2.0 / (col_count - 1)};

    // Allocate heights and colors arrays.
    float* heights = (float*)calloc(row_count * col_count, sizeof(float));
    cvec4* colors = (cvec4*)calloc(row_count * col_count, sizeof(cvec4));

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

            idx++;
        }
    }

    // Create the surface shape.
    DvzShape shape = dvz_shape_surface(row_count, col_count, heights, colors, o, u, v, 0);
    dvz_shape_unindex(&shape, DVZ_CONTOUR_EDGES);

    // NOTE: we need to use non-indexed meshes for mesh wireframe.
    // Create the visual.
    int flags = DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);

    // Wireframe.
    dvz_mesh_wireframe(visual, .5);

    // Lighting.
    dvz_mesh_light_pos(visual, (vec3){-1, +1, +10});
    dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    dvz_arcball_initial(vt.arcball, (vec3){0.42339, -0.39686, -0.00554});
    dvz_panel_update(vt.panel);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(heights);
    FREE(colors);
    dvz_shape_destroy(&shape);

    return 0;
}



static inline void _gui_callback(DvzApp* app, DvzId canvas_id, DvzGuiEvent ev)
{
    VisualTest* vt = ev.user_data;
    ANN(vt);

    vec4* stroke = (vec4*)vt->user_data;

    dvz_gui_pos((vec2){0, 0}, (vec2){0, 0});
    dvz_gui_size((vec2){200, 300});
    dvz_gui_begin("Wireframe", dvz_gui_flags(DVZ_DIALOG_FLAGS_OVERLAY));
    bool width_changed = dvz_gui_slider("Width", 0, 10.0, &stroke[0][3]);
    bool stroke_changed = dvz_gui_colorpicker("Color", (float*)*stroke, 0);
    dvz_gui_end();

    if (width_changed || stroke_changed)
        dvz_mesh_stroke(vt->visual, stroke[0]);
}

int test_mesh_obj(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh_obj", VISUAL_TEST_ARCBALL, DVZ_CANVAS_FLAGS_IMGUI);

    // Load obj shape.
    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);
    DvzShape shape = dvz_shape_obj(path);
    if (!shape.vertex_count)
    {
        dvz_shape_destroy(&shape);
        return 0;
    }

    // dvz_mesh_stroke(visual, (vec4){STROKE, stroke_width});

    // NOTE: we need to use non-indexed meshes for mesh wireframe.
    dvz_shape_unindex(&shape, DVZ_CONTOUR_EDGES);

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);

    // Lighting.
    if (flags & DVZ_MESH_FLAGS_LIGHTING)
    {
        dvz_mesh_light_pos(visual, (vec3){-1, +1, +10});
        dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16});
    }

    vec4 stroke = {.25, .25, .25, 1.0};
    dvz_mesh_wireframe(visual, stroke[3]);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    dvz_arcball_initial(vt.arcball, (vec3){-2.7, -.7, -.1});
    dvz_panel_update(vt.panel);

    vt.visual = visual;
    vt.user_data = &stroke[0];
    dvz_app_gui(vt.app, vt.figure->canvas_id, _gui_callback, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);

    return 0;
}
