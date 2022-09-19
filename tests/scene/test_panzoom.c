/*************************************************************************************************/
/*  Testing panzoom                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_panzoom.h"
#include "scene/panzoom.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Panzoom test utils                                                                           */
/*************************************************************************************************/

#define PAN(x, y)                                                                                 \
    dvz_panzoom_pan_shift(&pz, (vec2){WIDTH * x, HEIGHT * y}, (vec2){WIDTH / 2, HEIGHT / 2});     \
    dvz_panzoom_end(&pz);

#define ZOOM(x, y, cx, cy)                                                                        \
    dvz_panzoom_zoom_shift(&pz, (vec2){WIDTH * x, HEIGHT * y}, (vec2){WIDTH * cx, HEIGHT * cy});  \
    dvz_panzoom_end(&pz);

#define RESET dvz_panzoom_reset(&pz);

#define AP(x, y)                                                                                  \
    AC(pz.pan[0], x, EPS);                                                                        \
    AC(pz.pan[1], y, EPS);

#define SHOW                                                                                      \
    log_info(                                                                                     \
        "pan: (%.2f, %.2f)  zoom: (%.2f, %.2f)", pz.pan[0], pz.pan[1], pz.zoom[0], pz.zoom[1]);   \
    dvz_panzoom_mvp(&pz);                                                                         \
    glm_mat4_print(pz.mvp.view, stdout);



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

int test_panzoom_1(TstSuite* suite)
{
    ANN(suite);

    DvzPanzoom pz = dvz_panzoom(WIDTH, HEIGHT, 0);

    // Test pan.
    {
        PAN(0, 0);
        AP(0, 0);
        // SHOW;

        PAN(.5, 0);
        AP(1, 0);
        // SHOW;

        PAN(.5, 0);
        AP(2, 0);

        PAN(-1, .5);
        AP(0, -1);
    }

    RESET;

    // Test zoom.
    {
        ZOOM(0, 0, .5, .5);
        AP(0, 0);

        ZOOM(.5, -1, .5, .5);
        AT(pz.zoom[0] > 1);
        AT(pz.zoom[1] < 1);
    }

    // Zoom with shift center.
    RESET;
    {
        ZOOM(10, 10, .5, -.5);
        AT(pz.zoom[0] > 1e6);
        AT(pz.zoom[1] > 1e6);
        AT(pz.zoom[0] == pz.zoom[1]);
        AP(WIDTH / 2, -HEIGHT / 2);
        // SHOW;
    }

    dvz_panzoom_destroy(&pz);
    return 0;
}
