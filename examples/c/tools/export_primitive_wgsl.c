/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Export a scene-generated primitive WGSL DRP2 stream                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datoviz/drp2.h"
#include "datoviz/fileio.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Export a scene-generated primitive triangle DRP2 stream using WGSL shaders.
 *
 * @param argc number of command-line arguments
 * @param argv command-line arguments; argv[1] optionally overrides the output path
 * @return zero on success, non-zero on failure
 */
int main(int argc, char** argv)
{
    const char* output =
        argc >= 2 ? argv[1] : "examples/webgpu/streams/scene_primitive_wgsl.json";

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return 1;

    DvzFigure* figure = dvz_figure(scene, 640, 640, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (figure == NULL || panel == NULL || visual == NULL)
    {
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[3][3] = {
        {-0.62f, -0.54f, 0.0f},
        { 0.62f, -0.54f, 0.0f},
        { 0.00f,  0.62f, 0.0f},
    };
    DvzColor colors[3] = {
        {235, 72, 72, 255},
        {54, 179, 126, 255},
        {70, 132, 255, 255},
    };

    if (dvz_visual_set_data(visual, "position", positions, 3) != 0 ||
        dvz_visual_set_data(visual, "color", colors, 3) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 0;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    if (stream == NULL || dvz_diagnostic_report_count(&report) != 0)
    {
        dvz_scene_destroy(scene);
        return 1;
    }

    char* json = dvz_drp2_stream_json(stream, "scene_primitive_wgsl");
    int rc = 1;
    if (json != NULL)
    {
        DvzSize size = (DvzSize)strlen(json);
        rc = dvz_write_bytes(output, "wb", size, (const uint8_t*)json);
    }

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return rc == 0 ? 0 : 1;
}
