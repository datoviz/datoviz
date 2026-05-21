/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hosted_glfw_smoke - host-owned GLFW loop using Datoviz hosted rendering.
 *
 * GLFW is used here only as an external toolkit surrogate. Datoviz does not create the GLFW
 * window and does not enter dvz_app_run(); the host provides Vulkan instance extensions,
 * creates VkSurfaceKHR from the Datoviz-owned VkInstance, forwards resize metadata, and calls
 * dvz_app_window_render_once() from its own loop.
 *
 * Build:  just build
 * Run:    ./build/examples/c/tools/hosted_glfw_smoke [frames]
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/input/pointer.h"
#include "datoviz/scene.h"
#include "datoviz/window/backend.h"



typedef struct
{
    DvzAppWindow* app_window;
    bool repaint_requested;
    uint32_t request_count;
} HostedGlfwState;



/**
 * Mark that Datoviz requested a host repaint.
 *
 * @param win Datoviz app-window requesting a frame
 * @param user_data hosted GLFW state
 */
static void _request_frame_callback(DvzAppWindow* win, void* user_data)
{
    (void)win;
    HostedGlfwState* state = (HostedGlfwState*)user_data;
    if (state == NULL)
        return;
    state->repaint_requested = true;
    state->request_count++;
}



/**
 * Emit the current host framebuffer/logical size to Datoviz.
 *
 * @param window GLFW window owned by the host
 * @param app_window Datoviz hosted app-window
 */
static void _emit_resize(GLFWwindow* window, DvzAppWindow* app_window)
{
    if (window == NULL || app_window == NULL)
        return;

    int fb_width = 0;
    int fb_height = 0;
    int win_width = 0;
    int win_height = 0;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    glfwGetWindowSize(window, &win_width, &win_height);
    glfwGetWindowContentScale(window, &scale_x, &scale_y);
    if (scale_x <= 0.0f)
        scale_x = 1.0f;
    if (scale_y <= 0.0f)
        scale_y = 1.0f;

    (void)dvz_app_window_emit_resize(
        app_window, fb_width > 0 ? (uint32_t)fb_width : 0,
        fb_height > 0 ? (uint32_t)fb_height : 0, win_width > 0 ? (uint32_t)win_width : 0,
        win_height > 0 ? (uint32_t)win_height : 0, scale_x, scale_y);
}



/**
 * Return the hosted Datoviz app-window associated with a GLFW host window.
 *
 * @param window GLFW window owned by the host
 * @return hosted app-window, or NULL when unavailable
 */
static DvzAppWindow* _hosted_app_window(GLFWwindow* window)
{
    if (window == NULL)
        return NULL;
    HostedGlfwState* state = (HostedGlfwState*)glfwGetWindowUserPointer(window);
    return state != NULL ? state->app_window : NULL;
}



/**
 * Forward GLFW cursor movement to Datoviz through the hosted input API.
 *
 * @param window GLFW window owned by the host
 * @param xpos pointer x position
 * @param ypos pointer y position
 */
static void _cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    DvzAppWindow* app_window = _hosted_app_window(window);
    if (app_window == NULL)
        return;
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(window, &win_width, &win_height);
    (void)dvz_app_window_emit_pointer(
        app_window, DVZ_POINTER_EVENT_MOVE, (float)xpos, (float)ypos, (float)win_width,
        (float)win_height, DVZ_POINTER_BUTTON_NONE, 0);
}



/**
 * Forward GLFW mouse-button events to Datoviz through the hosted input API.
 *
 * @param window GLFW window owned by the host
 * @param button GLFW mouse button
 * @param action GLFW button action
 * @param mods keyboard modifier bit mask
 */
static void _mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    DvzAppWindow* app_window = _hosted_app_window(window);
    if (app_window == NULL)
        return;
    double xpos = 0.0;
    double ypos = 0.0;
    int win_width = 0;
    int win_height = 0;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &win_width, &win_height);
    DvzPointerEventType type =
        action == GLFW_PRESS ? DVZ_POINTER_EVENT_PRESS : DVZ_POINTER_EVENT_RELEASE;
    (void)dvz_app_window_emit_pointer(
        app_window, type, (float)xpos, (float)ypos, (float)win_width, (float)win_height,
        dvz_pointer_button_from_glfw(button), mods);
}



/**
 * Forward GLFW scroll events to Datoviz through the hosted input API.
 *
 * @param window GLFW window owned by the host
 * @param dx horizontal wheel delta
 * @param dy vertical wheel delta
 */
static void _scroll_callback(GLFWwindow* window, double dx, double dy)
{
    DvzAppWindow* app_window = _hosted_app_window(window);
    if (app_window == NULL)
        return;
    double xpos = 0.0;
    double ypos = 0.0;
    int win_width = 0;
    int win_height = 0;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &win_width, &win_height);
#if defined(__APPLE__)
    dy = -dy;
#endif
    (void)dvz_app_window_emit_wheel(
        app_window, (float)xpos, (float)ypos, (float)win_width, (float)win_height, (float)dx,
        (float)dy, 0);
}



/**
 * Forward GLFW key events to Datoviz through the hosted input API.
 *
 * @param window GLFW window owned by the host
 * @param key GLFW key code
 * @param scancode platform scancode
 * @param action GLFW key action
 * @param mods keyboard modifier bit mask
 */
static void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    DvzAppWindow* app_window = _hosted_app_window(window);
    if (app_window == NULL)
        return;
    DvzKeyboardEventType type = DVZ_KEYBOARD_EVENT_NONE;
    if (action == GLFW_PRESS)
        type = DVZ_KEYBOARD_EVENT_PRESS;
    else if (action == GLFW_REPEAT)
        type = DVZ_KEYBOARD_EVENT_REPEAT;
    else if (action == GLFW_RELEASE)
        type = DVZ_KEYBOARD_EVENT_RELEASE;
    if (type != DVZ_KEYBOARD_EVENT_NONE)
        (void)dvz_app_window_emit_key(app_window, type, (DvzKeyCode)key, mods);
}



/**
 * Forward GLFW framebuffer resize events to Datoviz through the hosted input API.
 *
 * @param window GLFW window owned by the host
 * @param width framebuffer width
 * @param height framebuffer height
 */
static void _framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)width;
    (void)height;
    _emit_resize(window, _hosted_app_window(window));
}



/**
 * Forward GLFW content-scale changes to Datoviz through the hosted input API.
 *
 * @param window GLFW window owned by the host
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 */
static void _content_scale_callback(GLFWwindow* window, float scale_x, float scale_y)
{
    (void)scale_x;
    (void)scale_y;
    _emit_resize(window, _hosted_app_window(window));
}



static DvzWindowExternalSurfaceInfo
_surface_info(GLFWwindow* window, VkInstance instance, VkSurfaceKHR surface, bool owned_by_datoviz)
{
    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);

    float scale_x = 1.0f;
    float scale_y = 1.0f;
    glfwGetWindowContentScale(window, &scale_x, &scale_y);
    if (scale_x <= 0.0f)
        scale_x = 1.0f;
    if (scale_y <= 0.0f)
        scale_y = 1.0f;

    DvzWindowExternalSurfaceInfo info = {
        .instance = instance,
        .surface = surface,
        .extent = {
            .width = fb_width > 0 ? (uint32_t)fb_width : 0,
            .height = fb_height > 0 ? (uint32_t)fb_height : 0,
        },
        .scale_x = scale_x,
        .scale_y = scale_y,
        .owned_by_datoviz = owned_by_datoviz,
    };
    return info;
}



static DvzScene* _make_scene(DvzFigure** out_figure, DvzPanel** out_panel)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return NULL;

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return NULL;
    }

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    DvzVisual* visual = panel != NULL ? dvz_point(scene, 0) : NULL;
    if (panel == NULL || visual == NULL)
    {
        dvz_scene_destroy(scene);
        return NULL;
    }

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f},
    };
    uint8_t colors[3][4] = {
        {255,   0,   0, 255},
        {  0, 255,   0, 255},
        {  0,   0, 255, 255},
    };
    float sizes[3] = {24.0f, 24.0f, 24.0f};

    dvz_visual_set_data(visual, "position", positions, 3);
    dvz_visual_set_data(visual, "color", colors, 3);
    dvz_visual_set_data(visual, "diameter", sizes, 3);
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.08f, 0.10f, 0.14f, 1.0f);

    if (out_figure != NULL)
        *out_figure = figure;
    if (out_panel != NULL)
        *out_panel = panel;
    return scene;
}



int main(int argc, char** argv)
{
    uint32_t max_frames = 120;
    if (argc > 1)
        max_frames = (uint32_t)strtoul(argv[1], NULL, 10);

    if (!dvz_window_glfw_init())
    {
        fprintf(stderr, "hosted_glfw_smoke: skipped, GLFW could not initialize\n");
        return 0;
    }

    uint32_t extension_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    if (extension_count == 0 || extensions == NULL)
    {
        fprintf(stderr, "hosted_glfw_smoke: skipped, GLFW returned no Vulkan extensions\n");
        glfwTerminate();
        return 0;
    }

    DvzFigure* figure = NULL;
    DvzPanel* panel = NULL;
    DvzScene* scene = _make_scene(&figure, &panel);
    if (scene == NULL || figure == NULL || panel == NULL)
    {
        fprintf(stderr, "hosted_glfw_smoke: failed to create scene\n");
        glfwTerminate();
        return 1;
    }

    DvzAppConfig app_cfg = dvz_app_config();
    app_cfg.instance_extension_count = extension_count;
    app_cfg.instance_extensions = extensions;
    app_cfg.enable_canvas_extensions = true;
    app_cfg.enable_glfw_extensions = false;
    DvzApp* app = dvz_app_with_config(scene, &app_cfg);
    if (app == NULL)
    {
        fprintf(stderr, "hosted_glfw_smoke: skipped, Datoviz GPU context creation failed\n");
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 0;
    }

    VkInstance instance = dvz_app_vk_instance(app);
    if (instance == VK_NULL_HANDLE)
    {
        fprintf(stderr, "hosted_glfw_smoke: Datoviz returned no Vulkan instance\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "hosted_glfw_smoke", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "hosted_glfw_smoke: skipped, GLFW window creation failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 0;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult surface_res = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (surface_res != VK_SUCCESS || surface == VK_NULL_HANDLE)
    {
        fprintf(
            stderr, "hosted_glfw_smoke: skipped, surface creation failed (%d)\n",
            (int)surface_res);
        glfwDestroyWindow(window);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 0;
    }

    DvzWindowExternalSurfaceInfo info = _surface_info(window, instance, surface, false);
    DvzAppWindow* app_window = dvz_app_window_external_surface(app, figure, &info);
    if (app_window == NULL)
    {
        fprintf(stderr, "hosted_glfw_smoke: hosted app-window creation failed\n");
        vkDestroySurfaceKHR(instance, surface, NULL);
        glfwDestroyWindow(window);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        glfwTerminate();
        return 1;
    }

    HostedGlfwState host_state = {.app_window = app_window};
    glfwSetWindowUserPointer(window, &host_state);
    dvz_app_window_set_request_frame_callback(app_window, _request_frame_callback, &host_state);
    glfwSetCursorPosCallback(window, _cursor_pos_callback);
    glfwSetMouseButtonCallback(window, _mouse_button_callback);
    glfwSetScrollCallback(window, _scroll_callback);
    glfwSetKeyCallback(window, _key_callback);
    glfwSetFramebufferSizeCallback(window, _framebuffer_size_callback);
    glfwSetWindowContentScaleCallback(window, _content_scale_callback);
    _emit_resize(window, app_window);
    DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
    if (panzoom_controller == NULL ||
        dvz_panel_bind_controller(panel, panzoom_controller, DVZ_DIM_MASK_XY) != 0)
    {
        fprintf(stderr, "failed to create or bind panzoom controller\n");
        dvz_app_destroy(app);
        vkDestroySurfaceKHR(instance, surface, NULL);
        dvz_scene_destroy(scene);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    dvz_panel_connect_input(panel, dvz_app_window_input(app_window));

    uint32_t frame = 0;
    while (!glfwWindowShouldClose(window) && (max_frames == 0 || frame < max_frames))
    {
        if (max_frames == 0 && !host_state.repaint_requested)
            glfwWaitEventsTimeout(0.1);
        else
            glfwPollEvents();

        info = _surface_info(window, instance, surface, false);
        if (dvz_app_window_update_external_surface(app_window, &info) != 0)
        {
            fprintf(stderr, "hosted_glfw_smoke: surface update failed\n");
            break;
        }

        if (max_frames == 0 && !host_state.repaint_requested)
            continue;
        host_state.repaint_requested = false;
        int rc = dvz_app_window_render_once(app_window);
        if (rc < 0)
        {
            fprintf(stderr, "hosted_glfw_smoke: render failed (%d)\n", rc);
            break;
        }
        if (rc == DVZ_CANVAS_FRAME_READY)
            frame++;
    }

    info = _surface_info(window, instance, surface, true);
    if (dvz_app_window_update_external_surface(app_window, &info) == 0)
        surface = VK_NULL_HANDLE;

    dvz_app_destroy(app);
    if (surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, surface, NULL);
    glfwDestroyWindow(window);
    dvz_scene_destroy(scene);
    glfwTerminate();

    printf(
        "hosted_glfw_smoke: rendered %u frame(s), %u request(s)\n", frame,
        host_state.request_count);
    return 0;
}
