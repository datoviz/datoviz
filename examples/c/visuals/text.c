/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text - This example places retained semantic text items in panel coordinates.
 *
 * What to look for: each text item provides a string, x/y screen position, font size, angle, and
 * color role, then the text system lowers it to atlas glyphs. Compare the headline, smaller
 * annotations, and rotated label to see how titles, units, and short scientific notes can be added
 * without managing glyph quads manually.
 *
 * Scenario: visual.text
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/text
 * Run:    ./build/examples/c/visuals/text --live
 * Smoke:  ./build/examples/c/visuals/text --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define TEXT_COUNT 5u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_text_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TextVisualState
{
    DvzText* text;
    uint32_t logical_width;
    uint32_t logical_height;
} TextVisualState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the uniform font scale from the design-space text layout to the current panel.
 *
 * @param logical_width current logical panel width
 * @param logical_height current logical panel height
 * @return uniform layout scale
 */
static float _font_scale(uint32_t logical_width, uint32_t logical_height)
{
    if (logical_width == 0 || logical_height == 0)
        return 1.0f;
    const float sx = (float)logical_width / (float)WIDTH;
    const float sy = (float)logical_height / (float)HEIGHT;
    return sx < sy ? sx : sy;
}



/**
 * Update retained text items for the current logical panel size.
 *
 * @param state text visual state
 * @param logical_width current logical panel width
 * @param logical_height current logical panel height
 * @return true when the text items were updated
 */
static bool _set_text_items(
    TextVisualState* state, uint32_t logical_width, uint32_t logical_height)
{
    ANN(state);
    ANN(state->text);

    if (logical_width == 0 || logical_height == 0)
        return false;

    if (state->logical_width == logical_width && state->logical_height == logical_height)
        return true;

    const float sx = (float)logical_width / (float)WIDTH;
    const float sy = (float)logical_height / (float)HEIGHT;
    const float scale = _font_scale(logical_width, logical_height);

    const char* strings[TEXT_COUNT] = {
        "Retained text",       "semantic strings, panel anchored",
        "MSDF atlas renderer", "screen placement in logical pixels",
        "rotated label",
    };
    const float x[TEXT_COUNT] = {128.0f, 132.0f, 134.0f, 136.0f, 845.0f};
    const float y[TEXT_COUNT] = {180.0f, 310.0f, 415.0f, 510.0f, 585.0f};
    const float sizes[TEXT_COUNT] = {60.0f, 34.0f, 28.0f, 22.0f, 26.0f};
    const float angles[TEXT_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, -0.34f};
    const ExampleStyleColorRole roles[TEXT_COUNT] = {
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
        EXAMPLE_STYLE_COLOR_WARNING,
    };

    DvzTextItem items[TEXT_COUNT] = {0};
    for (uint32_t i = 0; i < TEXT_COUNT; i++)
    {
        DvzColor color = example_graphite_cyan_color(roles[i]);
        items[i] = (DvzTextItem){DVZ_STRUCT_INIT_FIELDS(DvzTextItem),
            .string = strings[i],
            .position = {x[i] * sx, y[i] * sy, 0.0},
            .anchor = {0.0f, 0.5f},
            .size_px = sizes[i] * scale,
            .color = color,
            .angle = angles[i]};
    }
    if (dvz_text_set_items(state->text, items, TEXT_COUNT) != 0)
        return false;

    state->logical_width = logical_width;
    state->logical_height = logical_height;
    return true;
}



/**
 * Add the retained text examples to one panel.
 *
 * @param panel target panel
 * @param logical_width current logical panel width
 * @param logical_height current logical panel height
 * @param out_state output text visual state
 * @return true when all text objects were added
 */
static bool _add_texts(
    DvzPanel* panel, uint32_t logical_width, uint32_t logical_height, TextVisualState** out_state)
{
    ANN(panel);
    ANN(out_state);
    *out_state = NULL;

    TextVisualState* state = (TextVisualState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;

    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        goto error;
    state->text = text;

    DvzTextStyle style = dvz_text_style();
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    if (dvz_text_set_style(text, &style) != 0)
        goto error;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    if (dvz_text_set_placement(text, &placement) != 0)
        goto error;

    if (!_set_text_items(state, logical_width, logical_height))
        goto error;

    *out_state = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained semantic text scenario.
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

    TextVisualState* state = NULL;
    if (!_add_texts(panel, ctx->width, ctx->height, &state))
        return false;
    if (out_user != NULL)
        *out_user = state;
    return true;
}



/**
 * Handle portable scenario events.
 *
 * @param ctx scenario context
 * @param event portable event
 * @param user scenario state
 */
static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    TextVisualState* state = (TextVisualState*)user;
    if (ctx == NULL || event == NULL || state == NULL)
        return;
    if (event->kind != DVZ_SCENARIO_EVENT_RESIZE)
        return;

    (void)_set_text_items(state, ctx->width, ctx->height);
}



/**
 * Free the text visual state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the semantic text scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_text_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_text",
        .title = "Text",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .event = _scenario_event,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained semantic text example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_text_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
