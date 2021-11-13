/*************************************************************************************************/
/*  Mutex                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MUTEX
#define DVZ_HEADER_MUTEX



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_macros.h"
#include "_time.h"

#include "tinycthread.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef cnd_t DvzCond;



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

typedef mtx_t DvzMutex;



/*************************************************************************************************/
/*  Mutex functions                                                                              */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Initialize an mutex.
 *
 * @param mutex the mutex to initialize
 */
static inline int dvz_mutex_init(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    // NOTE: thrd_success is 1, not 0 (!?)
    return mtx_init(mutex, 0) != thrd_success;
}



/**
 * Create a mutex.
 *
 * @returns mutex
 */
static inline DvzMutex dvz_mutex()
{
    DvzMutex mutex = {0};
    dvz_mutex_init(&mutex);
    return mutex;
}



/**
 * Lock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_lock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_lock(mutex) != thrd_success;
}



/**
 * Unlock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_unlock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_unlock(mutex) != thrd_success;
}



/**
 * Destroy an mutex.
 *
 * @param mutex the mutex to destroy
 */
static inline void dvz_mutex_destroy(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    mtx_destroy(mutex);
}



/*************************************************************************************************/
/*  Cond functions                                                                               */
/*************************************************************************************************/

/**
 * Initialize a cond.
 *
 * @param cond the cond to initialize
 */
static inline int dvz_cond_init(DvzCond* cond)
{
    ASSERT(cond != NULL);
    return cnd_init(cond);
}



/**
 * Create a cond.
 *
 * @returns cond
 */
static inline DvzCond dvz_cond()
{
    DvzCond cond = {0};
    dvz_cond_init(&cond);
    return cond;
}



/**
 * Signal a cond.
 *
 * @param cond the cond
 */
static inline int dvz_cond_signal(DvzCond* cond)
{
    ASSERT(cond != NULL);
    return cnd_signal(cond);
}



/**
 * Wait until a cond is signaled.
 *
 * @param cond the cond
 */
static inline int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex)
{
    ASSERT(cond != NULL);
    return cnd_wait(cond, mutex);
}



/**
 * Wait until the cond is signaled, or until wait.
 *
 * @param cond the cond
 * @param wait waiting limit
 */
static inline int dvz_cond_timedwait(DvzCond* cond, DvzMutex* mutex, struct timespec* wait)
{
    ASSERT(cond != NULL);
    return cnd_timedwait(cond, mutex, wait);
}



/**
 * Destroy a cond.
 *
 * @param cond the cond
 */
static inline void dvz_cond_destroy(DvzCond* cond)
{
    ASSERT(cond != NULL);
    cnd_destroy(cond);
}



EXTERN_C_OFF

#endif
