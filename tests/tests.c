/*************************************************************************************************/
/*  Testing suite                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_thread.h"
#include "testing.h"
#include "tests.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

static int _thread_callback(void* user_data)
{
    ASSERT(user_data != NULL);
    dvz_sleep(10);
    *((int*)user_data) = 42;
    log_debug("from thread");
    return 0;
}

int dvz_test_thread_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    int data = 0;
    DvzThread thread = dvz_thread(_thread_callback, &data);
    AT(data == 0);
    dvz_thread_join(&thread);
    AT(data == 42);
    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests()
{
    TstSuite suite = tst_suite();

    tst_suite_add(&suite, "test_1", dvz_test_thread_1, NULL);

    tst_suite_run(&suite, NULL);
    tst_suite_destroy(&suite);
    return 0;
}
