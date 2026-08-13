/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene geometry visual tests                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

static int _scene_textured_mesh_sampling_executes(TstContext* suite);



/**
 * Allocate a small valid triangle-list geometry for mesh replacement tests.
 *
 * @param vertex_count number of vertices
 * @param index_count number of triangle-list indices, or zero for nonindexed geometry
 * @return owned geometry, or NULL on allocation failure
 */
static DvzGeometry* _mesh_replacement_geometry(uint32_t vertex_count, uint32_t index_count)
{
    DvzGeometry* geometry = dvz_geometry(vertex_count, index_count);
    if (geometry == NULL)
        return NULL;

    for (uint32_t i = 0; i < vertex_count; i++)
    {
        geometry->positions[i][0] = (double)i / (double)vertex_count;
        geometry->positions[i][1] = (i % 2 == 0) ? -0.5 : 0.5;
        geometry->positions[i][2] = 0.0;
        geometry->normals[i][2] = 1.0;
        geometry->colors[i] = (DvzColor){255, 255, 255, 255};
    }
    for (uint32_t i = 0; i < index_count; i++)
        geometry->indices[i] = i % vertex_count;
    return geometry;
}



/**
 * Return whether a stream contains one draw with the requested geometry counts.
 *
 * @param stream emitted scene stream
 * @param indexed whether the expected draw is indexed
 * @param count expected vertex or index count
 * @param instance_count expected instance count
 * @return whether the draw was found
 */
static bool _stream_has_mesh_draw(
    const DvzDrp2CommandStream* stream, bool indexed, uint32_t count, uint32_t instance_count)
{
    ANN(stream);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (indexed && command->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            if (
                command->u.draw_indexed.index_count == count &&
                command->u.draw_indexed.instance_count == instance_count)
            {
                return true;
            }
        }
        if (!indexed && command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            if (
                command->u.draw.vertex_count == count &&
                command->u.draw.instance_count == instance_count)
            {
                return true;
            }
        }
    }
    return false;
}



static uint32_t _stream_destroy_buffer_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command->type == DVZ_DRP2_COMMAND_DESTROY_BUFFER)
            count++;
    }
    return count;
}



/**
 * Emit the current figure state with the standard GLSL scene fixture configuration.
 *
 * @param figure figure to emit
 * @return owned command stream
 */
static DvzDrp2CommandStream* _emit_mesh_replacement_stream(DvzFigure* figure)
{
    ANN(figure);
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig config = dvz_frame_plan_emit_config();
    config.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &config);
    if (dvz_diagnostic_report_count(&report) != 0)
    {
        _test_scene_stream_destroy(stream);
        return NULL;
    }
    return stream;
}



static int _scene_primitive_emit_executes(
    TstContext* suite, DvzPrimitiveTopology topology, uint32_t vertex_count)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_primitive(scene, topology, 0);
    AT(visual != NULL);

    /* Build vertex_count positions on a unit triangle / strip path; details don't matter. */
    float* positions = dvz_calloc(vertex_count * 3, sizeof(float));
    uint8_t (*colors)[4] = dvz_calloc(vertex_count, 4);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        positions[i * 3 + 0] = (float)i / (float)vertex_count - 0.5f;
        positions[i * 3 + 1] = (i % 2 == 0) ? -0.4f : 0.4f;
        positions[i * 3 + 2] = 0.0f;
        colors[i][0] = (uint8_t)(255 * i / vertex_count);
        colors[i][1] = 128;
        colors[i][2] = (uint8_t)(255 - 255 * i / vertex_count);
        colors[i][3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, vertex_count) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, vertex_count) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}


static int _scene_path_emit_executes(TstContext* suite, uint32_t vertex_count)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    float* positions = dvz_calloc(vertex_count * 3, sizeof(float));
    uint8_t (*colors)[4] = dvz_calloc(vertex_count, 4);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        positions[i * 3 + 0] = (float)i / (float)vertex_count - 0.5f;
        positions[i * 3 + 1] = (i % 2 == 0) ? -0.4f : 0.4f;
        positions[i * 3 + 2] = 0.0f;
        colors[i][0] = 255;
        colors[i][1] = (uint8_t)(255 * i / vertex_count);
        colors[i][2] = 64;
        colors[i][3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, vertex_count) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, vertex_count) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}


static int _scene_mesh_emit_executes(TstContext* suite)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_STANDARD;
    material.standard.roughness = 0.35f;
    material.standard.specular = 0.5f;
    material.standard.rim_strength = 0.15f;
    AT(dvz_visual_set_material(visual, &material) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_primitive_triangle_list_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(suite, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
}


int test_scene_primitive_lit_glsl_uses_spirv(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_vertex = false;
    bool found_fragment = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
            continue;
        if (strcmp(command->u.create_shader_module.builtin_family, "scene.primitive") != 0 ||
            strcmp(command->u.create_shader_module.builtin_variant, "lit") != 0)
            continue;

        AT(strcmp(command->u.create_shader_module.format, "spirv") == 0);
        AT(command->u.create_shader_module.spirv != NULL);
        AT(command->u.create_shader_module.spirv_size > 0);
        found_vertex = found_vertex ||
                       strcmp(command->u.create_shader_module.stage, "VERTEX") == 0;
        found_fragment = found_fragment ||
                         strcmp(command->u.create_shader_module.stage, "FRAGMENT") == 0;
    }
    AT(found_vertex);
    AT(found_fragment);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_emit_wgsl_instanced_quads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};
    float sizes[3] = {8.0f, 14.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_count == 3);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 6);
            AT(command->u.draw.instance_count == 3);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    char* json = dvz_drp2_stream_json(stream, "scene_point_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
    AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);
    AT(strstr(json, "\"vertex_count\": 6") != NULL);
    AT(strstr(json, "\"instance_count\": 3") != NULL);
    AT(strstr(json, "quad_corner") != NULL);
    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_point_wgsl_from_c",
           "spec/drp2/fixtures/positive/scene_point_wgsl_from_c.json") == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pixel_emit_wgsl_instanced_quads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_pixel(scene, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};
    float sizes[3] = {8.0f, 14.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_count == 3);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 6);
            AT(command->u.draw.instance_count == 3);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    char* json = dvz_drp2_stream_json(stream, "scene_pixel_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
    AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);
    AT(strstr(json, "\"vertex_count\": 6") != NULL);
    AT(strstr(json, "\"instance_count\": 3") != NULL);
    AT(strstr(json, "quad_corner") != NULL);
    AT(strstr(json, "dot(input.corner") == NULL);
    dvz_drp2_stream_json_destroy(json);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_primitive_line_strip_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(suite, DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP, 4);
}


int test_scene_primitive_triangle_list_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.6f, -0.5f, 0.0f},
        { 0.6f, -0.5f, 0.0f},
        { 0.0f,  0.6f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "scene_primitive_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"format\": \"glsl\"") == NULL);
    AT(strstr(json, "@vertex") != NULL);
    AT(strstr(json, "@fragment") != NULL);
    AT(strstr(json, "\"vertex_buffers\": [") != NULL);
    AT(strstr(json, "\"binding_type\": \"uniform_buffer\"") != NULL);
    AT(strstr(json, "VertexIn") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_primitive_wgsl_from_c",
           "spec/drp2/fixtures/positive/scene_primitive_wgsl_from_c.json") == 0);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(_visual_family_state(visual)->mesh_default_color);
    bool found_color_attr = false;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        found_color_attr = found_color_attr || strcmp(visual->attrs[i].name, "color") == 0;
    AT(found_color_attr);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    uint32_t set_index_order = UINT32_MAX;
    uint32_t draw_indexed_order = UINT32_MAX;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
        {
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
            if (found_set_index)
                set_index_order = i;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
            if (found_draw_indexed)
                draw_indexed_order = i;
        }
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(set_index_order < draw_indexed_order);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_geometry_replacement_uses_logical_index_count(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzGeometry* large = _mesh_replacement_geometry(9, 9);
    DvzGeometry* small = _mesh_replacement_geometry(3, 3);
    DvzGeometry* large_again = _mesh_replacement_geometry(12, 12);
    ANN(large);
    ANN(small);
    ANN(large_again);

    AT(dvz_mesh_set_geometry(mesh, large) == 0);
    DvzDrp2CommandStream* stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 9, 1));
    _test_scene_stream_destroy(stream);

    AT(dvz_mesh_set_geometry(mesh, small) == 0);
    stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 3, 1));
    _test_scene_stream_destroy(stream);

    DvzSceneBuffer* index_buffer = _visual_family_state(mesh)->buffer;
    ANN(index_buffer);
    uint64_t retained_capacity = index_buffer->capacity;
    AT(dvz_scene_buffer_set_data(index_buffer, NULL, 0) == DVZ_OK);
    AT(index_buffer->capacity == retained_capacity);
    stream = _emit_mesh_replacement_stream(figure);
    AT(stream == NULL);
    AT(dvz_scene_buffer_set_data(index_buffer, small->indices, 3 * sizeof(DvzIndex)) == DVZ_OK);

    AT(dvz_mesh_set_geometry(mesh, large_again) == 0);
    stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 12, 1));
    _test_scene_stream_destroy(stream);

    dvz_geometry_destroy(large);
    dvz_geometry_destroy(small);
    dvz_geometry_destroy(large_again);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_geometry_replacement_switches_indexing(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzGeometry* indexed = _mesh_replacement_geometry(3, 3);
    DvzGeometry* nonindexed = _mesh_replacement_geometry(6, 0);
    DvzGeometry* indexed_again = _mesh_replacement_geometry(4, 6);
    ANN(indexed);
    ANN(nonindexed);
    ANN(indexed_again);

    AT(dvz_mesh_set_geometry(mesh, indexed) == 0);
    DvzSceneBuffer* first_index_buffer = _visual_family_state(mesh)->buffer;
    ANN(first_index_buffer);
    DvzId first_index_id = first_index_buffer->id;
    DvzDrp2CommandStream* stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 3, 1));
    _test_scene_stream_destroy(stream);

    AT(dvz_mesh_set_geometry(mesh, nonindexed) == 0);
    AT(scene->buffer_retirement_count == 1);
    uint32_t emitter_count_before_failure = scene->emitter->resources.count;
    DvzCapabilitySnapshot invalid_caps = dvz_capability_snapshot();
    invalid_caps.max_texture_dimension_2d = 1;
    DvzDiagnosticReport invalid_report;
    dvz_diagnostic_report_init(&invalid_report);
    DvzFramePlanEmitConfig invalid_config = dvz_frame_plan_emit_config();
    invalid_config.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* failed_stream =
        _test_scene_emit_stream_ex(figure, &invalid_caps, &invalid_report, &invalid_config);
    AT(failed_stream == NULL);
    AT(scene->buffer_retirement_count == 1);
    AT(scene->emitter->resources.count == emitter_count_before_failure);
    stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, false, 6, 1));
    AT(_stream_destroy_buffer_count(stream) == 1);
    AT(_visual_family_state(mesh)->buffer == NULL);
    AT(scene->buffer_retirement_count == 0);
    _test_scene_stream_destroy(stream);

    AT(dvz_mesh_set_geometry(mesh, indexed_again) == 0);
    AT(_visual_family_state(mesh)->buffer == first_index_buffer);
    AT(_visual_family_state(mesh)->buffer->id != first_index_id);
    stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 6, 1));
    AT(_stream_destroy_buffer_count(stream) == 0);
    _test_scene_stream_destroy(stream);

    uint32_t live_resource_count = scene->emitter->resources.count;
    for (uint32_t i = 0; i < 8; i++)
    {
        AT(dvz_mesh_set_geometry(mesh, nonindexed) == 0);
        stream = _emit_mesh_replacement_stream(figure);
        ANN(stream);
        AT(_stream_destroy_buffer_count(stream) == 1);
        _test_scene_stream_destroy(stream);
        AT(scene->emitter->resources.count + 1 == live_resource_count);

        AT(dvz_mesh_set_geometry(mesh, indexed_again) == 0);
        stream = _emit_mesh_replacement_stream(figure);
        ANN(stream);
        AT(_stream_destroy_buffer_count(stream) == 0);
        _test_scene_stream_destroy(stream);
        AT(scene->emitter->resources.count == live_resource_count);
    }

    dvz_geometry_destroy(indexed);
    dvz_geometry_destroy(nonindexed);
    dvz_geometry_destroy(indexed_again);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_instance_count_shrink_uses_logical_extent(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    float transforms[3][16] = {
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -0.5f, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, +0.0f, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, +0.5f, 0, 0, 1},
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_data(mesh, "instance_transform", transforms, 3) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    DvzDrp2CommandStream* stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 6, 3));
    _test_scene_stream_destroy(stream);

    AT(dvz_visual_set_data(mesh, "instance_transform", transforms, 1) == 0);
    stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 6, 1));
    _test_scene_stream_destroy(stream);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_geometry_replacement_failure_rolls_back(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzGeometry* retained = _mesh_replacement_geometry(3, 3);
    DvzGeometry* invalid = _mesh_replacement_geometry(4, 3);
    ANN(retained);
    ANN(invalid);
    invalid->indices[2] = 4;

    AT(dvz_mesh_set_geometry(mesh, retained) == 0);
    AT(dvz_mesh_set_geometry(mesh, invalid) == -1);

    DvzVisualDataView position = {0};
    AT(dvz_visual_data(mesh, "position", &position) == 0);
    AT(position.item_count == 3);
    DvzDrp2CommandStream* stream = _emit_mesh_replacement_stream(figure);
    ANN(stream);
    AT(_stream_has_mesh_draw(stream, true, 3, 1));
    _test_scene_stream_destroy(stream);

    dvz_geometry_destroy(retained);
    dvz_geometry_destroy(invalid);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_instance_transform_emits_instanced_draw(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.25f, -0.25f, 0.0f}, {-0.25f, 0.25f, 0.0f},
        {0.25f, -0.25f, 0.0f},  {0.25f, 0.25f, 0.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    float transforms[2][16] = {
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -0.4f, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, +0.4f, 0, 0, 1},
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "instance_transform", transforms, 2) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_instance_pipeline = false;
    bool found_instanced_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const uint32_t transform_binding = 2;
            found_instance_pipeline =
                cmd->u.create_render_pipeline.binding_count >= 3 &&
                cmd->u.create_render_pipeline.binding_step_modes[transform_binding] ==
                    DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_instanced_draw = cmd->u.draw_indexed.index_count == 6 &&
                                   cmd->u.draw_indexed.instance_count == 2;
        }
    }
    AT(found_instance_pipeline);
    AT(found_instanced_draw);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure lit mesh scene renders request depth attachments and depth-enabled pipelines.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_scene_mesh_emits_depth_attachment(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.1f}, {-0.8f, 0.8f, 0.1f},
        {0.8f, -0.8f, 0.1f},  {0.8f, 0.8f, 0.1f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_depth_pass = false;
    bool found_named_depth_pass = false;
    bool found_named_depth_texture = false;
    bool found_depth_pipeline = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_named_depth_texture =
                found_named_depth_texture ||
                (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                 cmd->u.create_texture.format == DVZ_FORMAT_D32_SFLOAT);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_depth_pass = found_depth_pass || cmd->u.begin_render_pass.has_depth_attachment;
            found_named_depth_pass =
                found_named_depth_pass || cmd->u.begin_render_pass.depth_texture_id != 0;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_depth_pipeline =
                found_depth_pipeline ||
                (cmd->u.create_render_pipeline.has_depth_attachment &&
                 cmd->u.create_render_pipeline.depth_write_enabled &&
                 cmd->u.create_render_pipeline.depth_compare_op == DVZ_COMPARE_OP_LESS_OR_EQUAL);
        }
    }
    AT(found_depth_pass);
    AT(found_named_depth_pass);
    AT(found_named_depth_texture);
    AT(found_depth_pipeline);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify textured mesh lowering emits a texture-backed mesh pipeline and draw.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_textured_mesh_emits_texture_pipeline(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f},
        {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    static const uint8_t pixels[2 * 2 * 4] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 255, 255,
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = pixels, .bytes_per_row = 2 * 4, .rows_per_image = 2}) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_field(visual, "texture", field) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    // A blended overlay creates a separate transparent pass. The opaque textured-mesh pipeline's
    // combined set-1 layout must still satisfy its material contract in the mixed-pass stream.
    DvzVisual* overlay = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(overlay);
    DvzColor overlay_colors[3] = {{32, 128, 255, 48}, {32, 128, 255, 48}, {32, 128, 255, 48}};
    AT(dvz_visual_set_data(overlay, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(overlay, "color", overlay_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(overlay, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, overlay, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_texture = false;
    bool found_upload = false;
    bool found_image_bind_group = false;
    bool found_material_upload = false;
    bool found_draw_indexed = false;
    bool found_depth_pipeline = false;
    bool found_linear_sampler = false;
    uint64_t texture_id = 0;
    uint64_t material_id = 0;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            if (cmd->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM &&
                cmd->u.create_texture.width == 2 && cmd->u.create_texture.height == 2 &&
                cmd->u.create_texture.depth == 1)
            {
                found_texture = true;
                texture_id = cmd->u.create_texture.id;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            found_upload = found_upload || (cmd->u.write_texture.width == 2 &&
                                            cmd->u.write_texture.height == 2 &&
                                            cmd->u.write_texture.depth == 1 &&
                                            cmd->u.write_texture.bytes_per_row == 2 * 4);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            if (cmd->u.write_buffer.size == sizeof(DvzSceneMaterialParams))
            {
                found_material_upload = true;
                material_id = cmd->u.write_buffer.buffer_id;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SAMPLER)
        {
            found_linear_sampler =
                found_linear_sampler ||
                (cmd->u.create_sampler.mag_filter == DVZ_DRP2_FILTER_LINEAR &&
                 cmd->u.create_sampler.min_filter == DVZ_DRP2_FILTER_LINEAR);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP && texture_id != 0)
        {
            bool has_material = false;
            bool has_texture = false;
            bool has_sampler = false;
            for (uint32_t j = 0; j < cmd->u.create_bind_group.entry_count; j++)
            {
                const DvzDrp2BindGroupEntry* entry = &cmd->u.create_bind_group.entries[j];
                has_material =
                    has_material ||
                    (entry->binding == 0 &&
                     entry->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                     entry->resource_id == material_id);
                has_texture = has_texture ||
                              (entry->binding == 1 &&
                               entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
                               entry->resource_id == texture_id);
                has_sampler = has_sampler ||
                              (entry->binding == 2 &&
                               entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLER);
            }
            found_image_bind_group =
                found_image_bind_group || (has_material && has_texture && has_sampler);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_depth_pipeline =
                found_depth_pipeline ||
                (label != NULL && strstr(label, "_pipe_mesh_textured_t") != NULL &&
                 cmd->u.create_render_pipeline.has_depth_attachment &&
                 cmd->u.create_render_pipeline.depth_write_enabled &&
                 cmd->u.create_render_pipeline.binding_count == 4 &&
                 cmd->u.create_render_pipeline.attr_count == 4);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_draw_indexed = found_draw_indexed || cmd->u.draw_indexed.index_count == 6;
        }
    }

    AT(found_texture);
    AT(found_upload);
    AT(found_material_upload);
    AT(found_image_bind_group);
    AT(found_linear_sampler);
    AT(found_depth_pipeline);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 6);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    _test_scene_stream_destroy(stream);

    DvzFieldSamplingDesc sampling = dvz_field_sampling_desc();
    sampling.min_filter = DVZ_FIELD_FILTER_NEAREST;
    sampling.mag_filter = DVZ_FIELD_FILTER_NEAREST;
    AT(dvz_visual_set_field_sampling(visual, "texture", &sampling) == DVZ_OK);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    dvz_diagnostic_report_init(&report);
    stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    bool found_nearest_sampler = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_SAMPLER)
        {
            found_nearest_sampler =
                found_nearest_sampler ||
                (cmd->u.create_sampler.mag_filter == DVZ_DRP2_FILTER_NEAREST &&
                 cmd->u.create_sampler.min_filter == DVZ_DRP2_FILTER_NEAREST);
        }
    }
    AT(found_nearest_sampler);
    char* json = dvz_drp2_stream_json(stream, "scene_textured_mesh_nearest_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateSampler\"") != NULL);
    AT(strstr(json, "\"mag_filter\": \"nearest\"") != NULL);
    AT(strstr(json, "\"min_filter\": \"nearest\"") != NULL);
    dvz_drp2_stream_json_destroy(json);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_primitive_emits_draw_indexed(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    if (stream == NULL && dvz_diagnostic_report_count(&report) > 0)
        log_error("%s", dvz_diagnostic_report_get(&report, 0));
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_shared_index_buffer_emits_one_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual0 = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* visual1 = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual0);
    ANN(visual1);

    vec3 positions0[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.0f, 0.0f},
        {-0.2f, -0.8f, 0.0f}, {-0.2f, 0.0f, 0.0f},
    };
    vec3 positions1[4] = {
        {0.2f, 0.0f, 0.0f}, {0.2f, 0.8f, 0.0f},
        {0.8f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors0[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    DvzColor colors1[4] = {
        {0, 0, 255, 255}, {0, 0, 255, 255}, {0, 0, 255, 255}, {0, 0, 255, 255},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual0, "position", positions0, 4) == 0);
    AT(dvz_visual_set_data(visual0, "color", colors0, 4) == 0);
    AT(dvz_visual_set_data(visual1, "position", positions1, 4) == 0);
    AT(dvz_visual_set_data(visual1, "color", colors1, 4) == 0);
    AT(dvz_visual_set_buffer(visual0, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_buffer(visual1, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual0, NULL) == 0);
    AT(dvz_panel_add_visual(panel, visual1, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    AT(_stream_create_buffer_size_count(stream, sizeof(indices)) == 1);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(indices)) == 1);
    AT(_stream_set_index_buffer_count(stream) == 2);
    AT(_stream_draw_indexed_count(stream) == 2);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


static int _scene_textured_mesh_sampling_executes(TstContext* suite)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 255, 255, 255}, {128, 255, 255, 255},
        {255, 128, 255, 255}, {255, 255, 128, 255},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f},
        {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    static const uint8_t pixels[2 * 2 * 4] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 255, 255,
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);
    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    field_desc.color_role = DVZ_COLOR_ROLE_LINEAR_COLOR;
    field_desc.width = 2;
    field_desc.height = 2;
    DvzSampledField* field = dvz_sampled_field(scene, &field_desc);
    ANN(field);
    DvzFieldDataView view = dvz_field_data_view();
    view.data = pixels;
    view.bytes_per_row = 2 * 4;
    view.rows_per_image = 2;
    AT(dvz_sampled_field_set_data(field, &view) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == DVZ_OK);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == DVZ_OK);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == DVZ_OK);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == DVZ_OK);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_field(visual, "texture", field) == DVZ_OK);
    DvzFieldSamplingDesc sampling = dvz_field_sampling_desc();
    sampling.min_filter = DVZ_FIELD_FILTER_NEAREST;
    sampling.mag_filter = DVZ_FIELD_FILTER_NEAREST;
    AT(dvz_visual_set_field_sampling(visual, "texture", &sampling) == DVZ_OK);
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_UNLIT;
    AT(dvz_visual_set_material(visual, &material) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream =
        _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_mesh_emit_executes(suite);
}



int test_scene_textured_mesh_sampling_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_textured_mesh_sampling_executes(suite);
}


int test_scene_path_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_path_emit_executes(suite, 4);
}


/**
 * Verify line-width path visuals lower to the scene.path stroke pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_line_width_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    vec3 positions[5] = {
        {-0.75f, -0.25f, 0.0f},
        {-0.35f,  0.25f, 0.0f},
        { 0.00f, -0.10f, 0.0f},
        { 0.35f,  0.35f, 0.0f},
        { 0.75f, -0.25f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 0, 0, 255},
        {255, 255, 0, 255},
        {0, 255, 255, 255},
        {0, 128, 255, 255},
        {255, 255, 255, 255},
    };
    float stroke_widths[5] = {3.0f, 6.0f, 9.0f, 5.0f, 2.0f};
    uint32_t subpaths[2] = {3, 2};

    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", stroke_widths, 5) == 0);
    AT(dvz_path_set_subpaths(visual, 2, subpaths) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_set_index = false;
    bool found_draw_indexed = false;
    bool found_material_bg = false;
    uint32_t set_vertex_buffer_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_pathg") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.binding_count == 8);
                AT(cmd->u.create_render_pipeline.attr_count == 8);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
            set_vertex_buffer_count++;
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 18;
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            found_material_bg = found_material_bg || cmd->u.set_bind_group.slot == 1;
    }

    AT(found_pipeline);
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(found_material_bg);
    AT(set_vertex_buffer_count == 8);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify repeated first/last path points are lowered as closed joins, not open caps.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_repeated_endpoint_closes_subpath(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    vec3 positions[5] = {
        {-0.50f, -0.50f, 0.0f},
        {+0.50f, -0.50f, 0.0f},
        {+0.50f, +0.50f, 0.0f},
        {-0.50f, +0.50f, 0.0f},
        {-0.50f, -0.50f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float stroke_widths[5] = {8.0f, 8.0f, 8.0f, 8.0f, 8.0f};
    uint32_t subpath = 5;

    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", stroke_widths, 5) == 0);
    AT(dvz_path_set_subpaths(visual, 1, &subpath) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.path.closed", 0);
    AT(plan != NULL);
    _scene_emit_visual_uploads(figure, plan, NULL);

    const uint32_t has_prev = 0x04u;
    const uint32_t has_next = 0x08u;
    const uint32_t subpath_start = 0x10u;
    const uint32_t subpath_end = 0x20u;
    const DvzPathGpuCache* cache = &_visual_family_state(visual)->path.gpu;
    AT(cache->segment_count == 4);
    AT(cache->vertex_count == 16);

    const uint32_t first_start = cache->path_flags[0];
    AT((first_start & has_prev) != 0);
    AT((first_start & has_next) != 0);
    AT((first_start & subpath_start) == 0);
    AC(cache->position_prev[0], positions[3][0], 1e-6);
    AC(cache->position_prev[1], positions[3][1], 1e-6);
    AC(cache->position_start[0], positions[0][0], 1e-6);
    AC(cache->position_start[1], positions[0][1], 1e-6);
    AC(cache->position_end[0], positions[1][0], 1e-6);
    AC(cache->position_end[1], positions[1][1], 1e-6);
    AC(cache->position_next[0], positions[2][0], 1e-6);
    AC(cache->position_next[1], positions[2][1], 1e-6);

    const uint32_t last_end_index = 4 * 3 + 2;
    const uint32_t last_end = cache->path_flags[last_end_index];
    AT((last_end & has_prev) != 0);
    AT((last_end & has_next) != 0);
    AT((last_end & subpath_end) == 0);
    AC(cache->position_prev[3 * last_end_index + 0], positions[2][0], 1e-6);
    AC(cache->position_prev[3 * last_end_index + 1], positions[2][1], 1e-6);
    AC(cache->position_start[3 * last_end_index + 0], positions[3][0], 1e-6);
    AC(cache->position_start[3 * last_end_index + 1], positions[3][1], 1e-6);
    AC(cache->position_end[3 * last_end_index + 0], positions[4][0], 1e-6);
    AC(cache->position_end[3 * last_end_index + 1], positions[4][1], 1e-6);
    AC(cache->position_next[3 * last_end_index + 0], positions[1][0], 1e-6);
    AC(cache->position_next[3 * last_end_index + 1], positions[1][1], 1e-6);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify sharp closed-ring sentinels keep both seam-side adjacency points.
 *
 * @param suite the active test suite
 * @param item the test item
 * @return 0 on success
 */
int test_scene_path_closed_star_cache_adjacency(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    enum
    {
        STAR_POINT_COUNT = 11,
    };
    const float tau = 6.28318530718f;
    vec3 positions[STAR_POINT_COUNT] = {{0}};
    DvzColor colors[STAR_POINT_COUNT] = {{0}};
    float stroke_widths[STAR_POINT_COUNT] = {0};
    for (uint32_t i = 0; i < STAR_POINT_COUNT; i++)
    {
        const uint32_t k = i % 10u;
        const float radius = (k % 2u) == 0u ? 0.45f : 0.14f;
        const float a = -0.25f * tau + tau * (float)k / 10.0f;
        positions[i][0] = radius * cosf(a);
        positions[i][1] = radius * sinf(a);
        positions[i][2] = 0.0f;
        colors[i] = (DvzColor){255, 255, 255, 255};
        stroke_widths[i] = 10.0f;
    }

    uint32_t subpath = STAR_POINT_COUNT;
    AT(dvz_visual_set_data(visual, "position", positions, STAR_POINT_COUNT) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, STAR_POINT_COUNT) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", stroke_widths, STAR_POINT_COUNT) == 0);
    AT(dvz_path_set_subpaths(visual, 1, &subpath) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.path.closed_star", 0);
    AT(plan != NULL);
    _scene_emit_visual_uploads(figure, plan, NULL);

    const uint32_t has_prev = 0x04u;
    const uint32_t has_next = 0x08u;
    const uint32_t subpath_start = 0x10u;
    const uint32_t subpath_end = 0x20u;
    const DvzPathGpuCache* cache = &_visual_family_state(visual)->path.gpu;
    AT(cache->segment_count == STAR_POINT_COUNT - 1);
    AT(cache->vertex_count == 4u * (STAR_POINT_COUNT - 1u));

    const uint32_t first_start = cache->path_flags[0];
    AT((first_start & has_prev) != 0);
    AT((first_start & has_next) != 0);
    AT((first_start & subpath_start) == 0);
    AC(cache->position_prev[0], positions[STAR_POINT_COUNT - 2][0], 1e-6);
    AC(cache->position_prev[1], positions[STAR_POINT_COUNT - 2][1], 1e-6);
    AC(cache->position_start[0], positions[0][0], 1e-6);
    AC(cache->position_start[1], positions[0][1], 1e-6);
    AC(cache->position_end[0], positions[1][0], 1e-6);
    AC(cache->position_end[1], positions[1][1], 1e-6);
    AC(cache->position_next[0], positions[2][0], 1e-6);
    AC(cache->position_next[1], positions[2][1], 1e-6);

    const uint32_t last_end_index = 4u * (STAR_POINT_COUNT - 2u) + 2u;
    const uint32_t last_end = cache->path_flags[last_end_index];
    AT((last_end & has_prev) != 0);
    AT((last_end & has_next) != 0);
    AT((last_end & subpath_end) == 0);
    AC(cache->position_prev[3u * last_end_index + 0u], positions[STAR_POINT_COUNT - 3][0], 1e-6);
    AC(cache->position_prev[3u * last_end_index + 1u], positions[STAR_POINT_COUNT - 3][1], 1e-6);
    AC(cache->position_start[3u * last_end_index + 0u], positions[STAR_POINT_COUNT - 2][0], 1e-6);
    AC(cache->position_start[3u * last_end_index + 1u], positions[STAR_POINT_COUNT - 2][1], 1e-6);
    AC(cache->position_end[3u * last_end_index + 0u], positions[STAR_POINT_COUNT - 1][0], 1e-6);
    AC(cache->position_end[3u * last_end_index + 1u], positions[STAR_POINT_COUNT - 1][1], 1e-6);
    AC(cache->position_next[3u * last_end_index + 0u], positions[1][0], 1e-6);
    AC(cache->position_next[3u * last_end_index + 1u], positions[1][1], 1e-6);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* TRIANGLE_STRIP: TL, BL, TR, BR */
    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {-0.5f,  0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.5f,  0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    /* 4x4 RGBA8 checker pattern. */
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t y = 0; y < 4; y++)
    {
        for (uint32_t x = 0; x < 4; x++)
        {
            uint32_t i = (y * 4 + x) * 4;
            uint8_t v = ((x ^ y) & 1) ? 255 : 0;
            pixels[i+0] = v; pixels[i+1] = v; pixels[i+2] = v; pixels[i+3] = 255;
        }
    }

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(_scene_visual_set_texture_rgba8(visual, (const uint8_t*)pixels, 4, 4, 4u * 4u * 4u) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}
