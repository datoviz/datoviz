/*************************************************************************************************/
/*  Testing animation                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_shape.h"
#include "presenter.h"
#include "renderer.h"
#include "scene/app.h"
#include "scene/shape.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Animation tests                                                                              */
/*************************************************************************************************/

int test_shape_1(TstSuite* suite)
{
    ANN(suite);

    cvec4 color = {255, 0, 0, 255};
    const uint32_t count = 30;

    DvzShape square = dvz_shape_square(color);
    DvzShape disc = dvz_shape_disc(count, color);

    dvz_shape_destroy(&disc);
    dvz_shape_destroy(&square);
    return 0;
}
