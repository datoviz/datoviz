/*************************************************************************************************/
/*  Testing suite                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_thread.h"
#include "test.h"
#include "test_fifo.h"
#include "test_obj.h"
#include "test_thread.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests(const char* match)
{
    TstSuite suite = tst_suite();

    // Testing thread utils.
    TEST(utils_thread_1)
    TEST(utils_mutex_1)
    TEST(utils_cond_1)
    TEST(utils_atomic_1)

    // Testing obj.
    TEST(utils_obj_1)

    // Testing FIFO.
    TEST(utils_obj_1)
    TEST(utils_fifo_1)
    TEST(utils_fifo_2)
    TEST(utils_fifo_resize)
    TEST(utils_fifo_discard)
    TEST(utils_fifo_first)
    TEST(utils_deq_1)
    TEST(utils_deq_2)

    // Testing vklite.
    TEST(vklite_app)

    tst_suite_run(&suite, match);
    tst_suite_destroy(&suite);
    return 0;
}
