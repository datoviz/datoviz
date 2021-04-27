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



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_scene_single(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_POINT, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);
    _point_data(visual);

    return _scene_run(scene, "single");
}
