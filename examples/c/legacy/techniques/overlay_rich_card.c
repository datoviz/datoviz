/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* overlay_rich_card - public rich overlay cards lowered through a raster image.
 *
 * Build:  just example-c overlay_rich_card
 * Run:    ./build/examples/c/techniques/overlay_rich_card
 * Smoke:  ./build/examples/c/techniques/overlay_rich_card 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       980
#define HEIGHT      680
#define FIELD_COLS  16u
#define FIELD_ROWS  10u
#define FIELD_WIDTH 320u
#define FIELD_HEIGHT 200u
#define FIELD_EXTENT_X 1.9f
#define FIELD_EXTENT_Y 1.2f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LiveProbeState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzPanelDesc panel_desc;
    DvzOverlayCard* rich;
    double figure_width;
    double figure_height;
    bool cursor_valid;
    bool cursor_dirty;
    bool query_pending;
    double cursor_x;
    double cursor_y;
    double last_panel_position[2];
    double last_rgba[4];
    bool last_hit;
    bool has_last_result;
} LiveProbeState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Linearly interpolate one 8-bit channel.
 *
 * @param a start channel
 * @param b end channel
 * @param t interpolation factor in [0, 255]
 * @return interpolated channel
 */
static uint8_t _lerp_u8(uint8_t a, uint8_t b, uint32_t t)
{
    if (t > 255u)
        t = 255u;
    uint32_t inv = 255u - t;
    return (uint8_t)((inv * (uint32_t)a + t * (uint32_t)b) / 255u);
}


/**
 * Compute a deterministic scalar value for one field bin.
 *
 * @param col field column
 * @param row field row
 * @return scalar value in [0, 255]
 */
static uint32_t _field_value(uint32_t col, uint32_t row)
{
    uint32_t ridge = 17u * col + 29u * row + 11u * ((col + 3u) * (row + 5u));
    uint32_t hotspot = col >= 9u && col <= 12u && row >= 5u && row <= 7u ? 72u : 0u;
    uint32_t value = (ridge % 176u) + 34u + hotspot;
    return value > 255u ? 255u : value;
}


/**
 * Map a scalar field value to the example palette.
 *
 * @param value scalar value in [0, 255]
 * @param rgba output RGBA8 color
 */
static void _field_color(uint32_t value, uint8_t rgba[4])
{
    if (rgba == NULL)
        return;

    if (value < 128u)
    {
        uint32_t t = 2u * value;
        rgba[0] = _lerp_u8(27, 45, t);
        rgba[1] = _lerp_u8(54, 157, t);
        rgba[2] = _lerp_u8(88, 183, t);
    }
    else
    {
        uint32_t t = 2u * (value - 128u);
        rgba[0] = _lerp_u8(45, 232, t);
        rgba[1] = _lerp_u8(157, 176, t);
        rgba[2] = _lerp_u8(183, 77, t);
    }
    rgba[3] = 255;
}


/**
 * Return whether a texture pixel is on a cell boundary.
 *
 * @param x texture x coordinate
 * @param y texture y coordinate
 * @return true if the pixel belongs to a grid line
 */
static bool _field_grid_pixel(uint32_t x, uint32_t y)
{
    uint32_t cell_x = FIELD_WIDTH / FIELD_COLS;
    uint32_t cell_y = FIELD_HEIGHT / FIELD_ROWS;
    return x % cell_x == 0u || y % cell_y == 0u;
}


/**
 * Fill a deterministic binned field used by the probe overlay-card example.
 *
 * @param pixels output RGBA8 pixels
 */
static void _fill_probe_field(uint8_t pixels[FIELD_WIDTH * FIELD_HEIGHT * 4])
{
    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            uint32_t col = (x * FIELD_COLS) / FIELD_WIDTH;
            uint32_t row = (y * FIELD_ROWS) / FIELD_HEIGHT;
            uint32_t value = _field_value(col, row);

            uint8_t rgba[4] = {0};
            _field_color(value, rgba);
            if (_field_grid_pixel(x, y))
            {
                rgba[0] = (uint8_t)((3u * (uint32_t)rgba[0]) / 5u);
                rgba[1] = (uint8_t)((3u * (uint32_t)rgba[1]) / 5u);
                rgba[2] = (uint8_t)((3u * (uint32_t)rgba[2]) / 5u);
            }

            uint32_t idx = 4u * (y * FIELD_WIDTH + x);
            pixels[idx + 0] = rgba[0];
            pixels[idx + 1] = rgba[1];
            pixels[idx + 2] = rgba[2];
            pixels[idx + 3] = rgba[3];
        }
    }
}


/**
 * Add the binned field as a normal image visual.
 *
 * @param scene the scene
 * @param panel destination panel
 * @param pixels RGBA8 field pixels
 * @return true on success, false on error
 */
static bool _add_probe_field(
    DvzScene* scene, DvzPanel* panel, uint8_t pixels[FIELD_WIDTH * FIELD_HEIGHT * 4])
{
    DvzVisual* field = dvz_image(scene, 0);
    if (field == NULL)
        return false;

    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extent[1] = {{FIELD_EXTENT_X, FIELD_EXTENT_Y}};
    vec2 anchor[1] = {{0.0f, 0.0f}};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = position, .item_count = 1},
        {.attr_name = "extent", .data = extent, .item_count = 1},
        {.attr_name = "anchor", .data = anchor, .item_count = 1},
    };

    int rc = dvz_visual_set_data_many(field, updates, 3);
    if (rc != 0)
        return false;
    rc = dvz_visual_set_texture_rgba8(field, (const uint8_t*)pixels, FIELD_WIDTH, FIELD_HEIGHT);
    if (rc != 0)
        return false;
    dvz_visual_set_query_capabilities(field, DVZ_QUERY_CAPABILITY_PIXEL);
    rc = dvz_panel_add_visual(panel, field, &(DvzVisualAttachDesc){.z_layer = -1});
    return rc == 0;
}


/**
 * Create a compact plain overlay card with GPU text.
 *
 * @param overlay the overlay
 * @return the created card, or NULL on error
 */
static DvzOverlayCard* _add_plain_header(DvzOverlay* overlay)
{
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = dvz_color_rgba(9, 16, 26, 238);
    style.text_color = dvz_color_rgb(241, 246, 255);
    style.padding_px[0] = 14.0f;
    style.padding_px[1] = 8.0f;
    style.height_px = 34.0f;
    style.min_width_px = 290.0f;
    style.glyph_advance_px = 8.1f;
    style.text_size_px = 16.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;

    return dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "image probe readout",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_TOP_LEFT,
            .offset_px = {18.0f, 18.0f},
            .style = &style,
        });
}


/**
 * Convert a normalized probe channel to one byte.
 *
 * @param value normalized channel value
 * @return byte channel value in [0, 255]
 */
static uint32_t _probe_channel_byte(double value)
{
    if (value < 0.0)
        value = 0.0;
    if (value > 1.0)
        value = 1.0;
    return (uint32_t)(255.0 * value + 0.5);
}


/**
 * Return a compact label for a sampled probe intensity.
 *
 * @param intensity normalized RGB intensity
 * @return response label
 */
static const char* _probe_response_label(double intensity)
{
    if (intensity >= 0.72)
        return "high";
    if (intensity <= 0.28)
        return "low";
    return "normal";
}


/**
 * Return whether a live query result differs from the last displayed result.
 *
 * @param state live probe state
 * @param query query result to compare
 * @return true when the card should be refreshed
 */
static bool _query_changed(const LiveProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != query->hit)
        return true;

    for (uint32_t i = 0; i < 2; i++)
    {
        double delta = query->panel_position[i] - state->last_panel_position[i];
        if (delta < 0.0)
            delta = -delta;
        if (delta >= 0.5)
            return true;
    }

    if (!query->hit)
        return false;

    for (uint32_t i = 0; i < 4; i++)
    {
        double delta = query->vector[i] - state->last_rgba[i];
        if (delta < 0.0)
            delta = -delta;
        if (delta >= (1.0 / 255.0))
            return true;
    }
    return false;
}


/**
 * Store the last rich-card query result.
 *
 * @param state live probe state
 * @param query query result to store
 */
static void _store_query_result(LiveProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;

    state->has_last_result = true;
    state->last_hit = query->hit;
    state->last_panel_position[0] = query->panel_position[0];
    state->last_panel_position[1] = query->panel_position[1];
    for (uint32_t i = 0; i < 4; i++)
        state->last_rgba[i] = query->vector[i];
}


/**
 * Convert the latest window cursor position to panel-local probe coordinates.
 *
 * @param state live probe state
 * @param out_x output panel-local x coordinate
 * @param out_y output panel-local y coordinate
 * @return true when the cursor is inside the panel rectangle
 */
static bool _probe_cursor_panel_position(const LiveProbeState* state, double* out_x, double* out_y)
{
    if (state == NULL || out_x == NULL || out_y == NULL)
        return false;

    double x0 = state->panel_desc.x * state->figure_width;
    double y0 = state->panel_desc.y * state->figure_height;
    double w = state->panel_desc.width * state->figure_width;
    double h = state->panel_desc.height * state->figure_height;
    if (w <= 0.0 || h <= 0.0)
        return false;

    double x = state->cursor_x - x0;
    double y = state->cursor_y - y0;
    if (x < 0.0 || x > w || y < 0.0 || y > h)
        return false;

    *out_x = x;
    *out_y = y;
    return true;
}


/**
 * Set the rich-text payload of the live probe card.
 *
 * @param card overlay card to update
 * @param source rich text source
 * @return 0 on success, -1 on error
 */
static int _set_probe_card_rich_text(DvzOverlayCard* card, const char* source)
{
    if (card == NULL || source == NULL)
        return -1;
    return dvz_overlay_card_set_rich_text(
        card,
        &(DvzOverlayRichTextDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayRichTextDesc),
            .source = source,
            .max_width_px = 330.0f,
            .char_width_px = 7.2f,
            .line_height_px = 13.5f,
            .scale = 2.0f,
            .text_color = {237, 242, 248, 255},
            .background_color = {0, 0, 0, 0},
        });
}


/**
 * Update the rich overlay card from one resolved GPU image query result.
 *
 * @param state live probe state
 * @param query resolved query result
 */
static void _update_probe_card_from_result(LiveProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || state->rich == NULL || query == NULL)
        return;

    char source[512] = {0};
    int n = 0;
    if (
        query->status == DVZ_QUERY_STATUS_HIT && query->hit &&
        query->value_kind == DVZ_QUERY_VALUE_VEC4)
    {
        uint32_t r = _probe_channel_byte(query->vector[0]);
        uint32_t g = _probe_channel_byte(query->vector[1]);
        uint32_t b = _probe_channel_byte(query->vector[2]);
        uint32_t a = _probe_channel_byte(query->vector[3]);
        double intensity = (0.2126 * query->vector[0]) + (0.7152 * query->vector[1]) +
                           (0.0722 * query->vector[2]);
        const char* response = _probe_response_label(intensity);
        n = dvz_snprintf(
            source, sizeof(source),
            "<b>Live image probe</b> at x=%0.0f y=%0.0f. "
            "<u>GPU RGBA(%u,%u,%u,%u)</u> gives intensity %.2f, with "
            "<color=#F7BB54>%s response</color> from the image visual.",
            query->panel_position[0], query->panel_position[1], r, g, b, a, intensity,
            response);
    }
    else
    {
        n = dvz_snprintf(
            source, sizeof(source),
            "<b>Live image probe</b> at x=%0.0f y=%0.0f. "
            "<color=#E0567A>No image sample</color> under the cursor.",
            query->panel_position[0], query->panel_position[1]);
    }
    if (n <= 0 || (size_t)n >= sizeof(source))
        return;

    int rc = _set_probe_card_rich_text(state->rich, source);
    if (rc == 0)
        _store_query_result(state, query);
}



/**
 * Queue one latest-wins live probe request for the current cursor.
 *
 * @param state live probe state
 */
static void _queue_probe_query(LiveProbeState* state)
{
    if (state == NULL || !state->cursor_valid || state->query_pending)
        return;

    double panel_x = 0.0;
    double panel_y = 0.0;
    if (!_probe_cursor_panel_position(state, &panel_x, &panel_y))
    {
        state->cursor_dirty = false;
        return;
    }

    int rc = dvz_panel_query(
        state->panel, panel_x, panel_y,
        &(DvzQueryRequest){
            .request_id = 0,
            .target = DVZ_SCENE_TARGET_PIXEL,
        });
    if (rc != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_query() failed\n");
        return;
    }

    state->cursor_dirty = false;
    state->query_pending = true;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Track the latest pointer position for live image probes.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data live probe state
 */
static void
_overlay_probe_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    LiveProbeState* state = (LiveProbeState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_CLICK)
        return;

    state->cursor_valid = true;
    state->cursor_dirty = true;
    state->cursor_x = event->pos[0];
    state->cursor_y = event->pos[1];
}


/**
 * Poll resolved query results and queue the next cursor query.
 *
 * @param win view whose frame just completed
 * @param user_data live probe state
 */
static void _overlay_probe_frame(DvzView* win, void* user_data)
{
    (void)win;
    LiveProbeState* state = (LiveProbeState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id == 0)
            state->query_pending = false;
        if (_query_changed(state, &query))
            _update_probe_card_from_result(state, &query);
    }

    if (state->cursor_dirty)
        _queue_probe_query(state);
}



/*************************************************************************************************/
/*  Main                                                                                         */
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

    DvzPanelDesc panel_desc = {.x = 0.045f, .y = 0.06f, .width = 0.91f, .height = 0.88f};
    DvzPanel* panel = dvz_panel(figure, &panel_desc);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.020f, 0.022f, 0.028f, 1.0f));

    uint8_t field_pixels[FIELD_WIDTH * FIELD_HEIGHT * 4] = {0};
    _fill_probe_field(field_pixels);
    bool ok = _add_probe_field(scene, panel, field_pixels);
    EXAMPLE_CHECK(ok, "failed to create probe field");

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    EXAMPLE_CHECK(overlay != NULL, "dvz_overlay() failed");
    EXAMPLE_CHECK(_add_plain_header(overlay) != NULL, "failed to create header overlay card");

    DvzOverlayCardStyle rich_style = dvz_overlay_card_style();
    rich_style.background_color = dvz_color_rgba(13, 20, 30, 242);
    rich_style.padding_px[0] = 16.0f;
    rich_style.padding_px[1] = 14.0f;
    rich_style.min_width_px = 330.0f;
    DvzOverlayCard* rich = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "fallback",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT,
            .offset_px = {22.0f, 22.0f},
            .style = &rich_style,
        });
    EXAMPLE_CHECK(rich != NULL, "failed to create rich overlay card shell");

    int rc = _set_probe_card_rich_text(
        rich,
        "<b>Live image probe</b> is ready. Move the cursor over the "
        "<color=#4AB8D9>image visual</color> to update this card from GPU-readback data.");
    EXAMPLE_CHECK(rc == 0, "failed to set rich overlay card text");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "overlay_rich_card");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    LiveProbeState state = {
        .scene = scene,
        .panel = panel,
        .panel_desc = panel_desc,
        .rich = rich,
        .figure_width = WIDTH,
        .figure_height = HEIGHT,
    };
    dvz_input_subscribe_pointer(router, _overlay_probe_pointer, &state);
    dvz_view_set_frame_callback(win, _overlay_probe_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
