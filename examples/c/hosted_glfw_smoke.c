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
 * Run:    ./build/examples/c/hosted_glfw_smoke [frames]
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/scene.h"
#include "datoviz/window/backend.h"



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



static DvzScene* _make_scene(DvzFigure** out_figure)
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
    dvz_visual_set_data(visual, "size", sizes, 3);
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.08f, 0.10f, 0.14f, 1.0f);

    if (out_figure != NULL)
        *out_figure = figure;
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
    DvzScene* scene = _make_scene(&figure);
    if (scene == NULL || figure == NULL)
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

    uint32_t frame = 0;
    while (!glfwWindowShouldClose(window) && (max_frames == 0 || frame < max_frames))
    {
        glfwPollEvents();

        info = _surface_info(window, instance, surface, false);
        if (dvz_app_window_update_external_surface(app_window, &info) != 0)
        {
            fprintf(stderr, "hosted_glfw_smoke: surface update failed\n");
            break;
        }

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

    printf("hosted_glfw_smoke: rendered %u frame(s)\n", frame);
    return 0;
}
