#include "../include/datoviz/scene.h"
#include "../include/datoviz/visuals.h"
#include "../src/interact_utils.h"
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



static DvzVisual* _add_visual(DvzPanel* panel)
{
    ASSERT(panel != NULL);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_POINT, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);
    _point_data(visual, 50);
    return visual;
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_scene_empty(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    _white_background(canvas);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_IMAGE, 0);
    ASSERT(visual);

    dvz_app_run(canvas->app, 10);
    int res = _scene_run(scene, "empty");

    _dark_background(canvas);
    return res;
}


int test_scene_empty_visuals(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    DvzVisual* visual_0 = dvz_scene_visual(panel, DVZ_VISUAL_MARKER, 0);
    dvz_app_run(canvas->app, 3);

    DvzVisual* visual_1 = dvz_scene_visual(panel, DVZ_VISUAL_POINT, 0);
    dvz_app_run(canvas->app, 3);

    // NOTE: the call below causes an infinite loop and crash. The scene update system needs to be
    // refactored.
    // _point_data(visual_0);

    ASSERT(visual_0);
    ASSERT(visual_1);

    int res = _scene_run(scene, "empty_visuals");
    return res;
}



int test_scene_single(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    _white_background(canvas);
    dvz_canvas_dpi_scaling(canvas, 1.5);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);

    // BUG FIX: check that the DPI > 1 doesn't cause the marker size to increase at each call.
    DvzVisual* visual = _add_visual(panel);
    for (uint32_t i = 0; i < 5; i++)
    {
        _point_data(visual, 50);
        dvz_app_run(canvas->app, 5);
    }

    int res = _scene_run(scene, "single");

    dvz_canvas_dpi_scaling(canvas, 1);
    _dark_background(canvas);

    return res;
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



int test_scene_multiple(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);

    // Create two visuals.
    DvzVisual* v0 = dvz_scene_visual(panel, DVZ_VISUAL_POINT, 0);
    DvzVisual* v1 = dvz_scene_visual(panel, DVZ_VISUAL_POINT, 0);

    // Marker size.
    dvz_visual_data(v0, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){20});
    dvz_visual_data(v1, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){20});

    // Create visual data.
    uint32_t n = 11;
    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = -1 + 2 * (float)i / (n - 1);
        pos[i][1] = 0;
        dvz_colormap(DVZ_CMAP_HSV, i * 4, color[i]);
    }

    // Set visual data.
    dvz_visual_data(v0, DVZ_PROP_POS, 0, n / 2, &pos[n / 2]);
    dvz_visual_data(v0, DVZ_PROP_COLOR, 0, n / 2, &color[n / 2]);

    dvz_visual_data(v1, DVZ_PROP_POS, 0, n / 2, pos);
    dvz_visual_data(v1, DVZ_PROP_COLOR, 0, n / 2, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _scene_run(scene, "multiple");
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



int test_scene_different_controllers(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 2);

    _add_visual(dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0));
    _add_visual(dvz_scene_panel(scene, 0, 1, DVZ_CONTROLLER_PANZOOM, 0));

    return _scene_run(scene, "different_controllers");
}



int test_scene_link(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 3);

    DvzPanel* p0 = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_ARCBALL, 0);
    DvzPanel* p1 = dvz_scene_panel(scene, 0, 1, DVZ_CONTROLLER_ARCBALL, 0);
    DvzPanel* p2 = dvz_scene_panel(scene, 0, 2, DVZ_CONTROLLER_ARCBALL, 0);

    _add_visual(p0);
    _add_visual(p1);
    _add_visual(p2);

    dvz_panel_link(&scene->grid, p0, p1);
    dvz_panel_link(&scene->grid, p0, p2);

    return _scene_run(scene, "link");
}



/*************************************************************************************************/
/*  Dynamic scene tests                                                                          */
/*************************************************************************************************/

int test_scene_dynamic_axes(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);
    _white_background(canvas);

    // Run a few frames.
    dvz_app_run(canvas->app, 5);

    // Create the scene.
    DvzScene* scene = dvz_scene(canvas, 1, 1);

    // Run a few frames.
    dvz_app_run(canvas->app, 5);

    // Add a panel.
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);

    // Run a few frames.
    dvz_app_run(canvas->app, 5);

    // Add the visual.
    _add_visual(panel);

    // Pan.
    // NOTE: improve API for manual interact update.
    _panzoom_pan(&panel->controller->interacts[0].u.p, (vec2){-.5, 0});
    _panzoom_update_mvp(
        canvas->viewport, &panel->controller->interacts[0].u.p,
        &panel->controller->interacts[0].mvp);

    // Run the test and check the screenshot.
    int res = _scene_run(scene, "dynamic_axes");

    _dark_background(canvas);
    return res;
}
