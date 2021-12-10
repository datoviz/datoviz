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
#include "tinycthread.h"
MUTE_OFF



/*************************************************************************************************/
/*  Thread functions                                                                             */
/*************************************************************************************************/

DvzThread dvz_thread(DvzThreadCallback callback, void* user_data)
{
    INIT(DvzThread, thread);
    if (thrd_create(&thread.thread, callback, user_data) != thrd_success)
        log_error("thread creation failed");
    if (dvz_mutex_init(&thread.lock) != 0)
        log_error("mutex creation failed");
    thread.lock_idx = dvz_atomic();
    dvz_obj_created(&thread.obj);
    return thread;
}



void dvz_thread_lock(DvzThread* thread)
{
    ASSERT(thread != NULL);
    if (!dvz_obj_is_created(&thread->obj))
        return;
    // The lock idx is used to ensure that nested thread_lock() will work as expected. Only the
    // first lock is effective. Only the last unlock is effective.
    ASSERT(thread->lock_idx != NULL);
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
    ASSERT(thread != NULL);
    if (!dvz_obj_is_created(&thread->obj))
        return;
    ASSERT(thread->lock_idx != NULL);
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
    ASSERT(thread != NULL);
    thrd_join(thread->thread, NULL);
    dvz_mutex_destroy(&thread->lock);
    dvz_atomic_destroy(thread->lock_idx);
    dvz_obj_destroyed(&thread->obj);
}
