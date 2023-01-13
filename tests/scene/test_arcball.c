/*************************************************************************************************/
/*  Testing arcball                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_arcball.h"
#include "scene/arcball.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Arcball test utils                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Arcball tests                                                                                */
/*************************************************************************************************/

int test_arcball_1(TstSuite* suite)
{
    ANN(suite);
    DvzArcball arcball = dvz_arcball(WIDTH, HEIGHT, 0);

    vec2 cur_pos = {0};
    vec2 last_pos = {0};
    vec3 angles = {0, 0, 0};

    // Identity.
    dvz_arcball_angles(&arcball, angles);
    dvz_arcball_print(&arcball);

    dvz_arcball_rotate(&arcball, cur_pos, last_pos);
    dvz_arcball_print(&arcball);

    // From angles.
    angles[0] = M_PI / 4;
    dvz_arcball_angles(&arcball, angles);
    dvz_arcball_print(&arcball);

    // Reset
    dvz_arcball_reset(&arcball);

    // Rotation.
    last_pos[0] = +10;
    last_pos[1] = -10;
    dvz_arcball_rotate(&arcball, cur_pos, last_pos);
    dvz_arcball_print(&arcball);

    dvz_arcball_destroy(&arcball);
    return 0;
}
