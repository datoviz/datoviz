/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene app tests                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../_scene.h"
#include "../_technique.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/window/backend.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP

typedef struct
{
    uint32_t calls;
    double last_t;
    double last_dt;
    double total_dt;
} AppTimerProbe;


typedef struct
{
    uint32_t calls;
    DvzAppWindow* last_window;
} AppRequestFrameProbe;


typedef struct
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzVisual* visual;
} AppSsaoQuad;



/**
 * Record one app-driven timer callback.
 *
 * @param animation animation handle
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time
 * @param user_data timer probe storage
 */
static void _app_timer_probe_callback(
    DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    AppTimerProbe* probe = (AppTimerProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_t = t;
    probe->last_dt = dt;
    probe->total_dt += dt;
}


/**
 * Record one app-window request-frame callback.
 *
 * @param win app-window requesting a frame
 * @param user_data request-frame probe storage
 */
static void _app_request_frame_probe_callback(DvzAppWindow* win, void* user_data)
{
    AppRequestFrameProbe* probe = (AppRequestFrameProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_window = win;
}



/**
 * Add one indexed quad mesh to the panel used by SSAO offscreen tests.
 *
 * @param scene scene owner
 * @param panel destination panel
 * @param xmin minimum X coordinate
 * @param xmax maximum X coordinate
 * @param ymin minimum Y coordinate
 * @param ymax maximum Y coordinate
 * @param z clip-depth-like scene coordinate
 * @param color per-vertex color
 * @return created quad handles
 */
static AppSsaoQuad _app_ssao_add_quad(
    DvzScene* scene, DvzPanel* panel, float xmin, float xmax, float ymin, float ymax, float z,
    DvzColor color)
{
    ANN(scene);
    ANN(panel);

    AppSsaoQuad out = {.scene = scene, .panel = panel};
    out.visual = dvz_mesh(scene, 0);
    if (out.visual == NULL)
        return out;

    float positions[4][3] = {
        {xmin, ymin, z},
        {xmax, ymin, z},
        {xmin, ymax, z},
        {xmax, ymax, z},
    };
    DvzColor colors[4] = {0};
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    for (uint32_t i = 0; i < 4; i++)
        dvz_memcpy(colors[i], sizeof(DvzColor), color, sizeof(DvzColor));

    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL)
        return out;
    if (!dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)))
        return out;

    if (dvz_visual_set_data(out.visual, "position", positions, 4) != 0 ||
        dvz_visual_set_data(out.visual, "color", colors, 4) != 0 ||
        dvz_visual_set_data(out.visual, "normal", normals, 4) != 0 ||
        !dvz_visual_set_buffer(out.visual, "index", index_buffer) ||
        dvz_panel_add_visual(panel, out.visual, NULL) != 0)
    {
        out.visual = NULL;
        return out;
    }
    return out;
}



/**
 * Sum the RGB luminance of a captured image.
 *
 * @param rgba captured RGBA8 buffer
 * @param pixel_count number of pixels
 * @return RGB luminance sum
 */
static uint64_t _app_rgb_sum(const uint8_t* rgba, uint32_t pixel_count)
{
    ANN(rgba);
    uint64_t sum = 0;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        const uint8_t* px = &rgba[4 * i];
        sum += (uint64_t)px[0] + (uint64_t)px[1] + (uint64_t)px[2];
    }
    return sum;
}



#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
/**
 * Return an external-surface description for a GLFW-hosted app test.
 *
 * @param instance Vulkan instance used to create the surface
 * @param surface borrowed Vulkan surface handle
 * @param width framebuffer width in pixels
 * @param height framebuffer height in pixels
 * @return external surface description
 */
static DvzWindowExternalSurfaceInfo _app_glfw_surface_info(
    VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height)
{
    DvzWindowExternalSurfaceInfo info = {0};
    info.instance = instance;
    info.surface = surface;
    info.extent.width = width;
    info.extent.height = height;
    info.scale_x = 1.0f;
    info.scale_y = 1.0f;
    info.owned_by_datoviz = false;
    return info;
}



#endif



/**
 * Create a minimal scene/figure/panel used by app timer integration tests.
 *
 * @param out_figure destination for the created figure handle
 * @return scene handle, or NULL on failure
 */
static DvzScene* _app_timer_test_scene(DvzFigure** out_figure)
{
    ANN(out_figure);
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return NULL;

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return NULL;
    }
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
    {
        dvz_scene_destroy(scene);
        return NULL;
    }
    (void)panel;

    *out_figure = figure;
    return scene;
}



int test_app_offscreen(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Build scene */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f};
    uint8_t colors[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 20.0f, 15.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Create app and offscreen window */
    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    AT(dvz_app_vk_instance(app) != VK_NULL_HANDLE);
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    dvz_app_window_request_frame(win);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(dvz_app_window_emit_resize(win, 64, 64, 64, 64, 1.0f, 1.0f) == 0);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);

    /* Exercise host-driven and Datoviz-owned frame paths. */
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_render_once(app) == 0);
    dvz_app_run(app, 1);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_timer_advances_in_app_run(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 4.0);
    AppTimerProbe probe = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _app_timer_probe_callback, &probe);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_timer_advances_in_app_run skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 2);

    AT(probe.calls == 2);
    AC(probe.last_t, 0.25, EPS);
    AC(probe.last_dt, 0.25, EPS);
    AC(probe.total_dt, 0.25, EPS);
    AC(dvz_scene_clock_time(scene), 0.25, EPS);
    AC(dvz_scene_clock_dt(scene), 0.25, EPS);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_timer_advances_in_render_once(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 8.0);
    AppTimerProbe probe = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _app_timer_probe_callback, &probe);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_timer_advances_in_render_once skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);

    AT(probe.calls == 2);
    AC(probe.last_t, 0.125, EPS);
    AC(probe.last_dt, 0.125, EPS);
    AC(probe.total_dt, 0.125, EPS);
    AC(dvz_scene_clock_time(scene), 0.125, EPS);
    AC(dvz_scene_clock_dt(scene), 0.125, EPS);

    dvz_anim_stop(timer);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 2);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_render_enabled_gate(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 8.0);
    AppTimerProbe timer_probe = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _app_timer_probe_callback, &timer_probe);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_render_enabled_gate skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    AT(dvz_app_window_render_enabled(win));

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);

    dvz_app_window_set_render_enabled(win, false);
    AT(!dvz_app_window_render_enabled(win));
    AT(dvz_app_window_render_once(win) == 0);
    AT(request_probe.calls == 0);
    AT(timer_probe.calls == 0);
    AC(dvz_scene_clock_time(scene), 0.0, EPS);

    dvz_app_window_set_render_enabled(win, true);
    AT(dvz_app_window_render_enabled(win));
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(timer_probe.calls == 1);
    AC(dvz_scene_clock_time(scene), 0.0, EPS);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
int test_app_external_surface_release_waits(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    if (!dvz_window_glfw_init())
    {
        log_warn("test_app_external_surface_release_waits skipped: GLFW could not initialize");
        return 0;
    }

    uint32_t extension_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    if (extension_count == 0 || extensions == NULL)
    {
        log_warn(
            "test_app_external_surface_release_waits skipped: GLFW returned no Vulkan extensions");
        glfwTerminate();
        return 0;
    }

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    DvzAppConfig app_cfg = dvz_app_config();
    app_cfg.instance_extension_count = extension_count;
    app_cfg.instance_extensions = extensions;
    app_cfg.enable_canvas_extensions = true;
    app_cfg.enable_glfw_extensions = false;
    DvzApp* app = dvz_app_with_config(scene, &app_cfg);
    if (app == NULL)
    {
        log_warn("test_app_external_surface_release_waits skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 0;
    }

    VkInstance instance = dvz_app_vk_instance(app);
    AT(instance != VK_NULL_HANDLE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* glfw_window =
        glfwCreateWindow(64, 64, "test_app_external_surface_release_waits", NULL, NULL);
    if (glfw_window == NULL)
    {
        log_warn("test_app_external_surface_release_waits skipped: GLFW window creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 0;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult surface_res = glfwCreateWindowSurface(instance, glfw_window, NULL, &surface);
    if (surface_res != VK_SUCCESS || surface == VK_NULL_HANDLE)
    {
        log_warn(
            "test_app_external_surface_release_waits skipped: surface creation failed (%d)",
            (int)surface_res);
        glfwDestroyWindow(glfw_window);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 0;
    }

    DvzWindowExternalSurfaceInfo surface_info =
        _app_glfw_surface_info(instance, surface, 64, 64);
    DvzAppWindow* win = dvz_app_window_external_surface(app, figure, &surface_info);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);

    AT(dvz_app_window_release_external_surface(win) == DVZ_CANVAS_FRAME_WAIT_SURFACE);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_WAIT_SURFACE);
    AT(dvz_app_render_once(app) == DVZ_CANVAS_FRAME_WAIT_SURFACE);

    vkDestroySurfaceKHR(instance, surface, NULL);
    glfwDestroyWindow(glfw_window);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    glfwTerminate();
    return 0;
}
#endif


int test_app_offscreen_panel_three_visuals_all_drawn(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    AT(panel != NULL);

    /* Three non-overlapping points: red (left), green (center), blue (right). */
    float pos_r[3] = {-0.6f, 0.0f, 0.0f};
    float pos_g[3] = { 0.0f, 0.0f, 0.0f};
    float pos_b[3] = { 0.6f, 0.0f, 0.0f};
    DvzColor red   = {220, 20, 20, 255};
    DvzColor green = {20, 220, 20, 255};
    DvzColor blue  = {20, 20, 220, 255};
    float size = 10.0f;

    DvzVisual* vr = dvz_point(scene, 0);
    DvzVisual* vg = dvz_point(scene, 0);
    DvzVisual* vb = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color",    &red,  1) == 0);
    AT(dvz_visual_set_data(vr, "size",     &size, 1) == 0);
    AT(dvz_visual_set_data(vg, "position", pos_g, 1) == 0);
    AT(dvz_visual_set_data(vg, "color",    &green, 1) == 0);
    AT(dvz_visual_set_data(vg, "size",     &size, 1) == 0);
    AT(dvz_visual_set_data(vb, "position", pos_b, 1) == 0);
    AT(dvz_visual_set_data(vb, "color",    &blue, 1) == 0);
    AT(dvz_visual_set_data(vb, "size",     &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, vr, NULL) == 0);
    AT(dvz_panel_add_visual(panel, vg, NULL) == 0);
    AT(dvz_panel_add_visual(panel, vb, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_panel_three_visuals_all_drawn skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t red_count = 0, green_count = 0, blue_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);

        red_count = green_count = blue_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* px = &rgba[4 * i];
            if (px[0] > 150 && px[0] > px[1] + 80 && px[0] > px[2] + 80)
                red_count++;
            if (px[1] > 150 && px[1] > px[0] + 80 && px[1] > px[2] + 80)
                green_count++;
            if (px[2] > 150 && px[2] > px[0] + 80 && px[2] > px[1] + 80)
                blue_count++;
        }
        dvz_free(rgba);
        if (red_count > 0 && green_count > 0 && blue_count > 0)
            break;
    }
    AT(red_count > 0);
    AT(green_count > 0);
    AT(blue_count > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure overlapping point visuals use depth testing in a normal non-EDL pass.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_point_depth_orders_overlap(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* near_visual = dvz_point(scene, 0);
    DvzVisual* far_visual = dvz_point(scene, 0);
    AT(near_visual != NULL);
    AT(far_visual != NULL);

    float near_pos[3] = {0.0f, 0.0f, 0.1f};
    float far_pos[3] = {0.0f, 0.0f, 0.8f};
    DvzColor near_color = {32, 64, 255, 255};
    DvzColor far_color = {255, 32, 32, 255};
    float size = 36.0f;

    AT(dvz_visual_set_data(near_visual, "position", near_pos, 1) == 0);
    AT(dvz_visual_set_data(near_visual, "color", &near_color, 1) == 0);
    AT(dvz_visual_set_data(near_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, near_visual, NULL) == 0);

    AT(dvz_visual_set_data(far_visual, "position", far_pos, 1) == 0);
    AT(dvz_visual_set_data(far_visual, "color", &far_color, 1) == 0);
    AT(dvz_visual_set_data(far_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, far_visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_depth_orders_overlap skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(center[2] > 180);
    AT(center[2] > center[0] + 40);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure point depth cueing darkens farther points in an offscreen render.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_point_depth_cue_darkens_far(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[2][3] = {{-0.45f, 0.0f, 0.0f}, {0.45f, 0.0f, 0.8f}};
    DvzColor colors[2] = {{255, 64, 64, 255}, {255, 64, 64, 255}};
    float sizes[2] = {20.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_visual_set_depth_cue(
           visual,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
               .falloff = DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL,
               .near_depth = 0.50f,
               .far_depth = 1.0f,
               .strength = 1.0f,
               .density = 3.0f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_depth_cue_darkens_far skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* near_px = _pixel_at(rgba, width, height, width / 4, height / 2);
    const uint8_t* far_px = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
    AT(near_px[0] > 180);
    AT(far_px[0] + 80 < near_px[0]);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Build scene with ONE large yellow point at center. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255}; /* yellow */
    float size = 32.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    /* Count pixels with r>200 && g>200 (yellow-ish from the point). */
    uint32_t yellow_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200)
            yellow_count++;
    }
    AT(yellow_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure pixel visuals render nonblank square marks through the offscreen app path.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_app_offscreen_pixel_square_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_pixel(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 0, 255, 255};
    float size = 18.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_pixel_square_has_nonblank_pixels skipped: GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t magenta_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[2] > 200)
            magenta_count++;
    }
    AT(magenta_count > 0);

    const uint8_t* corner = _pixel_at(rgba, width, height, 40, 40);
    AT(corner[0] > 200 && corner[2] > 200);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure panel EDL renders an offscreen point scene through the app runtime path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_points_edl_renders(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);
    float positions[4][3] = {
        {-0.25f, -0.15f, -0.20f},
        {+0.20f, -0.05f, +0.15f},
        {-0.05f, +0.20f, +0.35f},
        {+0.18f, +0.18f, -0.35f},
    };
    DvzColor colors[4] = {
        {255, 90, 80, 255},
        {80, 220, 130, 255},
        {80, 140, 255, 255},
        {240, 220, 80, 255},
    };
    float sizes[4] = {28.0f, 30.0f, 26.0f, 24.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);
    AT(dvz_panel_set_edl(
        panel, &(DvzEdlDesc){.radius = 2.0f, .strength = 65.0f, .depth_scale = 1.0f}));

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_points_edl_renders skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);
    AT(dvz_panel_set_edl(panel, NULL));
    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t lit_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* px = &rgba[4 * i];
        if (px[0] > 40 || px[1] > 40 || px[2] > 40)
            lit_count++;
    }
    AT(lit_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure SSAO visibly darkens an offscreen mesh scene versus the same scene without SSAO.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_mesh_ssao_changes_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzColor back_color = {188, 196, 205, 255};
    DvzColor front_color = {224, 150, 92, 255};
    AppSsaoQuad back =
        _app_ssao_add_quad(scene, panel, -0.82f, +0.82f, -0.72f, +0.72f, 0.65f, back_color);
    AppSsaoQuad front =
        _app_ssao_add_quad(scene, panel, -0.22f, +0.52f, -0.28f, +0.46f, 0.25f, front_color);
    AT(back.visual != NULL);
    AT(front.visual != NULL);
    dvz_panel_set_background_color(panel, 0.03f, 0.035f, 0.045f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_mesh_ssao_changes_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);
    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 96);
    AT(height0 == 96);

    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 3.0f, .strength = 8.0f, .bias = 0.0f,
                            .sample_count = 16}));
    dvz_app_run(app, 1);
    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);
    AT(width1 == width0);
    AT(height1 == height0);

    uint32_t darkened_count = 0;
    uint32_t changed_count = 0;
    const uint32_t pixel_count = width0 * height0;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        const uint8_t* a = &rgba0[4 * i];
        const uint8_t* b = &rgba1[4 * i];
        int lum0 = (int)a[0] + (int)a[1] + (int)a[2];
        int lum1 = (int)b[0] + (int)b[1] + (int)b[2];
        if (lum0 != lum1)
            changed_count++;
        if (lum0 > 80 && lum1 + 24 < lum0)
            darkened_count++;
    }
    AT(changed_count > 0);
    AT(darkened_count > 8);
    AT(_app_rgb_sum(rgba1, pixel_count) < _app_rgb_sum(rgba0, pixel_count));

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_records_dvzr_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 24.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_records_dvzr_frames skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    const char* path = "/tmp/dvz_app_offscreen_recording.dvzr";
    AT(dvz_app_window_record_start(win, path) == 0);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_window_record_stop(win) == 0);

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    ANN(recording);
    AT(dvz_drp2_recording_frame_count(recording) == 3);
    AT(dvz_drp2_recording_raw_fallback_count(recording) == 0);
    const DvzDrp2CommandStream* stream = dvz_drp2_recording_stream(recording);
    ANN(stream);
    AT(dvz_drp2_stream_count(stream) > 0);

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_recording_execute_all(recording, runtime);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_recording_close(recording);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* Large quad covering most of the panel. TRIANGLE_STRIP order: TL, BL, TR, BR */
    float positions[4][3] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };

    /* Solid red 4x4 texture. */
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[i * 4 + 0] = 255;
        pixels[i * 4 + 1] = 0;
        pixels[i * 4 + 2] = 0;
        pixels[i * 4 + 3] = 255;
    }

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    /* Count pixels that are red-dominant (from the solid red texture). */
    uint32_t red_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50)
            red_count++;
    }
    AT(red_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure batched text renders visible pixels through the offscreen app path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_text_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    AT(panel != NULL);

    DvzVisual* text = dvz_text(scene, 0);
    AT(text != NULL);
    const char* strings[1] = {"HI"};
    float positions[1][3] = {{8.0f, 8.0f, 0.0f}};
    float pivots[1][2] = {{0.0f, 0.0f}};
    float sizes[1] = {16.0f};
    float angles[1] = {0.0f};
    DvzColor colors[1] = {{0, 255, 0, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "pivot", .data = pivots, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_text_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t green_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &rgba[4 * i];
        if (pixel[1] > 120 && pixel[0] < 80 && pixel[2] < 80)
            green_count++;
    }
    AT(green_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_field_partial_update_changes_region(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.95f, -0.95f, 0.0f}, {-0.95f, 0.95f, 0.0f},
        {0.95f, -0.95f, 0.0f},  {0.95f, 0.95f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_image_field_partial_update_changes_region skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 64);
    AT(height0 == 64);

    float patch[8];
    for (uint32_t i = 0; i < 8; i++)
        patch[i] = 1.0f;
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 2, .y = 0, .z = 0, .width = 2, .height = 4, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 4,
        }));

    dvz_app_run(app, 1);

    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);
    AT(width1 == 64);
    AT(height1 == 64);

    const uint8_t* left0 = _pixel_at(rgba0, width0, height0, 16, 32);
    const uint8_t* right0 = _pixel_at(rgba0, width0, height0, 48, 32);
    const uint8_t* left1 = _pixel_at(rgba1, width1, height1, 16, 32);
    const uint8_t* right1 = _pixel_at(rgba1, width1, height1, 48, 32);

    AT(left0[2] > 180);
    AT(right0[2] > 180);
    AT((int)left1[0] - (int)left0[0] < 40);
    AT(abs((int)left1[2] - (int)left0[2]) < 40);
    AT(right1[0] > 180);
    AT(right1[2] < 80);

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_lit_primitive_depth_orders_overlap(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* near_visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* far_visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(near_visual);
    ANN(far_visual);

    float near_positions[6][3] = {
        {-0.9f, -0.9f, 0.1f}, {-0.9f, 0.9f, 0.1f},  {0.9f, -0.9f, 0.1f},
        {0.9f, -0.9f, 0.1f},  {-0.9f, 0.9f, 0.1f},  {0.9f, 0.9f, 0.1f},
    };
    float far_positions[6][3] = {
        {-0.9f, -0.9f, 0.8f}, {-0.9f, 0.9f, 0.8f},  {0.9f, -0.9f, 0.8f},
        {0.9f, -0.9f, 0.8f},  {-0.9f, 0.9f, 0.8f},  {0.9f, 0.9f, 0.8f},
    };
    float normals[6][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzColor near_colors[6];
    DvzColor far_colors[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        near_colors[i][0] = 32;
        near_colors[i][1] = 64;
        near_colors[i][2] = 255;
        near_colors[i][3] = 255;
        far_colors[i][0] = 255;
        far_colors[i][1] = 32;
        far_colors[i][2] = 32;
        far_colors[i][3] = 255;
    }

    AT(dvz_visual_set_data(near_visual, "position", near_positions, 6) == 0);
    AT(dvz_visual_set_data(near_visual, "color", near_colors, 6) == 0);
    AT(dvz_visual_set_data(near_visual, "normal", normals, 6) == 0);
    AT(dvz_panel_add_visual(panel, near_visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           near_visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    AT(dvz_visual_set_data(far_visual, "position", far_positions, 6) == 0);
    AT(dvz_visual_set_data(far_visual, "color", far_colors, 6) == 0);
    AT(dvz_visual_set_data(far_visual, "normal", normals, 6) == 0);
    AT(dvz_panel_add_visual(panel, far_visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           far_visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_lit_primitive_depth_orders_overlap skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(center[2] > 180);
    AT(center[2] > center[0] + 40);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure lit primitive depth cueing darkens farther geometry in an offscreen render.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_lit_primitive_depth_cue_darkens_far(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[12][3] = {
        {-0.9f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f}, {-0.1f, -0.8f, 0.0f},
        {-0.1f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f},
        {0.1f, -0.8f, 0.8f},  {0.1f, 0.8f, 0.8f},  {0.9f, -0.8f, 0.8f},
        {0.9f, -0.8f, 0.8f},  {0.1f, 0.8f, 0.8f},  {0.9f, 0.8f, 0.8f},
    };
    float normals[12][3];
    DvzColor colors[12];
    for (uint32_t i = 0; i < 12; i++)
    {
        normals[i][0] = 0.0f;
        normals[i][1] = 0.0f;
        normals[i][2] = 1.0f;
        colors[i][0] = 255;
        colors[i][1] = 48;
        colors[i][2] = 48;
        colors[i][3] = 255;
    }

    AT(dvz_visual_set_data(visual, "position", positions, 12) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 12) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 12) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);
    AT(dvz_visual_set_depth_cue(
           visual,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.50f,
               .far_depth = 0.95f,
               .strength = 1.0f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_lit_primitive_depth_cue_darkens_far skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* near_px = _pixel_at(rgba, width, height, width / 4, height / 2);
    const uint8_t* far_px = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
    AT(near_px[0] > 180);
    AT(far_px[0] + 80 < near_px[0]);
    AT(far_px[1] + 20 < near_px[1]);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure an indexed mesh contributes visible pixels through the app offscreen path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_app_offscreen_mesh_renders_nonblank(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 64, 64, 255},
        {64, 255, 64, 255},
        {64, 64, 255, 255},
        {255, 224, 64, 255},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_mesh_renders_nonblank skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(!(center[0] == 13 && center[1] == 13 && center[2] == 20));

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Rotate one cube point into a stable off-axis view for mesh depth tests.
 *
 * @param x the input x coordinate
 * @param y the input y coordinate
 * @param z the input z coordinate
 * @param out the rotated output coordinate
 */


static void _rotated_mesh_rotate_point(float x, float y, float z, float* out)
{
    ANN(out);
    const float ax = -0.65f;
    const float ay = +0.75f;
    const float cx = cosf(ax);
    const float sx = sinf(ax);
    const float cy = cosf(ay);
    const float sy = sinf(ay);

    const float y1 = cx * y - sx * z;
    const float z1 = sx * y + cx * z;
    const float x2 = cy * x + sy * z1;
    const float z2 = -sy * x + cy * z1;

    out[0] = x2;
    out[1] = y1;
    out[2] = z2;
}



/**
 * Build an indexed cube with duplicated vertices and per-face normals.
 *
 * @param positions the output vertex positions
 * @param colors the output vertex colors
 * @param normals the output vertex normals
 * @param indices the output triangle indices
 */


static void _rotated_mesh_build_cube(
    float positions[24][3], DvzColor colors[24], float normals[24][3], DvzIndex indices[36])
{
    const float s = 0.58f;
    const float face_positions[6][4][3] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const float face_normals[6][3] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };
    const DvzColor face_colors[6] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        float rotated_normal[3] = {0};
        _rotated_mesh_rotate_point(
            face_normals[face][0], face_normals[face][1], face_normals[face][2],
            rotated_normal);

        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            _rotated_mesh_rotate_point(
                face_positions[face][corner][0], face_positions[face][corner][1],
                face_positions[face][corner][2], positions[vertex]);
            colors[vertex][0] = face_colors[face][0];
            colors[vertex][1] = face_colors[face][1];
            colors[vertex][2] = face_colors[face][2];
            colors[vertex][3] = face_colors[face][3];
            normals[vertex][0] = rotated_normal[0];
            normals[vertex][1] = rotated_normal[1];
            normals[vertex][2] = rotated_normal[2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
}


/**
 * Build an indexed object-space cube with duplicated vertices and per-face normals.
 *
 * @param positions the output vertex positions
 * @param colors the output vertex colors
 * @param normals the output vertex normals
 * @param indices the output triangle indices
 */


static void _mesh_build_cube_object_space(
    float positions[24][3], DvzColor colors[24], float normals[24][3], DvzIndex indices[36])
{
    const float s = 0.58f;
    const float face_positions[6][4][3] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const float face_normals[6][3] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };
    const DvzColor face_colors[6] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            positions[vertex][0] = face_positions[face][corner][0];
            positions[vertex][1] = face_positions[face][corner][1];
            positions[vertex][2] = face_positions[face][corner][2];
            colors[vertex][0] = face_colors[face][0];
            colors[vertex][1] = face_colors[face][1];
            colors[vertex][2] = face_colors[face][2];
            colors[vertex][3] = face_colors[face][3];
            normals[vertex][0] = face_normals[face][0];
            normals[vertex][1] = face_normals[face][1];
            normals[vertex][2] = face_normals[face][2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
}



/**
 * Ensure a rotated indexed mesh resolves hidden faces through depth, not draw order.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_app_offscreen_rotated_mesh_depth_orders_faces(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    _rotated_mesh_build_cube(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 24) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 24) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 24) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.35f, 0.55f, 0.75f},
               .ambient = 0.25f,
               .diffuse = 0.85f,
           }) == 0);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_rotated_mesh_depth_orders_faces skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 128, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 96);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(center[0] > center[1] + 8);
    AT(center[0] > center[2] + 24);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure an object-space cube renders through panel camera and arcball transforms.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_app_offscreen_camera_arcball_mesh_renders_cube(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.fov_y = GLM_PI_4f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);
    dvz_panel_set_arcball(panel, NULL, 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    ANN(arcball);
    dvz_arcball_initial(arcball, (vec3){+0.6f, -1.2f, +3.0f});

    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    _mesh_build_cube_object_space(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 24) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 24) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 24) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.35f, 0.55f, 0.75f},
               .ambient = 0.25f,
               .diffuse = 0.85f,
           }) == 0);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_camera_arcball_mesh_renders_cube skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 128, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 96);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(!(center[0] == 13 && center[1] == 13 && center[2] == 20));

    const uint8_t* right = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
    AT(right[1] > right[0]);
    AT(right[1] > right[2]);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_shared_field_mixed_runtime_updates(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale0);
    ANN(scale1);
    dvz_scale_set_domain(scale0, 0.0, 1.0);
    dvz_scale_set_domain(scale1, 0.0, 1.0);

    DvzColormap* colormap0 = dvz_colormap(scene, NULL);
    DvzColormap* colormap1 = dvz_colormap(scene, NULL);
    ANN(colormap0);
    ANN(colormap1);
    DvzColormapStop base_stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap0, base_stops, 2);
    dvz_colormap_set_stops(colormap1, base_stops, 2);
    dvz_scale_set_colormap(scale0, colormap0);
    dvz_scale_set_colormap(scale1, colormap1);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));

    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    float left_positions[4][3] = {
        {-1.0f, -0.95f, 0.0f}, {-1.0f, 0.95f, 0.0f},
        {0.0f, -0.95f, 0.0f},  {0.0f, 0.95f, 0.0f},
    };
    float right_positions[4][3] = {
        {0.0f, -0.95f, 0.0f}, {0.0f, 0.95f, 0.0f},
        {1.0f, -0.95f, 0.0f}, {1.0f, 0.95f, 0.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", left_positions, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "colormap", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", right_positions, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "colormap", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_shared_field_mixed_runtime_updates skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);

    DvzColormapStop updated_stops[2] = {
        {.position = 0.0, .rgba = {0, 255, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 0, 255}},
    };
    dvz_colormap_set_stops(colormap0, updated_stops, 2);

    float patch[8];
    for (uint32_t i = 0; i < 8; i++)
        patch[i] = 1.0f;
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 2, .y = 0, .z = 0, .width = 2, .height = 4, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 4,
        }));

    dvz_app_run(app, 1);

    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);

    const uint8_t* left_left0 = _pixel_at(rgba0, width0, height0, 24, 32);
    const uint8_t* left_left1 = _pixel_at(rgba1, width1, height1, 24, 32);
    const uint8_t* right_left0 = _pixel_at(rgba0, width0, height0, 60, 32);
    const uint8_t* right_left1 = _pixel_at(rgba1, width1, height1, 60, 32);
    const uint8_t* right_right1 = _pixel_at(rgba1, width1, height1, 84, 32);

    AT(left_left0[2] > 180);
    AT((int)left_left1[1] > (int)left_left0[1] + 40);
    AT((int)left_left1[2] + 40 < (int)left_left0[2]);

    AT(right_left0[2] > 180);
    AT(abs((int)right_left1[0] - (int)right_left0[0]) < 40);
    AT(abs((int)right_left1[1] - (int)right_left0[1]) < 40);
    AT(abs((int)right_left1[2] - (int)right_left0[2]) < 40);

    AT(right_right1[0] > 180);
    AT(right_right1[2] < 120);

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_retained_render_second_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 32.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_retained_render_second_frame skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t yellow_counts[2] = {0, 0};
    for (uint32_t frame = 0; frame < 2; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 64);
        AT(height == 64);

        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* pixel = &rgba[4 * i];
            if (pixel[0] > 200 && pixel[1] > 200)
                yellow_counts[frame]++;
        }
        dvz_free(rgba);
    }
    AT(yellow_counts[0] > 0);
    AT(yellow_counts[1] > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_retained_render_second_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[i * 4 + 0] = 255; pixels[i * 4 + 1] = 0;
        pixels[i * 4 + 2] = 0;   pixels[i * 4 + 3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_retained_render_second_frame skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Both frames should show red pixels from the retained texture. */
    uint32_t red_counts[2] = {0, 0};
    for (uint32_t frame = 0; frame < 2; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 64);
        AT(height == 64);

        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* pixel = &rgba[4 * i];
            if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50)
                red_counts[frame]++;
        }
        dvz_free(rgba);
    }
    AT(red_counts[0] > 0);
    AT(red_counts[1] > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure a reused offscreen app/runtime survives repeated resizes with mixed retained visuals.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_resize_reuses_runtime_with_mesh_and_image(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* image = dvz_image(scene, 0);
    AT(mesh != NULL);
    AT(image != NULL);

    float mesh_positions[4][3] = {
        {-0.9f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f},
        {-0.1f, -0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f},
    };
    DvzColor mesh_colors[4] = {
        {64, 255, 64, 255},
        {64, 255, 64, 255},
        {64, 255, 64, 255},
        {64, 255, 64, 255},
    };
    float mesh_normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)));

    AT(dvz_visual_set_data(mesh, "position", mesh_positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", mesh_colors, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_visual_set_primitive_shading(
           mesh,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    float image_positions[4][3] = {
        {0.1f, -0.8f, 0.0f}, {0.1f, 0.8f, 0.0f},
        {0.9f, -0.8f, 0.0f}, {0.9f, 0.8f, 0.0f},
    };
    float image_texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 3] = 255;
    }

    AT(dvz_visual_set_data(image, "position", image_positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", image_texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_resize_reuses_runtime_with_mesh_and_image skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    const uint32_t sizes[][2] = {
        {96, 64},
        {128, 72},
        {80, 96},
        {144, 80},
        {96, 64},
    };

    for (uint32_t frame = 0; frame < sizeof(sizes) / sizeof(sizes[0]); frame++)
    {
        uint32_t expected_width = sizes[frame][0];
        uint32_t expected_height = sizes[frame][1];
        AT(dvz_app_window_resize(win, expected_width, expected_height) == 0);
        AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);

        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == expected_width);
        AT(height == expected_height);

        const uint8_t* mesh_center = _pixel_at(rgba, width, height, width / 4, height / 2);
        AT(mesh_center[1] > 180);
        AT(mesh_center[0] < 140);
        AT(mesh_center[2] < 140);

        const uint8_t* image_center = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
        AT(image_center[0] > 200);
        AT(image_center[1] < 80);
        AT(image_center[2] < 80);

        dvz_free(rgba);
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure app-owned request execution stays steady across repeated pick/probe frames.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_pick_probe_request_steady_state(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
    AT(panel != NULL);

    DvzVisual* points = dvz_point(scene, 0);
    AT(points != NULL);
    dvz_visual_set_pick_capabilities(points, DVZ_PICK_CAPABILITY_ITEM);
    float point_pos[1][3] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_pick_probe_request_steady_state skipped: GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    for (uint32_t frame = 0; frame < 8; frame++)
    {
        uint64_t pick_id = 100 + frame;
        uint64_t probe_id = 200 + frame;
        AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = pick_id}) == 0);
        AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = probe_id}) == 0);
        AT(scene->pending_pick_count == 1);
        AT(scene->pending_probe_count == 1);

        AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
        AT(scene->pending_pick_count == 0);
        AT(scene->pending_probe_count == 0);

        DvzPickResult pick = {0};
        DvzProbeResult probe = {0};
        AT(dvz_scene_poll_pick(scene, &pick));
        AT(dvz_scene_poll_probe(scene, &probe));
        AT(pick.hit);
        AT(pick.request_id == pick_id);
        AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
        AT(pick.resolved_id == 0);
        AT(probe.hit);
        AT(probe.request_id == probe_id);
        AT(probe.value_kind == DVZ_PROBE_VALUE_VEC4);
        AT(probe.vector[0] > 0.9);
        AT(probe.vector[1] < 0.1);
        AT(probe.vector[2] < 0.1);
        AT(probe.vector[3] > 0.9);

        AT(!dvz_scene_poll_pick(scene, &pick));
        AT(!dvz_scene_poll_probe(scene, &probe));
        AT(scene->pick_result_count == 0);
        AT(scene->probe_result_count == 0);
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



int test_app_offscreen_two_panel_points_light_both_halves(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    AT(figure != NULL);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(left != NULL);
    AT(right != NULL);

    DvzVisual* left_visual = dvz_point(scene, 0);
    DvzVisual* right_visual = dvz_point(scene, 0);
    AT(left_visual != NULL);
    AT(right_visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor red = {255, 32, 32, 255};
    DvzColor green = {32, 255, 32, 255};
    float size = 24.0f;

    AT(dvz_visual_set_data(left_visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(left_visual, "color", &red, 1) == 0);
    AT(dvz_visual_set_data(left_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(left, left_visual, NULL) == 0);

    AT(dvz_visual_set_data(right_visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(right_visual, "color", &green, 1) == 0);
    AT(dvz_visual_set_data(right_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(right, right_visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_two_panel_points_light_both_halves skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    AT(win != NULL);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t red_count = 0;
    uint32_t green_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 96);
        AT(height == 64);

        red_count = 0;
        green_count = 0;
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint8_t* pixel = &rgba[4 * (y * width + x)];
                if (pixel[0] > 150 && pixel[0] > pixel[1] + 40)
                    red_count++;
                if (pixel[1] > 150 && pixel[1] > pixel[0] + 40)
                    green_count++;
            }
        }
        dvz_free(rgba);
        if (red_count > 0 && green_count > 0)
            break;
    }
    AT(red_count > 0);
    AT(green_count > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_clear_color(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Scene with NO visuals — all pixels should show the clear color. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    (void)panel;
    AT(panel != NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_clear_color skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);

    /* Default clear color is (0.05, 0.05, 0.08, 1.0) — very dark, R<20, G<20, B<25.
       All pixels must be dark (no stray bright pixels from missing clear). */
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* px = &rgba[4 * i];
        if (px[0] > 30 || px[1] > 30 || px[2] > 30)
            bright_count++;
    }
    AT(bright_count == 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_capture_rejects_wrong_dimensions(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    (void)panel;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_wrong_dimensions skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Ask for a dimension that doesn't match the 64x64 offscreen canvas. */
    uint8_t buf[128 * 128 * 4];
    tst_log_capture_begin(suite);
    AT(dvz_canvas_capture_rgba_into(canvas, 128, 128, buf, sizeof(buf)) != 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_capture_rejects_undersized_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    (void)panel;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_undersized_buffer skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Buffer is one byte short of the required 64*64*4 bytes. */
    size_t required = 64 * 64 * 4;
    uint8_t* buf = dvz_malloc(required - 1);
    ANN(buf);
    tst_log_capture_begin(suite);
    AT(dvz_canvas_capture_rgba_into(canvas, 64, 64, buf, required - 1) != 0);
    dvz_free(buf);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure a retained volume visual renders a sampled 3D field into an offscreen app frame.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_slice_renders_field(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_slice_renders_field skipped: GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t white_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        white_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 220 && px[1] > 220 && px[2] > 220)
                white_count++;
        }
        dvz_free(rgba);
        if (white_count > (width * height) / 2)
            break;
    }
    AT(white_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure MIP volume rendering traverses the 3D field instead of sampling only the middle slice.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_mip_renders_bright_slice(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 4,
               });
    ANN(field);
    const uint8_t voxels[16] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        255, 255, 255, 255,
    };
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_mip_renders_bright_slice skipped: GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t white_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        white_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 220 && px[1] > 220 && px[2] > 220)
                white_count++;
        }
        dvz_free(rgba);
        if (white_count > (width * height) / 2)
            break;
    }
    AT(white_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure composite volume rendering accumulates scalar density through the 3D field.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_composite_renders_field(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 64) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_composite_renders_field skipped: GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t bright_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        bright_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 120 && px[1] > 120 && px[2] > 120)
                bright_count++;
        }
        dvz_free(rgba);
        if (bright_count > (width * height) / 2)
            break;
    }
    AT(bright_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure alpha-blended volume rays stop behind an opaque primitive depth buffer.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_depth_occluded_by_primitive(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* occluder = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(occluder);
    ANN(volume);

    float quad[6][3] = {
        {-0.65f, -0.65f, 0.1f}, {-0.65f, 0.65f, 0.1f}, {0.65f, -0.65f, 0.1f},
        {0.65f, -0.65f, 0.1f},  {-0.65f, 0.65f, 0.1f}, {0.65f, 0.65f, 0.1f},
    };
    DvzColor black[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        black[i][0] = 0;
        black[i][1] = 0;
        black[i][2] = 0;
        black[i][3] = 255;
    }
    AT(dvz_visual_set_data(occluder, "position", quad, 6) == 0);
    AT(dvz_visual_set_data(occluder, "color", black, 6) == 0);
    AT(dvz_panel_add_visual(panel, occluder, NULL) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 4,
               });
    ANN(field);
    const uint8_t voxels[16] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        255, 255, 255, 255,
    };
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_depth_occluded_by_primitive skipped: GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
    }
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    const uint8_t* corner = _pixel_at(rgba, width, height, width / 8, height / 8);
    AT(center[0] < 40 && center[1] < 40 && center[2] < 40);
    AT(corner[0] > 120 || corner[1] > 120 || corner[2] > 120);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


#endif


/**
 * Register scene app tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_app(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    TEST_SIMPLE(test_app_offscreen);
    TEST_SIMPLE(test_app_offscreen_timer_advances_in_app_run);
    TEST_SIMPLE(test_app_offscreen_timer_advances_in_render_once);
    TEST_SIMPLE(test_app_offscreen_render_enabled_gate);
#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
    TEST_SIMPLE(test_app_external_surface_release_waits);
#endif
    TEST_SIMPLE(test_app_offscreen_panel_three_visuals_all_drawn);
    TEST_SIMPLE(test_app_offscreen_point_depth_orders_overlap);
    TEST_SIMPLE(test_app_offscreen_point_depth_cue_darkens_far);
    TEST_SIMPLE(test_app_offscreen_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_pixel_square_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_points_edl_renders);
    TEST_SIMPLE(test_app_offscreen_mesh_ssao_changes_pixels);
    TEST_SIMPLE(test_app_offscreen_records_dvzr_frames);
    TEST_SIMPLE(test_app_offscreen_image_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_text_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_field_partial_update_changes_region);
    TEST_SIMPLE(test_app_offscreen_lit_primitive_depth_orders_overlap);
    TEST_SIMPLE(test_app_offscreen_lit_primitive_depth_cue_darkens_far);
    TEST_SIMPLE(test_app_offscreen_mesh_renders_nonblank);
    TEST_SIMPLE(test_app_offscreen_rotated_mesh_depth_orders_faces);
    TEST_SIMPLE(test_app_offscreen_camera_arcball_mesh_renders_cube);
    TEST_SIMPLE(test_app_offscreen_shared_field_mixed_runtime_updates);
    TEST_SIMPLE(test_app_offscreen_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_image_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_resize_reuses_runtime_with_mesh_and_image);
    TEST_SIMPLE(test_app_offscreen_pick_probe_request_steady_state);
    TEST_SIMPLE(test_app_offscreen_two_panel_points_light_both_halves);
    TEST_SIMPLE(test_app_offscreen_clear_color);
    TEST_SIMPLE(test_app_offscreen_volume_slice_renders_field);
    TEST_SIMPLE(test_app_offscreen_volume_mip_renders_bright_slice);
    TEST_SIMPLE(test_app_offscreen_volume_composite_renders_field);
    TEST_SIMPLE(test_app_offscreen_volume_depth_occluded_by_primitive);
    TEST_SIMPLE(test_app_capture_rejects_wrong_dimensions);
    TEST_SIMPLE(test_app_capture_rejects_undersized_buffer);
#endif

    return 0;
}
