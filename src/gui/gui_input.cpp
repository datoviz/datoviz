/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */
#include "datoviz/window.h"
#include "gui_input_internal.h"
#define GLFW_INCLUDE_NONE
#include "imgui.h"
#include <GLFW/glfw3.h>

// The bundled backend exposes this pure mapping separately from its OS-polling callbacks.
ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode, int scancode);

/**
 * Feed one validated native observation into the current GUI context without OS queries.
 * @param event borrowed validated host observation
 */
void _dvz_gui_input_apply(const DvzHostInputEvent* event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type)
    {
    case DVZ_HOST_INPUT_MOVE:
        io.AddMousePosEvent((float)event->x, (float)event->y);
        break;
    case DVZ_HOST_INPUT_BUTTON:
        io.AddMousePosEvent((float)event->x, (float)event->y);
        {
            int mods = event->gui_mods >= 0 ? event->gui_mods : event->mods;
            io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
            io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
            io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
            io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
        }
        if (event->button < 5)
            io.AddMouseButtonEvent(event->button, event->action == GLFW_PRESS);
        break;
    case DVZ_HOST_INPUT_WHEEL:
        io.AddMousePosEvent((float)event->x, (float)event->y);
        io.AddMouseWheelEvent((float)event->dx, (float)event->dy);
        break;
    case DVZ_HOST_INPUT_KEY:
    {
        int mods = event->gui_mods >= 0 ? event->gui_mods : event->mods;
        io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
        if (event->action != GLFW_REPEAT)
        {
            int token = event->logical_key != 0 ? event->logical_key : event->key;
            ImGuiKey key = ImGui_ImplGlfw_KeyToImGuiKey(token, event->scancode);
            if (key != ImGuiKey_None)
            {
                io.AddKeyEvent(key, event->action == GLFW_PRESS);
                io.SetKeyEventNativeData(key, token, event->scancode);
            }
        }
        break;
    }
    case DVZ_HOST_INPUT_TEXT:
    {
        // Decode the validated borrowed span without requiring a trailing NUL or allocation.
        uint32_t i = 0;
        while (i < event->byte_size)
        {
            unsigned char first = (unsigned char)event->utf8[i++];
            uint32_t value = first;
            int extra = 0;
            if (first >= 0xf0)
            {
                value = first & 7;
                extra = 3;
            }
            else if (first >= 0xe0)
            {
                value = first & 15;
                extra = 2;
            }
            else if (first >= 0xc0)
            {
                value = first & 31;
                extra = 1;
            }
            for (int j = 0; j < extra; j++)
                value = (value << 6) | ((unsigned char)event->utf8[i++] & 63);
            io.AddInputCharacter(value);
        }
        break;
    }
    case DVZ_HOST_INPUT_FOCUS:
        io.AddFocusEvent(event->focused);
        break;
    case DVZ_HOST_INPUT_ENTER:
        if (!event->entered)
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        break;
    default:
        break;
    }
}



/**
 * Populate replay GUI metrics/time, retaining GLFW presentation without input polling.
 * @param window replay-owned host
 * @param previous_frame_ns optional previous actual GUI processing timestamp, updated in place
 */
void _dvz_gui_input_frame(DvzWindow* window, uint64_t* previous_frame_ns)
{
    ImGuiIO& io = ImGui::GetIO();
    const DvzHostInputState* state = dvz_window_host_input_state(window);
    const DvzWindowMetrics* metrics = dvz_window_metrics(window);
    io.DisplaySize = ImVec2((float)metrics->native_size.width, (float)metrics->native_size.height);
    io.DisplayFramebufferScale =
        ImVec2(metrics->framebuffer_scale.x, metrics->framebuffer_scale.y);
    double delta = state->delta_seconds;
    uint64_t now = state->timestamp_ns;
    if (delta == 0)
    {
        // Paced replay uses elapsed time at actual GUI processing, including scheduler waits.
        now = dvz_input_timestamp_ns();
        delta = previous_frame_ns != NULL && *previous_frame_ns > 0 && now > *previous_frame_ns
                    ? (double)(now - *previous_frame_ns) * 1e-9
                    : 1.0 / 60.0;
    }
    if (previous_frame_ns != NULL)
        *previous_frame_ns = now;
    io.DeltaTime = (float)delta;
}
