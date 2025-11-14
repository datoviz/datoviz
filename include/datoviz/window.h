/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "datoviz/input/router.h"
#include "datoviz/window/backend.h"
#include "datoviz/window/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Window host                                                                                  */
/*************************************************************************************************/

DVZ_EXPORT DvzWindowHost* dvz_window_host(void);

DVZ_EXPORT void dvz_window_host_destroy(DvzWindowHost* host);

DVZ_EXPORT void
dvz_window_host_register_backend(DvzWindowHost* host, const DvzWindowBackend* backend);

DVZ_EXPORT void dvz_window_host_poll(DvzWindowHost* host);

DVZ_EXPORT void dvz_window_host_request_frame(DvzWindowHost* host, DvzWindow* window);



/*************************************************************************************************/
/*  Window instances                                                                             */
/*************************************************************************************************/

DVZ_EXPORT DvzWindowConfig dvz_window_default_config(void);

DVZ_EXPORT DvzWindow*
dvz_window_create(DvzWindowHost* host, DvzBackend backend, const DvzWindowConfig* config);

DVZ_EXPORT void dvz_window_destroy(DvzWindow* window);

DVZ_EXPORT const DvzWindowSurface* dvz_window_surface(const DvzWindow* window);

DVZ_EXPORT DvzInputRouter* dvz_window_router(DvzWindow* window);

DVZ_EXPORT void dvz_window_set_user_data(DvzWindow* window, void* user_data);

DVZ_EXPORT void* dvz_window_user_data(const DvzWindow* window);

DVZ_EXPORT bool dvz_window_frame_pending(const DvzWindow* window);

DVZ_EXPORT DvzBackend dvz_window_backend_type(const DvzWindow* window);

EXTERN_C_OFF
