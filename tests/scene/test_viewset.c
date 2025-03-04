/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing viewset                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_viewset.h"
#include "_cglm.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/transform.h"
#include "scene/viewset.h"
#include "scene/visual.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Viewset tests                                                                                */
/*************************************************************************************************/

int test_viewset_1(TstSuite* suite, TstItem* tstitem)
{
    DvzBatch* batch = dvz_batch();

    uint32_t n = 10;

    // Create a visual.
    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);

    DvzId canvas_id = 1;
    vec2 offset = {0, 0};
    vec2 shape = {0, 0};

    // Create a viewset.
    DvzViewset* viewset = dvz_viewset(batch, canvas_id);

    // Create a view.
    DvzView* view = dvz_view(viewset, offset, shape);
    dvz_view_clear(view);

    // Add the visual to the view.
    DvzTransform* tr = dvz_transform(batch, 0);
    dvz_view_add(view, visual, 0, n, 0, 1, tr, 0);
    dvz_visual_show(visual, true);

    dvz_viewset_build(viewset);
    // dvz_requester_print(batch);

    dvz_view_destroy(view);

    dvz_viewset_destroy(viewset);
    dvz_batch_destroy(batch);
    return 0;
}



int test_viewset_mouse(TstSuite* suite, TstItem* tstitem)
{
    float eps = 1e-6;
    float content_scale = 1;
    float x0 = 10, y0 = 20, w = 100, h = 200;
    // Center.
    float xc = x0 + w / 2.0;
    float yc = y0 + h / 2.0;
    // Bottom right.
    float x1 = x0 + w;
    float y1 = y0 + h;

    DvzMouseEvent evl = {0};
    DvzView view = {.offset = {x0, y0}, .shape = {w, h}};
    DvzMouseEvent ev = {.type = DVZ_MOUSE_EVENT_MOVE, .pos = {x0, y0}};

    // Top left corner.
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.pos[0], x0, eps);
        AC(evl.pos[1], y0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.pos[0], 0, eps);
        AC(evl.pos[1], 0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        AC(evl.pos[0], -1, eps);
        AC(evl.pos[1], +1, eps);
    }

    // Center.
    ev.pos[0] = xc;
    ev.pos[1] = yc;
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.pos[0], xc, eps);
        AC(evl.pos[1], yc, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.pos[0], w / 2, eps);
        AC(evl.pos[1], h / 2, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        AC(evl.pos[0], 0, eps);
        AC(evl.pos[1], 0, eps);
    }

    // Bottom right.
    ev.pos[0] = x1;
    ev.pos[1] = y1;
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.pos[0], x1, eps);
        AC(evl.pos[1], y1, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.pos[0], w, eps);
        AC(evl.pos[1], h, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        AC(evl.pos[0], +1, eps);
        AC(evl.pos[1], -1, eps);
    }

    // Far top right.
    ev.pos[0] = x0 + 2 * w;
    ev.pos[1] = y0;
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.pos[0], x0 + 2 * w, eps);
        AC(evl.pos[1], y0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.pos[0], 2 * w, eps);
        AC(evl.pos[1], 0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        glm_vec2_print(evl.pos, stdout);
        AC(evl.pos[0], +3, eps);
        AC(evl.pos[1], +1, eps);
    }

    return 0;
}
