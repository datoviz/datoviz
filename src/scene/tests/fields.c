/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field and scale tests                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <string.h>

#include "_assertions.h"
#include "../_scene.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_scale_colormap_colorbar_core(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 100, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Depth",
                   .unit = "um",
                   .format =
                       (DvzFormatDesc){
                           .precision = 2,
                           .show_unit = true,
                           .unit = "um",
                           .suffix = " depth",
                       },
               });
    ANN(scale);
    AT(scene->scale_count == 1);
    AT(scale->scene == scene);
    AT(scale->kind == DVZ_SCALE_CONTINUOUS);
    AT(strcmp(scale->label, "Depth") == 0);
    AT(strcmp(scale->unit, "um") == 0);
    AT(scale->format.precision == 2);
    AT(strcmp(scale->format.unit, "um") == 0);
    AT(strcmp(scale->format.suffix, " depth") == 0);

    dvz_scale_set_domain(scale, -600.0, 0.0);
    dvz_scale_set_view_range(scale, -300.0, -50.0);
    AT(scale->has_domain);
    AT(scale->domain_min == -600.0);
    AT(scale->domain_max == 0.0);
    AT(scale->has_view_range);
    AT(scale->view_min == -300.0);
    AT(scale->view_max == -50.0);

    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_MAGMA);
    ANN(colormap);
    AT(scene->colormap_count == 1);
    AT(colormap->builtin == DVZ_BUILTIN_COLORMAP_MAGMA);

    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 255, 255}},
    };
    dvz_colormap_set_center(colormap, 0.5);
    dvz_colormap_set_stops(colormap, stops, 2);
    AT(colormap->has_center);
    AT(colormap->center == 0.5);
    AT(colormap->stop_count == 2);
    AT(colormap->stops[1].rgba[0] == 255);

    dvz_scale_set_colormap(scale, colormap);
    AT(scale->colormap == colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale, &(DvzColorbarDesc){
                          .orientation = DVZ_COLORBAR_ORIENTATION_HORIZONTAL,
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_BOTTOM,
                          .title = "Depth map",
                      });
    ANN(colorbar);
    AT(scene->colorbar_count == 1);
    AT(panel->colorbar_count == 1);
    AT(panel->colorbars[0] == colorbar);
    AT(colorbar->scene == scene);
    AT(colorbar->panel == panel);
    AT(colorbar->scale == scale);
    AT(colorbar->orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL);
    AT(colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM);
    AT(strcmp(colorbar->title, "Depth map") == 0);

    dvz_colorbar_set_format(
        colorbar, &(DvzFormatDesc){
                       .precision = 0,
                       .unit = "um",
                   });
    AT(colorbar->has_format);
    AT(colorbar->format.precision == 0);
    AT(strcmp(colorbar->format.unit, "um") == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_colorbar_rejects_cross_scene_scale(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzFigure* figure = dvz_figure(scene0, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzScale* foreign_scale = dvz_scale(scene1, NULL);
    ANN(foreign_scale);

    DvzColorbar* colorbar = dvz_colorbar(panel, foreign_scale, NULL);
    AT(colorbar == NULL);
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_image_visual_binds_colormap_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Intensity",
                   .unit = "a.u.",
               });
    ANN(scale);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);
    AT(image->scale == scale);
    AT(strcmp(image->scale_slot, "colormap") == 0);

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    float positions[4 * 3] = {
        -1.0f, +1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        +1.0f, +1.0f, 0.0f,
        +1.0f, -1.0f, 0.0f,
    };
    float texcoords[4 * 2] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };
    static const uint8_t pixels[4 * 4 * 4] = {0};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 16, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"scale\":{\"id\":\"s0\",\"slot\":\"colormap\"}") != NULL);
    AT(strstr(json, "\"field\":{\"id\":\"f0\",\"slot\":\"field\"}") != NULL);
    AT(strstr(json, "\"fields\":[{\"id\":\"f0\"") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_scale_rejects_cross_scene_scale(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzScale* foreign_scale = dvz_scale(scene1, NULL);
    ANN(foreign_scale);
    DvzVisual* image = dvz_image(scene0, 0);
    ANN(image);

    AT(dvz_visual_set_scale(image, "colormap", foreign_scale) != 0);
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_visual_buffer_rejects_cross_scene_buffer(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzSceneBuffer* foreign_buffer = dvz_scene_buffer(
        scene1, &(DvzSceneBufferDesc){.usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    ANN(foreign_buffer);
    DvzVisual* visual = dvz_primitive(scene0, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    AT(!dvz_visual_set_buffer(visual, "index", foreign_buffer));
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_image_scalar_texture_uses_bound_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);
    AT(dvz_visual_set_scale(visual, "colormap", scale) == 0);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    float pixels[4 * 4];
    for (uint32_t i = 0; i < 16; i++)
        pixels[i] = 1.0f;

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_f32(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(visual->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)visual->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_r16_float_field_uses_bound_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    uint16_t values[16];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 0x3c00u; /* half-float 1.0 */

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(uint16_t), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(image->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)image->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_r16_snorm_field_uses_bound_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, -1.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    int16_t values[16];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 32767; /* SNORM 1.0 */

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_SNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(int16_t), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(image->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)image->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_field_rejects_cross_scene_field(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzVisual* image = dvz_image(scene0, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene1, &(DvzSampledFieldDesc){
                    .dim = DVZ_FIELD_DIM_2D,
                    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                    .width = 2,
                    .height = 2,
                    .depth = 1,
                });
    ANN(field);

    AT(!dvz_visual_set_field(image, "field", field));
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}


int test_scene_sampled_field_update_region(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    uint8_t base[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));

    uint8_t patch[4] = {1, 2, 3, 4};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 1, .z = 0, .width = 2, .height = 2, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 2}));

    uint8_t* data = (uint8_t*)field->data;
    AT(data[5] == 1);
    AT(data[6] == 2);
    AT(data[9] == 3);
    AT(data[10] == 4);
    AT(field->dirty);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_rejects_unsupported_format(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RG32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_VECTOR_2,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    AT(field == NULL);
    AT(_captured_log_contains(suite, "unsupported sampled field format"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_visual_rejects_3d_field(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 4,
               });
    ANN(field);

    AT(!dvz_visual_set_field(image, "field", field));
    AT(_captured_log_contains(suite, "require a 2D sampled field"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_visual_binds_3d_field(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    AT(volume->type == DVZ_VISUAL_TYPE_VOLUME);

    DvzSampledField* field3d = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 4,
               });
    ANN(field3d);
    AT(dvz_visual_set_field(volume, "field", field3d));
    AT(volume->field == field3d);
    AT(strcmp(volume->field_slot, "field") == 0);
    AT(volume->texture.dirty);

    DvzSampledField* field2d = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field2d);
    AT(!dvz_visual_set_field(volume, "field", field2d));
    AT(_captured_log_contains(suite, "require a 3D sampled field"));

    AT(dvz_visual_set_field(volume, "field", NULL));
    AT(volume->field == NULL);
    AT(volume->field_slot[0] == '\0');

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_field_emit_realizes_3d_texture(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);

    uint16_t base[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = base,
                   .bytes_per_row = 2 * sizeof(uint16_t),
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t texture_id = 0;
    bool created_texture = false;
    bool wrote_full_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            texture_id = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
        {
            created_texture = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
            AT(cmd->u.create_texture.depth == 2);
            AT(cmd->u.create_texture.format == VK_FORMAT_R16_UNORM);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            wrote_full_texture = true;
            AT(cmd->u.write_texture.origin_x == 0);
            AT(cmd->u.write_texture.origin_y == 0);
            AT(cmd->u.write_texture.origin_z == 0);
            AT(cmd->u.write_texture.width == 2);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 2);
            AT(cmd->u.write_texture.bytes_per_row == 2 * sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
        }
    }
    AT(texture_id != 0);
    AT(created_texture);
    AT(wrote_full_texture);
    AT(!field->dirty);
    AT(!volume->texture.dirty);
    dvz_drp2_stream_destroy(stream0);

    uint16_t patch[2] = {101, 102};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 0, .z = 1, .width = 1, .height = 2, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = sizeof(uint16_t),
            .rows_per_image = 2,
        }));

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool recreated_texture = false;
    bool wrote_partial_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
            recreated_texture = true;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            const uint16_t* uploaded = (const uint16_t*)cmd->u.write_texture.data_raw;
            wrote_partial_texture = true;
            AT(cmd->u.write_texture.origin_x == 1);
            AT(cmd->u.write_texture.origin_y == 0);
            AT(cmd->u.write_texture.origin_z == 1);
            AT(cmd->u.write_texture.width == 1);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 1);
            AT(cmd->u.write_texture.bytes_per_row == sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
            ANN(uploaded);
            AT(uploaded[0] == 101);
            AT(uploaded[1] == 102);
        }
    }
    AT(!recreated_texture);
    AT(wrote_partial_texture);
    AT(!field->dirty);
    AT(!volume->texture.dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_retained_controls(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);

    const DvzVolumeState* state = dvz_volume_state(volume);
    ANN(state);
    AT(state->opacity == 1.0f);
    AT(state->sampling == DVZ_VOLUME_SAMPLING_LINEAR);
    AT(state->render_mode == DVZ_VOLUME_RENDER_SLICE);
    AT(state->step_count == 64);
    AT(!state->clipping_enabled);
    AT(state->clip_min[0] == 0.0);
    AT(state->clip_max[2] == 1.0);
    uint64_t version0 = state->version;

    AT(dvz_volume_set_opacity(volume, 0.35f) == 0);
    AT(dvz_volume_state(volume)->opacity == 0.35f);
    AT(dvz_volume_state(volume)->version != version0);
    uint64_t version1 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);
    AT(dvz_volume_state(volume)->sampling == DVZ_VOLUME_SAMPLING_NEAREST);
    AT(dvz_volume_state(volume)->version != version1);
    uint64_t version2 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_state(volume)->render_mode == DVZ_VOLUME_RENDER_MIP);
    AT(dvz_volume_state(volume)->version != version2);
    uint64_t version3 = dvz_volume_state(volume)->version;

    AT(dvz_volume_set_step_count(volume, 32) == 0);
    AT(dvz_volume_state(volume)->step_count == 32);
    AT(dvz_volume_state(volume)->version != version3);

    double clip_min[3] = {0.1, 0.2, 0.3};
    double clip_max[3] = {0.9, 0.8, 0.7};
    AT(dvz_volume_set_clipping_box(volume, clip_min, clip_max) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(state->clipping_enabled);
    AT(state->clip_min[1] == 0.2);
    AT(state->clip_max[2] == 0.7);

    AT(dvz_volume_clear_clipping(volume) == 0);
    state = dvz_volume_state(volume);
    ANN(state);
    AT(!state->clipping_enabled);
    AT(state->clip_min[0] == 0.0);
    AT(state->clip_max[2] == 1.0);

    AT(dvz_volume_set_opacity(volume, -0.1f) != 0);
    AT(_captured_log_contains(suite, "volume opacity must be finite"));
    double invalid_min[3] = {0.0, 0.8, 0.0};
    double invalid_max[3] = {1.0, 0.7, 1.0};
    AT(dvz_volume_set_clipping_box(volume, invalid_min, invalid_max) != 0);
    AT(_captured_log_contains(suite, "volume clipping box coordinates"));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    AT(dvz_visual_set_scale(volume, "colormap", scale) == 0);
    AT(volume->scale == scale);
    AT(strcmp(volume->scale_slot, "colormap") == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    AT(dvz_volume_state(image) == NULL);
    AT(dvz_volume_set_sampling(image, DVZ_VOLUME_SAMPLING_LINEAR) != 0);
    AT(_captured_log_contains(suite, "requires a volume visual"));
    AT(dvz_volume_set_render_mode(image, DVZ_VOLUME_RENDER_SLICE) != 0);
    AT(_captured_log_contains(suite, "requires a volume visual"));
    AT(dvz_volume_set_step_count(volume, 0) != 0);
    AT(_captured_log_contains(suite, "volume step count must be in"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_volume_visual_metadata_lowering(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 8,
                   .height = 6,
                   .depth = 4,
               });
    ANN(field);
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    AT(dvz_visual_set_scale(volume, "colormap", scale) == 0);
    AT(dvz_volume_set_opacity(volume, 0.5f) == 0);
    AT(dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_NEAREST) == 0);
    double clip_min[3] = {0.1, 0.2, 0.3};
    double clip_max[3] = {0.9, 0.8, 0.7};
    AT(dvz_volume_set_clipping_box(volume, clip_min, clip_max) == 0);

    DvzFramePlanVisualMeta metadata = {0};
    AT(_scene_visual_frame_plan_metadata(figure, volume, 0, &metadata));
    AT(metadata.has_metadata);
    AT(metadata.visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(metadata.visual_index == 0);
    AT(metadata.has_volume);
    AT(metadata.field_format == DVZ_FIELD_FORMAT_R8_UNORM);
    AT(metadata.field_width == 8);
    AT(metadata.field_height == 6);
    AT(metadata.field_depth == 4);
    AT(metadata.scale_index == 0);
    AT(metadata.volume_state.opacity == 0.5f);
    AT(metadata.volume_state.sampling == DVZ_VOLUME_SAMPLING_NEAREST);
    AT(metadata.volume_state.clipping_enabled);
    AT(metadata.volume_state.clip_min[2] == 0.3);
    AT(metadata.volume_state.clip_max[1] == 0.8);
    AT(metadata.texture_id[0] != '\0');
    AT(strcmp(metadata.volume_texture_id, metadata.texture_id) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.volume.metadata", 0);
    ANN(plan);
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "volume.debug"));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, 0);
    ANN(node);
    AT(node->u.render.visual_metadata[0].has_metadata);
    AT(node->u.render.visual_metadata[0].has_volume);
    AT(node->u.render.visual_metadata[0].volume_state.opacity == 0.5f);
    AT(strcmp(node->u.render.visual_metadata[0].volume_texture_id, metadata.texture_id) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_3d_emits_runtime_texture_upload(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);

    uint16_t base[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = base,
                   .bytes_per_row = 2 * sizeof(uint16_t),
                   .rows_per_image = 2,
               }));

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzFramePlan* frame0 = dvz_frame_plan("figure.field3d.runtime", 0);
    ANN(frame0);
    AT(_scene_emit_sampled_field_texture_upload(frame0, "tex.field.volume", field));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame0, "visual.texture.volume"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t texture_id = 0;
    bool created_texture = false;
    bool wrote_full_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            texture_id = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
        {
            created_texture = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
            AT(cmd->u.create_texture.depth == 2);
            AT(cmd->u.create_texture.format == VK_FORMAT_R16_UNORM);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            wrote_full_texture = true;
            AT(cmd->u.write_texture.origin_z == 0);
            AT(cmd->u.write_texture.width == 2);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 2);
            AT(cmd->u.write_texture.bytes_per_row == 2 * sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
        }
    }
    AT(texture_id != 0);
    AT(created_texture);
    AT(wrote_full_texture);

    field->dirty = false;
    field->dirty_full = false;
    field->dirty_region = (DvzFieldRegion){0};
    uint16_t patch[2] = {101, 102};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 0, .z = 1, .width = 1, .height = 2, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = sizeof(uint16_t),
            .rows_per_image = 2,
        }));

    DvzFramePlan* frame1 = dvz_frame_plan("figure.field3d.runtime", 1);
    ANN(frame1);
    AT(_scene_emit_sampled_field_texture_upload(frame1, "tex.field.volume", field));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame1, "visual.texture.volume"));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool recreated_texture = false;
    bool wrote_partial_texture = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == texture_id)
            recreated_texture = true;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == texture_id)
        {
            const uint16_t* uploaded = (const uint16_t*)cmd->u.write_texture.data_raw;
            wrote_partial_texture = true;
            AT(cmd->u.write_texture.origin_x == 1);
            AT(cmd->u.write_texture.origin_y == 0);
            AT(cmd->u.write_texture.origin_z == 1);
            AT(cmd->u.write_texture.width == 1);
            AT(cmd->u.write_texture.height == 2);
            AT(cmd->u.write_texture.depth == 1);
            AT(cmd->u.write_texture.bytes_per_row == sizeof(uint16_t));
            AT(cmd->u.write_texture.rows_per_image == 2);
            ANN(uploaded);
            AT(uploaded[0] == 101);
            AT(uploaded[1] == 102);
        }
    }
    AT(!recreated_texture);
    AT(wrote_partial_texture);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_update_region_rejects_out_of_bounds(
    TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    uint8_t base[16] = {0};
    uint8_t patch[4] = {1, 2, 3, 4};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));
    AT(!dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 3, .y = 3, .z = 0, .width = 2, .height = 2, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(_captured_log_contains(suite, "update region exceeds field dimensions"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_sampled_field_destroy_clears_visual_binding(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);

    uint8_t rgba[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = rgba, .bytes_per_row = 8, .rows_per_image = 2}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(image->field == field);
    AT(strcmp(image->field_slot, "field") == 0);

    AT(dvz_sampled_field_destroy(field));
    AT(image->field == NULL);
    AT(image->field_slot[0] == '\0');

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_shared_field_update_marks_two_visuals_dirty(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 2 * sizeof(float), .rows_per_image = 2}));
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_visual_set_field(image1, "field", field));

    image0->texture.dirty = false;
    image1->texture.dirty = false;
    field->dirty = false;

    float patch[1] = {1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 1, .z = 0, .width = 1, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch, .bytes_per_row = sizeof(float), .rows_per_image = 1}));
    AT(field->dirty);
    AT(image0->texture.dirty);
    AT(image1->texture.dirty);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_field_partial_update_emits_texture_subregion(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(float), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    dvz_drp2_stream_destroy(stream0);

    float patch[2] = {1.0f, 1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 1,
        }));

    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);

    uint32_t write_texture_count = 0;
    bool found_region = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        write_texture_count++;
        if (cmd->u.write_texture.origin_x == 1 && cmd->u.write_texture.origin_y == 2 &&
            cmd->u.write_texture.width == 2 && cmd->u.write_texture.height == 1)
        {
            found_region = true;
        }
    }
    AT(write_texture_count == 1);
    AT(found_region);
    AT(!field->dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure a retained image field resize emits a new texture allocation extent.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_field_resize_emits_texture_reallocation(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    uint8_t pixels[2 * 2 * 4] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 2 * 4, .rows_per_image = 2}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex0 = 0;
    bool created_tex0 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex0 = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex0)
        {
            created_tex0 = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
        }
    }
    AT(tex0 != 0);
    AT(created_tex0);
    dvz_drp2_stream_destroy(stream0);

    uint8_t resized[4 * 3 * 4] = {0};
    AT(dvz_sampled_field_resize(
        field, 4, 3, 1,
        &(DvzFieldDataView){.data = resized, .bytes_per_row = 4 * 4, .rows_per_image = 3}));
    const DvzSampledFieldDesc* desc = dvz_sampled_field_desc(field);
    ANN(desc);
    AT(desc->width == 4);
    AT(desc->height == 3);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex1 = 0;
    bool created_tex1 = false;
    bool wrote_resized = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            tex1 = cmd->u.write_texture.texture_id;
            if (cmd->u.write_texture.origin_x == 0 && cmd->u.write_texture.origin_y == 0 &&
                cmd->u.write_texture.width == 4 && cmd->u.write_texture.height == 3)
            {
                wrote_resized = true;
            }
        }
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex1)
        {
            created_tex1 = true;
            AT(cmd->u.create_texture.width == 4);
            AT(cmd->u.create_texture.height == 3);
        }
    }
    AT(tex1 != 0);
    AT(tex1 != tex0);
    AT(created_tex1);
    AT(wrote_resized);
    AT(!field->dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_shared_field_mixed_full_and_partial_uploads(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale0);
    ANN(scale1);
    dvz_scale_set_domain(scale0, 0.0, 1.0);
    dvz_scale_set_domain(scale1, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale0, colormap);
    dvz_scale_set_colormap(scale1, colormap);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(float), .rows_per_image = 4}));

    float positions0[4][3] = {
        {-1.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        { 0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 0.0f},
    };
    float positions1[4][3] = {
        {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", positions0, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "colormap", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", positions1, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "colormap", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    dvz_drp2_stream_destroy(stream0);

    float patch[2] = {1.0f, 1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 1,
        }));
    dvz_scale_set_view_range(scale0, 0.0, 1.0);

    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);

    uint32_t write_texture_count = 0;
    bool found_full = false;
    bool found_partial = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        write_texture_count++;
        if (cmd->u.write_texture.origin_x == 0 && cmd->u.write_texture.origin_y == 0 &&
            cmd->u.write_texture.width == 4 && cmd->u.write_texture.height == 4)
        {
            found_full = true;
        }
        if (cmd->u.write_texture.origin_x == 1 && cmd->u.write_texture.origin_y == 2 &&
            cmd->u.write_texture.width == 2 && cmd->u.write_texture.height == 1)
        {
            found_partial = true;
        }
    }
    AT(write_texture_count == 2);
    AT(found_full);
    AT(found_partial);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Register scene field and scale tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_fields(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TEST_SIMPLE(test_scene_scale_colormap_colorbar_core);
    TEST_SIMPLE(test_scene_colorbar_rejects_cross_scene_scale);
    TEST_SIMPLE(test_scene_image_visual_binds_colormap_scale);
    TEST_SIMPLE(test_scene_visual_scale_rejects_cross_scene_scale);
    TEST_SIMPLE(test_scene_visual_buffer_rejects_cross_scene_buffer);
    TEST_SIMPLE(test_scene_image_scalar_texture_uses_bound_scale);
    TEST_SIMPLE(test_scene_image_r16_float_field_uses_bound_scale);
    TEST_SIMPLE(test_scene_image_r16_snorm_field_uses_bound_scale);
    TEST_SIMPLE(test_scene_visual_field_rejects_cross_scene_field);
    TEST_SIMPLE(test_scene_sampled_field_update_region);
    TEST_SIMPLE(test_scene_sampled_field_rejects_unsupported_format);
    TEST_SIMPLE(test_scene_image_visual_rejects_3d_field);
    TEST_SIMPLE(test_scene_volume_visual_binds_3d_field);
    TEST_SIMPLE(test_scene_volume_field_emit_realizes_3d_texture);
    TEST_SIMPLE(test_scene_volume_retained_controls);
    TEST_SIMPLE(test_scene_volume_visual_metadata_lowering);
    TEST_SIMPLE(test_scene_sampled_field_3d_emits_runtime_texture_upload);
    TEST_SIMPLE(test_scene_sampled_field_update_region_rejects_out_of_bounds);
    TEST_SIMPLE(test_scene_sampled_field_destroy_clears_visual_binding);
    TEST_SIMPLE(test_scene_shared_field_update_marks_two_visuals_dirty);
    TEST_SIMPLE(test_scene_image_field_partial_update_emits_texture_subregion);
    TEST_SIMPLE(test_scene_image_field_resize_emits_texture_reallocation);
    TEST_SIMPLE(test_scene_shared_field_mixed_full_and_partial_uploads);

    return 0;
}
