/*************************************************************************************************/
/*  Visual test                                                                                  */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VISUAL_TEST
#define DVZ_HEADER_VISUAL_TEST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "request.h"
#include "scene/app.h"
#include "scene/camera.h"
#include "scene/dual.h"
#include "scene/scene.h"
#include "scene/shape.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VISUAL_TEST_NONE,
    VISUAL_TEST_PANZOOM,
    VISUAL_TEST_ARCBALL,
} VisualTestType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VisualTest VisualTest;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VisualTest
{
    DvzApp* app;
    DvzRequester* rqr;
    DvzScene* scene;
    DvzFigure* figure;
    DvzPanel* panel;
    DvzPanzoom* panzoom;
    DvzArcball* arcball;
    DvzCamera* camera;
    DvzVisual* volume;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static VisualTest visual_test_start(VisualTestType type)
{

    // NOTE: use as follows:

    // VisualTest vt = visual_test_start(VISUAL_TEST_ARCBALL);

    // // Volume visual.
    // DvzVisual* volume = dvz_volume(vt.rqr, 0);
    // dvz_volume_alloc(volume, 1);

    // // Add the visual to the panel AFTER setting the visual's data.
    // dvz_panel_visual(vt.panel, volume);

    // visual_test_end(vt);

    // return 0;


    // Create app objects.
    DvzApp* app = dvz_app(0);
    DvzRequester* rqr = dvz_app_requester(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(rqr);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_VSYNC);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    DvzArcball* arcball = NULL;
    DvzPanzoom* panzoom = NULL;
    DvzCamera* camera = NULL;

    switch (type)
    {
    case VISUAL_TEST_ARCBALL:

        // Arcball.
        arcball = dvz_panel_arcball(app, panel);
        ANN(arcball);

        // Perspective camera.
        camera = dvz_panel_camera(panel);

        break;

    case VISUAL_TEST_PANZOOM:
        panzoom = dvz_panel_panzoom(app, panel);
        break;

    default:
        break;
    }

    VisualTest vt = {
        .app = app,
        .rqr = rqr,
        .scene = scene,
        .figure = figure,
        .panel = panel,
        .panzoom = panzoom,
        .arcball = arcball,
        .camera = camera,
    };
    return vt;
}



static void visual_test_end(VisualTest vt)
{
    // Run the scene.
    dvz_scene_run(vt.scene, vt.app, N_FRAMES);

    // Cleanup.
    if (vt.camera != NULL)
        dvz_camera_destroy(vt.camera);
    dvz_panel_destroy(vt.panel);
    dvz_figure_destroy(vt.figure);
    dvz_scene_destroy(vt.scene);
    dvz_app_destroy(vt.app);
}



#endif
