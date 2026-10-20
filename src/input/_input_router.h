/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "datoviz/input/router.h"



/**
 * Acquire one activation of the router's canonical gesture recognizer.
 *
 * @param router borrowed input router
 * @return true on success, false if the activation count would overflow
 */
bool _dvz_input_router_gestures_acquire(DvzInputRouter* router);



/**
 * Release one activation, resetting history when the last activation is removed.
 *
 * @param router borrowed input router with an active recognition handle
 */
void _dvz_input_router_gestures_release(DvzInputRouter* router);

/** Validate a borrowed UTF-8 commit before any host or router dispatch. */
bool _dvz_input_utf8_valid(const char* utf8, uint32_t byte_size);
