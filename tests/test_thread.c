/*************************************************************************************************/
/*  Testing thread                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_mutex.h"
#include "_thread.h"
#include "test.h"
#include "test_thread.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

static int _thread_callback(void* user_data)
{
    ANN(user_data);
    dvz_sleep(10);
    *((int*)user_data) = 42;
    log_debug("from thread");
    return 0;
}

int test_thread_1(TstSuite* suite)
{
    ANN(suite);
    int data = 0;
    DvzThread* thread = dvz_thread(_thread_callback, &data);
    AT(data == 0);
    dvz_thread_join(thread);
    AT(data == 42);
    return 0;
}



static int _mutex_callback(void* user_data)
{
    ANN(user_data);
    DvzMutex* mutex = (DvzMutex*)user_data;
    dvz_sleep(10);
    dvz_mutex_lock(mutex);
    return 0;
}

int test_mutex_1(TstSuite* suite)
{
    ANN(suite);
    DvzMutex mutex = dvz_mutex();

    DvzThread* thread = dvz_thread(_mutex_callback, &mutex);
    dvz_mutex_lock(&mutex);
    dvz_sleep(20);
    dvz_mutex_unlock(&mutex);

    dvz_thread_join(thread);
    dvz_mutex_destroy(&mutex);
    return 0;
}



static int _cond_callback(void* user_data)
{
    ANN(user_data);
    DvzCond* cond = (DvzCond*)user_data;
    dvz_sleep(10);
    dvz_cond_signal(cond);
    return 0;
}

int test_cond_1(TstSuite* suite)
{
    ANN(suite);
    DvzCond cond = dvz_cond();
    DvzMutex mutex = dvz_mutex();

    DvzThread* thread = dvz_thread(_cond_callback, &cond);
    dvz_cond_wait(&cond, &mutex);

    dvz_thread_join(thread);
    dvz_mutex_destroy(&mutex);
    dvz_cond_destroy(&cond);
    return 0;
}



int test_atomic_1(TstSuite* suite)
{
    ANN(suite);
    DvzAtomic atomic = dvz_atomic();
    dvz_atomic_set(atomic, 42);
    AT(dvz_atomic_get(atomic) == 42)
    dvz_atomic_destroy(atomic);
    return 0;
}
