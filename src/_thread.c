/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Threading utilities                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_thread.h"
#include "_atomic.h"
#include "_macros.h"
#include "_mutex.h"
#include "_obj.h"

MUTE_ON
// #include "tinycthread.h"
MUTE_OFF



/*************************************************************************************************/
/*  Thread functions                                                                             */
/*************************************************************************************************/

DvzThread* dvz_thread(DvzThreadCallback callback, void* user_data)
{
    DvzThread* thread = (DvzThread*)calloc(1, sizeof(DvzThread));
    log_trace("creating thread");
    // if (tct_thrd_create(&thread->thread, callback, user_data) != tct_thrd_success)
    if (pthread_create(&thread->thread, NULL, callback, user_data))
        log_error("thread creation failed");
    if (dvz_mutex_init(&thread->lock) != 0)
        log_error("mutex creation failed");
    thread->lock_idx = dvz_atomic();
    dvz_obj_created(&thread->obj);
    return thread;
}



void dvz_thread_lock(DvzThread* thread)
{
    ANN(thread);
    if (!dvz_obj_is_created(&thread->obj))
        return;
    // The lock idx is used to ensure that nested thread_lock() will work as expected. Only the
    // first lock is effective. Only the last unlock is effective.
    ANN(thread->lock_idx);
    int lock_idx = dvz_atomic_get(thread->lock_idx);
    ASSERT(lock_idx >= 0);
    if (lock_idx == 0)
    {
        log_trace("acquire lock");
        dvz_mutex_lock(&thread->lock);
    }
    dvz_atomic_set(thread->lock_idx, lock_idx + 1);
}



void dvz_thread_unlock(DvzThread* thread)
{
    ANN(thread);
    if (!dvz_obj_is_created(&thread->obj))
        return;
    ANN(thread->lock_idx);
    int lock_idx = dvz_atomic_get(thread->lock_idx);
    ASSERT(lock_idx >= 0);
    if (lock_idx == 1)
    {
        log_trace("release lock");
        dvz_mutex_unlock(&thread->lock);
    }
    if (lock_idx >= 1)
        dvz_atomic_set(thread->lock_idx, lock_idx - 1);
    // else
    //     log_error("lock_idx = 0 ???");
}



void dvz_thread_join(DvzThread* thread)
{
    ANN(thread);
    log_trace("joining thread");
    // tct_thrd_join(thread->thread, NULL);
    pthread_join(thread->thread, NULL);
    dvz_mutex_destroy(&thread->lock);
    dvz_atomic_destroy(thread->lock_idx);
    dvz_obj_destroyed(&thread->obj);
    FREE(thread);
}
