/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Frame stream API                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/stream/frame_stream.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Stream API                                                                                   */
/*************************************************************************************************/

/**
 * Return a default stream configuration (1920×1080 @ 60 FPS, RGBA8).
 *
 * @returns a configuration initialized with common output parameters
 */
DVZ_EXPORT DvzStreamConfig dvz_stream_default_config(void);



/**
 * Allocate a stream tied to a Vulkan device and the requested configuration.
 *
 * @param device the device that owns the stream (may be NULL if not required)
 * @param sink_registry registry that holds available sink backends
 * @param cfg optional configuration, falls back to the default when NULL
 * @returns a new stream handle or NULL when allocation fails
 */
DVZ_EXPORT DvzStream* dvz_stream_create(
    DvzDevice* device, DvzStreamSinkRegistry* sink_registry, const DvzStreamConfig* cfg);



/**
 * Stop the stream, detach sinks, and free the associated resources.
 *
 * @param stream stream handle to destroy (NULL-safe)
 */
DVZ_EXPORT void dvz_stream_destroy(DvzStream* stream);



/**
 * Attach a backend sink to a stream before it starts streaming.
 *
 * @param stream target stream
 * @param backend sink backend descriptor with callbacks
 * @param config backend-specific configuration passed through to the backend
 * @returns 0 on success or -1 if the backend is invalid, the stream is running,
 *           the backend probe fails, or the backend creation fails
 */
DVZ_EXPORT int
dvz_stream_attach_sink(DvzStream* stream, const DvzStreamSinkBackend* backend, const void* config);



/**
 * Look up a registered sink backend by name and attach it to the stream.
 *
 * @param stream target stream
 * @param backend_name backend name (use "auto" or NULL for automatic selection)
 * @param config backend-specific configuration passed through to the backend
 * @returns 0 on success or -1 when the named backend cannot be found or attached
 */
DVZ_EXPORT int
dvz_stream_attach_sink_name(DvzStream* stream, const char* backend_name, const void* config);



/**
 * Start the stream by providing a frame description and launching every sink.
 *
 * @param stream stream to start
 * @param frame description of the frame image and synchronization data (required)
 * @returns 0 on success or -1 if the stream is already running, lacks a frame,
 *           or a sink fails to start
 */
DVZ_EXPORT int dvz_stream_start(DvzStream* stream, const DvzStreamFrame* frame);



/**
 * Submit the current frame to all sinks, forwarding the timeline wait value.
 *
 * @param stream active stream
 * @param wait_value timeline semaphore value sinks should wait for
 * @returns 0 when every sink accepts the submission or the first non-zero sink error
 */
DVZ_EXPORT int dvz_stream_submit(DvzStream* stream, uint64_t wait_value);



/**
 * Update the frame description while the stream is running, restarting sinks if needed.
 *
 * @param stream active stream
 * @param frame new frame metadata to apply
 * @returns 0 on success or a sink error code (negative when a restart fails)
 */
DVZ_EXPORT int dvz_stream_update(DvzStream* stream, const DvzStreamFrame* frame);



/**
 * Stop the stream and all attached sinks.
 *
 * @param stream stream to stop (NULL-safe)
 * @returns 0 when the stream is stopped or already idle
 */
DVZ_EXPORT int dvz_stream_stop(DvzStream* stream);



/**
 * Return the Vulkan device associated with the stream.
 *
 * @param stream stream handle
 * @returns the owning device or NULL when the stream is NULL
 */
DVZ_EXPORT DvzDevice* dvz_stream_device(DvzStream* stream);



/**
 * Return the configuration currently driving the stream.
 *
 * @param stream stream handle
 * @returns pointer to the configuration or NULL when the stream is NULL
 */
DVZ_EXPORT const DvzStreamConfig* dvz_stream_config(DvzStream* stream);



/**
 * Register a sink backend for later attachment by name or automatic selection.
 *
 * @param registry registry that owns the backend database
 * @param backend backend descriptor with callbacks and a unique name
 */
DVZ_EXPORT void dvz_stream_sink_registry_register(
    DvzStreamSinkRegistry* registry, const DvzStreamSinkBackend* backend);



/**
 * Find a registered sink backend by name.
 *
 * @param registry registry to query
 * @param name backend name to look up
 * @returns the backend descriptor or NULL when no match is found
 */
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_registry_find(
    DvzStreamSinkRegistry* registry, const char* name);



/**
 * Pick a sink backend by name or probe registered backends automatically.
 *
 * @param registry registry to query
 * @param name requested backend name, "auto", or NULL for automatic selection
 * @param config configuration forwarded to the backend probe callbacks
 * @returns the selected backend or NULL when none are available
 */
DVZ_EXPORT const DvzStreamSinkBackend*
dvz_stream_sink_registry_pick(
    DvzStreamSinkRegistry* registry, const char* name, const void* config);


/**
 * Allocate a stream sink registry instance.
 *
 * @returns allocated registry or NULL on failure
 */
DVZ_EXPORT DvzStreamSinkRegistry* dvz_stream_sink_registry_create(void);


/**
 * Destroy a stream sink registry and free its internal storage.
 *
 * @param registry registry to destroy (NULL-safe)
 */
DVZ_EXPORT void dvz_stream_sink_registry_destroy(DvzStreamSinkRegistry* registry);


/**
 * Return the shared, lazily initialized sink registry.
 *
 * @returns global registry instance (never NULL after the first call)
 */
DVZ_EXPORT DvzStreamSinkRegistry* dvz_stream_sink_registry_default(void);


/**
 * Destroy the shared registry created by dvz_stream_sink_registry_default().
 *
 * @note Only use when you want to tear down the global state (e.g., tests).
 */
DVZ_EXPORT void dvz_stream_sink_registry_default_destroy(void);



EXTERN_C_OFF
