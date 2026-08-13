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
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz_testing.h"
#include "../_gui.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiViewportSmoke
{
    DvzGuiViewport* viewport;
    uint32_t frame;
    uint32_t shown_count;
    uint32_t hidden_count;
    uint32_t transition_drawable_count;
    uint32_t visible_transition_count;
    uint32_t visible_transition_shown_count;
} GuiViewportSmoke;



typedef struct GuiInputRecorder
{
    uint32_t count;
    DvzPointerEventType last_type;
} GuiInputRecorder;



typedef struct GuiTestGpuResources
{
    DvzGpuCtx* gpu_ctx;
    DvzWindowHost* window_host;
} GuiTestGpuResources;



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
 * Create selected-GPU resources for a GLFW presentation app smoke.
 *
 * @param suite test context
 * @param[out] resources borrowed app resources to initialize
 * @return NULL on success, otherwise a skip reason
 */
static const char* _gui_test_gpu_resources_create(TstContext* suite, GuiTestGpuResources* resources)
{
    ANN(suite);
    ANN(resources);
    *resources = {};
    resources->window_host = dvz_window_host();
    if (resources->window_host == NULL)
        return "window host creation failed";

    DvzGpuCtxConfig config = dvz_testing_gpu_ctx_config(suite);
    if (dvz_canvas_configure_gpu_ctx(
            resources->window_host, DVZ_BACKEND_GLFW, DVZ_CANVAS_RENDER_MODE_PRESENT, &config) !=
        DVZ_OK)
    {
        dvz_window_host_destroy(resources->window_host);
        resources->window_host = NULL;
        return "GLFW GPU context configuration failed";
    }
    resources->gpu_ctx = dvz_gpu_ctx(&config);
    if (resources->gpu_ctx == NULL)
    {
        dvz_window_host_destroy(resources->window_host);
        resources->window_host = NULL;
        return "GPU context creation failed";
    }
    return NULL;
}



/**
 * Destroy selected-GPU resources after every borrowing app has been destroyed.
 *
 * @param resources resources to destroy
 */
static void _gui_test_gpu_resources_destroy(GuiTestGpuResources* resources)
{
    ANN(resources);
    if (resources->gpu_ctx != NULL)
        dvz_gpu_ctx_destroy(resources->gpu_ctx);
    if (resources->window_host != NULL)
        dvz_window_host_destroy(resources->window_host);
    *resources = {};
}



/**
 * Create an app borrowing the selected GPU context and GLFW window host.
 *
 * @param scene app scene
 * @param config optional app configuration
 * @param resources selected GPU resources
 * @return created app or NULL
 */
static DvzApp* _gui_test_app(
    DvzScene* scene, const DvzAppConfig* config, const GuiTestGpuResources* resources)
{
    ANN(scene);
    ANN(resources);
    DvzAppResources app_resources = dvz_app_resources();
    app_resources.gpu_ctx = resources->gpu_ctx;
    app_resources.window_host = resources->window_host;
    return dvz_app_with_resources(scene, config, &app_resources);
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
    DvzPanel* panel = dvz_panel(figure, &full);
    if (panel == NULL)
        return NULL;
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.04f, 0.05f, 0.07f, 1.0f));
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
    else if (smoke->frame == 2)
    {
        igSetNextWindowSize(ImVec2{260, 220}, ImGuiCond_Always);
        igSetNextWindowCollapsed(false, ImGuiCond_Always);
    }
    else
    {
        igSetNextWindowCollapsed(true, ImGuiCond_Always);
    }

    bool expect_visible = smoke->frame <= 2;
    bool shown = dvz_gui_viewport_window(
        smoke->viewport, "GUI viewport smoke", NULL, ImGuiWindowFlags_NoSavedSettings);
    if (shown)
        smoke->shown_count++;
    else
        smoke->hidden_count++;

    DvzGuiViewportDebugState debug = {};
    if (_dvz_gui_viewport_debug_state(smoke->viewport, &debug) && debug.display_drawable &&
        !debug.display_ready)
    {
        smoke->transition_drawable_count++;
        if (expect_visible)
        {
            smoke->visible_transition_count++;
            if (shown)
                smoke->visible_transition_shown_count++;
        }
    }
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
    AT(config.struct_size == DVZ_STRUCT_SIZE(DvzGuiConfig));
    AT(config.flags == 0);
    AT((config.gui_flags & DVZ_GUI_FLAGS_DOCKING) != 0);
    AT((config.gui_flags & DVZ_GUI_FLAGS_DOCKSPACE) != 0);
    AT(config.default_window_width == 200);
    AT(config.ini_path == NULL);
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
    bool (*range_float)(
        DvzGui*, const char*, float*, float*, float, float, float, const char*) =
        dvz_gui_range_float;
    bool (*color_edit4)(DvzGui*, const char*, float*, int) = dvz_gui_color_edit4;
    bool (*color_edit_dvz)(DvzGui*, const char*, DvzColor*, int) = dvz_gui_color_edit_dvz;
    bool (*color_picker4)(DvzGui*, const char*, float*, int) = dvz_gui_color_picker4;
    void (*separator_text)(DvzGui*, const char*) = dvz_gui_separator_text;
    bool (*collapsing_header)(DvzGui*, const char*, int) = dvz_gui_collapsing_header;
    void (*same_line)(DvzGui*, float, float) = dvz_gui_same_line;
    DvzResult (*dock_window_once)(DvzGui*, const char*, DvzGuiDockSlot, float) =
        dvz_gui_dock_window_once;
    bool (*current_window_docked)(DvzGui*) = dvz_gui_current_window_docked;
    bool (*current_window_rect)(DvzGui*, DvzRect*) = dvz_gui_current_window_rect;

    AT(slider_int != NULL);
    AT(slider_float2 != NULL);
    AT(slider_float3 != NULL);
    AT(slider_float4 != NULL);
    AT(slider_range_float != NULL);
    AT(range_float != NULL);
    AT(color_edit4 != NULL);
    AT(color_edit_dvz != NULL);
    AT(color_picker4 != NULL);
    AT(separator_text != NULL);
    AT(collapsing_header != NULL);
    AT(same_line != NULL);
    AT(dock_window_once != NULL);
    AT(current_window_docked != NULL);
    AT(current_window_rect != NULL);
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

    GuiTestGpuResources gpu_resources = {};
    tst_expect_error_begin(suite);
    const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
    (void)tst_expect_error_end(suite);
    if (gpu_skip != NULL)
    {
        tst_skip(suite, gpu_skip);
        return 0;
    }
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* source_figure = _gui_test_figure(scene, 160, 120);
    DvzFigure* host_figure = _gui_test_figure(scene, 640, 480);
    AT(source_figure != NULL);
    AT(host_figure != NULL);

    DvzApp* app = _gui_test_app(scene, NULL, &gpu_resources);
    if (app == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzView* source_win = dvz_view_offscreen(app, source_figure, 160, 120);
    tst_expect_error_begin(suite);
    DvzView* host_win =
        dvz_view_window(app, host_figure, 640, 480, "test_gui_viewport_resize_hidden_smoke");
    (void)tst_expect_error_end(suite);
    if (source_win == NULL || host_win == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: view creation failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGuiConfig invalid_gui = dvz_gui_config();
    invalid_gui.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    invalid_gui = dvz_gui_config();
    invalid_gui.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GUI creation failed");
        tst_skip(suite, "GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    config.resize_step = 1;
    config.resize_delay_frames = 0;

    AT(dvz_view_resize_scaled(source_win, 160, 120, 2.0f) == 0);
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    dvz_view_logical_size(source_win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(source_win, &framebuffer_width, &framebuffer_height);
    AT(logical_width == 160);
    AT(logical_height == 120);
    AT(framebuffer_width == 320);
    AT(framebuffer_height == 240);
    AC(dvz_view_device_scale(source_win), 2.0f, 1e-6f);

    AT(dvz_view_resize_scaled_xy(source_win, 160, 120, 2.0f, 1.5f) == 0);
    dvz_view_logical_size(source_win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(source_win, &framebuffer_width, &framebuffer_height);
    DvzScaleXY device_scale = dvz_view_device_scale_xy(source_win);
    AT(logical_width == 160);
    AT(logical_height == 120);
    AT(framebuffer_width == 320);
    AT(framebuffer_height == 180);
    AC(device_scale.x, 2.0f, 1e-6f);
    AC(device_scale.y, 1.5f, 1e-6f);

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
    dvz_app_run(app, 6);

    AT(smoke.shown_count > 0);
    AT(smoke.hidden_count > 0);
    AT(smoke.transition_drawable_count > 0);
    AT(smoke.visible_transition_count > 0);
    AT(smoke.visible_transition_shown_count == smoke.visible_transition_count);
    AT(!dvz_view_render_enabled(source_win));

    DvzGuiViewportDebugState debug = {};
    AT(_dvz_gui_viewport_debug_state(smoke.viewport, &debug));
    AT(debug.has_frame);
    AT(debug.image_valid);
    AT(debug.displayed_resource_generation > 0);
    AT(debug.display_drawable);
    AT(debug.display_ready);
    AT(debug.pending_width == 0);
    AT(debug.pending_height == 0);
    AT(debug.displayed_framebuffer_width == debug.requested_framebuffer_width);
    AT(debug.displayed_framebuffer_height == debug.requested_framebuffer_height);

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
    _gui_test_gpu_resources_destroy(&gpu_resources);
    return 0;
}


static int test_gui_config_inherits_app_font_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

    DvzAppConfig app_config = dvz_app_config();
    app_config.font_sans_family = "App Sans";
    app_config.font_sans_style = "Book";
    app_config.font_mono_family = "App Mono";
    app_config.font_ui_size_px = 19.0f;
    app_config.font_mono_size_px = 17.0f;
    app_config.font_text_size_px = 23.0f;

    for (uint32_t i = 0; i < 2; i++)
    {
        GuiTestGpuResources gpu_resources = {};
        tst_expect_error_begin(suite);
        const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
        (void)tst_expect_error_end(suite);
        if (gpu_skip != NULL)
        {
            tst_skip(suite, gpu_skip);
            return 0;
        }

        DvzScene* scene = dvz_scene();
        ANN(scene);
        DvzFigure* figure = _gui_test_figure(scene, 320, 240);
        ANN(figure);

        DvzApp* app = _gui_test_app(scene, &app_config, &gpu_resources);
        if (app == NULL)
        {
            log_warn("test_gui_config_inherits_app_font_defaults skipped: GPU context creation failed");
            tst_skip(suite, "GPU context creation failed");
            dvz_scene_destroy(scene);
            _gui_test_gpu_resources_destroy(&gpu_resources);
            return 0;
        }

        const char* title =
            i == 0 ? "test_gui_null_font_defaults" : "test_gui_explicit_font_defaults";
        tst_expect_error_begin(suite);
        DvzView* win = dvz_view_window(app, figure, 320, 240, title);
        (void)tst_expect_error_end(suite);
        if (win == NULL)
        {
            log_warn("test_gui_config_inherits_app_font_defaults skipped: view creation failed");
            tst_skip(suite, "view creation failed");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            _gui_test_gpu_resources_destroy(&gpu_resources);
            return 0;
        }

        DvzGuiConfig gui_config = dvz_gui_config();
        DvzGui* gui = dvz_view_gui(win, i == 0 ? NULL : &gui_config);
        if (gui == NULL)
        {
            log_warn("test_gui_config_inherits_app_font_defaults skipped: GUI creation failed");
            tst_skip(suite, "GUI creation failed");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            _gui_test_gpu_resources_destroy(&gpu_resources);
            return 0;
        }

        DvzFontDefaults fonts = _dvz_gui_font_defaults(gui);
        AT(strcmp(fonts.sans_family, "App Sans") == 0);
        AT(strcmp(fonts.sans_style, "Book") == 0);
        AT(strcmp(fonts.mono_family, "App Mono") == 0);
        AC(fonts.ui_size_px, 19.0f, 1e-6f);
        AC(fonts.mono_size_px, 17.0f, 1e-6f);
        AC(fonts.text_size_px, 23.0f, 1e-6f);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
    }
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

    GuiTestGpuResources gpu_resources = {};
    tst_expect_error_begin(suite);
    const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
    (void)tst_expect_error_end(suite);
    if (gpu_skip != NULL)
    {
        tst_skip(suite, gpu_skip);
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

    DvzApp* app = _gui_test_app(scene, NULL, &gpu_resources);
    if (app == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    tst_expect_error_begin(suite);
    DvzView* host_win =
        dvz_view_window(app, host_figure, 640, 480, "test_gui_multi_viewport_input_routers");
    (void)tst_expect_error_end(suite);
    if (host_win == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GLFW window creation failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GUI creation failed");
        tst_skip(suite, "GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
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
    DvzCallbackId id_a =
        dvz_input_subscribe_pointer(router_a, _gui_record_pointer, &recorder_a);
    DvzCallbackId id_b =
        dvz_input_subscribe_pointer(router_b, _gui_record_pointer, &recorder_b);

    _gui_emit_pointer(router_a, DVZ_POINTER_EVENT_MOVE);
    AT(recorder_a.count == 1);
    AT(recorder_a.last_type == DVZ_POINTER_EVENT_MOVE);
    AT(recorder_b.count == 0);

    _gui_emit_pointer(router_b, DVZ_POINTER_EVENT_PRESS);
    AT(recorder_a.count == 1);
    AT(recorder_b.count == 1);
    AT(recorder_b.last_type == DVZ_POINTER_EVENT_PRESS);

    dvz_input_unsubscribe(router_a, id_a);
    dvz_input_unsubscribe(router_b, id_b);
    dvz_gui_viewport_destroy(viewport_a);
    dvz_gui_viewport_destroy(viewport_b);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _gui_test_gpu_resources_destroy(&gpu_resources);
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
        _tst_desc.run_flags = TST_RUN_CASE_ADAPTER_SUPPORTED;                                     \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

    TST_CASE(test_gui_imgui_public_header);
    TST_CASE(test_gui_viewport_config_defaults);
    TST_CASE(test_gui_config_font_defaults);
    TST_CASE(test_gui_widget_wrapper_symbols);
    TST_GUI_GPU_CASE(test_gui_config_inherits_app_font_defaults);
    TST_GUI_GPU_CASE(test_gui_viewport_resize_hidden_smoke);
    TST_GUI_GPU_CASE(test_gui_multi_viewport_input_routers);

#undef TST_GUI_GPU_CASE
    return 0;
}
