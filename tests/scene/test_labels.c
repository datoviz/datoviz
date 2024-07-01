/*************************************************************************************************/
/*  Testing labels                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_labels.h"
#include "../../src/scene/labels_utils.h"
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



static inline void _test_factored(DvzLabels* labels, double lmin, double lmax)
{
    ANN(labels);

    double lstep = (lmax - lmin) / 10;
    double offset = 0;
    int32_t exponent = 0;
    uint32_t precision = 4;

    _find_exponent_offset(lmin, lmax, &exponent, &offset);
    printf("TICK LABELS\n");
    printf("----------------------------\n");
    printf("lmin       : %e\n", lmin);
    printf("lmax       : %e\n", lmax);
    printf("lstep      : %e\n", lstep);
    printf("exponent   : %d\n", exponent);
    printf("offset     : %e\n\n", offset);
    dvz_labels_generate(
        labels, DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED, precision, exponent, offset, lmin, lmax,
        lstep);
    dvz_labels_print(labels);

    printf("\n\n");
}

int test_labels_factored(TstSuite* suite)
{
    DvzLabels* labels = dvz_labels();

    _test_factored(labels, 0, 1);
    _test_factored(labels, -10, +10);
    _test_factored(labels, 0, 10);
    _test_factored(labels, 1e3 - 1, 1e3 + 1);

    dvz_labels_destroy(labels);
    return 0;
}
