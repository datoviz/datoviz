/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* vector - retained straight and curved vector visual smoke example.
 *
 * Build:  just example-c visuals/vector
 * Run:    ./build/examples/c/visuals/vector
 * Smoke:  ./build/examples/c/visuals/vector 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH             1100u
#define HEIGHT            720u
#define FIELD_COLS        15u
#define FIELD_ROWS        9u
#define FIELD_COUNT       (FIELD_COLS * FIELD_ROWS)
#define CURVE_COUNT       4u
#define CURVE_POINT_COUNT 6u
#define CURVE_TOTAL_COUNT (CURVE_COUNT * CURVE_POINT_COUNT)

static const float PI = 3.14159265359f;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the vector visual smoke example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    uint32_t frames = example_frame_count(argc, argv);
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, 0.055f, 0.058f, 0.066f, 1.0f);

    vec3 straight_position[FIELD_COUNT] = {{0}};
    vec3 straight_vector[FIELD_COUNT] = {{0}};
    DvzColor straight_color[FIELD_COUNT] = {{0}};
    float straight_width[FIELD_COUNT] = {0};

    uint32_t idx = 0;
    for (uint32_t row = 0; row < FIELD_ROWS; row++)
    {
        for (uint32_t col = 0; col < FIELD_COLS; col++)
        {
            float x = -0.88f + 1.76f * (float)col / (float)(FIELD_COLS - 1u);
            float y = -0.72f + 1.44f * (float)row / (float)(FIELD_ROWS - 1u);
            float angle = 2.7f * x - 2.1f * y;
            float mag = 0.075f + 0.028f * sinf(3.0f * x + 1.8f * y);

            straight_position[idx][0] = x;
            straight_position[idx][1] = y;
            straight_vector[idx][0] = mag * cosf(angle);
            straight_vector[idx][1] = mag * sinf(angle);
            straight_color[idx] = (DvzColor){
                .r = (uint8_t)(95u + (col * 110u) / (FIELD_COLS - 1u)),
                .g = (uint8_t)(155u + (row * 70u) / (FIELD_ROWS - 1u)),
                .b = 235,
                .a = 245,
            };
            straight_width[idx] = 3.0f;
            idx++;
        }
    }

    DvzVisual* straight = dvz_vector(scene, 0);
    EXAMPLE_CHECK(straight != NULL, "dvz_vector(straight) failed");
    int rc = dvz_visual_set_data(straight, "position", straight_position, FIELD_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(straight position) failed");
    rc = dvz_visual_set_data(straight, "vector", straight_vector, FIELD_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(straight vector) failed");
    rc = dvz_visual_set_data(straight, "color", straight_color, FIELD_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(straight color) failed");
    rc = dvz_visual_set_data(straight, "stroke_width", straight_width, FIELD_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(straight width) failed");

    vec3 curve_position[CURVE_TOTAL_COUNT] = {{0}};
    DvzColor curve_color[CURVE_TOTAL_COUNT] = {{0}};
    float curve_width[CURVE_TOTAL_COUNT] = {0};
    uint32_t subpaths[CURVE_COUNT] = {0};

    for (uint32_t curve = 0; curve < CURVE_COUNT; curve++)
    {
        subpaths[curve] = CURVE_POINT_COUNT;
        float lane = -0.58f + 0.38f * (float)curve;
        float phase = 0.45f * (float)curve;
        for (uint32_t point = 0; point < CURVE_POINT_COUNT; point++)
        {
            uint32_t dst = curve * CURVE_POINT_COUNT + point;
            float t = (float)point / (float)(CURVE_POINT_COUNT - 1u);
            float arc = -0.85f + 1.7f * t;
            curve_position[dst][0] = arc;
            curve_position[dst][1] = lane + 0.12f * sinf(PI * t + phase);
            curve_color[dst] = (DvzColor){
                .r = 255,
                .g = (uint8_t)(182u - curve * 22u),
                .b = (uint8_t)(82u + curve * 34u),
                .a = 245,
            };
            curve_width[dst] = 5.0f;
        }
    }

    DvzVisual* curved = dvz_vector(scene, 0);
    EXAMPLE_CHECK(curved != NULL, "dvz_vector(curved) failed");
    rc = dvz_visual_set_data(curved, "position", curve_position, CURVE_TOTAL_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(curved position) failed");
    rc = dvz_visual_set_data(curved, "color", curve_color, CURVE_TOTAL_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(curved color) failed");
    rc = dvz_visual_set_data(curved, "stroke_width", curve_width, CURVE_TOTAL_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(curved width) failed");
    rc = dvz_vector_set_subpaths(curved, CURVE_COUNT, subpaths);
    EXAMPLE_CHECK(rc == 0, "dvz_vector_set_subpaths() failed");

    rc = dvz_panel_add_visual(panel, straight, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(straight) failed");
    rc = dvz_panel_add_visual(panel, curved, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(curved) failed");

    DvzAppConfig app_config = dvz_app_config();
    if (frames > 0)
        app_config.schedule_mode = DVZ_APP_SCHEDULE_CONTINUOUS;
    app = dvz_app_with_config(scene, &app_config);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "vector");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    dvz_view_request_frame(win);

    dvz_app_run(app, frames);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
