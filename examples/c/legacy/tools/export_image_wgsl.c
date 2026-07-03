/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Export a scene-generated image WGSL DRP2 stream                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "datoviz/drp2.h"
#include "datoviz/fileio.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Export a scene-generated image DRP2 stream using WGSL shaders.
 *
 * @param argc number of command-line arguments
 * @param argv command-line arguments; argv[1] optionally overrides the output path
 * @return zero on success, non-zero on failure
 */
int main(int argc, char** argv)
{
    const char* output =
        argc >= 2 ? argv[1] : "examples/webgpu/streams/scene_image_wgsl.json";

    int ret = 1;
    DvzScene* scene = NULL;
    DvzSceneFrameArtifact* artifact = NULL;
    char* json = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, 640, 640, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    DvzVisual* visual = dvz_image(scene, 0);
    EXAMPLE_CHECK(figure != NULL && panel != NULL && visual != NULL, "scene setup failed");

    vec3 positions[4] = {
        {-0.75f, -0.75f, 0.0f},
        {-0.75f,  0.75f, 0.0f},
        { 0.75f, -0.75f, 0.0f},
        { 0.75f,  0.75f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[2 * 2 * 4] = {
        235, 72, 72, 255,
        54, 179, 126, 255,
        70, 132, 255, 255,
        255, 220, 90, 255,
    };

    int rc = dvz_visual_set_data(visual, "position", positions, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(position) failed");

    rc = dvz_visual_set_data(visual, "texcoords", texcoords, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(texcoords) failed");

    rc = dvz_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 2, 2, sizeof(pixels));
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_texture_rgba8() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    artifact = dvz_figure_emit_frame(figure, &caps, &report, &emit_cfg);
    uint32_t diagnostic_count = dvz_diagnostic_report_count(&report);
    EXAMPLE_CHECK(artifact != NULL && diagnostic_count == 0, "frame artifact emission failed");

    json = dvz_scene_frame_artifact_json(artifact, "scene_image_wgsl");
    if (json != NULL)
    {
        DvzSize size = (DvzSize)strlen(json);
        ret = dvz_write_bytes(output, "wb", size, (const uint8_t*)json) == 0 ? 0 : 1;
    }

cleanup:
    dvz_drp2_stream_json_destroy(json);
    if (artifact != NULL)
        dvz_scene_frame_artifact_destroy(artifact);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
