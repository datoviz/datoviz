/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mutex                                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "datoviz/common/macros.h"
#include "mutex_internal.h"



/*************************************************************************************************/
/*  Mutex functions                                                                              */
/*************************************************************************************************/


int dvz_mutex_init(DvzMutex* mutex)
{
    ANN(mutex);
    return pthread_mutex_init(mutex, 0);
}



DvzMutex dvz_mutex(void)
{
    INIT(DvzMutex, mutex);
    dvz_mutex_init(&mutex);
    return mutex;
}



int dvz_mutex_lock(DvzMutex* mutex)
{
    ANN(mutex);
    return pthread_mutex_lock(mutex);
}



int dvz_mutex_unlock(DvzMutex* mutex)
{
    ANN(mutex);
    return pthread_mutex_unlock(mutex);
}



void dvz_mutex_destroy(DvzMutex* mutex)
{
    ANN(mutex);
    pthread_mutex_destroy(mutex);
}



/*************************************************************************************************/
/*  Cond functions                                                                               */
/*************************************************************************************************/

int dvz_cond_init(DvzCond* cond)
{
    ANN(cond);
    return pthread_cond_init(cond, 0);
}



DvzCond dvz_cond(void)
{

    INIT(DvzCond, cond);
    dvz_cond_init(&cond);
    return cond;
}



int dvz_cond_signal(DvzCond* cond)
{
    ANN(cond);
    return pthread_cond_signal(cond);
}



int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex)
{
    ANN(cond);
    return pthread_cond_wait(cond, mutex);
}



void dvz_cond_destroy(DvzCond* cond)
{
    ANN(cond);
    pthread_cond_destroy(cond);
}
