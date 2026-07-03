/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Common functions                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Error callback function type.
typedef void (*DvzErrorCallback)(const char* message);



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON


/**
 * Register an error callback, a C function taking as input a string.
 *
 * @param cb the error callback
 */
DVZ_EXPORT void dvz_error_callback(DvzErrorCallback cb);


/**
 * Release memory returned by Datoviz public APIs.
 *
 * Use this function for owned buffers returned through public API calls such as file loading,
 * shader compilation, screenshots, and readbacks. Passing NULL is allowed.
 *
 * @param pointer pointer returned by a Datoviz public API, or NULL
 */
DVZ_EXPORT void dvz_memory_free(void* pointer);


/**
 * Return a monotonic timestamp in nanoseconds.
 *
 * @return monotonic timestamp in nanoseconds
 */
DVZ_EXPORT uint64_t dvz_time_monotonic_ns(void);



EXTERN_C_OFF
