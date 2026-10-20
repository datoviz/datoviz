/* SPDX-License-Identifier: MIT */
#pragma once
#include "datoviz/window/input.h"

/**
 * Feed recorded state into the current GUI context without reading OS input.
 * @param event borrowed validated host observation
 */
void _dvz_gui_input_apply(const DvzHostInputEvent* event);

/**
 * Prepare replay GUI metrics/time without GLFW polling or cursor warps.
 * @param window replay-owned host
 * @param previous_frame_ns optional previous processing timestamp, updated in place
 */
void _dvz_gui_input_frame(DvzWindow* window, uint64_t* previous_frame_ns);
