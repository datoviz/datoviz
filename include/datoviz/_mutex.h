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

#if USE_PTHREAD
#include <pthread.h>
#endif



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCond DvzCond;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCond
{
#if USE_PTHREAD
    pthread_cond_t cond;
    // pthread_mutex_t mutex;
#else
    // TODO
#endif
};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#if USE_PTHREAD
#define DvzMutex pthread_mutex_t
#else
#define DvzMutex TODO
#endif



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
#if USE_PTHREAD
    return pthread_mutex_init(mutex, 0);
#endif
}



/**
 * Lock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_lock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
#if USE_PTHREAD
    return pthread_mutex_lock(mutex);
#endif
}



/**
 * Unlock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_unlock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
#if USE_PTHREAD
    return pthread_mutex_unlock(mutex);
#endif
}



/**
 * Destroy an mutex.
 *
 * @param mutex the mutex to destroy
 */
static inline int dvz_mutex_destroy(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
#if USE_PTHREAD
    return pthread_mutex_destroy(mutex);
#endif
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
#if USE_PTHREAD
    return pthread_cond_init(&cond->cond, 0);
#endif
}



/**
 * Signal a cond.
 *
 * @param cond the cond
 */
static inline int dvz_cond_signal(DvzCond* cond)
{
    ASSERT(cond != NULL);
#if USE_PTHREAD
    pthread_cond_signal(&cond->cond);
#endif
}



/**
 * Wait until a cond is signaled.
 *
 * @param cond the cond
 */
static inline int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex)
{
    ASSERT(cond != NULL);
#if USE_PTHREAD
    pthread_cond_wait(&cond->cond, mutex);
#endif
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
#if USE_PTHREAD
    return pthread_cond_timedwait(&cond->cond, mutex, wait);
#endif
}



/**
 * Destroy a cond.
 *
 * @param cond the cond
 */
static inline int dvz_cond_destroy(DvzCond* cond)
{
    ASSERT(cond != NULL);
#if USE_PTHREAD
    pthread_cond_destroy(&cond->cond);
#endif
}



#endif
