/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar_2d_3d - lab retained 2D physical scale bar beside a 3D arcball point panel.
 *
 * Build:  just example-c lab/scalebar_2d_3d
 * Run:    ./build/examples/c/lab/scalebar_2d_3d
 * Smoke:  ./build/examples/c/lab/scalebar_2d_3d 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1100u
#define HEIGHT       620u
#define POINT_COUNT  240u
#define CLOUD_COUNT 125u

#define TAU 6.28318530718f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Add a deterministic 2D scatter cloud in a meter-valued physical domain.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success, false on error
 */
static bool _add_physical_scatter(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.040);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 0.026);
    if (rc != 0)
        return false;

    vec3 data_positions[POINT_COUNT] = {0};
    vec3 visual_positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)(POINT_COUNT - 1u);
        const float a = TAU * (13.0f * t + 0.21f * sinf(17.0f * t));
        const float r = 0.0035f + 0.0155f * sqrtf(t);
        data_positions[i][0] = 0.020f + r * cosf(a);
        data_positions[i][1] = 0.013f + 0.70f * r * sinf(a);
        data_positions[i][2] = 0.0f;
        colors[i] = dvz_color_rgba(
            _u8(0.15f + 0.70f * t), _u8(0.76f - 0.36f * t),
            _u8(0.92f - 0.48f * sinf(0.5f * a) * sinf(0.5f * a)), 230);
        diameters[i] = 6.0f + 6.0f * sinf(TAU * t) * sinf(TAU * t);
    }

    rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    rc = dvz_visual_set_data_many(visual, updates, 3);
    if (rc != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Attach the retained 2D scale bar to the physical-domain panel.
 *
 * @param panel panel receiving the annotation
 * @return true on success, false on error
 */
static bool _add_2d_scalebar(DvzPanel* panel)
{
    ANN(panel);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 125.0f,
            .min_length_px = 75.0f,
            .max_length_px = 185.0f,
            .offset_px = {26.0f, 24.0f},
            .tick_length_px = 9.0f,
            .line_width_px = 2.0f,
            .line_color = {245, 248, 252, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 18.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                .color = {255, 236, 176, 255},
            },
        });
    return scalebar != NULL;
}



/**
 * Attach a world-referenced scale bar to the 3D arcball panel.
 *
 * @param panel panel receiving the annotation
 * @return true on success, false on error
 */
static bool _add_3d_scalebar(DvzPanel* panel)
{
    ANN(panel);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .reference_mode = DVZ_SCALEBAR_REFERENCE_WORLD_POINT,
            .reference_position = {0.0, 0.0, 0.0},
            .reference_direction = {1.0, 0.0, 0.0},
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 125.0f,
            .min_length_px = 75.0f,
            .max_length_px = 185.0f,
            .offset_px = {28.0f, 24.0f},
            .tick_length_px = 9.0f,
            .line_width_px = 2.0f,
            .line_color = {235, 246, 255, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 18.0f,
                .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                .color = {178, 226, 255, 255},
            },
        });
    return scalebar != NULL;
}



/**
 * Add a compact 3D point cloud for the companion arcball panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success, false on error
 */
static bool _add_3d_point_cloud(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.20f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.74f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    if (!ok)
        return false;

    vec3 positions[CLOUD_COUNT] = {0};
    DvzColor colors[CLOUD_COUNT] = {0};
    float diameters[CLOUD_COUNT] = {0};
    uint32_t count = 0;
    for (uint32_t z = 0; z < 5u; z++)
    {
        for (uint32_t y = 0; y < 5u; y++)
        {
            for (uint32_t x = 0; x < 5u; x++)
            {
                const float fx = -1.0f + 0.5f * (float)x;
                const float fy = -1.0f + 0.5f * (float)y;
                const float fz = -1.0f + 0.5f * (float)z;
                const float d = sqrtf(fx * fx + fy * fy + fz * fz);
                if (d > 1.24f || count >= CLOUD_COUNT)
                    continue;
                positions[count][0] = fx;
                positions[count][1] = fy;
                positions[count][2] = fz;
                colors[count] = dvz_color_rgb(
                    _u8(0.40f + 0.50f * (float)x / 4.0f),
                    _u8(0.50f + 0.42f * (float)y / 4.0f),
                    _u8(0.96f - 0.32f * (float)z / 4.0f));
                diameters[count] = 12.0f + 8.0f * (1.24f - d);
                count++;
            }
        }
    }

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter", .data = diameters, .item_count = count},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 3);
    if (rc != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel_2d =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.00f, .y = 0.00f, .width = 0.50f, .height = 1.0f});
    EXAMPLE_CHECK(panel_2d != NULL, "dvz_panel(2d) failed");
    DvzPanel* panel_3d =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.50f, .y = 0.00f, .width = 0.50f, .height = 1.0f});
    EXAMPLE_CHECK(panel_3d != NULL, "dvz_panel(3d) failed");

    dvz_panel_set_background_color(panel_2d, 0.040f, 0.050f, 0.060f, 1.0f);
    dvz_panel_set_background_color(panel_3d, 0.040f, 0.044f, 0.052f, 1.0f);

    bool ok = _add_physical_scatter(scene, panel_2d);
    EXAMPLE_CHECK(ok, "_add_physical_scatter() failed");
    ok = _add_2d_scalebar(panel_2d);
    EXAMPLE_CHECK(ok, "_add_2d_scalebar() failed");
    ok = _add_3d_point_cloud(scene, panel_3d);
    EXAMPLE_CHECK(ok, "_add_3d_point_cloud() failed");
    ok = _add_3d_scalebar(panel_3d);
    EXAMPLE_CHECK(ok, "_add_3d_scalebar() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "scalebar_2d_3d");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel_2d, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzArcball* arcball = dvz_view_arcball(win, panel_3d, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.64f, -0.14f, +0.28f});
    dvz_view_request_frame(win);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
