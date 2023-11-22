/*************************************************************************************************/
/*  Testing labels                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_labels.h"
#include "scene/labels.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Labels test utils                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Labels tests                                                                                 */
/*************************************************************************************************/

int test_labels_1(TstSuite* suite)
{
    ANN(suite);
    DvzLabels* labels = dvz_labels();

    dvz_labels_generate(labels, DVZ_TICKS_FORMAT_DECIMAL, 1, -10.0, 10.0, 2.5);
    dvz_labels_print(labels);
    printf("\n");

    dvz_labels_generate(labels, DVZ_TICKS_FORMAT_DECIMAL, 2, -1.0, 1.0, 0.25);
    dvz_labels_print(labels);
    printf("\n");

    dvz_labels_generate(labels, DVZ_TICKS_FORMAT_SCIENTIFIC, 2, 0, .0001, .00001);
    dvz_labels_print(labels);

    dvz_labels_destroy(labels);
    return 0;
}
