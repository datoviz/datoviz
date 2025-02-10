/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing server                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_server.h"
#include "server.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600



/*************************************************************************************************/
/*  Server tests                                                                                 */
/*************************************************************************************************/

int test_server_1(TstSuite* suite)
{
    ANN(suite);

    DvzServer* server = dvz_server(0);
    ANN(server);

    DvzBatch* batch = dvz_batch();
    ANN(batch);

    // Generate test rendering requests.
    DvzScene* scene = dvz_scene(batch);
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = dvz_panel(figure, 0, 0, WIDTH, HEIGHT);
    dvz_demo_panel(panel);

    DvzId canvas_id = dvz_figure_id(figure);
    ASSERT(canvas_id != DVZ_ID_NONE);

    // Submit the requests to the server.
    uint8_t* rgba = dvz_scene_render(scene, server, canvas_id, 0);
    ANN(rgba);

    // Save it to a file.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/server.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgba);

    // Cleanup.
    dvz_server_destroy(server);
    return 0;
}
