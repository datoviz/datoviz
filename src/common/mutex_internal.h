/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal mutex helpers                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <pthread.h>

#include "datoviz/common/macros.h"

struct timespec;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef pthread_cond_t DvzCond;

typedef pthread_mutex_t DvzMutex;



EXTERN_C_ON

/*************************************************************************************************/
/*  Mutex functions                                                                              */
/*************************************************************************************************/

int dvz_mutex_init(DvzMutex* mutex);

DvzMutex dvz_mutex(void);

int dvz_mutex_lock(DvzMutex* mutex);

int dvz_mutex_unlock(DvzMutex* mutex);

void dvz_mutex_destroy(DvzMutex* mutex);



/*************************************************************************************************/
/*  Cond functions                                                                               */
/*************************************************************************************************/

int dvz_cond_init(DvzCond* cond);

DvzCond dvz_cond(void);

int dvz_cond_signal(DvzCond* cond);

int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex);

void dvz_cond_destroy(DvzCond* cond);



EXTERN_C_OFF
