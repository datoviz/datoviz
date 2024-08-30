/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Threading utilities                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_THREAD
#define DVZ_HEADER_THREAD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include "tinycthread.h"

#include "_atomic.h"
#include "_macros.h"
#include "_mutex.h"
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
    // tct_thrd_t thread;
    pthread_t thread;
    DvzMutex lock;
    DvzAtomic lock_idx; // used to allow nested callbacks and avoid deadlocks: only 1 lock
};



EXTERN_C_ON

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
DvzThread* dvz_thread(DvzThreadCallback callback, void* user_data);



/**
 * Acquire a mutex lock associated to the thread.
 *
 * @param thread the thread
 */
void dvz_thread_lock(DvzThread* thread);



/**
 * Release a mutex lock associated to the thread.
 *
 * @param thread the thread
 */
void dvz_thread_unlock(DvzThread* thread);



/**
 * Destroy a thread after the thread function has finished running.
 *
 * @param thread the thread
 */
void dvz_thread_join(DvzThread* thread);



EXTERN_C_OFF

#endif
