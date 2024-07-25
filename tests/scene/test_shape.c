/*************************************************************************************************/
/*  Testing shape                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_shape.h"
#include "datoviz.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Shape tests                                                                                  */
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



int test_shape_obj(TstSuite* suite)
{
    ANN(suite);

    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);

    DvzShape shape = dvz_shape_obj(path);
    dvz_shape_print(&shape);
    dvz_shape_destroy(&shape);
    return 0;
}
