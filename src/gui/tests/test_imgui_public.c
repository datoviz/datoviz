/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Raw ImGui public C API tests                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_gui.h"

#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Compile a Datoviz GUI callback that uses raw cimgui calls from the public header.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data opaque user data
 */
static void _gui_raw_imgui_public_callback(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)gui;
    (void)win;
    (void)user_data;

    bool open = true;
    if (igBegin("Public cimgui smoke", &open, 0))
    {
        (void)igButton("raw button", (ImVec2){0.0f, 0.0f});
    }
    igEnd();
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Check that datoviz/imgui.h is usable from C and links raw ig* symbols.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_gui_imgui_public_header(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGuiCallback callback = _gui_raw_imgui_public_callback;
    AT(callback != NULL);

    const char* version = igGetVersion();
    AT(version != NULL);
    AT(version[0] != '\0');
    return 0;
}
