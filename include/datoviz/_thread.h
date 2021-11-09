/*************************************************************************************************/
/*  Threading utilities                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_THREAD
#define DVZ_HEADER_THREAD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_obj.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzThread DvzThread;

typedef void* (*DvzThreadCallback)(void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzThread
{
    DvzObject obj;
    pthread_t thread;
    pthread_mutex_t lock;
    atomic(int, lock_idx); // used to allow nested callbacks and avoid deadlocks: only 1 lock
};



/*************************************************************************************************/
/*  Thread functions                                                                             */
/*************************************************************************************************/

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
    if (pthread_create(&thread.thread, NULL, callback, user_data) != 0)
        log_error("thread creation failed");
    if (pthread_mutex_init(&thread.lock, NULL) != 0)
        log_error("mutex creation failed");
    atomic_init(&thread.lock_idx, 0);
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
    int lock_idx = atomic_load(&thread->lock_idx);
    ASSERT(lock_idx >= 0);
    if (lock_idx == 0)
    {
        log_trace("acquire lock");
        pthread_mutex_lock(&thread->lock);
    }
    atomic_store(&thread->lock_idx, lock_idx + 1);
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
    int lock_idx = atomic_load(&thread->lock_idx);
    ASSERT(lock_idx >= 0);
    if (lock_idx == 1)
    {
        log_trace("release lock");
        pthread_mutex_unlock(&thread->lock);
    }
    if (lock_idx >= 1)
        atomic_store(&thread->lock_idx, lock_idx - 1);
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
    pthread_join(thread->thread, NULL);
    pthread_mutex_destroy(&thread->lock);
    dvz_obj_destroyed(&thread->obj);
}



#endif
