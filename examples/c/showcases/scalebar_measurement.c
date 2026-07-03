/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar_measurement - scale-aware overview, detail, and 3D measurement workflow.
 *
 * Scenario: scalebar_measurement_workflow
 * Style: showcase workflow, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c showcases/scalebar_measurement
 * Run:    ./build/examples/c/showcases/scalebar_measurement --live
 * Smoke:  ./build/examples/c/showcases/scalebar_measurement --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_scalebar_measurement_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define OVERVIEW_WIDTH   224u
#define OVERVIEW_HEIGHT  160u
#define DETAIL_WIDTH     180u
#define DETAIL_HEIGHT    136u
#define DETAIL_POINTS    120u
#define ZOOM_BOX_SEG     4u
#define CLOUD_COUNT      125u
#define ROTATION_SPEED_RAD_PER_SEC 0.35f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ScalebarMeasurementState
{
    DvzTrack* specimen_rotation;
    DvzAnimation* specimen_animation;
} ScalebarMeasurementState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Convert a normalized float channel to an 8-bit channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel
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
 * Return a deterministic synthetic microscopy-like scalar sample.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @param detail whether to emphasize the zoomed field
 * @return normalized sample value
 */
static float _sample_field(float x, float y, bool detail)
{
    float value = 0.10f + 0.10f * sinf(TAU * (2.2f * x + 0.7f * y));
    value += 0.06f * cosf(TAU * (0.4f * x - 3.8f * y));

    for (uint32_t i = 0; i < 9u; i++)
    {
        const float fi = (float)i;
        const float cx = fmodf(0.17f + 0.137f * fi, 0.96f);
        const float cy = fmodf(0.21f + 0.219f * fi, 0.94f);
        const float dx = x - cx;
        const float dy = y - cy;
        const float sigma = detail ? 0.030f + 0.004f * (float)(i % 3u) : 0.044f;
        const float d2 = (dx * dx + 1.35f * dy * dy) / (2.0f * sigma * sigma);
        value += (0.33f + 0.05f * (float)(i % 4u)) * expf(-d2);
    }

    return fminf(fmaxf(value, 0.0f), 1.0f);
}



/**
 * Fill one RGBA field texture with the shared graphite/cyan release palette.
 *
 * @param pixels output RGBA8 texture
 * @param width texture width
 * @param height texture height
 * @param detail whether to use the zoomed-field variant
 */
static void _fill_field_texture(uint8_t* pixels, uint32_t width, uint32_t height, bool detail)
{
    ANN(pixels);

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            const float u = width > 1u ? (float)x / (float)(width - 1u) : 0.0f;
            const float v = height > 1u ? (float)y / (float)(height - 1u) : 0.0f;
            const float sample = _sample_field(u, v, detail);
            const float ridge = 0.5f + 0.5f * sinf(TAU * (7.0f * u + 2.4f * v));
            const uint32_t k = 4u * (y * width + x);

            pixels[k + 0] = _u8(0.07f + 0.16f * sample + 0.05f * ridge);
            pixels[k + 1] = _u8(0.12f + 0.55f * sample + 0.06f * ridge);
            pixels[k + 2] = _u8(0.16f + 0.74f * sample);
            pixels[k + 3] = 255u;
        }
    }
}



/**
 * Copy one Datoviz color into a scale-bar descriptor array.
 *
 * @param out output RGBA8 array
 * @param color source color
 * @param alpha alpha channel override
 */
static void _copy_color(uint8_t out[4], DvzColor color, uint8_t alpha)
{
    ANN(out);
    out[0] = color.r;
    out[1] = color.g;
    out[2] = color.b;
    out[3] = alpha;
}



/**
 * Configure a panel with the shared feature-example background.
 *
 * @param panel target panel
 * @return true when panel layout was configured
 */
static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);
    example_graphite_cyan_set_panel_background(panel);
    return true;
}



/**
 * Set one panel data domain.
 *
 * @param panel target panel
 * @param xmin X minimum
 * @param xmax X maximum
 * @param ymin Y minimum
 * @param ymax Y maximum
 * @return true when both domain calls succeed
 */
static bool _set_domain(DvzPanel* panel, double xmin, double xmax, double ymin, double ymax)
{
    ANN(panel);
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);
    return rc == 0;
}



/**
 * Add an image visual whose corners are specified in panel data coordinates.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param pixels RGBA8 texture pixels
 * @param width texture width
 * @param height texture height
 * @param xmin data-domain X minimum
 * @param xmax data-domain X maximum
 * @param ymin data-domain Y minimum
 * @param ymax data-domain Y maximum
 * @return true when the image was added
 */
static bool _add_image(
    DvzScene* scene, DvzPanel* panel, uint8_t* pixels, uint32_t width, uint32_t height,
    float xmin, float xmax, float ymin, float ymax)
{
    ANN(scene);
    ANN(panel);
    ANN(pixels);

    vec3 data_positions[4] = {
        {xmin, ymin, 0.0f},
        {xmin, ymax, 0.0f},
        {xmax, ymin, 0.0f},
        {xmax, ymax, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
        return false;
    if (dvz_visual_set_data(image, "position", data_positions, 4) != 0)
        return false;
    if (dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0)
        return false;
    if (dvz_visual_set_texture_rgba8(image, (const uint8_t*)pixels, width, height) != 0)
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/**
 * Add a zoom-box overlay to the overview panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the overlay was added
 */
static bool _add_zoom_box(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 starts[ZOOM_BOX_SEG] = {
        {3.10f, 2.25f, 0.0f},
        {5.10f, 2.25f, 0.0f},
        {5.10f, 3.65f, 0.0f},
        {3.10f, 3.65f, 0.0f},
    };
    vec3 ends[ZOOM_BOX_SEG] = {
        {5.10f, 2.25f, 0.0f},
        {5.10f, 3.65f, 0.0f},
        {3.10f, 3.65f, 0.0f},
        {3.10f, 2.25f, 0.0f},
    };
    DvzColor colors[ZOOM_BOX_SEG] = {{0}};
    float widths[ZOOM_BOX_SEG] = {0};
    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    for (uint32_t i = 0; i < ZOOM_BOX_SEG; i++)
    {
        colors[i] = dvz_color_rgba(accent.r, accent.g, accent.b, 230);
        widths[i] = 2.2f;
    }

    DvzVisual* box = dvz_segment(scene, 0);
    if (box == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = ZOOM_BOX_SEG},
        {.attr_name = "position_end", .data = ends, .item_count = ZOOM_BOX_SEG},
        {.attr_name = "color", .data = colors, .item_count = ZOOM_BOX_SEG},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = ZOOM_BOX_SEG},
    };
    if (dvz_visual_set_data_many(box, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(box, DVZ_SEGMENT_CAP_SQUARE, DVZ_SEGMENT_CAP_SQUARE) != 0)
        return false;
    if (dvz_visual_set_depth_test(box, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, box, NULL) == 0;
}



/**
 * Add deterministic highlighted objects to the detail panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the points were added
 */
static bool _add_detail_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[DETAIL_POINTS] = {{0}};
    DvzColor colors[DETAIL_POINTS] = {{0}};
    float diameters[DETAIL_POINTS] = {0};
    for (uint32_t i = 0; i < DETAIL_POINTS; i++)
    {
        const float t = (float)i / (float)(DETAIL_POINTS - 1u);
        const float a = TAU * (9.0f * t + 0.07f * sinf(21.0f * t));
        const float r = 0.10f + 0.78f * sqrtf(t);
        data_positions[i][0] = 4.10f + 0.54f * r * cosf(a);
        data_positions[i][1] = 2.95f + 0.34f * r * sinf(a);
        data_positions[i][2] = 0.0f;

        const float pulse = sinf(TAU * (3.0f * t + 0.11f));
        colors[i] = dvz_color_rgba(
            _u8(0.20f + 0.20f * t), _u8(0.66f + 0.22f * t),
            _u8(0.76f + 0.12f * pulse * pulse), 210);
        diameters[i] = 4.4f + 5.4f * pulse * pulse;
    }

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = DETAIL_POINTS},
        {.attr_name = "color", .data = colors, .item_count = DETAIL_POINTS},
        {.attr_name = "diameter_px", .data = diameters, .item_count = DETAIL_POINTS},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(points, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, points, NULL) == 0;
}



/**
 * Add a panel-domain scale bar with consistent release-example styling.
 *
 * @param panel panel receiving the annotation
 * @param anchor scale-bar anchor
 * @param unit base unit label used by the SI-prefix formatter
 * @param data_to_unit factor from panel data units to base units
 * @param color line and label color
 * @param label_position label position
 * @return true when the scale bar was added
 */
static bool _add_panel_scalebar(
    DvzPanel* panel, DvzSceneAnchor anchor, const char* unit, double data_to_unit, DvzColor color,
    DvzScaleBarLabelPosition label_position)
{
    ANN(panel);
    ANN(unit);

    DvzScaleBarDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension = DVZ_DIM_X,
        .anchor = anchor,
        .label_position = label_position,
        .target_length_px = 132.0f,
        .min_length_px = 82.0f,
        .max_length_px = 190.0f,
        .offset_px = {26.0f, 24.0f},
        .tick_length_px = 8.0f,
        .line_width_px = 2.0f,
        .unit = unit,
        .data_to_unit = data_to_unit,
        .label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = 16.0f,
            .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        },
    };
    _copy_color(desc.line_color, color, 255u);
    _copy_color(desc.label_style.color, color, 255u);
    DvzScaleBar* scalebar = dvz_scalebar(panel, &desc);
    return scalebar != NULL;
}



/**
 * Add a view-plane 3D scale bar to the specimen panel.
 *
 * @param panel panel receiving the annotation
 * @return true when the scale bar was added
 */
static bool _add_world_scalebar(DvzPanel* panel)
{
    ANN(panel);

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzScaleBarDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension = DVZ_DIM_X,
        .reference_mode = DVZ_SCALEBAR_REFERENCE_VIEW_PLANE,
        .reference_position = {0.0, 0.0, 0.0},
        .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
        .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
        .target_length_px = 122.0f,
        .min_length_px = 78.0f,
        .max_length_px = 176.0f,
        .offset_px = {24.0f, 24.0f},
        .tick_length_px = 8.0f,
        .line_width_px = 2.0f,
        .unit = "m",
        .data_to_unit = 0.001,
        .label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = 16.0f,
            .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        },
    };
    _copy_color(desc.line_color, color, 255u);
    _copy_color(desc.label_style.color, color, 255u);
    DvzScaleBar* scalebar = dvz_scalebar(panel, &desc);
    return scalebar != NULL;
}



/**
 * Add a compact 3D bead cloud for the world-referenced scale bar.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out optional created visual output
 * @return true when the visual and camera were configured
 */
static bool _add_3d_cloud(DvzScene* scene, DvzPanel* panel, DvzVisual** out)
{
    ANN(scene);
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[0] = 0.18f;
    camera_desc.view.eye[1] = -0.10f;
    camera_desc.view.eye[2] = 3.35f;
    camera_desc.view.up[1] = 1.0f;
    camera_desc.projection.fov_y = 0.70f;
    camera_desc.projection.near_clip = 0.1f;
    camera_desc.projection.far_clip = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
        return false;

    vec3 positions[CLOUD_COUNT] = {{0}};
    DvzColor colors[CLOUD_COUNT] = {{0}};
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
                colors[count] = dvz_color_rgba(
                    _u8(0.22f + 0.15f * (float)x / 4.0f),
                    _u8(0.58f + 0.30f * (float)y / 4.0f),
                    _u8(0.84f + 0.12f * (1.0f - (float)z / 4.0f)), 230);
                diameters[count] = 11.0f + 8.0f * (1.24f - d);
                count++;
            }
        }
    }

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter_px", .data = diameters, .item_count = count},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        return false;
    if (dvz_panel_add_visual(panel, points, NULL) != 0)
        return false;
    if (out != NULL)
        *out = points;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize the scale-aware measurement workflow scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    ScalebarMeasurementState* state =
        (ScalebarMeasurementState*)calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    uint8_t overview_pixels[OVERVIEW_WIDTH * OVERVIEW_HEIGHT * 4u] = {0};
    uint8_t detail_pixels[DETAIL_WIDTH * DETAIL_HEIGHT * 4u] = {0};

    _fill_field_texture(overview_pixels, OVERVIEW_WIDTH, OVERVIEW_HEIGHT, false);
    _fill_field_texture(detail_pixels, DETAIL_WIDTH, DETAIL_HEIGHT, true);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 2, 2);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    ok = dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 36.0f, .right_px = 30.0f, .top_px = 24.0f,
                                 .bottom_px = 30.0f});
    EXAMPLE_CHECK(ok, "dvz_grid_set_margins() failed");
    ok = dvz_grid_set_gutter(grid, 28.0f, 26.0f);
    EXAMPLE_CHECK(ok, "dvz_grid_set_gutter() failed");

    DvzPanel* overview = dvz_grid_panel_span(grid, 0, 0, 2, 1);
    DvzPanel* detail = dvz_grid_panel(grid, 0, 1);
    DvzPanel* specimen = dvz_grid_panel(grid, 1, 1);
    EXAMPLE_CHECK(
        overview != NULL && detail != NULL && specimen != NULL, "dvz_grid_panel() failed");

    ok = _configure_panel(overview);
    EXAMPLE_CHECK(ok, "_configure_panel(overview) failed");
    ok = _configure_panel(detail);
    EXAMPLE_CHECK(ok, "_configure_panel(detail) failed");
    example_graphite_cyan_set_panel_background(specimen);

    ok = _set_domain(overview, 0.0, 12.0, 0.0, 8.0);
    EXAMPLE_CHECK(ok, "_set_domain(overview) failed");
    ok = _set_domain(detail, 3.10, 5.10, 2.25, 3.65);
    EXAMPLE_CHECK(ok, "_set_domain(detail) failed");

    ok = _add_image(
        ctx->scene, overview, overview_pixels, OVERVIEW_WIDTH, OVERVIEW_HEIGHT, 0.0f, 12.0f,
        0.0f, 8.0f);
    EXAMPLE_CHECK(ok, "_add_image(overview) failed");
    ok = _add_image(
        ctx->scene, detail, detail_pixels, DETAIL_WIDTH, DETAIL_HEIGHT, 3.10f, 5.10f, 2.25f,
        3.65f);
    EXAMPLE_CHECK(ok, "_add_image(detail) failed");
    ok = _add_zoom_box(ctx->scene, overview);
    EXAMPLE_CHECK(ok, "_add_zoom_box() failed");
    ok = _add_detail_points(ctx->scene, detail);
    EXAMPLE_CHECK(ok, "_add_detail_points() failed");
    DvzVisual* specimen_cloud = NULL;
    ok = _add_3d_cloud(ctx->scene, specimen, &specimen_cloud);
    EXAMPLE_CHECK(ok, "_add_3d_cloud() failed");

    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    ok = _add_panel_scalebar(
        overview, DVZ_SCENE_ANCHOR_BOTTOM_LEFT, "m", 0.001, primary,
        DVZ_SCALEBAR_LABEL_ABOVE);
    EXAMPLE_CHECK(ok, "_add_panel_scalebar(overview) failed");
    ok = _add_panel_scalebar(
        detail, DVZ_SCENE_ANCHOR_BOTTOM_RIGHT, "m", 0.001, secondary,
        DVZ_SCALEBAR_LABEL_ABOVE);
    EXAMPLE_CHECK(ok, "_add_panel_scalebar(detail) failed");
    ok = _add_world_scalebar(specimen);
    EXAMPLE_CHECK(ok, "_add_world_scalebar() failed");

    DvzPanzoom* overview_panzoom =
        dvz_scenario_panzoom(ctx, overview, NULL, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(overview_panzoom != NULL, "failed to bind overview panzoom controller");
    DvzPanzoom* detail_panzoom =
        dvz_scenario_panzoom(ctx, detail, NULL, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(detail_panzoom != NULL, "failed to bind detail panzoom controller");

    DvzController* arcball_controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to bind arcball controller");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, specimen, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");
    dvz_arcball_set(arcball, (vec3){+0.56f, -0.18f, +0.30f});

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[1] = 1.0f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    state->specimen_rotation = dvz_track_rotation(&rotation_desc);
    EXAMPLE_CHECK(state->specimen_rotation != NULL, "dvz_track_rotation(specimen) failed");
    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = state->specimen_rotation;
    state->specimen_animation =
        dvz_anim_visual_transform(ctx->scene, specimen_cloud, &transform_desc);
    EXAMPLE_CHECK(
        state->specimen_animation != NULL, "dvz_anim_visual_transform(specimen) failed");
    dvz_anim_set_interaction_policy(
        state->specimen_animation, arcball_controller, DVZ_ANIM_INTERACTION_PAUSE, 0.0);
    dvz_anim_set_speed(state->specimen_animation, ROTATION_SPEED_RAD_PER_SEC);
    dvz_anim_start(state->specimen_animation, 0.0);

    ok = true;
cleanup:
    return ok;
}



/**
 * Destroy the scale-aware measurement workflow scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ScalebarMeasurementState* state = (ScalebarMeasurementState*)user;
    if (state == NULL)
        return;
    dvz_track_destroy(state->specimen_rotation);
    free(state);
}



DvzScenarioSpec dvz_showcase_scalebar_measurement_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "scalebar_measurement_workflow",
        .title = "scalebar_measurement",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_IMAGE_VISUAL |
                        DVZ_SCENARIO_REQ_TEXT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_ARCBALL |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/**
 * Run the scale-aware measurement workflow through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_scalebar_measurement_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
