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
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/visual.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Visual tests                                                                                 */
/*************************************************************************************************/

int test_visual_1(TstSuite* suite, TstItem* tstitem)
{
    DvzBatch* batch = dvz_batch();

    uint32_t n = 10000;

    // Create a visual.
    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_trivial");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, 0, sizeof(vec3), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, sizeof(vec3), sizeof(DvzColor), DVZ_FORMAT_COLOR, 0);

    // Slots.
    _common_setup(visual);

    // MVP.
    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);
    dvz_visual_mvp(visual, &mvp);

    // Viewport.
    DvzViewport viewport = {0};
    dvz_viewport_default(WIDTH, HEIGHT, &viewport);
    dvz_visual_viewport(visual, &viewport);

    // Create the visual.
    dvz_visual_alloc(visual, n, n, 0);

    // Vertex data.
    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_visual_data(visual, 0, 0, n, pos);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_visual_data(visual, 1, 0, n, color);

    // Important: upload the data to the GPU.
    dvz_visual_update(visual);

    // Create a canvas.
    DvzRequest req = dvz_create_canvas(
        batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR,
        DVZ_APP_FLAGS_OFFSCREEN | DVZ_CANVAS_FLAGS_PUSH_SCALE);
    DvzId canvas_id = req.id;
    req = dvz_set_background(batch, canvas_id, (cvec4){32, 64, 128, 255});

    // Record the commands.
    dvz_record_begin(batch, canvas_id);
    dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_visual_instance(visual, canvas_id, 0, 0, n, 0, 1);
    dvz_record_end(batch, canvas_id);

    // Render to a PNG.
    render_requests(batch, get_gpu(suite), canvas_id, "visual_1");

    // Cleanup
    dvz_visual_destroy(visual);
    dvz_batch_destroy(batch);
    FREE(pos);
    FREE(color);
    return 0;
}
