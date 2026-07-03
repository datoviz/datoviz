/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* marker - retained marker visual with deterministic symbol, fill, stroke, size, and angle
 * variation. The top row is a triangle compass probe: angle is mathematical counter-clockwise and
 * triangle diameter_px is the screen-space bounding-box diameter.
 *
 * Scenario: visual.marker
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/marker
 * Run:    ./build/examples/c/visuals/marker --live
 * Smoke:  ./build/examples/c/visuals/marker --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define MARKER_COLS 7u
#define PI_F        3.14159265358979323846f



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_marker_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the marker symbol for one display column.
 *
 * @param index marker column index
 * @return marker symbol enum value
 */
static uint32_t _marker_symbol(uint32_t index)
{
    switch (index)
    {
    case 1:
        return DVZ_SYMBOL_SQUARE;
    case 2:
        return DVZ_SYMBOL_TRIANGLE;
    case 3:
        return DVZ_SYMBOL_DIAMOND;
    case 4:
        return DVZ_SYMBOL_CROSS;
    case 5:
        return DVZ_SYMBOL_RING;
    case 6:
        return DVZ_SYMBOL_TARGET;
    case 7:
        return DVZ_SYMBOL_ASTERISK;
    case 8:
        return DVZ_SYMBOL_CHEVRON;
    case 9:
        return DVZ_SYMBOL_CLOVER;
    case 10:
        return DVZ_SYMBOL_CLUB;
    case 11:
        return DVZ_SYMBOL_ARROW;
    case 12:
        return DVZ_SYMBOL_ELLIPSE;
    case 13:
        return DVZ_SYMBOL_HBAR;
    case 14:
        return DVZ_SYMBOL_HEART;
    case 15:
        return DVZ_SYMBOL_INFINITY;
    case 16:
        return DVZ_SYMBOL_PIN;
    case 17:
        return DVZ_SYMBOL_SPADE;
    case 18:
        return DVZ_SYMBOL_TAG;
    case 19:
        return DVZ_SYMBOL_VBAR;
    case 20:
        return DVZ_SYMBOL_ROUNDED_RECT;
    default:
        return DVZ_SYMBOL_DISC;
    }
}



/**
 * Return one graphite/cyan fill color role.
 *
 * @param row marker row index
 * @param col marker column index
 * @return marker fill color role
 */
static ExampleStyleColorRole _marker_fill_role(uint32_t row, uint32_t col)
{
    static const ExampleStyleColorRole roles[][MARKER_COLS] = {
        {
            EXAMPLE_STYLE_COLOR_TEXT,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_TEXT,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_WARNING,
        },
        {
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_TEXT,
        },
        {
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_WARNING,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_WARNING,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ERROR,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        },
    };
    const uint32_t row_count = DVZ_ARRAY_COUNT(roles);
    return roles[row % row_count][col % MARKER_COLS];
}



/**
 * Add one marker row with a visual-level stroke color role.
 *
 * @param scene scene owning the marker visual
 * @param panel panel receiving the marker visual
 * @param row marker row index
 * @param y row y coordinate
 * @param edge_role outline color role for the row
 * @return true when the row was added
 */
static bool
_add_marker_row(
    DvzScene* scene, DvzPanel* panel, uint32_t row, float y, ExampleStyleColorRole edge_role)
{
    DvzVisual* visual = dvz_marker(scene, 0);
    if (visual == NULL)
        return false;

    DvzColor edge_color = example_graphite_cyan_color(edge_role);
    edge_color.a = 245;

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.edge_color = edge_color;
    style.stroke_width_px = 2.25f;
    if (dvz_marker_set_style(visual, &style) != 0)
        return false;

    vec3 positions[MARKER_COLS] = {{0}};
    DvzColor colors[MARKER_COLS] = {{0}};
    float diameters[MARKER_COLS] = {0};
    float angles[MARKER_COLS] = {0};
    uint32_t symbols[MARKER_COLS] = {0};

    for (uint32_t col = 0; col < MARKER_COLS; col++)
    {
        const float t = MARKER_COLS > 1u ? (float)col / (float)(MARKER_COLS - 1u) : 0.0f;
        positions[col][0] = -0.78f + 1.56f * t;
        positions[col][1] = y;
        positions[col][2] = 0.0f;
        colors[col] = example_graphite_cyan_color(_marker_fill_role(row, col));
        colors[col].a = 238;
        diameters[col] = 42.0f + 8.0f * (float)((row + col) % 3u);
        angles[col] = 0.18f * (float)(row + col);
        symbols[col] = _marker_symbol(row * MARKER_COLS + col);
    }
    if (row == 0)
    {
        static const float compass_angles[MARKER_COLS] = {
            0.0f,
            0.5f * PI_F,
            PI_F,
            1.5f * PI_F,
            0.25f * PI_F,
            -0.25f * PI_F,
            0.0f,
        };
        static const float compass_y_offsets[MARKER_COLS] = {
            -0.06f,
            +0.07f,
            -0.02f,
            +0.05f,
            -0.08f,
            +0.03f,
            +0.00f,
        };
        for (uint32_t col = 0; col < MARKER_COLS; col++)
        {
            positions[col][1] = y + compass_y_offsets[col];
            diameters[col] = col + 1u == MARKER_COLS ? 50.0f : 64.0f;
            angles[col] = compass_angles[col];
            symbols[col] = col + 1u == MARKER_COLS ? DVZ_SYMBOL_SQUARE : DVZ_SYMBOL_TRIANGLE;
        }
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = MARKER_COLS},
        {.attr_name = "color", .data = colors, .item_count = MARKER_COLS},
        {.attr_name = "diameter_px", .data = diameters, .item_count = MARKER_COLS},
        {.attr_name = "angle", .data = angles, .item_count = MARKER_COLS},
        {.attr_name = "symbol", .data = symbols, .item_count = MARKER_COLS},
    };
    if (dvz_visual_set_data_many(visual, updates, 5) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained marker visual scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    const float row_y[] = {+0.58f, +0.12f, -0.36f};
    const ExampleStyleColorRole edge_roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };
    for (uint32_t row = 0; row < DVZ_ARRAY_COUNT(row_y); row++)
    {
        if (!_add_marker_row(ctx->scene, panel, row, row_y[row], edge_roles[row]))
            return false;
    }

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Return the marker visual scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_marker_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_marker",
        .title = "Marker",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained marker visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_marker_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
