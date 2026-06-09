/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* external_surface - hosted Vulkan surface API boundary.
 *
 * Scenario: feature.external_surface
 * Style: features, native app integration
 *
 * Build:  just example-c features/external_surface
 * Run:    ./build/examples/c/features/external_surface
 *
 * A real hosted integration must create a VkSurfaceKHR with the host toolkit. This example keeps
 * the feature minimal and platform-neutral by showing the Datoviz side of that contract.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "datoviz/window.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  640u
#define HEIGHT 480u



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);

    DvzAppConfig config = dvz_app_config();
    config.enable_glfw_extensions = true;
    app = dvz_app_with_config(scene, &config);
    EXAMPLE_CHECK(app != NULL, "dvz_app_with_config() failed (no GPU?)");

    VkInstance instance = dvz_app_vk_instance(app);
    EXAMPLE_CHECK(instance != VK_NULL_HANDLE, "dvz_app_vk_instance() failed");

    DvzWindowExternalSurfaceInfo surface = dvz_window_external_surface_info();
    surface.instance = instance;
    surface.surface = VK_NULL_HANDLE;
    surface.extent.width = WIDTH;
    surface.extent.height = HEIGHT;
    surface.scale_x = 1.0f;
    surface.scale_y = 1.0f;
    surface.owned_by_datoviz = false;

    DvzView* view = dvz_view_external_surface(app, figure, &surface);
    EXAMPLE_CHECK(view == NULL, "null external VkSurfaceKHR unexpectedly created a view");

    DvzView* ffi_view = dvz_view_external_surface_ffi(
        app, figure, (void*)instance, 0, WIDTH, HEIGHT, 1.0f, 1.0f, false);
    EXAMPLE_CHECK(ffi_view == NULL, "null FFI VkSurfaceKHR unexpectedly created a view");

    dvz_fprintf(
        stdout,
        "external_surface: Datoviz app owns VkInstance; host must supply a live VkSurfaceKHR\n");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
