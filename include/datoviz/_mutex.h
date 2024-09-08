/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mutex                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MUTEX
#define DVZ_HEADER_MUTEX



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include "tinycthread.h"
#include <pthread.h>

#include "_macros.h"
#include "_time.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// typedef tct_cnd_t DvzCond;
typedef pthread_cond_t DvzCond;



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

// typedef tct_mtx_t DvzMutex;
typedef pthread_mutex_t DvzMutex;



EXTERN_C_ON

/*************************************************************************************************/
/*  Mutex functions                                                                              */
/*************************************************************************************************/

/**
 * Initialize an mutex.
 *
 * @param mutex the mutex to initialize
 */
int dvz_mutex_init(DvzMutex* mutex);



/**
 * Create a mutex.
 *
 * @returns mutex
 */
DvzMutex dvz_mutex(void);



/**
 * Lock a mutex.
 *
 * @param mutex the mutex
 */
int dvz_mutex_lock(DvzMutex* mutex);



/**
 * Unlock a mutex.
 *
 * @param mutex the mutex
 */
int dvz_mutex_unlock(DvzMutex* mutex);



/**
 * Destroy an mutex.
 *
 * @param mutex the mutex to destroy
 */
void dvz_mutex_destroy(DvzMutex* mutex);



/*************************************************************************************************/
/*  Cond functions                                                                               */
/*************************************************************************************************/

/**
 * Initialize a cond.
 *
 * @param cond the cond to initialize
 */
int dvz_cond_init(DvzCond* cond);



/**
 * Create a cond.
 *
 * @returns cond
 */
DvzCond dvz_cond(void);


/**
 * Signal a cond.
 *
 * @param cond the cond
 */
int dvz_cond_signal(DvzCond* cond);



/**
 * Wait until a cond is signaled.
 *
 * @param cond the cond
 */
int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex);



/**
 * Wait until the cond is signaled, or until wait.
 *
 * @param cond the cond
 * @param wait waiting limit
 */
int dvz_cond_timedwait(DvzCond* cond, DvzMutex* mutex, struct timespec* wait);



/**
 * Destroy a cond.
 *
 * @param cond the cond
 */
void dvz_cond_destroy(DvzCond* cond);



EXTERN_C_OFF

#endif
