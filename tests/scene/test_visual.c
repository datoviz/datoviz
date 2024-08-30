/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Testing visual                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_visual.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/visual.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Visual tests                                                                                 */
/*************************************************************************************************/

int test_visual_1(TstSuite* suite)
{
    DvzBatch* batch = dvz_batch();

    uint32_t n = 10000;

    // Create a visual.
    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_trivial");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, 0, sizeof(vec3), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, sizeof(vec3), sizeof(cvec4), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);

    // MVP.
    DvzMVP mvp = dvz_mvp_default();
    dvz_visual_mvp(visual, &mvp);

    // Viewport.
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    dvz_visual_viewport(visual, &viewport);

    // Create the visual.
    dvz_visual_alloc(visual, n, n, 0);

    // Vertex data.
    // Position.
    vec3* pos = dvz_mock_pos2D(n, 0.25);
    dvz_visual_data(visual, 0, 0, n, pos);

    // Color.
    cvec4* color = dvz_mock_color(n, 128);
    dvz_visual_data(visual, 1, 0, n, color);

    // Important: upload the data to the GPU.
    dvz_visual_update(visual);

    // Create a board.
    DvzRequest req = dvz_create_board(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;
    req = dvz_set_background(batch, board_id, (cvec4){32, 64, 128, 255});

    // Record the commands.
    dvz_record_begin(batch, board_id);
    dvz_record_viewport(batch, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_visual_instance(visual, board_id, 0, 0, n, 0, 1);
    dvz_record_end(batch, board_id);

    // Render to a PNG.
    render_requests(batch, get_gpu(suite), board_id, "visual_1");

    // Cleanup
    dvz_visual_destroy(visual);
    dvz_batch_destroy(batch);
    FREE(pos);
    FREE(color);
    return 0;
}
