/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Non-CI diagnostic benchmark for retained GUI trees and embedded scene viewports.
 *
 * Build: cmake --build build --target example_c_lab_gui_viewport_bench
 * Run:   ./build/examples/c/lab/gui_viewport_bench --profile combined --warmup 16 --frames 120
 *
 * Profiles isolate an empty GUI, an expanded retained tree, an atlas-scale mesh in an embedded
 * viewport, or the combined workload. The measured run enables DVZ_APP_FRAME_TIMING so Datoviz
 * prints its normal per-view phase breakdown in addition to the compact benchmark summary.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/common/functions.h"
#include "datoviz/geom.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"


#define HOST_WIDTH 1200u
#define HOST_HEIGHT 800u
#define DEFAULT_ROWS 697u
#define DEFAULT_COLS 699u
#define DEFAULT_TREE_ROWS 1140u
#define DEFAULT_WARMUP 16u
#define DEFAULT_FRAMES 120u
#define TREE_EVENT_CAPACITY 8u


typedef enum BenchProfile
{
    BENCH_PROFILE_EMPTY,
    BENCH_PROFILE_TREE,
    BENCH_PROFILE_VIEWPORT,
    BENCH_PROFILE_COMBINED,
} BenchProfile;


typedef struct BenchConfig
{
    BenchProfile profile;
    const char* profile_name;
    uint32_t rows;
    uint32_t cols;
    uint32_t tree_rows;
    uint32_t warmup;
    uint32_t frames;
} BenchConfig;


typedef struct BenchState
{
    BenchProfile profile;
    DvzGuiTree* tree;
    DvzGuiViewport* viewport;
    DvzVisual* source_mesh;
    uint64_t callback_count;
    uint64_t tree_draw_count;
    uint64_t viewport_draw_count;
    uint64_t dropped_events;
} BenchState;


static bool _has_tree(BenchProfile profile)
{
    return profile == BENCH_PROFILE_TREE || profile == BENCH_PROFILE_COMBINED;
}


static bool _has_viewport(BenchProfile profile)
{
    return profile == BENCH_PROFILE_VIEWPORT || profile == BENCH_PROFILE_COMBINED;
}


static bool _parse_u32(const char* text, uint32_t* value)
{
    if (text == NULL || value == NULL)
        return false;
    char* end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (*text == '\0' || end == NULL || *end != '\0' || parsed > UINT32_MAX)
        return false;
    *value = (uint32_t)parsed;
    return true;
}


static bool _parse_args(int argc, char** argv, BenchConfig* cfg)
{
    *cfg = (BenchConfig){
        .profile = BENCH_PROFILE_COMBINED,
        .profile_name = "combined",
        .rows = DEFAULT_ROWS,
        .cols = DEFAULT_COLS,
        .tree_rows = DEFAULT_TREE_ROWS,
        .warmup = DEFAULT_WARMUP,
        .frames = DEFAULT_FRAMES,
    };
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc)
        {
            cfg->profile_name = argv[++i];
            if (strcmp(cfg->profile_name, "empty") == 0)
                cfg->profile = BENCH_PROFILE_EMPTY;
            else if (strcmp(cfg->profile_name, "tree") == 0)
                cfg->profile = BENCH_PROFILE_TREE;
            else if (strcmp(cfg->profile_name, "viewport") == 0)
                cfg->profile = BENCH_PROFILE_VIEWPORT;
            else if (strcmp(cfg->profile_name, "combined") == 0)
                cfg->profile = BENCH_PROFILE_COMBINED;
            else
                return false;
        }
        else if (strcmp(argv[i], "--rows") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->rows))
                return false;
        }
        else if (strcmp(argv[i], "--cols") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->cols))
                return false;
        }
        else if (strcmp(argv[i], "--tree-rows") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->tree_rows))
                return false;
        }
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->warmup))
                return false;
        }
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->frames))
                return false;
        }
        else
            return false;
    }
    return cfg->rows >= 2 && cfg->cols >= 2 && cfg->tree_rows >= 1 && cfg->frames > 0;
}


static bool _configure_tree(BenchState* state, uint32_t row_count)
{
    uint64_t* keys = (uint64_t*)calloc(row_count, sizeof(uint64_t));
    uint32_t* parents = (uint32_t*)calloc(row_count, sizeof(uint32_t));
    char(*storage)[40] = (char(*)[40])calloc(row_count, sizeof(*storage));
    const char** labels = (const char**)calloc(row_count, sizeof(char*));
    if (keys == NULL || parents == NULL || storage == NULL || labels == NULL)
        goto failure;

    uint32_t group = 0;
    for (uint32_t i = 0; i < row_count; i++)
    {
        keys[i] = UINT64_C(1000000) + i;
        labels[i] = storage[i];
        if (i == 0)
        {
            parents[i] = UINT32_MAX;
            (void)snprintf(storage[i], sizeof(storage[i]), "Synthetic atlas root");
        }
        else if ((i - 1) % 10 == 0)
        {
            group = i;
            parents[i] = 0;
            (void)snprintf(storage[i], sizeof(storage[i]), "Group %u", (i - 1) / 10);
        }
        else
        {
            parents[i] = group;
            (void)snprintf(storage[i], sizeof(storage[i]), "Region %u", i);
        }
    }

    state->tree = dvz_gui_tree("gui_viewport_bench_tree", DVZ_GUI_DATA_WIDGET_FLAGS_NONE);
    const bool ok =
        state->tree != NULL &&
        dvz_gui_tree_set_rows(
            state->tree, row_count, keys, parents, labels, NULL,
            DVZ_GUI_DATA_SET_FLAGS_NONE) == DVZ_OK &&
        dvz_gui_tree_expand_all(state->tree) == DVZ_OK;
    free(keys);
    free(parents);
    free(storage);
    free(labels);
    return ok;

failure:
    free(keys);
    free(parents);
    free(storage);
    free(labels);
    return false;
}


static void _gui_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    BenchState* state = (BenchState*)user_data;
    if (state == NULL)
        return;
    state->callback_count++;

    if (_has_viewport(state->profile) && state->source_mesh != NULL)
    {
        mat4 transform = {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        };
        transform[3][0] = (state->callback_count & 1u) != 0 ? 1e-5f : -1e-5f;
        (void)dvz_visual_set_transform(state->source_mesh, transform);
    }

    if (state->profile == BENCH_PROFILE_EMPTY)
    {
        if (dvz_gui_begin(gui, "Empty GUI", NULL, 0))
            dvz_gui_text(gui, "Synthetic empty-GUI baseline");
        dvz_gui_end(gui);
        return;
    }

    if (_has_tree(state->profile) && state->tree != NULL)
    {
        (void)dvz_gui_dock_window_once(gui, "Retained tree", DVZ_GUI_DOCK_SLOT_LEFT, 360.0f);
        if (dvz_gui_begin(gui, "Retained tree", NULL, 0))
        {
            DvzGuiDataEvent events[TREE_EVENT_CAPACITY] = {0};
            uint32_t written = 0;
            uint32_t dropped = 0;
            if (dvz_gui_tree_draw(
                    gui, state->tree, events, TREE_EVENT_CAPACITY, &written, &dropped) == DVZ_OK)
                state->tree_draw_count++;
            state->dropped_events += dropped;
        }
        dvz_gui_end(gui);
    }

    if (_has_viewport(state->profile) && state->viewport != NULL)
    {
        (void)dvz_gui_dock_window_once(gui, "Embedded viewport", DVZ_GUI_DOCK_SLOT_CENTER, 0.0f);
        if (dvz_gui_viewport_window(state->viewport, "Embedded viewport", NULL, 0))
            state->viewport_draw_count++;
    }
}


static bool _enable_frame_timing(void)
{
#if OS_WINDOWS
    return _putenv_s("DVZ_APP_FRAME_TIMING", "1") == 0;
#else
    return setenv("DVZ_APP_FRAME_TIMING", "1", 1) == 0;
#endif
}


int main(int argc, char** argv)
{
    BenchConfig cfg = {0};
    if (!_parse_args(argc, argv, &cfg))
    {
        fprintf(
            stderr,
            "usage: %s [--profile empty|tree|viewport|combined] [--rows N] [--cols N] "
            "[--tree-rows N] [--warmup N] [--frames N]\n",
            argv[0]);
        return 2;
    }

    int ret = 1;
    DvzScene* scene = dvz_scene();
    DvzFigure* host_figure = scene != NULL ? dvz_figure(scene, HOST_WIDTH, HOST_HEIGHT, 0) : NULL;
    DvzPanel* host_panel = host_figure != NULL ? dvz_panel_full(host_figure) : NULL;
    DvzFigure* source_figure = NULL;
    DvzGeometry* geometry = NULL;
    DvzApp* app = NULL;
    DvzView* host_view = NULL;
    DvzGuiViewport* viewport = NULL;
    BenchState state = {.profile = cfg.profile};
    if (host_panel == NULL)
        goto cleanup;

    if (_has_viewport(cfg.profile))
    {
        source_figure = dvz_figure(scene, 800, 640, 0);
        DvzPanel* source_panel = source_figure != NULL ? dvz_panel_full(source_figure) : NULL;
        DvzVisual* mesh = dvz_mesh(scene, 0);
        state.source_mesh = mesh;
        DvzGeometrySurfaceGridDesc desc = dvz_geometry_surface_grid_desc();
        desc.rows = cfg.rows;
        desc.cols = cfg.cols;
        desc.origin[0] = -0.9;
        desc.origin[1] = -0.9;
        desc.col_basis[0] = 1.8 / (double)(cfg.cols - 1);
        desc.row_basis[1] = 1.8 / (double)(cfg.rows - 1);
        geometry = dvz_geometry_surface_grid(&desc);
        if (source_panel == NULL || mesh == NULL || geometry == NULL ||
            dvz_mesh_set_geometry(mesh, geometry) != DVZ_OK ||
            dvz_panel_add_visual(source_panel, mesh, NULL) != DVZ_OK)
            goto cleanup;
    }

    if (_has_tree(cfg.profile) && !_configure_tree(&state, cfg.tree_rows))
        goto cleanup;

    app = dvz_app(scene);
    host_view = app != NULL
                    ? dvz_view_window(app, host_figure, HOST_WIDTH, HOST_HEIGHT, "gui_viewport_bench")
                    : NULL;
    DvzGui* gui = host_view != NULL ? dvz_view_gui(host_view, NULL) : NULL;
    if (gui == NULL)
        goto cleanup;

    if (_has_viewport(cfg.profile))
    {
        DvzGuiViewportConfig viewport_config = dvz_gui_viewport_config();
        viewport_config.initial_width = 800;
        viewport_config.initial_height = 640;
        viewport = dvz_gui_viewport(gui, source_figure, &viewport_config);
        if (viewport == NULL)
            goto cleanup;
        state.viewport = viewport;
    }
    if (dvz_view_set_gui_callback(host_view, _gui_callback, &state) != DVZ_OK)
        goto cleanup;

    dvz_app_run(app, cfg.warmup);
    state.callback_count = 0;
    state.tree_draw_count = 0;
    state.viewport_draw_count = 0;
    state.dropped_events = 0;
    if (!_enable_frame_timing())
        goto cleanup;
    const uint64_t start = dvz_time_monotonic_ns();
    dvz_app_run(app, cfg.frames);
    const uint64_t elapsed_ns = dvz_time_monotonic_ns() - start;

    printf(
        "gui_viewport_bench: profile=%s rows=%u cols=%u tree_rows=%u warmup=%u frames=%u"
        " elapsed_ms=%.3f ms_per_frame=%.3f callbacks=%" PRIu64 " tree_draws=%" PRIu64
        " viewport_draws=%" PRIu64 " dropped_events=%" PRIu64 "\n",
        cfg.profile_name, cfg.rows, cfg.cols, cfg.tree_rows, cfg.warmup, cfg.frames,
        (double)elapsed_ns * 1e-6, (double)elapsed_ns * 1e-6 / (double)cfg.frames,
        state.callback_count, state.tree_draw_count, state.viewport_draw_count,
        state.dropped_events);
    ret = state.callback_count == cfg.frames ? 0 : 1;

cleanup:
    if (host_view != NULL)
        (void)dvz_view_set_gui_callback(host_view, NULL, NULL);
    if (viewport != NULL && host_view != NULL && dvz_view_canvas(host_view) != NULL)
        dvz_gui_viewport_destroy(viewport);
    dvz_gui_tree_destroy(state.tree);
    if (app != NULL)
        dvz_app_destroy(app);
    if (geometry != NULL)
        dvz_geometry_destroy(geometry);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
