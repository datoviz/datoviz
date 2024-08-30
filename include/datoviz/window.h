/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_WINDOW
#define DVZ_HEADER_WINDOW



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_WINDOW_FLAGS_NONE = 0x0000,
    DVZ_WINDOW_FLAGS_HIDDEN = 0x1000,
} DvzWindowFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWindow DvzWindow;

// Forward declarations.
typedef struct DvzInput DvzInput;
typedef struct DvzClient DvzClient;
typedef struct DvzGuiWindow DvzGuiWindow;


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindow
{
    DvzObject obj;

    DvzBackend backend;
    void* backend_window;
    uint32_t width, height;                         // screen size
    uint32_t framebuffer_width, framebuffer_height; // framebuffer size
    DvzGuiWindow* gui_window;
    bool is_captured; // true when ImGui is processing user events

    // Forward pointer.
    DvzInput* input;
    DvzClient* client;
};



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

/**
 * Create a blank window.
 *
 * This function is rarely used on its own. A bare window offers
 * no functionality that allows one to render to it with Vulkan. One needs a swapchain, an event
 * loop, and so on, which are provided instead at the level of the Canvas.
 *
 * @param host the host
 * @param width the window width, in pixels
 * @param height the window height, in pixels
 * @returns the window
 */
DvzWindow dvz_window(DvzBackend backend, uint32_t width, uint32_t height, int flags);



/**
 * Get the window size.
 *
 * @param window the window
 */
void dvz_window_poll_size(DvzWindow* window);



/**
 * Set the window size, in pixels.
 *
 * @param window the window
 * @param width the width, in pixels
 * @param height the height, in pixels
 */
void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height);



/**
 * Destroy a window.
 *
 * !!! warning
 *     This function must be imperatively called *after* `dvz_swapchain_destroy()`.
 *
 * @param window the window
 */
void dvz_window_destroy(DvzWindow* window);



#endif
