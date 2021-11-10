/*************************************************************************************************/
/*  Mutex                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MUTEX
#define DVZ_HEADER_MUTEX



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_macros.h"



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
static inline int dvz_mutex_init(DvzMutex* mutex) { return pthread_mutex_init(mutex, 0); }



/**
 * Lock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_lock(DvzMutex* mutex) { return pthread_mutex_lock(mutex); }



/**
 * Unlock a mutex.
 *
 * @param mutex the mutex
 */
static inline int dvz_mutex_unlock(DvzMutex* mutex) { return pthread_mutex_unlock(mutex); }



/**
 * Destroy an mutex.
 *
 * @param mutex the mutex to destroy
 */
static inline int dvz_mutex_destroy(DvzMutex* mutex) { return pthread_mutex_destroy(mutex); }



#endif
