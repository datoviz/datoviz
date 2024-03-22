/*************************************************************************************************/
/*  Testing axes                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axes.h"
#include "scene/axes.h"
#include "scene/panzoom.h"
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

    dvz_axes_update(axes);
}

int test_axes_1(TstSuite* suite)
{
    ANN(suite);

    VisualTest vt = visual_test_start(
        "axes_1", VISUAL_TEST_PANZOOM, DVZ_CANVAS_FLAGS_FPS | DVZ_RENDERER_FLAGS_WHITE_BACKGROUND);

    // Create the axes.
    int flags = 0;
    DvzAxes* axes = dvz_axes(vt.panel, flags);

    // Keyboard event.
    dvz_app_onkeyboard(vt.app, _axes_onkeyboard, axes);

    // Run the test.
    visual_test_end(vt);

    dvz_axes_destroy(axes);
    return 0;
}
