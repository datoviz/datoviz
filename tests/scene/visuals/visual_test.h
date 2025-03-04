/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual test                                                                                  */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VISUAL_TEST
#define DVZ_HEADER_VISUAL_TEST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "app.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "scene/camera.h"
#include "scene/scene.h"
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
    VISUAL_TEST_ORTHO,
    VISUAL_TEST_ARCBALL,
} VisualTestType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VisualTest VisualTest;

// Forward declarations.
typedef struct DvzAxis DvzAxis;
typedef struct DvzCamera DvzCamera;
typedef struct DvzVisual DvzVisual;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzArcball DvzArcball;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VisualTest
{
    const char* name;
    DvzApp* app;
    DvzBatch* batch;
    DvzScene* scene;
    DvzFigure* figure;
    DvzPanel* panel;
    DvzVisual* visual;
    DvzPanzoom* panzoom;
    DvzOrtho* ortho;
    DvzArcball* arcball;
    DvzCamera* camera;
    DvzVisual* volume;
    DvzAxis* haxis;
    DvzAxis* vaxis;
    uint32_t n, m, p;
    void* user_data;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static VisualTest visual_test_start(const char* name, VisualTestType type, int flags)
{

    // NOTE: use as follows:

    // VisualTest vt = visual_test_start(VISUAL_TEST_ARCBALL, 0);

    // // Volume visual.
    // DvzVisual* volume = dvz_volume(vt.batch, 0);

    // // Add the visual to the panel AFTER setting the visual's data.
    // dvz_panel_visual(vt.panel, volume, 0);

    // visual_test_end(vt);

    // return 0;


    // Create app objects.
    DvzApp* app = dvz_app(flags);
    dvz_app_create(app);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, flags);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    DvzArcball* arcball = NULL;
    DvzPanzoom* panzoom = NULL;
    DvzOrtho* ortho = NULL;
    DvzCamera* camera = NULL;

    switch (type)
    {
    case VISUAL_TEST_ARCBALL:

        // Arcball.
        arcball = dvz_panel_arcball(panel, 0);
        ANN(arcball);

        // Perspective camera.
        camera = dvz_panel_camera(panel, 0);

        break;

    case VISUAL_TEST_PANZOOM:
        panzoom = dvz_panel_panzoom(panel, 0);
        break;

    case VISUAL_TEST_ORTHO:
        ortho = dvz_panel_ortho(panel, 0);
        break;

    default:
        break;
    }

    VisualTest vt = {
        .name = name,
        .app = app,
        .batch = batch,
        .scene = scene,
        .figure = figure,
        .panel = panel,
        .panzoom = panzoom,
        .ortho = ortho,
        .arcball = arcball,
        .camera = camera,
    };
    return vt;
}



static void visual_test_end(VisualTest vt)
{
    // Make screenshot.
    dvz_scene_run(vt.scene, vt.app, 10);
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/visual_%s.png", ARTIFACTS_DIR, vt.name);
    dvz_app_screenshot(vt.app, vt.figure->canvas_id, imgpath);

    // Run the scene.
    dvz_scene_run(vt.scene, vt.app, N_FRAMES);

    // Cleanup.
    dvz_scene_destroy(vt.scene);
    dvz_app_destroy(vt.app);
}



#endif
