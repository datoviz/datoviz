/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**************************************************************************************************

 * DATOVIZ PUBLIC API HEADER FILE
 * ==============================
 * 2024-07-01
 * Cyrille Rossant
 * cyrille dot rossant at gmail com

This file exposes the public API of Datoviz, a C/C++ library for high-performance GPU scientific
visualization.

Datoviz is still an early stage library and the API may change at any time.

**************************************************************************************************/



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_APP
#define DVZ_HEADER_PUBLIC_APP



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "datoviz_keycodes.h"
#include "datoviz_macros.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "datoviz_version.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzBatch DvzBatch;



EXTERN_C_ON


/*************************************************************************************************/
/*************************************************************************************************/
/*  App API                                                                                    */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Create an app.
 *
 * @param flags the app creation flags
 * @returns the app
 */
DVZ_EXPORT DvzApp* dvz_app(int flags);



/**
 * Return the app batch.
 *
 * @param app the app
 * @returns the batch
 */
DVZ_EXPORT DvzBatch* dvz_app_batch(DvzApp* app);



/**
 * Run one frame.
 *
 * @param app the app
 */
DVZ_EXPORT void dvz_app_frame(DvzApp* app);



/**
 * Register a frame callback.
 *
 * @param app the app
 * @param callback the callback
 * @param user_data the user data
 */
DVZ_EXPORT void dvz_app_onframe(DvzApp* app, DvzAppFrameCallback callback, void* user_data);



/**
 * Register a mouse callback.
 *
 * @param app the app
 * @param callback the callback
 * @param user_data the user data
 */
DVZ_EXPORT void dvz_app_onmouse(DvzApp* app, DvzAppMouseCallback callback, void* user_data);



/**
 * Register a keyboard callback.
 *
 * @param app the app
 * @param callback the callback
 * @param user_data the user data
 */
DVZ_EXPORT void dvz_app_onkeyboard(DvzApp* app, DvzAppKeyboardCallback callback, void* user_data);



/**
 * Register a resize callback.
 *
 * @param app the app
 * @param callback the callback
 * @param user_data the user data
 */
DVZ_EXPORT void dvz_app_onresize(DvzApp* app, DvzAppResizeCallback callback, void* user_data);



/**
 * Create a timer.
 *
 * @param app the app
 * @param delay the delay, in seconds, until the first event
 * @param period the period, in seconds, between two events
 * @param max_count the maximum number of events
 * @returns the timer
 */
DVZ_EXPORT DvzTimerItem*
dvz_app_timer(DvzApp* app, double delay, double period, uint64_t max_count);



/**
 * Register a timer callback.
 *
 * @param app the app
 * @param callback the timer callback
 * @param user_data the user data
 */
DVZ_EXPORT void dvz_app_ontimer(DvzApp* app, DvzAppTimerCallback callback, void* user_data);



/**
 * Register a GUI callback.
 *
 * @param app the app
 * @param canvas_id the canvas ID
 * @param callback the GUI callback
 * @param user_data the user data
 */
DVZ_EXPORT void
dvz_app_gui(DvzApp* app, DvzId canvas_id, DvzAppGuiCallback callback, void* user_data);



/**
 * Start the application event loop.
 *
 * @param app the app
 * @param n_frames the maximum number of frames, 0 for infinite loop
 */
DVZ_EXPORT void dvz_app_run(DvzApp* app, uint64_t n_frames);



/**
 * Submit the current batch to the application.
 *
 * @param app the app
 */
DVZ_EXPORT void dvz_app_submit(DvzApp* app);



/**
 * Make a screenshot of a canvas.
 *
 * @param app the app
 * @param canvas_id the ID of the canvas
 * @param filename the path to the PNG file with the screenshot
 */
DVZ_EXPORT void dvz_app_screenshot(DvzApp* app, DvzId canvas_id, const char* filename);



/**
 * Return the precise display timestamps of the last `count` frames.
 *
 * @param app the app
 * @param canvas_id the ID of the canvas
 * @param count number of frames
 * @param[out] seconds a buffer holding at least `count` uint64_t values (seconds)
 * @param[out] nanoseconds a buffer holding at least `count` uint64_t values (nanoseconds)
 */
DVZ_EXPORT void dvz_app_timestamps(
    DvzApp* app, DvzId canvas_id, uint32_t count, uint64_t* seconds, uint64_t* nanoseconds);



/**
 * Wait until the GPU has finished processing.
 *
 * @param app the app
 */
DVZ_EXPORT void dvz_app_wait(DvzApp* app);



/**
 * Destroy the app.
 *
 * @param app the app
 */
DVZ_EXPORT void dvz_app_destroy(DvzApp* app);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Get the current time.
 *
 * @param[out] time fill a structure with seconds and nanoseconds integers
 */
DVZ_EXPORT void dvz_time(DvzTime* time);


/**
 * Display a time.
 *
 * @param time a time structure
 */
DVZ_EXPORT void dvz_time_print(DvzTime* time);



/**
 * Return the last mouse position and pressed button.
 *
 * @param app the app
 * @param canvas_id the canvas id
 * @param[out] x a pointer to the mouse x position
 * @param[out] y a pointer to the mouse y position
 * @param[out] button a pointer to the pressed button
 */
DVZ_EXPORT void
dvz_app_mouse(DvzApp* app, DvzId canvas_id, double* x, double* y, DvzMouseButton* button);



/**
 * Return the last keyboard key pressed.
 *
 * @param app the app
 * @param canvas_id the canvas id
 * @param[out] key a pointer to the last pressed key
 */
DVZ_EXPORT void dvz_app_keyboard(DvzApp* app, DvzId canvas_id, DvzKeyCode* key);



/**
 * Free a pointer.
 *
 * @param pointer a pointer
 */
DVZ_EXPORT void dvz_free(void* pointer);



EXTERN_C_OFF

#endif
