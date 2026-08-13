/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Non-CI diagnostic benchmark for Issue #138 rolling sampled-field uploads.
 *
 * Build: just example-c lab/rolling_field_bench
 * Run:   ./build/examples/c/lab/rolling_field_bench --mode 2d --warmup 16 --frames 240
 *
 * Modes are a one-row 2D field (1d), a 2D field, a 3D field, and a retained surface grid. The
 * benchmark intentionally emits FramePlan upload nodes but does not submit them: it measures
 * retained mutation, upload packing, and command shape independently from presentation and GPU
 * execution.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_scene.h"
#include "_stream.h"
#include "datoviz/common/functions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "frame_plan/frame_plan.h"
#include "scene_emit/internal.h"


#define BENCH_1D_WIDTH  4096u
#define BENCH_2D_WIDTH  512u
#define BENCH_2D_HEIGHT 512u
#define BENCH_3D_WIDTH  128u
#define BENCH_3D_HEIGHT 128u
#define BENCH_3D_DEPTH  64u
#define SURFACE_ROWS 128u
#define SURFACE_COLS 128u


typedef struct BenchConfig
{
    const char* mode;
    uint32_t warmup;
    uint32_t frames;
    uint32_t cadence;
} BenchConfig;


typedef struct BenchStats
{
    uint64_t acquisition_ns;
    uint64_t mutation_ns;
    uint64_t emit_ns;
    uint64_t mesh_commit_ns;
    uint64_t upload_bytes;
    uint64_t upload_commands;
    uint64_t index_write_bytes;
    uint64_t index_write_commands;
    uint64_t parked_frames;
} BenchStats;


static uint64_t _elapsed(uint64_t start) { return dvz_time_monotonic_ns() - start; }


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
    *cfg = (BenchConfig){.mode = "2d", .warmup = 16, .frames = 240, .cadence = 1};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc)
            cfg->mode = argv[++i];
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
        else if (strcmp(argv[i], "--cadence") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &cfg->cadence))
                return false;
        }
        else
            return false;
    }
    return cfg->frames > 0 && cfg->cadence > 0 &&
           (strcmp(cfg->mode, "1d") == 0 || strcmp(cfg->mode, "2d") == 0 ||
            strcmp(cfg->mode, "3d") == 0 || strcmp(cfg->mode, "surface") == 0);
}


static void _mode_extent(const char* mode, uint32_t* width, uint32_t* height, uint32_t* depth)
{
    *width = BENCH_2D_WIDTH;
    *height = BENCH_2D_HEIGHT;
    *depth = 1;
    if (strcmp(mode, "1d") == 0)
    {
        *width = BENCH_1D_WIDTH;
        *height = 1;
    }
    else if (strcmp(mode, "3d") == 0)
    {
        *width = BENCH_3D_WIDTH;
        *height = BENCH_3D_HEIGHT;
        *depth = BENCH_3D_DEPTH;
    }
}


static void _count_uploads(const DvzFramePlan* plan, BenchStats* stats)
{
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
            continue;
        stats->upload_commands++;
        stats->upload_bytes += node->u.upload.byte_size;
    }
}


static void _count_stream_writes(const DvzDrp2CommandStream* stream, BenchStats* stats)
{
    uint64_t index_buffer_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command != NULL && command->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            index_buffer_id = command->u.set_index_buffer.buffer_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL || command->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        stats->upload_commands++;
        stats->upload_bytes += command->u.write_buffer.size;
        if (command->u.write_buffer.buffer_id == index_buffer_id)
        {
            stats->index_write_commands++;
            stats->index_write_bytes += command->u.write_buffer.size;
        }
    }
}


static int _run_surface(const BenchConfig* cfg)
{
    const uint32_t count = SURFACE_ROWS * SURFACE_COLS;
    double* heights = (double*)dvz_calloc(count, sizeof(double));
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = scene != NULL ? dvz_figure(scene, 640, 480, 0) : NULL;
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzGeometrySurfaceGridDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .rows = SURFACE_ROWS, .cols = SURFACE_COLS, .heights = heights};
    DvzGeometry* geometry = heights != NULL ? dvz_geometry_surface_grid(&desc) : NULL;
    DvzVisual* mesh = scene != NULL ? dvz_mesh(scene, 0) : NULL;
    if (heights == NULL || panel == NULL || geometry == NULL || mesh == NULL ||
        dvz_mesh_set_geometry(mesh, geometry) != DVZ_OK ||
        dvz_panel_add_visual(panel, mesh, NULL) != DVZ_OK)
        goto failure;

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzSceneFrameArtifact* prime = dvz_figure_emit_frame(figure, &caps, &report, NULL);
    if (
        prime == NULL ||
        dvz_scene_frame_artifact_status(prime) != DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK)
    {
        dvz_scene_frame_artifact_destroy(prime);
        goto failure;
    }
    dvz_scene_frame_artifact_destroy(prime);

    BenchStats stats = {0};
    bool ok = true;
    for (uint32_t frame = 0; frame < cfg->warmup + cfg->frames; frame++)
    {
        const bool timed = frame >= cfg->warmup;
        if (frame % cfg->cadence != 0)
        {
            if (timed)
                stats.parked_frames++;
            continue;
        }
        uint64_t start = dvz_time_monotonic_ns();
        for (uint32_t i = 0; i < count; i++)
            heights[i] = 0.20 * (double)((frame + i) % SURFACE_COLS) / (double)SURFACE_COLS;
        if (timed)
            stats.acquisition_ns += _elapsed(start);
        start = dvz_time_monotonic_ns();
        ok = dvz_geometry_surface_grid_update_heights(geometry, heights, count) == DVZ_OK;
        if (timed)
            stats.mutation_ns += _elapsed(start);
        start = dvz_time_monotonic_ns();
        ok = ok && dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK;
        if (timed)
            stats.mesh_commit_ns += _elapsed(start);
        start = dvz_time_monotonic_ns();
        DvzSceneFrameArtifact* artifact =
            ok ? dvz_figure_emit_frame(figure, &caps, &report, NULL) : NULL;
        if (timed)
            stats.emit_ns += _elapsed(start);
        if (
            artifact == NULL ||
            dvz_scene_frame_artifact_status(artifact) != DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK)
            ok = false;
        if (ok && timed)
            _count_stream_writes(dvz_scene_frame_artifact_stream(artifact), &stats);
        dvz_scene_frame_artifact_destroy(artifact);
        if (!ok)
            break;
    }
    const uint64_t active = cfg->frames - stats.parked_frames;
    printf(
        "rolling_field_bench: mode=surface rows=%u cols=%u warmup=%u frames=%u "
        "cadence=%u active_frames=%" PRIu64 " parked_frames=%" PRIu64 " "
        "acquisition_ms=%.4f height_normal_ms=%.4f "
        "mesh_commit_ms=%.4f semantic_emit_ms=%.4f buffer_write_commands=%" PRIu64 " "
        "buffer_write_bytes=%" PRIu64 " index_write_commands=%" PRIu64 " "
        "index_write_bytes=%" PRIu64 "\n",
        SURFACE_ROWS, SURFACE_COLS, cfg->warmup, cfg->frames, cfg->cadence, active,
        stats.parked_frames, (double)stats.acquisition_ns * 1e-6,
        (double)stats.mutation_ns * 1e-6, (double)stats.mesh_commit_ns * 1e-6,
        (double)stats.emit_ns * 1e-6,
        stats.upload_commands, stats.upload_bytes, stats.index_write_commands,
        stats.index_write_bytes);
    dvz_geometry_destroy(geometry);
    dvz_scene_destroy(scene);
    dvz_free(heights);
    return ok ? 0 : 1;

failure:
    dvz_geometry_destroy(geometry);
    dvz_scene_destroy(scene);
    dvz_free(heights);
    return 1;
}


static int _run(const BenchConfig* cfg)
{
    if (strcmp(cfg->mode, "surface") == 0)
        return _run_surface(cfg);
    uint32_t width = 0, height = 0, depth = 0;
    _mode_extent(cfg->mode, &width, &height, &depth);
    const uint64_t item_count = (uint64_t)width * height * depth;
    float* values = (float*)dvz_calloc(item_count, sizeof(float));
    if (values == NULL)
        return 1;

    DvzScene* scene = dvz_scene();
    DvzSampledField* field = scene != NULL
                                 ? dvz_sampled_field(
                                       scene,
                                       &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                                           .dim = depth == 1 ? DVZ_FIELD_DIM_2D : DVZ_FIELD_DIM_3D,
                                           .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                                           .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                                           .width = width,
                                           .height = height,
                                           .depth = depth})
                                 : NULL;
    if (field == NULL ||
        dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = values,
                       .bytes_per_row = width * sizeof(float), .rows_per_image = height}) != DVZ_OK)
    {
        dvz_scene_destroy(scene);
        dvz_free(values);
        return 1;
    }

    BenchStats stats = {0};
    DvzFramePlan* prime = dvz_frame_plan("rolling-field-bench", 0);
    bool ok = prime != NULL && _scene_emit_sampled_field_texture_upload(prime, "bench.field", field);
    dvz_frame_plan_destroy(prime);
    field->dirty = false;
    field->dirty_full = false;
    if (!ok)
    {
        dvz_scene_destroy(scene);
        dvz_free(values);
        return 1;
    }
    const uint32_t total = cfg->warmup + cfg->frames;
    for (uint32_t frame = 0; frame < total; frame++)
    {
        const bool timed = frame >= cfg->warmup;
        const bool active = frame % cfg->cadence == 0;
        if (!active)
        {
            if (timed)
                stats.parked_frames++;
            continue;
        }
        const uint32_t row = frame % height;
        const uint32_t layer = (frame / height) % depth;
        uint64_t start = dvz_time_monotonic_ns();
        for (uint32_t x = 0; x < width; x++)
            values[((uint64_t)layer * height + row) * width + x] = (float)(frame + x) * 0.001f;
        if (timed)
            stats.acquisition_ns += _elapsed(start);

        start = dvz_time_monotonic_ns();
        DvzResult result = dvz_sampled_field_update_region(
            field,
            (DvzFieldRegion){
                .x = 0, .y = row, .z = layer, .width = width, .height = 1, .depth = 1},
            &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                .data = values + ((uint64_t)layer * height + row) * width,
                .bytes_per_row = width * sizeof(float), .rows_per_image = 1});
        if (timed)
            stats.mutation_ns += _elapsed(start);
        if (result != DVZ_OK)
        {
            ok = false;
            break;
        }

        DvzFramePlan* plan = dvz_frame_plan("rolling-field-bench", frame);
        start = dvz_time_monotonic_ns();
        bool emitted =
            plan != NULL && _scene_emit_sampled_field_texture_upload(plan, "bench.field", field);
        if (timed && plan != NULL)
        {
            stats.emit_ns += _elapsed(start);
            _count_uploads(plan, &stats);
        }
        dvz_frame_plan_destroy(plan);
        if (!emitted)
        {
            ok = false;
            break;
        }
        field->dirty = false;
        field->dirty_full = false;
    }

    const uint64_t active_frames = cfg->frames - stats.parked_frames;
    printf(
        "rolling_field_bench: mode=%s width=%u height=%u depth=%u warmup=%u frames=%u "
        "cadence=%u active_frames=%" PRIu64 " parked_frames=%" PRIu64 " "
        "acquisition_ms=%.4f mutation_ms=%.4f emit_ms=%.4f upload_commands=%" PRIu64 " "
        "upload_bytes=%" PRIu64 " bytes_per_active_frame=%.1f\n",
        cfg->mode, width, height, depth, cfg->warmup, cfg->frames, cfg->cadence, active_frames,
        stats.parked_frames, (double)stats.acquisition_ns * 1e-6,
        (double)stats.mutation_ns * 1e-6, (double)stats.emit_ns * 1e-6,
        stats.upload_commands, stats.upload_bytes,
        active_frames > 0 ? (double)stats.upload_bytes / (double)active_frames : 0.0);
    dvz_scene_destroy(scene);
    dvz_free(values);
    return ok ? 0 : 1;
}


int main(int argc, char** argv)
{
    BenchConfig cfg = {0};
    if (!_parse_args(argc, argv, &cfg))
    {
        fprintf(
            stderr, "usage: %s [--mode 1d|2d|3d|surface] [--warmup N] [--frames N] "
                    "[--cadence N]\n",
            argv[0]);
        return 2;
    }
    return _run(&cfg);
}
