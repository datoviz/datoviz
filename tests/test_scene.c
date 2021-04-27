#include "../include/datoviz/scene.h"
#include "../include/datoviz/visuals.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static int _scene_run(DvzScene* scene, const char* name)
{
    ASSERT(scene != NULL);
    DvzCanvas* canvas = scene->canvas;

    // Run the app.
    dvz_app_run(canvas->app, N_FRAMES);

    // Screenshot.
    char path[1024];
    snprintf(path, sizeof(path), "test_scene_%s", name);
    int res = check_canvas(canvas, path);

    // Destroy the scene (also destroy all panels and all visuals in the panels).
    dvz_scene_destroy(scene);

    return res;
}



static void _add_visual(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_POINT, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);
    _point_data(visual);
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_scene_single(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    _add_visual(panel);

    return _scene_run(scene, "single");
}



int test_scene_double(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 2);

    _add_visual(dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0));
    _add_visual(dvz_scene_panel(scene, 0, 1, DVZ_CONTROLLER_PANZOOM, 0));

    return _scene_run(scene, "double");
}



int test_scene_different_size(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 2, 3);

    for (uint32_t i = 0; i < 2; i++)
        for (uint32_t j = 0; j < 3; j++)
            _add_visual(dvz_scene_panel(scene, i, j, DVZ_CONTROLLER_PANZOOM, 0));

    dvz_panel_size(dvz_panel(&scene->grid, 0, 0), DVZ_GRID_HORIZONTAL, 2);
    dvz_panel_size(dvz_panel(&scene->grid, 1, 1), DVZ_GRID_VERTICAL, 0.5);

    return _scene_run(scene, "different_size");
}
