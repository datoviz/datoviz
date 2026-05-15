/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal Dear ImGui overlay                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/gui.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

DvzGui* _dvz_gui_create(DvzGpuCtx* gpu_ctx, DvzWindow* window, const DvzGuiConfig* config);
void _dvz_gui_destroy(DvzGui* gui);
void _dvz_gui_set_callback(DvzGui* gui, DvzGuiCallback callback, void* user_data);
void _dvz_gui_begin_frame(DvzGui* gui, DvzAppWindow* win, const DvzStreamFrame* frame);
void _dvz_gui_render_frame(DvzGui* gui, const DvzStreamFrame* frame);

EXTERN_C_OFF
