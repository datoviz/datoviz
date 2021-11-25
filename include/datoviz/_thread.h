/*************************************************************************************************/
/*  Threading utilities                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_THREAD
#define DVZ_HEADER_THREAD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_atomic.h"
#include "_macros.h"
#include "_mutex.h"
#include "_obj.h"

MUTE_ON
#include "tinycthread.h"
MUTE_OFF



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzThread DvzThread;

typedef int (*DvzThreadCallback)(void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzThread
{
    DvzObject obj;
    thrd_t thread;
    DvzMutex lock;
    DvzAtomic lock_idx; // used to allow nested callbacks and avoid deadlocks: only 1 lock
};



/*************************************************************************************************/
/*  Thread functions                                                                             */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Create a thread.
 *
 * Callback function signature: `void*(void*)`
 *
 * @param callback the function that will run in a background thread
 * @param user_data a pointer to arbitrary user data
 * @returns thread object
 */
static inline DvzThread dvz_thread(DvzThreadCallback callback, void* user_data)
{
    DvzThread thread = {0};
    if (thrd_create(&thread.thread, callback, user_data) != thrd_success)
        log_error("thread creation failed");
    if (dvz_mutex_init(&thread.lock) != 0)
        log_error("mutex creation failed");
    thread.lock_idx = dvz_atomic();
    dvz_obj_created(&thread.obj);
    return thread;
}



/**
 * Acquire a mutex lock associated to the thread.
 *
 * @param thread the thread
 */
static inline void dvz_thread_lock(DvzThread* thread)
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



/**
 * Release a mutex lock associated to the thread.
 *
 * @param thread the thread
 */
static inline void dvz_thread_unlock(DvzThread* thread)
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



/**
 * Destroy a thread after the thread function has finished running.
 *
 * @param thread the thread
 */
static inline void dvz_thread_join(DvzThread* thread)
{
    ASSERT(thread != NULL);
    thrd_join(thread->thread, NULL);
    dvz_mutex_destroy(&thread->lock);
    dvz_atomic_destroy(thread->lock_idx);
    dvz_obj_destroyed(&thread->obj);
}

EXTERN_C_OFF


#endif
