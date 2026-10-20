/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "datoviz/common/types.h"
#include "datoviz/input/router.h"
#include "datoviz/window/types.h"

/** Original host observations, before GUI arbitration. No derived gestures are accepted here. */
typedef enum DvzHostInputType
{
    DVZ_HOST_INPUT_MOVE = 1,
    DVZ_HOST_INPUT_BUTTON,
    DVZ_HOST_INPUT_WHEEL,
    DVZ_HOST_INPUT_KEY,
    DVZ_HOST_INPUT_TEXT,
    DVZ_HOST_INPUT_FOCUS,
    DVZ_HOST_INPUT_ENTER,
    DVZ_HOST_INPUT_METRICS,
    DVZ_HOST_INPUT_CANCEL,
} DvzHostInputType;

/** Live observations are rejected while replay owns the host. */
typedef enum DvzHostInputOrigin
{
    DVZ_HOST_INPUT_LIVE = 0,
    DVZ_HOST_INPUT_REPLAY = 1,
} DvzHostInputOrigin;

/**
 * Flat, typed host observation, not a serialized structure.
 * Coordinates are native client units; wheel offsets retain the GLFW source convention.
 * Button/key/action/modifier tokens use GLFW conventions (release=0, press=1, repeat=2).
 * logical_key is the observed GUI-layout key token, or zero to use the physical key token.
 * gui_mods is the GUI modifier snapshot; -1 requests fallback to mods.
 * Text is a borrowed UTF-8 span valid only during synchronous emission.
 * Metrics contains the complete resolved native/logical/framebuffer transformation.
 */
typedef struct DvzHostInputEvent
{
    DvzHostInputType type;
    uint64_t timestamp_ns;
    double x;
    double y;
    double dx;
    double dy;
    int32_t button;
    int32_t key;
    int32_t logical_key;
    int32_t gui_mods;
    int32_t scancode;
    int32_t action;
    int32_t mods;
    const char* utf8;
    uint32_t byte_size;
    bool focused;
    bool entered;
    DvzWindowMetrics metrics;
} DvzHostInputEvent;

/** Borrowed owner-thread snapshot; no retained pointers into this structure. */
typedef struct DvzHostInputState
{
    double x;
    double y;
    uint64_t timestamp_ns;
    uint32_t buttons;
    uint32_t held_key_count;
    int32_t mods;
    bool focused;
    bool entered;
    bool replaying;
    bool unsupported_operation;
    double delta_seconds;
} DvzHostInputState;

typedef void (*DvzHostInputCallback)(
    DvzWindow*, const DvzHostInputEvent*, DvzHostInputOrigin, void*);

EXTERN_C_ON

/**
 * Observe originals before GUI consumption on the owning thread.
 * Text/user data are borrowed; copy retained payloads before returning. Removing a subscription
 * suppresses remaining calls immediately; subscriptions added during emission start next time.
 * Destroying the host/window during dispatch is forbidden.
 * @param window borrowed window
 * @param callback synchronous observer
 * @param user_data borrowed callback data
 * @return subscription ID, or zero on failure
 */
DVZ_EXPORT DvzCallbackId
dvz_window_subscribe_host_input(DvzWindow* window, DvzHostInputCallback callback, void* user_data);

/**
 * Remove a host observer before releasing its callback data.
 * @param window borrowed window
 * @param id subscription ID
 * @return whether the observer existed
 */
DVZ_EXPORT bool dvz_window_unsubscribe_host_input(DvzWindow* window, DvzCallbackId id);

/**
 * Emit one validated original through virtual state, observers, GUI and scene routing.
 * All operations are serialized on the owner thread; nested emissions are rejected.
 * Replay emission requires exclusive replay mode; live input is rejected in that mode.
 * @param window borrowed window
 * @param event borrowed original observation
 * @param origin live or replay source
 * @return DVZ_OK, or DVZ_ERROR without dispatch for invalid/unowned input
 */
DVZ_EXPORT DvzResult dvz_window_emit_host_input(
    DvzWindow* window, const DvzHostInputEvent* event, DvzHostInputOrigin origin);

/**
 * Acquire/release exclusive virtual input, cancelling held state without activation.
 * Capture controls must exclude these synthetic cancellation boundaries from recorded segments.
 * @param window borrowed window
 * @param replay enable replay ownership
 * @return DVZ_OK, or DVZ_ERROR during dispatch or for invalid input
 */
DVZ_EXPORT DvzResult dvz_window_input_replay(DvzWindow* window, bool replay);

/**
 * Supply replay GUI processing time before a declared GUI pump.
 * @param window replay-owned window
 * @param timestamp_ns logical gesture/processing time in nanoseconds
 * @param delta_seconds positive controlled GUI step; zero uses elapsed wall time per GUI frame
 * @return DVZ_OK, or DVZ_ERROR for invalid input/non-replay hosts
 */
DVZ_EXPORT DvzResult
dvz_window_input_frame(DvzWindow* window, uint64_t timestamp_ns, double delta_seconds);

/**
 * Query virtual input state, initialized from the host when created.
 * @param window borrowed window
 * @return borrowed snapshot, or NULL
 */
DVZ_EXPORT const DvzHostInputState* dvz_window_host_input_state(const DvzWindow* window);

/**
 * Query queued GUI observations awaiting logical GUI processing.
 * @param window borrowed host window
 * @return queued GUI event count, or zero without a GUI integration
 */
DVZ_EXPORT uint32_t dvz_window_input_pending(DvzWindow* window);

EXTERN_C_OFF
