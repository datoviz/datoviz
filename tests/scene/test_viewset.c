/*************************************************************************************************/
/*  Testing viewset                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_viewset.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
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

int test_viewset_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    uint32_t n = 10;

    // Create a visual.
    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);

    DvzId canvas_id = 1;
    vec2 offset = {0, 0};
    vec2 shape = {0, 0};

    // Create a viewset.
    DvzViewset* viewset = dvz_viewset(rqr, canvas_id);

    // Create a view.
    DvzView* view = dvz_view(viewset, offset, shape);
    dvz_view_clear(view);

    // Add the visual to the view.
    dvz_view_add(view, visual, 0, n, 0, 1, NULL, 0);
    dvz_visual_visible(visual, true);

    dvz_viewset_build(viewset);
    // dvz_requester_print(rqr);

    dvz_view_destroy(view);

    dvz_viewset_destroy(viewset);
    dvz_requester_destroy(rqr);
    return 0;
}



int test_viewset_mouse(TstSuite* suite)
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
    DvzMouseEvent ev = {.type = DVZ_MOUSE_EVENT_MOVE, .content.m.pos = {x0, y0}};

    // Top left corner.
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.content.m.pos[0], x0, eps);
        AC(evl.content.m.pos[1], y0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.content.m.pos[0], 0, eps);
        AC(evl.content.m.pos[1], 0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        AC(evl.content.m.pos[0], -1, eps);
        AC(evl.content.m.pos[1], +1, eps);
    }

    // Center.
    ev.content.m.pos[0] = xc;
    ev.content.m.pos[1] = yc;
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.content.m.pos[0], xc, eps);
        AC(evl.content.m.pos[1], yc, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.content.m.pos[0], w / 2, eps);
        AC(evl.content.m.pos[1], h / 2, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        AC(evl.content.m.pos[0], 0, eps);
        AC(evl.content.m.pos[1], 0, eps);
    }

    // Bottom right.
    ev.content.m.pos[0] = x1;
    ev.content.m.pos[1] = y1;
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.content.m.pos[0], x1, eps);
        AC(evl.content.m.pos[1], y1, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.content.m.pos[0], w, eps);
        AC(evl.content.m.pos[1], h, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        AC(evl.content.m.pos[0], +1, eps);
        AC(evl.content.m.pos[1], -1, eps);
    }

    // Far top right.
    ev.content.m.pos[0] = x0 + 2 * w;
    ev.content.m.pos[1] = y0;
    {
        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_GLOBAL);
        AC(evl.content.m.pos[0], x0 + 2 * w, eps);
        AC(evl.content.m.pos[1], y0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_LOCAL);
        AC(evl.content.m.pos[0], 2 * w, eps);
        AC(evl.content.m.pos[1], 0, eps);

        evl = dvz_view_mouse(&view, ev, content_scale, DVZ_MOUSE_REFERENCE_SCALED);
        glm_vec2_print(evl.content.m.pos, stdout);
        AC(evl.content.m.pos[0], +3, eps);
        AC(evl.content.m.pos[1], +1, eps);
    }

    return 0;
}
