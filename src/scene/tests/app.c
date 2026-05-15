/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene app tests                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/scene.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Build scene */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f};
    uint8_t colors[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 20.0f, 15.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Create app and offscreen window */
    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    /* Exercise host-driven and Datoviz-owned frame paths. */
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_render_once(app) == 0);
    dvz_app_run(app, 1);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_panel_three_visuals_all_drawn(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    AT(panel != NULL);

    /* Three non-overlapping points: red (left), green (center), blue (right). */
    float pos_r[3] = {-0.6f, 0.0f, 0.0f};
    float pos_g[3] = { 0.0f, 0.0f, 0.0f};
    float pos_b[3] = { 0.6f, 0.0f, 0.0f};
    DvzColor red   = {220, 20, 20, 255};
    DvzColor green = {20, 220, 20, 255};
    DvzColor blue  = {20, 20, 220, 255};
    float size = 10.0f;

    DvzVisual* vr = dvz_point(scene, 0);
    DvzVisual* vg = dvz_point(scene, 0);
    DvzVisual* vb = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color",    &red,  1) == 0);
    AT(dvz_visual_set_data(vr, "size",     &size, 1) == 0);
    AT(dvz_visual_set_data(vg, "position", pos_g, 1) == 0);
    AT(dvz_visual_set_data(vg, "color",    &green, 1) == 0);
    AT(dvz_visual_set_data(vg, "size",     &size, 1) == 0);
    AT(dvz_visual_set_data(vb, "position", pos_b, 1) == 0);
    AT(dvz_visual_set_data(vb, "color",    &blue, 1) == 0);
    AT(dvz_visual_set_data(vb, "size",     &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, vr, NULL) == 0);
    AT(dvz_panel_add_visual(panel, vg, NULL) == 0);
    AT(dvz_panel_add_visual(panel, vb, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_panel_three_visuals_all_drawn skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t red_count = 0, green_count = 0, blue_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);

        red_count = green_count = blue_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* px = &rgba[4 * i];
            if (px[0] > 150 && px[0] > px[1] + 80 && px[0] > px[2] + 80)
                red_count++;
            if (px[1] > 150 && px[1] > px[0] + 80 && px[1] > px[2] + 80)
                green_count++;
            if (px[2] > 150 && px[2] > px[0] + 80 && px[2] > px[1] + 80)
                blue_count++;
        }
        dvz_free(rgba);
        if (red_count > 0 && green_count > 0 && blue_count > 0)
            break;
    }
    AT(red_count > 0);
    AT(green_count > 0);
    AT(blue_count > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Build scene with ONE large yellow point at center. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255}; /* yellow */
    float size = 32.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    /* Count pixels with r>200 && g>200 (yellow-ish from the point). */
    uint32_t yellow_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200)
            yellow_count++;
    }
    AT(yellow_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* Large quad covering most of the panel. TRIANGLE_STRIP order: TL, BL, TR, BR */
    float positions[4][3] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };

    /* Solid red 4x4 texture. */
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[i * 4 + 0] = 255;
        pixels[i * 4 + 1] = 0;
        pixels[i * 4 + 2] = 0;
        pixels[i * 4 + 3] = 255;
    }

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    /* Count pixels that are red-dominant (from the solid red texture). */
    uint32_t red_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50)
            red_count++;
    }
    AT(red_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_field_partial_update_changes_region(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        {-0.95f, -0.95f, 0.0f}, {-0.95f, 0.95f, 0.0f},
        {0.95f, -0.95f, 0.0f},  {0.95f, 0.95f, 0.0f},
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
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_image_field_partial_update_changes_region skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 64);
    AT(height0 == 64);

    float patch[8];
    for (uint32_t i = 0; i < 8; i++)
        patch[i] = 1.0f;
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 2, .y = 0, .z = 0, .width = 2, .height = 4, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 4,
        }));

    dvz_app_run(app, 1);

    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);
    AT(width1 == 64);
    AT(height1 == 64);

    const uint8_t* left0 = _pixel_at(rgba0, width0, height0, 16, 32);
    const uint8_t* right0 = _pixel_at(rgba0, width0, height0, 48, 32);
    const uint8_t* left1 = _pixel_at(rgba1, width1, height1, 16, 32);
    const uint8_t* right1 = _pixel_at(rgba1, width1, height1, 48, 32);

    AT(left0[2] > 180);
    AT(right0[2] > 180);
    AT((int)left1[0] - (int)left0[0] < 40);
    AT(abs((int)left1[2] - (int)left0[2]) < 40);
    AT(right1[0] > 180);
    AT(right1[2] < 80);

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_lit_primitive_depth_orders_overlap(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* near_visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* far_visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(near_visual);
    ANN(far_visual);

    float near_positions[6][3] = {
        {-0.9f, -0.9f, 0.1f}, {-0.9f, 0.9f, 0.1f},  {0.9f, -0.9f, 0.1f},
        {0.9f, -0.9f, 0.1f},  {-0.9f, 0.9f, 0.1f},  {0.9f, 0.9f, 0.1f},
    };
    float far_positions[6][3] = {
        {-0.9f, -0.9f, 0.8f}, {-0.9f, 0.9f, 0.8f},  {0.9f, -0.9f, 0.8f},
        {0.9f, -0.9f, 0.8f},  {-0.9f, 0.9f, 0.8f},  {0.9f, 0.9f, 0.8f},
    };
    float normals[6][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzColor near_colors[6];
    DvzColor far_colors[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        near_colors[i][0] = 32;
        near_colors[i][1] = 64;
        near_colors[i][2] = 255;
        near_colors[i][3] = 255;
        far_colors[i][0] = 255;
        far_colors[i][1] = 32;
        far_colors[i][2] = 32;
        far_colors[i][3] = 255;
    }

    AT(dvz_visual_set_data(near_visual, "position", near_positions, 6) == 0);
    AT(dvz_visual_set_data(near_visual, "color", near_colors, 6) == 0);
    AT(dvz_visual_set_data(near_visual, "normal", normals, 6) == 0);
    AT(dvz_panel_add_visual(panel, near_visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           near_visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    AT(dvz_visual_set_data(far_visual, "position", far_positions, 6) == 0);
    AT(dvz_visual_set_data(far_visual, "color", far_colors, 6) == 0);
    AT(dvz_visual_set_data(far_visual, "normal", normals, 6) == 0);
    AT(dvz_panel_add_visual(panel, far_visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           far_visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_lit_primitive_depth_orders_overlap skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(center[2] > 180);
    AT(center[2] > center[0] + 40);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure an indexed mesh contributes visible pixels through the app offscreen path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_app_offscreen_mesh_renders_nonblank(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 64, 64, 255},
        {64, 255, 64, 255},
        {64, 64, 255, 255},
        {255, 224, 64, 255},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_mesh_renders_nonblank skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(!(center[0] == 13 && center[1] == 13 && center[2] == 20));

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Rotate one cube point into a stable off-axis view for mesh depth tests.
 *
 * @param x the input x coordinate
 * @param y the input y coordinate
 * @param z the input z coordinate
 * @param out the rotated output coordinate
 */


static void _rotated_mesh_rotate_point(float x, float y, float z, float* out)
{
    ANN(out);
    const float ax = -0.65f;
    const float ay = +0.75f;
    const float cx = cosf(ax);
    const float sx = sinf(ax);
    const float cy = cosf(ay);
    const float sy = sinf(ay);

    const float y1 = cx * y - sx * z;
    const float z1 = sx * y + cx * z;
    const float x2 = cy * x + sy * z1;
    const float z2 = -sy * x + cy * z1;

    out[0] = x2;
    out[1] = y1;
    out[2] = z2;
}



/**
 * Build an indexed cube with duplicated vertices and per-face normals.
 *
 * @param positions the output vertex positions
 * @param colors the output vertex colors
 * @param normals the output vertex normals
 * @param indices the output triangle indices
 */


static void _rotated_mesh_build_cube(
    float positions[24][3], DvzColor colors[24], float normals[24][3], DvzIndex indices[36])
{
    const float s = 0.58f;
    const float face_positions[6][4][3] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const float face_normals[6][3] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };
    const DvzColor face_colors[6] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        float rotated_normal[3] = {0};
        _rotated_mesh_rotate_point(
            face_normals[face][0], face_normals[face][1], face_normals[face][2],
            rotated_normal);

        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            _rotated_mesh_rotate_point(
                face_positions[face][corner][0], face_positions[face][corner][1],
                face_positions[face][corner][2], positions[vertex]);
            colors[vertex][0] = face_colors[face][0];
            colors[vertex][1] = face_colors[face][1];
            colors[vertex][2] = face_colors[face][2];
            colors[vertex][3] = face_colors[face][3];
            normals[vertex][0] = rotated_normal[0];
            normals[vertex][1] = rotated_normal[1];
            normals[vertex][2] = rotated_normal[2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
}


/**
 * Build an indexed object-space cube with duplicated vertices and per-face normals.
 *
 * @param positions the output vertex positions
 * @param colors the output vertex colors
 * @param normals the output vertex normals
 * @param indices the output triangle indices
 */


static void _mesh_build_cube_object_space(
    float positions[24][3], DvzColor colors[24], float normals[24][3], DvzIndex indices[36])
{
    const float s = 0.58f;
    const float face_positions[6][4][3] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const float face_normals[6][3] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };
    const DvzColor face_colors[6] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            positions[vertex][0] = face_positions[face][corner][0];
            positions[vertex][1] = face_positions[face][corner][1];
            positions[vertex][2] = face_positions[face][corner][2];
            colors[vertex][0] = face_colors[face][0];
            colors[vertex][1] = face_colors[face][1];
            colors[vertex][2] = face_colors[face][2];
            colors[vertex][3] = face_colors[face][3];
            normals[vertex][0] = face_normals[face][0];
            normals[vertex][1] = face_normals[face][1];
            normals[vertex][2] = face_normals[face][2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
}



/**
 * Ensure a rotated indexed mesh resolves hidden faces through depth, not draw order.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_app_offscreen_rotated_mesh_depth_orders_faces(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    _rotated_mesh_build_cube(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 24) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 24) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 24) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.35f, 0.55f, 0.75f},
               .ambient = 0.25f,
               .diffuse = 0.85f,
           }) == 0);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_rotated_mesh_depth_orders_faces skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 128, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 96);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(center[0] > center[1] + 8);
    AT(center[0] > center[2] + 24);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure an object-space cube renders through panel camera and arcball transforms.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_app_offscreen_camera_arcball_mesh_renders_cube(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.fov_y = GLM_PI_4f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);
    dvz_panel_set_arcball(panel, NULL, 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    ANN(arcball);
    dvz_arcball_initial(arcball, (vec3){+0.6f, -1.2f, +3.0f});

    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    _mesh_build_cube_object_space(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 24) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 24) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 24) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.35f, 0.55f, 0.75f},
               .ambient = 0.25f,
               .diffuse = 0.85f,
           }) == 0);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_camera_arcball_mesh_renders_cube skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 128, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 96);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(!(center[0] == 13 && center[1] == 13 && center[2] == 20));

    const uint8_t* right = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
    AT(right[1] > right[0]);
    AT(right[1] > right[2]);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_shared_field_mixed_runtime_updates(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale0);
    ANN(scale1);
    dvz_scale_set_domain(scale0, 0.0, 1.0);
    dvz_scale_set_domain(scale1, 0.0, 1.0);

    DvzColormap* colormap0 = dvz_colormap(scene, NULL);
    DvzColormap* colormap1 = dvz_colormap(scene, NULL);
    ANN(colormap0);
    ANN(colormap1);
    DvzColormapStop base_stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap0, base_stops, 2);
    dvz_colormap_set_stops(colormap1, base_stops, 2);
    dvz_scale_set_colormap(scale0, colormap0);
    dvz_scale_set_colormap(scale1, colormap1);

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
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));

    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    float left_positions[4][3] = {
        {-1.0f, -0.95f, 0.0f}, {-1.0f, 0.95f, 0.0f},
        {0.0f, -0.95f, 0.0f},  {0.0f, 0.95f, 0.0f},
    };
    float right_positions[4][3] = {
        {0.0f, -0.95f, 0.0f}, {0.0f, 0.95f, 0.0f},
        {1.0f, -0.95f, 0.0f}, {1.0f, 0.95f, 0.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", left_positions, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "colormap", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", right_positions, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "colormap", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_shared_field_mixed_runtime_updates skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);

    DvzColormapStop updated_stops[2] = {
        {.position = 0.0, .rgba = {0, 255, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 0, 255}},
    };
    dvz_colormap_set_stops(colormap0, updated_stops, 2);

    float patch[8];
    for (uint32_t i = 0; i < 8; i++)
        patch[i] = 1.0f;
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 2, .y = 0, .z = 0, .width = 2, .height = 4, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 4,
        }));

    dvz_app_run(app, 1);

    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);

    const uint8_t* left_left0 = _pixel_at(rgba0, width0, height0, 24, 32);
    const uint8_t* left_left1 = _pixel_at(rgba1, width1, height1, 24, 32);
    const uint8_t* right_left0 = _pixel_at(rgba0, width0, height0, 60, 32);
    const uint8_t* right_left1 = _pixel_at(rgba1, width1, height1, 60, 32);
    const uint8_t* right_right1 = _pixel_at(rgba1, width1, height1, 84, 32);

    AT(left_left0[2] > 180);
    AT((int)left_left1[1] > (int)left_left0[1] + 40);
    AT((int)left_left1[2] + 40 < (int)left_left0[2]);

    AT(right_left0[2] > 180);
    AT(abs((int)right_left1[0] - (int)right_left0[0]) < 40);
    AT(abs((int)right_left1[1] - (int)right_left0[1]) < 40);
    AT(abs((int)right_left1[2] - (int)right_left0[2]) < 40);

    AT(right_right1[0] > 180);
    AT(right_right1[2] < 120);

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_retained_render_second_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 32.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_retained_render_second_frame skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t yellow_counts[2] = {0, 0};
    for (uint32_t frame = 0; frame < 2; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 64);
        AT(height == 64);

        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* pixel = &rgba[4 * i];
            if (pixel[0] > 200 && pixel[1] > 200)
                yellow_counts[frame]++;
        }
        dvz_free(rgba);
    }
    AT(yellow_counts[0] > 0);
    AT(yellow_counts[1] > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_retained_render_second_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[i * 4 + 0] = 255; pixels[i * 4 + 1] = 0;
        pixels[i * 4 + 2] = 0;   pixels[i * 4 + 3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_retained_render_second_frame skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Both frames should show red pixels from the retained texture. */
    uint32_t red_counts[2] = {0, 0};
    for (uint32_t frame = 0; frame < 2; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 64);
        AT(height == 64);

        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* pixel = &rgba[4 * i];
            if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50)
                red_counts[frame]++;
        }
        dvz_free(rgba);
    }
    AT(red_counts[0] > 0);
    AT(red_counts[1] > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_two_panel_points_light_both_halves(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    AT(figure != NULL);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(left != NULL);
    AT(right != NULL);

    DvzVisual* left_visual = dvz_point(scene, 0);
    DvzVisual* right_visual = dvz_point(scene, 0);
    AT(left_visual != NULL);
    AT(right_visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor red = {255, 32, 32, 255};
    DvzColor green = {32, 255, 32, 255};
    float size = 24.0f;

    AT(dvz_visual_set_data(left_visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(left_visual, "color", &red, 1) == 0);
    AT(dvz_visual_set_data(left_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(left, left_visual, NULL) == 0);

    AT(dvz_visual_set_data(right_visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(right_visual, "color", &green, 1) == 0);
    AT(dvz_visual_set_data(right_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(right, right_visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_two_panel_points_light_both_halves skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    AT(win != NULL);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t red_count = 0;
    uint32_t green_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 96);
        AT(height == 64);

        red_count = 0;
        green_count = 0;
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint8_t* pixel = &rgba[4 * (y * width + x)];
                if (pixel[0] > 150 && pixel[0] > pixel[1] + 40)
                    red_count++;
                if (pixel[1] > 150 && pixel[1] > pixel[0] + 40)
                    green_count++;
            }
        }
        dvz_free(rgba);
        if (red_count > 0 && green_count > 0)
            break;
    }
    AT(red_count > 0);
    AT(green_count > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_clear_color(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Scene with NO visuals — all pixels should show the clear color. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    (void)panel;
    AT(panel != NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_clear_color skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);

    /* Default clear color is (0.05, 0.05, 0.08, 1.0) — very dark, R<20, G<20, B<25.
       All pixels must be dark (no stray bright pixels from missing clear). */
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* px = &rgba[4 * i];
        if (px[0] > 30 || px[1] > 30 || px[2] > 30)
            bright_count++;
    }
    AT(bright_count == 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_capture_rejects_wrong_dimensions(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    (void)panel;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_wrong_dimensions skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Ask for a dimension that doesn't match the 64x64 offscreen canvas. */
    uint8_t buf[128 * 128 * 4];
    tst_log_capture_begin(suite);
    AT(dvz_canvas_capture_rgba_into(canvas, 128, 128, buf, sizeof(buf)) != 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_capture_rejects_undersized_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    (void)panel;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_undersized_buffer skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Buffer is one byte short of the required 64*64*4 bytes. */
    size_t required = 64 * 64 * 4;
    uint8_t* buf = dvz_malloc(required - 1);
    ANN(buf);
    tst_log_capture_begin(suite);
    AT(dvz_canvas_capture_rgba_into(canvas, 64, 64, buf, required - 1) != 0);
    dvz_free(buf);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


#endif


/**
 * Register scene app tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_app(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    TEST_SIMPLE(test_app_offscreen);
    TEST_SIMPLE(test_app_offscreen_panel_three_visuals_all_drawn);
    TEST_SIMPLE(test_app_offscreen_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_field_partial_update_changes_region);
    TEST_SIMPLE(test_app_offscreen_lit_primitive_depth_orders_overlap);
    TEST_SIMPLE(test_app_offscreen_mesh_renders_nonblank);
    TEST_SIMPLE(test_app_offscreen_rotated_mesh_depth_orders_faces);
    TEST_SIMPLE(test_app_offscreen_camera_arcball_mesh_renders_cube);
    TEST_SIMPLE(test_app_offscreen_shared_field_mixed_runtime_updates);
    TEST_SIMPLE(test_app_offscreen_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_image_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_two_panel_points_light_both_halves);
    TEST_SIMPLE(test_app_offscreen_clear_color);
    TEST_SIMPLE(test_app_capture_rejects_wrong_dimensions);
    TEST_SIMPLE(test_app_capture_rejects_undersized_buffer);
#endif

    return 0;
}
