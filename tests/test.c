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
#include "test_window.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests(const char* match)
{
    TstSuite suite = tst_suite();
    DvzTestCtx ctx = {0};
    suite.context = &ctx;

    // Testing thread utils.
    TEST(test_utils_thread_1)
    TEST(test_utils_mutex_1)
    TEST(test_utils_cond_1)
    TEST(test_utils_atomic_1)

    // Testing obj.
    TEST(test_utils_obj_1)

    // Testing FIFO.
    TEST(test_utils_obj_1)
    TEST(test_utils_fifo_1)
    TEST(test_utils_fifo_2)
    TEST(test_utils_fifo_resize)
    TEST(test_utils_fifo_discard)
    TEST(test_utils_fifo_first)
    TEST(test_utils_deq_1)
    TEST(test_utils_deq_2)

    // Testing window.
    TEST(test_window_1)

    // Testing vklite.
    TEST(test_vklite_host)

    SETUP(setup_host)
    TEST(test_vklite_commands)
    TEST(test_vklite_buffer_1)
    TEST(test_vklite_buffer_resize)
    TEST(test_vklite_load_shader)
    TEST(test_vklite_compute)
    TEST(test_vklite_push)
    TEST(test_vklite_images)
    TEST(test_vklite_sampler)
    TEST(test_vklite_barrier_buffer)
    TEST(test_vklite_barrier_image)
    TEST(test_vklite_submit)
    TEARDOWN(teardown_host)

    tst_suite_run(&suite, match);
    tst_suite_destroy(&suite);
    return 0;
}
