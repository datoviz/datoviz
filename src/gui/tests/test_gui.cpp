/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GUI tests                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_gui.h"

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/gui.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "imgui.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiViewportSmoke
{
    DvzGuiViewport* viewport;
    uint32_t frame;
    uint32_t shown_count;
    uint32_t hidden_count;
} GuiViewportSmoke;



typedef struct GuiInputRecorder
{
    uint32_t count;
    DvzPointerEventType last_type;
} GuiInputRecorder;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a GUI smoke can use the active GPU/GLFW path.
 *
 * @return whether the smoke can run
 */
static bool _gui_smoke_available(void)
{
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI && defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
    return true;
#else
    return false;
#endif
}



/**
 * Create a minimal figure with a full-size panel.
 *
 * @param scene scene that owns the figure
 * @param width figure width
 * @param height figure height
 * @return created figure, or NULL on failure
 */
static DvzFigure* _gui_test_figure(DvzScene* scene, uint32_t width, uint32_t height)
{
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, width, height, 0);
    if (figure == NULL)
        return NULL;

    DvzPanelDesc full = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, full);
    if (panel == NULL)
        return NULL;
    dvz_panel_set_background_color(panel, 0.04f, 0.05f, 0.07f, 1.0f);
    return figure;
}



/**
 * Build a viewport window with changing size and collapsed state.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data GUI viewport smoke state
 */
static void _gui_viewport_resize_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)gui;
    (void)win;
    GuiViewportSmoke* smoke = (GuiViewportSmoke*)user_data;
    ANN(smoke);

    if (smoke->frame == 0)
    {
        ImGui::SetNextWindowSize(ImVec2(360, 260), ImGuiCond_Always);
    }
    else if (smoke->frame == 1)
    {
        ImGui::SetNextWindowSize(ImVec2(260, 220), ImGuiCond_Always);
    }
    else
    {
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Always);
    }

    bool shown = dvz_gui_viewport_window(smoke->viewport, "GUI viewport smoke", NULL, 0);
    if (shown)
        smoke->shown_count++;
    else
        smoke->hidden_count++;
    smoke->frame++;
}



/**
 * Record a pointer event received by a viewport input router.
 *
 * @param router input router
 * @param event pointer event
 * @param user_data input recorder
 */
static void _gui_record_pointer(
    DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    GuiInputRecorder* recorder = (GuiInputRecorder*)user_data;
    ANN(recorder);
    recorder->count++;
    recorder->last_type = event->type;
}



/**
 * Emit one pointer event directly through a GUI viewport input router.
 *
 * @param router input router
 * @param type pointer event type
 */
static void _gui_emit_pointer(DvzInputRouter* router, DvzPointerEventType type)
{
    ANN(router);
    dvz_pointer_emit_position(
        router, type, 10.0f, 20.0f, 100.0f, 80.0f, DVZ_POINTER_BUTTON_NONE,
        DVZ_KEY_MODIFIER_SHIFT, 1.0f, dvz_input_timestamp_ns(), NULL);
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Check GUI viewport default configuration values.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_viewport_config_defaults(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    AT((config.flags & DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT) != 0);
    AT((config.flags & DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN) == 0);
    AT(config.initial_width == 640);
    AT(config.initial_height == 480);
    AT(config.min_width > 0);
    AT(config.min_height > 0);
    AT(config.resize_step > 0);
    AT(config.resize_delay_frames > 0);
    return 0;
}



/**
 * Smoke viewport resize handling and hidden-window render gating.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_viewport_resize_hidden_smoke(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* source_figure = _gui_test_figure(scene, 160, 120);
    DvzFigure* host_figure = _gui_test_figure(scene, 640, 480);
    AT(source_figure != NULL);
    AT(host_figure != NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzAppWindow* source_win = dvz_app_window(app, source_figure, 160, 120);
    DvzAppWindow* host_win =
        dvz_app_window_glfw(app, host_figure, 640, 480, "test_gui_viewport_resize_hidden_smoke");
    if (source_win == NULL || host_win == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: app-window creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGui* gui = dvz_app_window_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    config.resize_step = 1;
    config.resize_delay_frames = 0;
    GuiViewportSmoke smoke = {};
    smoke.viewport = dvz_gui_viewport_from_window(gui, source_win, &config);
    AT(smoke.viewport != NULL);

    dvz_app_window_set_gui_callback(host_win, _gui_viewport_resize_callback, &smoke);
    dvz_app_run(app, 4);

    AT(smoke.shown_count > 0);
    AT(smoke.hidden_count > 0);
    AT(!dvz_app_window_render_enabled(source_win));

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(dvz_app_window_canvas(source_win), &width, &height, &rgba) == 0);
    AT(width > 0);
    AT(height > 0);
    dvz_free(rgba);

    dvz_gui_viewport_destroy(smoke.viewport);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Smoke independent input routers for multiple GUI viewports.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_multi_viewport_input_routers(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* source_a = _gui_test_figure(scene, 160, 120);
    DvzFigure* source_b = _gui_test_figure(scene, 160, 120);
    DvzFigure* host_figure = _gui_test_figure(scene, 640, 480);
    AT(source_a != NULL);
    AT(source_b != NULL);
    AT(host_figure != NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzAppWindow* host_win =
        dvz_app_window_glfw(app, host_figure, 640, 480, "test_gui_multi_viewport_input_routers");
    if (host_win == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GLFW window creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGui* gui = dvz_app_window_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    config.initial_width = 160;
    config.initial_height = 120;
    DvzGuiViewport* viewport_a = dvz_gui_viewport(gui, source_a, &config);
    DvzGuiViewport* viewport_b = dvz_gui_viewport(gui, source_b, &config);
    AT(viewport_a != NULL);
    AT(viewport_b != NULL);

    DvzInputRouter* router_a = dvz_gui_viewport_input(viewport_a);
    DvzInputRouter* router_b = dvz_gui_viewport_input(viewport_b);
    AT(router_a != NULL);
    AT(router_b != NULL);
    AT(router_a != router_b);

    GuiInputRecorder recorder_a = {};
    GuiInputRecorder recorder_b = {};
    dvz_input_subscribe_pointer(router_a, _gui_record_pointer, &recorder_a);
    dvz_input_subscribe_pointer(router_b, _gui_record_pointer, &recorder_b);

    _gui_emit_pointer(router_a, DVZ_POINTER_EVENT_MOVE);
    AT(recorder_a.count == 1);
    AT(recorder_a.last_type == DVZ_POINTER_EVENT_MOVE);
    AT(recorder_b.count == 0);

    _gui_emit_pointer(router_b, DVZ_POINTER_EVENT_PRESS);
    AT(recorder_a.count == 1);
    AT(recorder_b.count == 1);
    AT(recorder_b.last_type == DVZ_POINTER_EVENT_PRESS);

    dvz_input_unsubscribe_pointer(router_a, _gui_record_pointer, &recorder_a);
    dvz_input_unsubscribe_pointer(router_b, _gui_record_pointer, &recorder_b);
    dvz_gui_viewport_destroy(viewport_a);
    dvz_gui_viewport_destroy(viewport_b);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Register GUI tests.
 *
 * @param suite test suite
 * @return 0 on success
 */
int test_gui(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "gui";
    TEST_SIMPLE(test_gui_viewport_config_defaults);
    TEST_SIMPLE(test_gui_viewport_resize_hidden_smoke);
    TEST_SIMPLE(test_gui_multi_viewport_input_routers);
    return 0;
}
