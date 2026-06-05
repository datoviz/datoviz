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
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"



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
 * @param win view
 * @param user_data GUI viewport smoke state
 */
static void _gui_viewport_resize_callback(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)gui;
    (void)win;
    GuiViewportSmoke* smoke = (GuiViewportSmoke*)user_data;
    ANN(smoke);

    if (igBegin("Raw cimgui smoke", NULL, 0))
    {
        igTextUnformatted("ig* raw calls are available", NULL);
        (void)igButton("raw button", ImVec2{0, 0});
    }
    igEnd();

    if (smoke->frame == 0)
    {
        igSetNextWindowSize(ImVec2{360, 260}, ImGuiCond_Always);
        igSetNextWindowCollapsed(false, ImGuiCond_Always);
    }
    else if (smoke->frame == 1)
    {
        igSetNextWindowSize(ImVec2{260, 220}, ImGuiCond_Always);
        igSetNextWindowCollapsed(false, ImGuiCond_Always);
    }
    else
    {
        igSetNextWindowCollapsed(true, ImGuiCond_Always);
    }

    bool shown = dvz_gui_viewport_window(
        smoke->viewport, "GUI viewport smoke", NULL, ImGuiWindowFlags_NoSavedSettings);
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
static int test_gui_viewport_config_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    AT(config.struct_size == DVZ_STRUCT_SIZE(DvzGuiViewportConfig));
    AT(config.flags == 0);
    AT((config.viewport_flags & DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT) != 0);
    AT((config.viewport_flags & DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN) == 0);
    AT(config.initial_width == 640);
    AT(config.initial_height == 480);
    AT(config.min_width > 0);
    AT(config.min_height > 0);
    AT(config.resize_step > 0);
    AT(config.resize_delay_frames > 0);
    return 0;
}


/**
 * Check GUI overlay default font policy.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_config_font_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGuiConfig config = dvz_gui_config();
    DvzFontDefaults defaults = dvz_font_defaults();
    AT(config.struct_size == DVZ_STRUCT_SIZE(DvzGuiConfig));
    AT(config.flags == 0);
    AT((config.gui_flags & DVZ_GUI_FLAGS_DOCKING) != 0);
    AT((config.gui_flags & DVZ_GUI_FLAGS_DOCKSPACE) != 0);
    AT(config.ini_path == NULL);
    AT(strcmp(config.font_defaults.sans.family, defaults.sans.family) == 0);
    AT(strcmp(config.font_defaults.sans.style, defaults.sans.style) == 0);
    AT(strcmp(config.font_defaults.mono.family, defaults.mono.family) == 0);
    AT(config.font_defaults.ui_size_px == defaults.ui_size_px);
    AT(config.font_defaults.mono_size_px == defaults.mono_size_px);
    AT(config.font_defaults.text_size_px == defaults.text_size_px);
    return 0;
}



/**
 * Check that the curated GUI widget wrappers are exported with C-callable signatures.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_widget_wrapper_symbols(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    bool (*slider_int)(DvzGui*, const char*, int*, int, int) = dvz_gui_slider_int;
    bool (*slider_float2)(DvzGui*, const char*, float*, float, float) = dvz_gui_slider_float2;
    bool (*slider_float3)(DvzGui*, const char*, float*, float, float) = dvz_gui_slider_float3;
    bool (*slider_float4)(DvzGui*, const char*, float*, float, float) = dvz_gui_slider_float4;
    bool (*slider_range_float)(
        DvzGui*, const char*, float*, float*, float, float, const char*) =
        dvz_gui_slider_range_float;
    bool (*slider_range_double)(
        DvzGui*, const char*, double*, double*, double, double, const char*) =
        dvz_gui_slider_range_double;
    bool (*range_float)(
        DvzGui*, const char*, float*, float*, float, float, float, const char*) =
        dvz_gui_range_float;
    bool (*color_edit4)(DvzGui*, const char*, float*, int) = dvz_gui_color_edit4;
    bool (*color_edit_dvz)(DvzGui*, const char*, DvzColor*, int) = dvz_gui_color_edit_dvz;
    bool (*color_picker4)(DvzGui*, const char*, float*, int) = dvz_gui_color_picker4;
    void (*separator_text)(DvzGui*, const char*) = dvz_gui_separator_text;
    bool (*collapsing_header)(DvzGui*, const char*, int) = dvz_gui_collapsing_header;
    void (*same_line)(DvzGui*, float, float) = dvz_gui_same_line;

    AT(slider_int != NULL);
    AT(slider_float2 != NULL);
    AT(slider_float3 != NULL);
    AT(slider_float4 != NULL);
    AT(slider_range_float != NULL);
    AT(slider_range_double != NULL);
    AT(range_float != NULL);
    AT(color_edit4 != NULL);
    AT(color_edit_dvz != NULL);
    AT(color_picker4 != NULL);
    AT(separator_text != NULL);
    AT(collapsing_header != NULL);
    AT(same_line != NULL);
    return 0;
}



/**
 * Smoke viewport resize handling and hidden-window render gating.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_viewport_resize_hidden_smoke(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

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
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzView* source_win = dvz_view_offscreen(app, source_figure, 160, 120);
    DvzView* host_win =
        dvz_view_glfw(app, host_figure, 640, 480, "test_gui_viewport_resize_hidden_smoke");
    if (source_win == NULL || host_win == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: view creation failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGuiConfig invalid_gui = dvz_gui_config();
    invalid_gui.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    invalid_gui = dvz_gui_config();
    invalid_gui.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    invalid_gui = dvz_gui_config();
    invalid_gui.font_defaults.sans.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GUI creation failed");
        tst_skip(suite, "GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    config.resize_step = 1;
    config.resize_delay_frames = 0;
    DvzGuiViewportConfig invalid_viewport = config;
    invalid_viewport.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_gui_viewport_from_window(gui, source_win, &invalid_viewport) == NULL);

    invalid_viewport = config;
    invalid_viewport.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_gui_viewport_from_window(gui, source_win, &invalid_viewport) == NULL);

    GuiViewportSmoke smoke = {};
    smoke.viewport = dvz_gui_viewport_from_window(gui, source_win, &config);
    AT(smoke.viewport != NULL);

    dvz_view_set_gui_callback(host_win, _gui_viewport_resize_callback, &smoke);
    dvz_app_run(app, 4);

    AT(smoke.shown_count > 0);
    AT(smoke.hidden_count > 0);
    AT(!dvz_view_render_enabled(source_win));

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(dvz_view_canvas(source_win), &width, &height, &rgba) == 0);
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
static int test_gui_multi_viewport_input_routers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

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
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzView* host_win =
        dvz_view_glfw(app, host_figure, 640, 480, "test_gui_multi_viewport_input_routers");
    if (host_win == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GLFW window creation failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GUI creation failed");
        tst_skip(suite, "GUI creation failed");
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
    TST_MODULE(suite, tags);

#define TST_GUI_GPU_CASE(test)                                                                   \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN | TST_RES_GLFW;          \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

    TST_CASE(test_gui_imgui_public_header);
    TST_CASE(test_gui_viewport_config_defaults);
    TST_CASE(test_gui_config_font_defaults);
    TST_CASE(test_gui_widget_wrapper_symbols);
    TST_GUI_GPU_CASE(test_gui_viewport_resize_hidden_smoke);
    TST_GUI_GPU_CASE(test_gui_multi_viewport_input_routers);

#undef TST_GUI_GPU_CASE
    return 0;
}
