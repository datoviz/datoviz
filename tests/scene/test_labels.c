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

    dvz_labels_generate(labels, DVZ_TICKS_FORMAT_DECIMAL, 1, 0, 0.0, -10.0, 10.0, 2.5);
    dvz_labels_print(labels);
    printf("\n");

    dvz_labels_generate(labels, DVZ_TICKS_FORMAT_DECIMAL, 2, 0, 0.0, -1.0, 1.0, 0.25);
    dvz_labels_print(labels);
    printf("\n");

    dvz_labels_generate(labels, DVZ_TICKS_FORMAT_SCIENTIFIC, 2, 0, 0.0, 0.0, .0001, .00001);
    dvz_labels_print(labels);
    printf("\n");

    dvz_labels_generate(
        labels, DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED, 2, -5, 0.0, 0.0, .0001, .00001);
    dvz_labels_print(labels);

    dvz_labels_destroy(labels);
    return 0;
}



int test_labels_exponent_offset(TstSuite* suite)
{
    double offset = 0;
    int32_t exponent = 0;
    double lmin = 1e9;
    double lmax = lmin + 1;
    double lstep = (lmax - lmin) / 10;

    _find_exponent_offset(lmin, lmax, &exponent, &offset);
    printf("lmin, lmax : %e, %e\n", lmin, lmax);
    printf("exponent   : %d\n", exponent);
    printf("offset     : %e\n\n", offset);

    // exponent = 0;
    DvzLabels* labels = dvz_labels();
    dvz_labels_generate(
        labels, DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED, 2, exponent, offset, lmin, lmax, lstep);
    dvz_labels_print(labels);
    dvz_labels_destroy(labels);

    return 0;
}
