/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App interop                                                                                  */
/*************************************************************************************************/
/* Opt-in native interop helpers for hosted Vulkan surfaces and toolkit integrations. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/app.h"
#include "datoviz/vk/vulkan.h"
#include "datoviz/window/backend.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  App Vulkan interop                                                                           */
/*************************************************************************************************/

/**
 * Return the Vulkan instance owned by the app.
 *
 * Hosted integrations use this borrowed handle to create their native VkSurfaceKHR after passing
 * required instance extensions to dvz_app_with_config().
 *
 * @param app the app
 * @return borrowed Vulkan instance handle, or VK_NULL_HANDLE when unavailable
 */
DVZ_EXPORT VkInstance dvz_app_vk_instance(DvzApp* app);



/*************************************************************************************************/
/*  Hosted surface views                                                                         */
/*************************************************************************************************/

/**
 * Create a hosted present view around an externally-owned Vulkan surface.
 *
 * The caller owns the native event loop and must create the Vulkan surface using the instance
 * extensions passed to dvz_app_with_config().  Datoviz owns only the rendering objects built on
 * top of the supplied surface unless surface->owned_by_datoviz is true.
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param surface external Vulkan surface description
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView* dvz_view_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Update the hosted external surface associated with a view.
 *
 * Use this when the host toolkit recreates or resizes its native surface.  A NULL surface handle is
 * accepted to mark the surface temporarily unavailable; rendering then returns
 * DVZ_CANVAS_FRAME_WAIT_SURFACE until a valid surface is supplied again.
 *
 * @param view view created with dvz_view_external_surface()
 * @param surface external Vulkan surface description
 * @return 0 on success, negative on error
 */
DVZ_EXPORT int dvz_view_update_external_surface(
    DvzView* view, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Release a hosted external surface before the host destroys it.
 *
 * Clears the request-frame callback, marks the surface temporarily unavailable, and runs one
 * render-once step so the present swapchain observes the unavailable surface and releases borrowed
 * surface-dependent objects. The host remains responsible for destroying the VkSurfaceKHR.
 *
 * @param view view created with dvz_view_external_surface()
 * @return DVZ_CANVAS_FRAME_WAIT_SURFACE on clean release, or a negative error code
 */
DVZ_EXPORT int dvz_view_release_external_surface(DvzView* view);



EXTERN_C_OFF
