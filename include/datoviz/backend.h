/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Backend                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BACKEND
#define DVZ_HEADER_BACKEND



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_macros.h"
#include "window.h"

#ifndef HAS_GLFW
#define HAS_GLFW 0
#endif
#if HAS_GLFW
#include <GLFW/glfw3.h>
#endif



EXTERN_C_ON

/*************************************************************************************************/
/*  Backend                                                                                      */
/*************************************************************************************************/

void dvz_backend_init(DvzBackend backend);

const char**
dvz_backend_required_extensions(DvzBackend backend, uint32_t* required_extension_count);

void dvz_backend_terminate(DvzBackend backend);

void* dvz_backend_window(DvzBackend backend, uint32_t width, uint32_t height, int flags);

void dvz_backend_poll_events(DvzBackend backend);

void dvz_backend_wait(DvzBackend backend);

void dvz_backend_window_clear_callbacks(DvzBackend backend, void* bwin);

void dvz_backend_window_destroy(DvzBackend backend, void* bwin);

void dvz_backend_set_window_size(DvzWindow* window, uint32_t width, uint32_t height);

void dvz_backend_get_window_size(
    DvzWindow* window, uint32_t* window_width, uint32_t* window_height);

void dvz_backend_get_framebuffer_size(
    DvzWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height);

bool dvz_backend_should_close(DvzWindow* window);

void dvz_backend_loop(DvzWindow* window, uint64_t max_frames);



EXTERN_C_OFF

#endif
