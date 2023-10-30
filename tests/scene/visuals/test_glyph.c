/*************************************************************************************************/
/*  Testing glyph                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_glyph.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/glyph.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Glyph tests                                                                                  */
/*************************************************************************************************/

static void _on_timer(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);
    float t = ev.content.t.time;

    // float x = sin(t);
    // float z = cos(t);
    // dvz_glyph_axis(visual, 0, 1, (vec3[]){{x, 0, z}}, 0);

    dvz_glyph_angle(visual, 0, 1, (float[]){M_PI * t}, 0);
    dvz_visual_update(visual);
}

int test_glyph_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("glyph", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 1;

    // Create the visual.
    DvzVisual* visual = dvz_glyph(vt.batch, 0);

    // Visual allocation.
    dvz_glyph_alloc(visual, n);

    // Position.
    dvz_glyph_position(visual, 0, 1, (vec3[]){{0, 0, 0}}, 0);

    // WARNING: rename to axis
    dvz_glyph_axis(visual, 0, 1, (vec3[]){{0, 0, 1}}, 0);
    // dvz_glyph_angle(visual, 0, 1, (float[]){M_PI / 4.0}, 0);
    dvz_glyph_anchor(visual, 0, 1, (vec2[]){{0, 0}}, 0);

    dvz_glyph_size(visual, (vec2){20, 40});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Animation.
    vt.visual = visual;
    // vt.user_data = (void*)colors;
    dvz_app_timer(vt.app, 0, 1. / 60., 0);
    dvz_app_ontimer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
