/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text - minimal MSDF text visual smoke example.
 *
 * Build:  just example-c visuals/text
 * Run:    ./build/examples/c/visuals/text
 * Smoke:  ./build/examples/c/visuals/text 120
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      960u
#define HEIGHT     540u
#define TEXT_COUNT 5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL || argv[1] == NULL)
        return 0;

    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/**
 * Convert figure pixels to panzoom visual coordinates.
 *
 * @param width figure width in pixels
 * @param height figure height in pixels
 * @param x x coordinate in figure pixels
 * @param y y coordinate in figure pixels
 * @param z z coordinate in visual space
 * @param out output visual coordinate
 */
static void _pixel_to_visual(
    uint32_t width, uint32_t height, float x, float y, float z, float out[3])
{
    ANN(out);
    out[0] = width > 0 ? 2.0f * x / (float)width - 1.0f : -1.0f;
    out[1] = height > 0 ? 1.0f - 2.0f * y / (float)height : 1.0f;
    out[2] = z;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the text visual smoke example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    uint32_t frames = _frame_count(argc, argv);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.055f, 0.065f, 0.085f, 1.0f);

    DvzVisual* text = dvz_text(scene, 0);
    if (text == NULL)
    {
        dvz_fprintf(stderr, "dvz_text() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_text_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) != 0)
    {
        dvz_fprintf(stderr, "dvz_text_set_renderer() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    const char* strings[TEXT_COUNT] = {
        "MSDF text smoke",
        "e b S  space  0123456789",
        "The quick brown fox jumps over 13 lazy glyphs.",
        "visual coordinates, pixel-sized glyphs",
        "rotated text",
    };
    float positions[TEXT_COUNT][3] = {{0}};
    float anchors[TEXT_COUNT][2] = {
        {0.0f, 0.5f},
        {0.0f, 0.5f},
        {0.0f, 0.5f},
        {0.0f, 0.5f},
        {0.5f, 0.5f},
    };
    float sizes[TEXT_COUNT] = {72.0f, 48.0f, 28.0f, 22.0f, 30.0f};
    float angles[TEXT_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, -0.32f};
    DvzColor colors[TEXT_COUNT] = {
        {236, 244, 255, 255},
        {128, 220, 255, 255},
        {214, 224, 238, 255},
        {166, 178, 196, 255},
        {255, 198, 110, 255},
    };
    _pixel_to_visual(WIDTH, HEIGHT, 54.0f, 112.0f, 0.0f, positions[0]);
    _pixel_to_visual(WIDTH, HEIGHT, 56.0f, 220.0f, 0.0f, positions[1]);
    _pixel_to_visual(WIDTH, HEIGHT, 58.0f, 318.0f, 0.0f, positions[2]);
    _pixel_to_visual(WIDTH, HEIGHT, 60.0f, 382.0f, 0.0f, positions[3]);
    _pixel_to_visual(WIDTH, HEIGHT, 760.0f, 410.0f, 0.0f, positions[4]);

    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = TEXT_COUNT},
        {.attr_name = "anchor", .data = anchors, .item_count = TEXT_COUNT},
        {.attr_name = "size", .data = sizes, .item_count = TEXT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = TEXT_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = TEXT_COUNT},
    };
    if (dvz_visual_set_strings(text, "text", strings, TEXT_COUNT) != 0 ||
        dvz_visual_set_data_many(text, updates, 5) != 0)
    {
        dvz_fprintf(stderr, "text visual data setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisualAttachDesc attach = {
        .z_layer = 1,
        .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
    };
    if (dvz_panel_add_visual(panel, text, &attach) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzController* controller = dvz_panzoom(scene, NULL);
    if (controller == NULL || dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) != 0)
    {
        dvz_fprintf(stderr, "failed to create or bind panzoom controller\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppConfig app_config = dvz_app_config();
    if (frames > 0)
        app_config.schedule_mode = DVZ_APP_SCHEDULE_CONTINUOUS;
    DvzApp* app = dvz_app_with_config(scene, &app_config);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "text");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzInputRouter* router = dvz_app_window_input(win);
    if (router == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_input() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_connect_input(panel, router);
    dvz_app_window_request_frame(win);

    dvz_app_run(app, frames);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
