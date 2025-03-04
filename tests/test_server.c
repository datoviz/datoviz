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
#include "fileio.h"
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

int test_server_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create a server.
    DvzServer* server = dvz_server(0);
    ANN(server);

    // Generate test rendering requests.
    DvzScene* scene = dvz_scene(NULL);
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = dvz_panel(figure, 0, 0, WIDTH, HEIGHT);
    dvz_demo_panel_2D(panel);

    // Submit the requests to the server.
    dvz_scene_render(scene, server);
    uint8_t* rgb = dvz_server_grab(server, dvz_figure_id(figure), 0);
    ANN(rgb);

    // Save it to a file.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/server.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);

    // Cleanup.
    dvz_server_destroy(server);
    return 0;
}
