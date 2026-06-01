/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene DPI tests                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <string.h>

#include "_assertions.h"
#include "../../drp2/_stream.h"
#include "frame_plan/frame_plan.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Verify DPI scaling reaches DRP2 viewport/scissor, shader viewport, and point sizes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_scene_dpi_physical_viewport_and_screen_scale(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 400, 300, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.25f, 0.0f, 0.5f, 1.0f});
    AT(panel != NULL);

    DvzVisual* point = dvz_point(scene, 0);
    AT(point != NULL);
    vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float size[1] = {8.0f};
    AT(dvz_visual_set_data(point, "position", pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", color, 1) == 0);
    AT(dvz_visual_set_data(point, "size", size, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    vec3 image_pos[1] = {{10.0f, 20.0f, 0.0f}};
    vec2 image_extent[1] = {{30.0f, 40.0f}};
    vec2 image_anchor[1] = {{-1.0f, -1.0f}};
    uint8_t image_pixels[4 * 4 * 4] = {0};
    AT(dvz_visual_set_data(image, "position_px", image_pos, 1) == 0);
    AT(dvz_visual_set_data(image, "extent_px", image_extent, 1) == 0);
    AT(dvz_visual_set_data(image, "anchor", image_anchor, 1) == 0);
    AT(dvz_visual_set_texture(image, image_pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, image,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 800;
    cfg.target_height = 600;
    cfg.device_scale_x = 2.0f;
    cfg.device_scale_y = 2.0f;
    cfg.user_scale = 1.5f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_viewport = false;
    bool found_scissor = false;
    bool found_viewport_uniform = false;
    bool found_size_upload = false;
    bool found_image_position_upload = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            AC(cmd->u.set_viewport.viewport[0], 200.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[2], 400.0f, 1e-6f);
            AC(cmd->u.set_viewport.viewport[3], 600.0f, 1e-6f);
            found_viewport = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            AC(cmd->u.set_scissor.scissor[0], 200.0f, 1e-6f);
            AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
            AC(cmd->u.set_scissor.scissor[2], 400.0f, 1e-6f);
            AC(cmd->u.set_scissor.scissor[3], 600.0f, 1e-6f);
            found_scissor = true;
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == sizeof(DvzSceneViewportUniform))
        {
            const DvzSceneViewportUniform* viewport =
                (const DvzSceneViewportUniform*)cmd->u.write_buffer.data_raw;
            ANN(viewport);
            AC(viewport->x, 200.0f, 1e-6f);
            AC(viewport->y, 0.0f, 1e-6f);
            AC(viewport->width, 400.0f, 1e-6f);
            AC(viewport->height, 600.0f, 1e-6f);
            found_viewport_uniform = true;
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == sizeof(float))
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            if (label != NULL && strstr(label, "_size") != NULL)
            {
                const float* uploaded = (const float*)cmd->u.write_buffer.data_raw;
                ANN(uploaded);
                AC(uploaded[0], 24.0f, 1e-6f);
                found_size_upload = true;
            }
        }
        else if (
            cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size == 18 * sizeof(float))
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            if (label != NULL && strstr(label, "_position") != NULL)
            {
                const float* uploaded = (const float*)cmd->u.write_buffer.data_raw;
                ANN(uploaded);
                const float expected[18] = {
                    20.0f, 40.0f, 0.0f, 20.0f, 120.0f, 0.0f, 80.0f, 40.0f, 0.0f,
                    80.0f, 40.0f, 0.0f, 20.0f, 120.0f, 0.0f, 80.0f, 120.0f, 0.0f,
                };
                bool matches = true;
                for (uint32_t j = 0; j < 18; j++)
                    matches = matches && fabsf(uploaded[j] - expected[j]) < 1e-6f;
                found_image_position_upload = found_image_position_upload || matches;
            }
        }
    }

    AT(found_viewport);
    AT(found_scissor);
    AT(found_viewport_uniform);
    AT(found_size_upload);
    AT(found_image_position_upload);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_dpi(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene,dpi";
    TST_CASE(test_scene_dpi_physical_viewport_and_screen_scale);
    return 0;
}
