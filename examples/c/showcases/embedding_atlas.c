/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* embedding_atlas - interactive prepared AI embedding atlas.
 *
 * Scenario: showcase_embedding_atlas
 * Style: showcase, graphite_cyan, 1600x1200 capture target
 *
 * Prepared data is loaded from `.cache/datoviz/examples/embedding_atlas/prepared/`.
 * Generate it with:
 *
 *   python tools/data/prepare_embedding_atlas.py --force
 *
 * Build:  just example-c showcases/embedding_atlas
 * Run:    ./build/examples/c/showcases/embedding_atlas --live
 * Smoke:  ./build/examples/c/showcases/embedding_atlas --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define EMBEDDING_CACHE_DIR        ".cache/datoviz/examples/embedding_atlas/prepared"
#define EMBEDDING_XY_PATH          EMBEDDING_CACHE_DIR "/xy.f32"
#define EMBEDDING_CLUSTER_PATH     EMBEDDING_CACHE_DIR "/cluster.u16"
#define EMBEDDING_COLOR_PATH       EMBEDDING_CACHE_DIR "/color.rgba8"
#define EMBEDDING_MAX_POINTS       200000u
#define EMBEDDING_CLUSTER_COUNT    6u
#define QUERY_ID                   31u

static const char* CLUSTER_NAMES[EMBEDDING_CLUSTER_COUNT] = {
    "syntax", "vision", "audio", "planning", "retrieval", "control",
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct EmbeddingAtlasData
{
    vec3* positions;
    DvzColor* colors;
    float* diameters;
    uint16_t* clusters;
    uint32_t count;
} EmbeddingAtlasData;


typedef struct EmbeddingAtlasState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzHover* hover;
    DvzOverlayCard* readout;
    EmbeddingAtlasData data;
    DvzQueryResult latest_hover_query;
    bool has_hover_query;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
} EmbeddingAtlasState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Free loaded embedding data.
 *
 * @param data embedding data
 */
static void _free_data(EmbeddingAtlasData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->clusters);
    dvz_free(data->diameters);
    dvz_free(data->colors);
    dvz_free(data->positions);
    memset(data, 0, sizeof(*data));
}


/**
 * Free the scenario state.
 *
 * @param state embedding-atlas state
 */
static void _free_state(EmbeddingAtlasState* state)
{
    if (state == NULL)
        return;
    _free_data(&state->data);
    dvz_free(state);
}


/**
 * Return file size in bytes.
 *
 * @param path file path
 * @param out_size output size
 * @return true on success
 */
static bool _file_size(const char* path, uint64_t* out_size)
{
    ANN(path);
    ANN(out_size);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return false;
    }
    long size = ftell(fp);
    fclose(fp);
    if (size < 0)
        return false;
    *out_size = (uint64_t)size;
    return true;
}


/**
 * Read an exact byte count from a file.
 *
 * @param path file path
 * @param data output buffer
 * @param size expected byte count
 * @return true on success
 */
static bool _read_exact(const char* path, void* data, uint64_t size)
{
    ANN(path);
    ANN(data);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    bool ok = fread(data, 1, (size_t)size, fp) == (size_t)size;
    fclose(fp);
    return ok;
}


/**
 * Load prepared embedding arrays from the local cache.
 *
 * @param data output embedding data
 * @return true when cache data was loaded
 */
static bool _load_cache(EmbeddingAtlasData* data)
{
    ANN(data);

    uint64_t xy_bytes = 0;
    if (!_file_size(EMBEDDING_XY_PATH, &xy_bytes) || xy_bytes == 0 || xy_bytes % (2u * sizeof(float)) != 0)
        return false;
    const uint32_t count = (uint32_t)(xy_bytes / (2u * sizeof(float)));
    if (count == 0 || count > EMBEDDING_MAX_POINTS)
        return false;

    uint64_t cluster_bytes = 0;
    uint64_t color_bytes = 0;
    if (!_file_size(EMBEDDING_CLUSTER_PATH, &cluster_bytes) ||
        !_file_size(EMBEDDING_COLOR_PATH, &color_bytes))
        return false;
    if (cluster_bytes != (uint64_t)count * sizeof(uint16_t) ||
        color_bytes != (uint64_t)count * sizeof(DvzColor))
        return false;

    float* xy = (float*)dvz_calloc((uint64_t)count * 2u, sizeof(*xy));
    data->positions = (vec3*)dvz_calloc(count, sizeof(*data->positions));
    data->colors = (DvzColor*)dvz_calloc(count, sizeof(*data->colors));
    data->diameters = (float*)dvz_calloc(count, sizeof(*data->diameters));
    data->clusters = (uint16_t*)dvz_calloc(count, sizeof(*data->clusters));
    if (
        xy == NULL || data->positions == NULL || data->colors == NULL ||
        data->diameters == NULL || data->clusters == NULL)
    {
        dvz_free(xy);
        _free_data(data);
        return false;
    }

    bool ok = _read_exact(EMBEDDING_XY_PATH, xy, xy_bytes) &&
              _read_exact(EMBEDDING_CLUSTER_PATH, data->clusters, cluster_bytes) &&
              _read_exact(EMBEDDING_COLOR_PATH, data->colors, color_bytes);
    if (!ok)
    {
        dvz_free(xy);
        _free_data(data);
        return false;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        data->positions[i][0] = xy[2u * i + 0u];
        data->positions[i][1] = xy[2u * i + 1u];
        data->positions[i][2] = 0.0f;
        data->diameters[i] = 5.2f;
        data->clusters[i] %= EMBEDDING_CLUSTER_COUNT;
    }
    dvz_free(xy);

    data->count = count;
    return true;
}


/**
 * Load prepared cache data.
 *
 * @param data output embedding data
 * @return true on success
 */
static bool _load_data(EmbeddingAtlasData* data)
{
    ANN(data);
    memset(data, 0, sizeof(*data));

    if (_load_cache(data))
        return true;

    dvz_fprintf(
        stderr, "embedding_atlas: missing prepared cache. Run "
                "`python tools/data/prepare_embedding_atlas.py --force` from the repository root.\n");
    return false;
}


/**
 * Convert data-space embedding positions into panel visual positions.
 *
 * @param panel panel with configured data domain
 * @param data embedding data
 * @return true on success
 */
static bool _convert_positions(DvzPanel* panel, EmbeddingAtlasData* data)
{
    ANN(panel);
    ANN(data);
    if (data->positions == NULL || data->count == 0)
        return false;

    vec3* visual_positions = (vec3*)dvz_calloc(data->count, sizeof(*visual_positions));
    if (visual_positions == NULL)
        return false;
    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data->positions, (float*)visual_positions, data->count);
    if (rc == 0)
        memcpy(data->positions, visual_positions, (uint64_t)data->count * sizeof(*data->positions));
    dvz_free(visual_positions);
    return rc == 0;
}


/**
 * Format and apply the overlay-card readout.
 *
 * @param state embedding-atlas state
 * @param query optional query result
 */
static void _update_readout(EmbeddingAtlasState* state, const DvzQueryResult* query)
{
    if (state == NULL || state->readout == NULL)
        return;

    char text[160] = {0};
    if (
        query != NULL && query->status == DVZ_QUERY_STATUS_HIT && query->hit &&
        query->visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT &&
        query->resolved_target == DVZ_SCENE_TARGET_ITEM && query->resolved_id < state->data.count)
    {
        const uint32_t id = (uint32_t)query->resolved_id;
        const uint32_t cluster = state->data.clusters[id] % EMBEDDING_CLUSTER_COUNT;
        snprintf(
            text, sizeof(text), "Embedding  item %" PRIu32 "  cluster %s  selected %" PRIu32
                                "  source prepared",
            id, CLUSTER_NAMES[cluster], dvz_selection_count(state->selection));
    }
    else
    {
        snprintf(
            text, sizeof(text), "Embedding Atlas  %" PRIu32 " points  selected %" PRIu32,
            state->data.count, dvz_selection_count(state->selection));
    }
    dvz_overlay_card_set_text(state->readout, text);
}


/**
 * Toggle retained selection for one queried embedding point.
 *
 * @param state embedding-atlas state
 * @param query point item query result
 */
static void _toggle_embedding_selection(EmbeddingAtlasState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    if (
        query->status != DVZ_QUERY_STATUS_HIT || !query->hit ||
        query->visual_family != DVZ_SCENE_VISUAL_FAMILY_POINT ||
        query->resolved_target != DVZ_SCENE_TARGET_ITEM || query->resolved_id >= state->data.count)
        return;

    if (dvz_selection_apply_query(state->selection, query) != 0)
        fprintf(stderr, "dvz_selection_apply_query() failed\n");
    _update_readout(state, query);
    fprintf(stdout, "toggle embedding id=%" PRIu64 "\n", query->resolved_id);
}


/**
 * Add the retained point visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param data embedding data
 * @return true on success
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel, const EmbeddingAtlasData* data)
{
    ANN(scene);
    ANN(panel);
    ANN(data);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data->positions, .item_count = data->count},
        {.attr_name = "color", .data = data->colors, .item_count = data->count},
        {.attr_name = "diameter", .data = data->diameters, .item_count = data->count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}


/**
 * Add a screen-space hover/selection readout card.
 *
 * @param panel panel owning the overlay
 * @return created card, or NULL
 */
static DvzOverlayCard* _add_readout(DvzPanel* panel)
{
    ANN(panel);

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    if (overlay == NULL)
        return NULL;

    DvzOverlayCardStyle style = dvz_overlay_card_style();
    DvzColor panel_bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.background_color = dvz_color_rgba(panel_bg.r, panel_bg.g, panel_bg.b, 226u);
    style.text_color = text;
    style.padding_px[0] = 12.0f;
    style.padding_px[1] = 7.0f;
    style.min_width_px = 520.0f;
    style.height_px = 34.0f;
    style.glyph_advance_px = 7.5f;
    style.text_size_px = 14.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.max_text_chars = 160u;

    return dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "Embedding Atlas",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT,
            .offset_px = {28.0f, -46.0f},
            .style = &style,
        });
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record pointer position and click intent in panel coordinates.
 *
 * @param event portable pointer event
 * @param user_data embedding-atlas state
 */
static void _embedding_pointer(const DvzScenarioPointerEvent* event, void* user_data)
{
    EmbeddingAtlasState* state = (EmbeddingAtlasState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (
        event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_PRESS)
        return;

    state->cursor_valid = dvz_scenario_panel_pointer_position(
        state->panel, event, &state->cursor_x, &state->cursor_y);
    if (event->type == DVZ_SCENARIO_POINTER_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        if (!state->cursor_valid)
            return;
        if (state->has_hover_query)
            _toggle_embedding_selection(state, &state->latest_hover_query);
        else
        {
            dvz_selection_clear(state->selection);
            _update_readout(state, NULL);
        }
    }
}


/**
 * Consume embedding query results, update hover/readout, and queue the next query.
 *
 * @param ctx scenario context
 * @param user_data embedding-atlas state
 */
static void _embedding_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    EmbeddingAtlasState* state = (EmbeddingAtlasState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    bool saw_query = false;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id != QUERY_ID)
            continue;

        saw_query = true;
        if (dvz_hover_apply_query(state->hover, &query) != 0)
            fprintf(stderr, "dvz_hover_apply_query() failed\n");
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM &&
            query.resolved_id < state->data.count)
        {
            state->latest_hover_query = query;
            state->has_hover_query = true;
            _update_readout(state, &query);
        }
        else
        {
            state->has_hover_query = false;
            _update_readout(state, NULL);
        }
    }
    if (saw_query && !state->has_hover_query)
        dvz_hover_clear(state->hover);

    if (state->cursor_valid)
    {
        DvzQueryRequest request = dvz_query_request();
        request.request_id = QUERY_ID;
        request.target = DVZ_SCENE_TARGET_ITEM;
        request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

        if (dvz_scenario_panel_query(state->panel, state->cursor_x, state->cursor_y, &request) != 0)
            fprintf(stderr, "dvz_scenario_panel_query() failed\n");
    }
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
    (void)ctx;
    if (event == NULL)
        return;
    if (event->kind == DVZ_SCENARIO_EVENT_POINTER)
        _embedding_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the embedding-atlas showcase.
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

    EmbeddingAtlasState* state = (EmbeddingAtlasState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (!_load_data(&state->data))
        goto error;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (!dvz_panel_set_reserve(
            panel, &(DvzPanelReserve){.left_px = 20.0f, .right_px = 16.0f, .bottom_px = 18.0f,
                                            .top_px = 12.0f}))
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -1.18, 1.18) != 0)
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.02, 1.02) != 0)
        goto error;
    if (!_convert_positions(panel, &state->data))
        goto error;

    DvzSelection* selection = dvz_selection(
        ctx->scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    if (selection == NULL)
        goto error;
    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT | DVZ_ITEM_STATE_VISUAL_SCALE;
    selection_style.selected.tint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    selection_style.selected.tint_mix = 1.0f;
    selection_style.selected.scale = 1.35f;
    selection_style.unselected.visual_flags = DVZ_ITEM_STATE_VISUAL_NONE;
    if (dvz_selection_set_visual_style(selection, &selection_style) != 0)
        goto error;

    DvzHover* hover = dvz_hover(
        ctx->scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    if (hover == NULL)
        goto error;
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = 1.55f;
    if (dvz_hover_set_visual_style(hover, &hover_style) != 0)
        goto error;

    if (!_add_points(ctx->scene, panel, &state->data))
        goto error;

    DvzOverlayCard* readout = _add_readout(panel);
    if (readout == NULL)
        goto error;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        goto error;

    state->scene = ctx->scene;
    state->panel = panel;
    state->selection = selection;
    state->hover = hover;
    state->readout = readout;
    _update_readout(state, NULL);

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}


/**
 * Destroy the embedding-atlas showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((EmbeddingAtlasState*)user);
}


/**
 * Return the embedding-atlas scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _embedding_atlas_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcase_embedding_atlas",
        .title = "embedding_atlas",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_QUERY_READBACK | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _embedding_post_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the embedding-atlas showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _embedding_atlas_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
