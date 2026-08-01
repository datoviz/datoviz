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
#include "datoviz/common/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef enum
{
    DVZ_LOG_LEVEL_ERROR = 0,
    DVZ_LOG_LEVEL_WARNING = 1,
    DVZ_LOG_LEVEL_INFO = 2,
} DvzLogLevel;


// Error callback function type.
typedef void (*DvzErrorCallback)(DvzLogLevel level, const char* message, void* user_data);



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON


/**
 * Register or clear the process-wide error callback.
 *
 * The callback and borrowed user data must remain valid until replaced or cleared. Passing NULL
 * for @p cb clears the callback.
 *
 * @param cb callback invoked for Datoviz errors, or NULL to clear the current callback
 * @param user_data borrowed opaque pointer passed unchanged to @p cb; may be NULL
 * @return DVZ_OK on success
 */
DVZ_EXPORT DvzResult dvz_error_set_callback(DvzErrorCallback cb, void* user_data);


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
 * Close an owned platform external handle.
 *
 * On Unix the value is treated as a file descriptor. On Windows it is treated as a native
 * `HANDLE` without exposing `windows.h` in the public API. Passing
 * `DVZ_EXTERNAL_HANDLE_INVALID` is allowed.
 *
 * @param handle owned platform external handle
 */
DVZ_EXPORT void dvz_external_handle_close(DvzExternalHandle handle);


/**
 * Return a monotonic timestamp in nanoseconds.
 *
 * @return monotonic timestamp in nanoseconds
 */
DVZ_EXPORT uint64_t dvz_time_monotonic_ns(void);



EXTERN_C_OFF
