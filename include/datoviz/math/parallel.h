/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Math parallel computing                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "types.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the number of processors on the current system.
 *
 *  @returns the number of processors
 */
DVZ_EXPORT int dvz_num_procs(void);



/**
 * Set the number of threads to use in OpenMP-aware functions.
 *
 *  @param num_threads the requested number of threads
 */
DVZ_EXPORT void dvz_threads_set(int num_threads);



/**
 * Get the number of threads to use in OpenMP-aware functions.
 *
 * @returns the current number of threads specified to OpenMP
 */
DVZ_EXPORT int dvz_threads_get(void);



/**
 * Set the number of threads to use in OpenMP-aware functions based on DVZ_NUM_THREADS, or take
 * half of dvz_num_procs().
 */
DVZ_EXPORT void dvz_threads_default(void);
