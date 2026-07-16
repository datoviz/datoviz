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
#include "datoviz/common/types.h"
#include "types.h"



EXTERN_C_ON



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the number of processors on the current system.
 *
 * @return number of processors reported by OpenMP, or zero when OpenMP is unavailable
 */
DVZ_EXPORT int dvz_num_procs(void);



/**
 * Set the number of threads to use in OpenMP-aware functions.
 *
 * Positive values request an absolute count. Nonpositive values are interpreted relative to the
 * processor count (`-1` means all but one processor); the resolved count must be positive.
 *
 * @param num_threads absolute or processor-relative requested thread count
 * @return DVZ_OK on success, DVZ_ERROR on validation error or when OpenMP is unavailable
 */
DVZ_EXPORT DvzResult dvz_threads_set(int num_threads);



/**
 * Get the number of threads to use in OpenMP-aware functions.
 *
 * @return configured OpenMP thread count, or zero before configuration or when OpenMP is unavailable
 */
DVZ_EXPORT int dvz_threads_get(void);



/**
 * Set the number of threads to use in OpenMP-aware functions based on DVZ_NUM_THREADS, or take
 * half of dvz_num_procs().
 *
 * @return DVZ_OK on success, DVZ_ERROR on validation error or when OpenMP is unavailable
 */
DVZ_EXPORT DvzResult dvz_threads_default(void);



EXTERN_C_OFF
