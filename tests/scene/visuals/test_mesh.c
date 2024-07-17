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

int test_mesh_obj(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh_obj", VISUAL_TEST_ARCBALL, 0);

    // Load obj shape.
    char path[1024];
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
