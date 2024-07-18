/*************************************************************************************************/
/*  Testing axes                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axes.h"
#include "scene/axes.h"
#include "scene/panzoom.h"
#include "scene/ticks.h"
#include "scene/transform.h"
#include "scene/viewport.h"
#include "scene/visuals/glyph.h"
#include "scene/visuals/marker.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Axes tests                                                                                   */
/*************************************************************************************************/

static void _axes_onkeyboard(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    DvzAxes* axes = (DvzAxes*)ev.user_data;
    ANN(axes);

    if (ev.content.k.type == DVZ_KEYBOARD_EVENT_RELEASE)
    {
        dvz_axes_update(axes);
    }
}

int test_axes_1(TstSuite* suite)
{
#if !HAS_MSDF
    return 1;
#endif
    ANN(suite);

    VisualTest vt =
        visual_test_start("axes_1", VISUAL_TEST_PANZOOM, DVZ_RENDERER_FLAGS_WHITE_BACKGROUND);

    // Create the axes.
    int flags = 0;
    DvzAxes* axes = dvz_axes(vt.panel, flags);

    // Keyboard event.
    // dvz_app_onkeyboard(vt.app, _axes_onkeyboard, axes);

    // Manual pan.
    DvzPanzoom* pz = vt.panel->panzoom;
    ANN(pz);
    dvz_panzoom_pan(pz, (vec2){1, 0});
    dvz_panel_update(vt.panel);
    dvz_scene_run(vt.scene, vt.app, 10);

    dvec2 xrange = {0};
    // dvec2 yrange = {0};
    vec2 xrange_ndc = {0};
    // vec2 yrange_ndc = {0};

    dvz_axes_xget(axes, xrange, xrange_ndc);
    log_error("%f %f", xrange[0], xrange[1]);
    dvz_axes_xset(axes, xrange, xrange_ndc);
    dvz_axes_xget(axes, xrange, xrange_ndc);
    log_error("%f %f", xrange[0], xrange[1]);

    // Run the scene.
    // dvz_scene_run(vt.scene, vt.app, 10);
    dvz_app_destroy(vt.app);

    dvz_panel_destroy(vt.panel);
    dvz_figure_destroy(vt.figure);
    dvz_scene_destroy(vt.scene);

    // Run the test.
    // visual_test_end(vt);

    dvz_axes_destroy(axes);
    return 0;
}
