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
#include "test_window.h"
// #include "test_vklite.h"
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

    // Testing window.
    TEST(window_1)

    // // Testing vklite.
    // TEST(vklite_host)
    // TEST(vklite_commands)
    // TEST(vklite_buffer_1)
    // TEST(vklite_buffer_resize)
    // // TEST(vklite_compute)
    // // TEST(vklite_push)
    // TEST(vklite_images)
    // TEST(vklite_sampler)
    // // TEST(vklite_barrier_buffer)
    // TEST(vklite_barrier_image)
    // // TEST(vklite_submit)

    tst_suite_run(&suite, match);
    tst_suite_destroy(&suite);
    return 0;
}
