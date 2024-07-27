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
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _onmouse(DvzApp* app, DvzId window_id, DvzMouseEvent ev)
{
    ANN(app);

    // Display arcball Euler angles when rotating the model.
    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzArcball* arcball = (DvzArcball*)vt->arcball;
    ANN(arcball);

    vec3 angles = {0};
    dvz_arcball_angles(arcball, angles);
    // glm_vec3_print(angles, stdout);
}



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
        dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});

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
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);

    return 0;
}



int test_mesh_surface(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh", VISUAL_TEST_ARCBALL, 0);

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

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);

    // Lighting.
    dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});
    dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    dvz_app_onmouse(vt.app, _onmouse, &vt);

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



int test_mesh_obj(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh_obj", VISUAL_TEST_ARCBALL, 0);

    // Load obj shape.
    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);
    DvzShape shape = dvz_shape_obj(path);
    if (!shape.vertex_count)
    {
        dvz_shape_destroy(&shape);
        return 0;
    }

    // Create the visual.
    int flags = DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);

    // Lighting.
    if (flags & DVZ_MESH_FLAGS_LIGHTING)
    {
        dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});
        dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16});
    }

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    dvz_app_onmouse(vt.app, _onmouse, &vt);

    dvz_arcball_initial(vt.arcball, (vec3){-2.7, -.7, -.1});
    dvz_panel_update(vt.panel);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);

    return 0;
}
