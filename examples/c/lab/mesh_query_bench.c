/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Non-CI diagnostic benchmark for GPU-backed mesh queries.
 *
 * Build: just example-c lab/mesh_query_bench
 * Run:   ./build/examples/c/lab/mesh_query_bench --target face --warmup 4 --queries 20
 *
 * The default 697x699 indexed surface grid is close to the atlas workload that motivated this
 * benchmark: 487,203 retained vertices and 971,616 triangles. The first query is reported
 * separately because it creates and uploads the retained query-side resources. Later queries
 * expose steady-state command construction, submission, readback, and decode costs.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/canvas/enums.h"
#include "datoviz/common/functions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "query/internal.h"


#define DEFAULT_ROWS 697u
#define DEFAULT_COLS 699u
#define DEFAULT_WARMUP 4u
#define DEFAULT_QUERIES 20u
#define VIEW_WIDTH 256u
#define VIEW_HEIGHT 256u


typedef struct BenchConfig
{
    DvzSceneTargetKind target;
    const char* target_name;
    uint32_t rows;
    uint32_t cols;
    uint32_t warmup;
    uint32_t queries;
} BenchConfig;


typedef struct BenchStats
{
    uint64_t elapsed_ns;
    DvzSceneQueryTiming timing;
    uint64_t resolved;
    uint64_t hits;
} BenchStats;


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
        .target = DVZ_SCENE_TARGET_FACE,
        .target_name = "face",
        .rows = DEFAULT_ROWS,
        .cols = DEFAULT_COLS,
        .warmup = DEFAULT_WARMUP,
        .queries = DEFAULT_QUERIES,
    };
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--target") == 0 && i + 1 < argc)
        {
            cfg->target_name = argv[++i];
            if (strcmp(cfg->target_name, "item") == 0)
                cfg->target = DVZ_SCENE_TARGET_ITEM;
            else if (strcmp(cfg->target_name, "face") == 0)
                cfg->target = DVZ_SCENE_TARGET_FACE;
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
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->warmup))
                return false;
        }
        else if (strcmp(argv[i], "--queries") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->queries))
                return false;
        }
        else
            return false;
    }
    return cfg->rows >= 2 && cfg->cols >= 2 && cfg->queries > 0;
}


static void _timing_add(DvzSceneQueryTiming* dst, const DvzSceneQueryTiming* src)
{
    dst->build_ns += src->build_ns;
    dst->emit_ns += src->emit_ns;
    dst->semantic_validation_ns += src->semantic_validation_ns;
    dst->backend_ns += src->backend_ns;
    dst->semantic_commit_ns += src->semantic_commit_ns;
    dst->download_ns += src->download_ns;
    dst->decode_ns += src->decode_ns;
    dst->readout_ns += src->readout_ns;
    if (src->derived_vertex_count > dst->derived_vertex_count)
        dst->derived_vertex_count = src->derived_vertex_count;
    dst->static_upload_bytes += src->static_upload_bytes;
    if (src->retained_resource_bytes > dst->retained_resource_bytes)
        dst->retained_resource_bytes = src->retained_resource_bytes;
    dst->submitted_count += src->submitted_count;
    dst->completed_count += src->completed_count;
    dst->failed_count += src->failed_count;
    dst->superseded_count += src->superseded_count;
    dst->coalesced_count += src->coalesced_count;
    dst->static_upload_count += src->static_upload_count;
}


static bool _query_once(
    DvzScene* scene, DvzPanel* panel, DvzView* view, const BenchConfig* cfg,
    uint64_t request_id, BenchStats* stats)
{
    DvzQueryRequest request = dvz_query_request();
    request.request_id = request_id;
    request.target = cfg->target;

    const uint64_t start = dvz_time_monotonic_ns();
    if (dvz_panel_query_px(panel, 0.5 * VIEW_WIDTH, 0.5 * VIEW_HEIGHT, &request) != DVZ_OK ||
        dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        return false;
    stats->elapsed_ns += dvz_time_monotonic_ns() - start;

    DvzSceneQueryTiming timing = {0};
    if (!_dvz_scene_query_timing_get(scene, &timing))
        return false;
    _timing_add(&stats->timing, &timing);

    DvzQueryResult result = {0};
    while (dvz_scene_poll_query(scene, &result))
    {
        if (result.request_id == request_id)
        {
            stats->resolved++;
            if (result.hit)
                stats->hits++;
        }
    }
    return true;
}


static void _print_stats(const char* phase, const BenchStats* stats, uint32_t count)
{
    const double divisor = count > 0 ? (double)count : 1.0;
    const double elapsed_ms = (double)stats->elapsed_ns * 1e-6;
    printf(
        "mesh_query_bench_phase: phase=%s count=%u resolved=%" PRIu64 " hits=%" PRIu64
        " elapsed_ms=%.3f ms_per_query=%.3f build_ms=%.3f emit_ms=%.3f validation_ms=%.3f"
        " backend_ms=%.3f commit_ms=%.3f download_ms=%.3f decode_ms=%.3f readout_ms=%.3f"
        " derived_vertices=%" PRIu64 " static_upload_bytes=%" PRIu64
        " retained_bytes=%" PRIu64 " static_uploads=%" PRIu64 " failed=%" PRIu64 "\n",
        phase, count, stats->resolved, stats->hits, elapsed_ms, elapsed_ms / divisor,
        (double)stats->timing.build_ns * 1e-6 / divisor,
        (double)stats->timing.emit_ns * 1e-6 / divisor,
        (double)stats->timing.semantic_validation_ns * 1e-6 / divisor,
        (double)stats->timing.backend_ns * 1e-6 / divisor,
        (double)stats->timing.semantic_commit_ns * 1e-6 / divisor,
        (double)stats->timing.download_ns * 1e-6 / divisor,
        (double)stats->timing.decode_ns * 1e-6 / divisor,
        (double)stats->timing.readout_ns * 1e-6 / divisor,
        stats->timing.derived_vertex_count, stats->timing.static_upload_bytes,
        stats->timing.retained_resource_bytes, stats->timing.static_upload_count,
        stats->timing.failed_count);
}


int main(int argc, char** argv)
{
    BenchConfig cfg = {0};
    if (!_parse_args(argc, argv, &cfg))
    {
        fprintf(
            stderr,
            "usage: %s [--target item|face] [--rows N] [--cols N] [--warmup N] [--queries N]\n",
            argv[0]);
        return 2;
    }

    int ret = 1;
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = scene != NULL ? dvz_figure(scene, VIEW_WIDTH, VIEW_HEIGHT, 0) : NULL;
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* mesh = scene != NULL ? dvz_mesh(scene, 0) : NULL;
    DvzGeometry* geometry = NULL;
    DvzApp* app = NULL;

    if (panel == NULL || mesh == NULL)
        goto cleanup;

    DvzGeometrySurfaceGridDesc desc = dvz_geometry_surface_grid_desc();
    desc.rows = cfg.rows;
    desc.cols = cfg.cols;
    desc.origin[0] = -0.9;
    desc.origin[1] = -0.9;
    desc.col_basis[0] = 1.8 / (double)(cfg.cols - 1);
    desc.row_basis[1] = 1.8 / (double)(cfg.rows - 1);
    geometry = dvz_geometry_surface_grid(&desc);
    if (geometry == NULL || dvz_mesh_set_geometry(mesh, geometry) != DVZ_OK ||
        dvz_visual_set_query_capabilities(
            mesh, DVZ_QUERY_CAPABILITY_ITEM | DVZ_QUERY_CAPABILITY_FACE) != DVZ_OK ||
        dvz_panel_add_visual(panel, mesh, NULL) != DVZ_OK)
        goto cleanup;

    app = dvz_app(scene);
    DvzView* view = app != NULL ? dvz_view_offscreen(app, figure, VIEW_WIDTH, VIEW_HEIGHT) : NULL;
    if (view == NULL || dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        goto cleanup;

    _dvz_scene_query_timing_enable(scene, true);
    BenchStats cold = {0};
    if (!_query_once(scene, panel, view, &cfg, 1, &cold))
        goto cleanup;

    BenchStats discarded = {0};
    for (uint32_t i = 0; i < cfg.warmup; i++)
    {
        if (!_query_once(scene, panel, view, &cfg, 2u + i, &discarded))
            goto cleanup;
    }

    BenchStats steady = {0};
    for (uint32_t i = 0; i < cfg.queries; i++)
    {
        if (!_query_once(scene, panel, view, &cfg, 2u + cfg.warmup + i, &steady))
            goto cleanup;
    }

    const uint64_t vertex_count = (uint64_t)cfg.rows * cfg.cols;
    const uint64_t triangle_count = 2u * (uint64_t)(cfg.rows - 1) * (cfg.cols - 1);
    printf(
        "mesh_query_bench: target=%s rows=%u cols=%u vertices=%" PRIu64
        " triangles=%" PRIu64 " warmup=%u queries=%u\n",
        cfg.target_name, cfg.rows, cfg.cols, vertex_count, triangle_count, cfg.warmup,
        cfg.queries);
    _print_stats("cold", &cold, 1);
    _print_stats("steady", &steady, cfg.queries);
    ret = steady.resolved == cfg.queries && steady.hits == cfg.queries ? 0 : 1;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (geometry != NULL)
        dvz_geometry_destroy(geometry);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
