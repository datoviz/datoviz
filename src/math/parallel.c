/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common mathematical macros                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "datoviz/math/parallel.h"



/*************************************************************************************************/
/*  OpenMP                                                                                       */
/*************************************************************************************************/

static int NUM_THREADS;

int dvz_num_procs(void)
{
#if DVZ_HAS_OPENMP
    return omp_get_num_procs();
#else
    return 0;
#endif
}



DvzResult dvz_threads_set(int num_threads)
{
#if DVZ_HAS_OPENMP
    int num_procs = dvz_num_procs();
    if (num_procs <= 0)
        return DVZ_ERROR;
    if (num_threads <= 0)
    {
        num_threads += num_procs;
    }
    num_threads = DVZ_MIN(num_threads, num_procs);
    if (num_threads < 1 || num_threads > num_procs)
        return DVZ_ERROR;
    log_info("setting the number of OpenMP threads to %d/%d", num_threads, num_procs);
    NUM_THREADS = num_threads;
    omp_set_num_threads(num_threads);
    return DVZ_OK;
#else
    (void)num_threads;
    return DVZ_ERROR;
#endif
}



int dvz_threads_get(void)
{
#if DVZ_HAS_OPENMP
    return NUM_THREADS;
#else
    return 0;
#endif
}



DvzResult dvz_threads_default(void)
{
#if DVZ_HAS_OPENMP
    // Set number of threads from DVZ_NUM_THREADS env variable.
    char* env = getenv("DVZ_NUM_THREADS");
    if (env == NULL)
    {
        int n = dvz_num_procs();
        n = DVZ_MAX(1, n / 2);
        ASSERT(1 <= n);
        return dvz_threads_set(n);
    }
    else
    {
        int num_threads = getenvint("DVZ_NUM_THREADS");
        return dvz_threads_set(num_threads);
    }
#else
    return DVZ_ERROR;
#endif
}
