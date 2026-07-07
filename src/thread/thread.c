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

#include <pthread.h>
#include <stddef.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "obj.h"
#include "thread_internal.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzThread
{
    DvzObject obj;
    pthread_t thread;
};



/*************************************************************************************************/
/*  Thread functions                                                                             */
/*************************************************************************************************/

DvzThread* dvz_thread(DvzThreadCallback callback, void* user_data)
{
    DvzThread* thread = (DvzThread*)dvz_calloc(1, sizeof(DvzThread));
    ANN(thread);
    // log_trace("creating thread");
    if (pthread_create(&thread->thread, NULL, callback, user_data))
        log_error("thread creation failed");
    dvz_obj_created(&thread->obj);
    return thread;
}


void dvz_thread_join(DvzThread* thread)
{
    ANN(thread);
    // log_trace("joining thread");
    pthread_join(thread->thread, NULL);
    dvz_obj_destroyed(&thread->obj);
    dvz_free(thread);
}
