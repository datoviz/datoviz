/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing thread */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "../atomic_internal.h"
#include "../thread_internal.h"
#include "_assertions.h"
#include "_time_utils.h"
#include "mutex_internal.h"
#include "test_thread.h"
#include "testing.h"



/*************************************************************************************************/
/*  Thread tests */
/*************************************************************************************************/

static void* _thread_callback(void* user_data)
{
    ANN(user_data);
    dvz_sleep(10);
    *((int*)user_data) = 42;
    return NULL;
}

int test_thread_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    int data = 0;
    DvzThread* thread = dvz_thread(_thread_callback, &data);
    dvz_thread_join(thread);
    AT(data == 42);
    return 0;
}



static void* _mutex_callback(void* user_data)
{
    ANN(user_data);
    DvzMutex* mutex = (DvzMutex*)user_data;
    dvz_sleep(10);
    dvz_mutex_lock(mutex);
    dvz_sleep(50);
    dvz_mutex_unlock(mutex);
    return NULL;
}

int test_mutex_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    DvzMutex mutex = dvz_mutex();

    DvzThread* thread = dvz_thread(_mutex_callback, &mutex);
    dvz_mutex_lock(&mutex);
    dvz_sleep(20);
    dvz_mutex_unlock(&mutex);
    dvz_sleep(80);

    dvz_thread_join(thread);
    dvz_mutex_destroy(&mutex);
    return 0;
}



static void* _cond_callback(void* user_data)
{
    ANN(user_data);
    DvzCond* cond = (DvzCond*)user_data;
    dvz_sleep(10);
    dvz_cond_signal(cond);
    return NULL;
}

int test_cond_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    DvzCond cond = dvz_cond();
    DvzMutex mutex = dvz_mutex();

    DvzThread* thread = dvz_thread(_cond_callback, &cond);
    dvz_mutex_lock(&mutex);
    dvz_cond_wait(&cond, &mutex);
    dvz_mutex_unlock(&mutex);

    dvz_thread_join(thread);
    dvz_mutex_destroy(&mutex);
    dvz_cond_destroy(&cond);
    return 0;
}



int test_atomic_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    DvzAtomic atomic = dvz_atomic();
    dvz_atomic_set(atomic, 42);
    AT(dvz_atomic_get(atomic) == 42)
    dvz_atomic_destroy(atomic);
    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_thread(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "thread";

    TST_MODULE(suite, "thread");
    TST_GROUP("thread");
    TST_CASE(test_thread_1);
    TST_CASE(test_mutex_1);
    TST_CASE(test_cond_1);
    TST_CASE(test_atomic_1);

    return 0;
}
