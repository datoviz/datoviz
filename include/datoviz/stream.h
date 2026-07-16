/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Frame stream API                                                                             */
/*************************************************************************************************/
/* Advanced/unstable low-level frame-stream API for runtime integrations and sinks. */

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
DVZ_EXPORT DvzStreamConfig dvz_stream_config(void);



/**
 * Allocate a stream tied to a Vulkan device and the requested configuration.
 *
 * The stream copies `cfg` and frame descriptors, but borrows the Vulkan device, sink registry,
 * backend descriptors, backend configurations, and Vulkan handles referenced by frames. The
 * registry must remain valid while attachment by name is possible. Backend descriptors and their
 * configuration payloads must remain valid until the corresponding sink is destroyed.
 *
 * @param device the device that owns the stream (may be NULL if not required)
 * @param sink_registry required borrowed registry that holds available sink backends
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
 * @param backend borrowed sink backend descriptor retained until stream destruction
 * @param config borrowed backend-specific configuration retained until sink destruction
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
 * @param config borrowed backend-specific configuration retained until sink destruction
 * @returns 0 on success or -1 when the named backend cannot be found or attached
 */
DVZ_EXPORT int
dvz_stream_attach_sink_name(DvzStream* stream, const char* backend_name, const void* config);



/**
 * Start the stream by providing a frame description and launching every sink.
 *
 * The descriptor is copied, but its Vulkan and operating-system handles remain borrowed and must
 * satisfy the ownership and lifetime contract in `DvzStreamFrame` until updated or stopped.
 *
 * @param stream stream to start
 * @param frame required frame image and synchronization descriptor
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
 * The descriptor is copied. Replaced borrowed handles may be released only after this call has
 * stopped or successfully updated every sink.
 *
 * @param stream active stream
 * @param frame required new frame metadata to apply
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
 * @returns the borrowed device supplied at creation, or NULL when absent
 */
DVZ_EXPORT DvzDevice* dvz_stream_device(DvzStream* stream);



/**
 * Return the configuration currently driving the stream.
 *
 * @param stream stream handle
 * @returns the borrowed internal configuration, valid until stream destruction, or NULL
 */
DVZ_EXPORT const DvzStreamConfig* dvz_stream_get_config(DvzStream* stream);



/**
 * Register a sink backend for later attachment by name or automatic selection.
 *
 * The registry stores the descriptor pointer rather than copying the descriptor or its name.
 * Both must remain valid until the registry is destroyed.
 *
 * @param registry registry that owns the backend database
 * @param backend borrowed backend descriptor with callbacks and a persistent unique name
 */
DVZ_EXPORT void dvz_stream_sink_registry_register(
    DvzStreamSinkRegistry* registry, const DvzStreamSinkBackend* backend);



/**
 * Find a registered sink backend by name.
 *
 * @param registry registry to query
 * @param name backend name to look up
 * @returns a borrowed registered backend descriptor, or NULL when no match is found
 */
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_registry_find(
    DvzStreamSinkRegistry* registry, const char* name);



/**
 * Pick a sink backend by name or probe registered backends automatically.
 *
 * @param registry registry to query
 * @param name requested backend name, "auto", or NULL for automatic selection
 * @param config configuration forwarded to the backend probe callbacks
 * @returns a borrowed registered backend descriptor, or NULL when none are available
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



EXTERN_C_OFF
