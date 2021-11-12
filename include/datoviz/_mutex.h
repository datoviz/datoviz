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

/**
 * Initialize an mutex.
 *
 * @param mutex the mutex to initialize
 */
static inline int dvz_mutex_init(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_init(mutex, 0);
}



/**
 * Lock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_lock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_lock(mutex);
}



/**
 * Unlock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_unlock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_unlock(mutex);
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



#endif
