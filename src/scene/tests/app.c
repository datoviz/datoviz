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

#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "../_scene.h"
#include "../_technique.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/window/backend.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_APP_GPU_RES (TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN)

#define TST_SCENE_APP_CASE(test, resource_flags, isolation_mode)                                  \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = (resource_flags);                                                   \
        _tst_desc.isolation = (isolation_mode);                                                   \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_APP_REQUIRE_VKLITE(ctx)                                                         \
    do                                                                                            \
    {                                                                                             \
        if (!_scene_vklite_runtime_available())                                                   \
        {                                                                                         \
            tst_skip((ctx), "Vulkan instance creation failed");                                   \
            return 0;                                                                             \
        }                                                                                         \
    } while (0)




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP

typedef struct
{
    uint32_t calls;
    double last_t;
    double last_dt;
    double total_dt;
} AppTimerProbe;


typedef struct
{
    uint32_t calls;
    DvzAppWindow* last_window;
} AppRequestFrameProbe;


typedef struct
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzVisual* visual;
} AppSsaoQuad;


typedef struct
{
    uint8_t rgb[3];
    bool skipped;
    const char* skip_reason;
} AppWboitCapture;


typedef struct
{
    uint8_t rgb[3];
    bool skipped;
    const char* skip_reason;
} AppSceneOcclusionCapture;


typedef struct
{
    uint32_t width;
    uint32_t height;
    uint64_t total_sum;
    uint64_t left_sum;
    uint64_t right_sum;
    bool skipped;
    const char* skip_reason;
} AppVolumeOcclusionCapture;


typedef struct
{
    uint32_t width;
    uint32_t height;
    uint8_t* rgba;
    bool skipped;
    const char* skip_reason;
} AppRgbaCapture;


typedef struct
{
    uint32_t error_count;
    bool sampled_bind_group_miss;
    bool contract_validation_failed;
} AppLogCapture;


typedef enum
{
    APP_VOLUME_OCCLUSION_MODE_DISABLED,
    APP_VOLUME_OCCLUSION_MODE_VOLUME,
    APP_VOLUME_OCCLUSION_MODE_SCENE,
} AppVolumeOcclusionMode;



/**
 * Record one app-driven timer callback.
 *
 * @param animation animation handle
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time
 * @param user_data timer probe storage
 */
static void _app_timer_probe_callback(
    DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    AppTimerProbe* probe = (AppTimerProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_t = t;
    probe->last_dt = dt;
    probe->total_dt += dt;
}


/**
 * Record one app-window request-frame callback.
 *
 * @param win app-window requesting a frame
 * @param user_data request-frame probe storage
 */
static void _app_request_frame_probe_callback(DvzAppWindow* win, void* user_data)
{
    AppRequestFrameProbe* probe = (AppRequestFrameProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_window = win;
}


/**
 * Summarize captured app draw errors during retained offscreen regression tests.
 *
 * @param suite the active test suite with log capture enabled
 * @param capture output log summary
 */
static void _app_log_capture_from_suite(TstContext* suite, AppLogCapture* capture)
{
    ANN(suite);
    ANN(capture);
    dvz_memset(capture, sizeof(AppLogCapture), 0, sizeof(AppLogCapture));
    uint32_t count = tst_log_capture_count(suite);
    for (uint32_t i = 0; i < count; i++)
    {
        const TstLogRecord* record = tst_log_capture_get(suite, i);
        if (record == NULL || record->level < LOG_ERROR)
            continue;
        capture->error_count++;
        capture->sampled_bind_group_miss =
            capture->sampled_bind_group_miss ||
            strstr(record->message, "DRP2 sampled bind group misses graph read resource") != NULL;
        capture->contract_validation_failed =
            capture->contract_validation_failed ||
            strstr(record->message, "emitted runtime DRP2 stream failed scene contract validation") != NULL;
    }
}



/**
 * Add one indexed quad mesh to the panel used by SSAO offscreen tests.
 *
 * @param scene scene owner
 * @param panel destination panel
 * @param xmin minimum X coordinate
 * @param xmax maximum X coordinate
 * @param ymin minimum Y coordinate
 * @param ymax maximum Y coordinate
 * @param z clip-depth-like scene coordinate
 * @param color per-vertex color
 * @return created quad handles
 */
static AppSsaoQuad _app_ssao_add_quad(
    DvzScene* scene, DvzPanel* panel, float xmin, float xmax, float ymin, float ymax, float z,
    DvzColor color)
{
    ANN(scene);
    ANN(panel);

    AppSsaoQuad out = {.scene = scene, .panel = panel};
    out.visual = dvz_mesh(scene, 0);
    if (out.visual == NULL)
        return out;

    float positions[4][3] = {
        {xmin, ymin, z},
        {xmax, ymin, z},
        {xmin, ymax, z},
        {xmax, ymax, z},
    };
    DvzColor colors[4] = {0};
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    for (uint32_t i = 0; i < 4; i++)
        dvz_memcpy(colors[i], sizeof(DvzColor), color, sizeof(DvzColor));

    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL)
        return out;
    if (!dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)))
        return out;

    if (dvz_visual_set_data(out.visual, "position", positions, 4) != 0 ||
        dvz_visual_set_data(out.visual, "color", colors, 4) != 0 ||
        dvz_visual_set_data(out.visual, "normal", normals, 4) != 0 ||
        !dvz_visual_set_buffer(out.visual, "index", index_buffer) ||
        dvz_panel_add_visual(panel, out.visual, NULL) != 0)
    {
        out.visual = NULL;
        return out;
    }
    return out;
}



/**
 * Add one full-panel transparent WBOIT primitive triangle.
 *
 * @param scene scene owner
 * @param panel destination panel
 * @param color per-vertex color
 * @return created visual, or NULL on failure
 */
static DvzVisual*
_app_wboit_add_layer(DvzScene* scene, DvzPanel* panel, DvzColor color)
{
    ANN(scene);
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        return NULL;

    float positions[3][3] = {
        {-0.9f, -0.9f, 0.0f},
        {0.9f, -0.9f, 0.0f},
        {0.0f, 0.9f, 0.0f},
    };
    DvzColor colors[3] = {0};
    for (uint32_t i = 0; i < 3; i++)
        dvz_memcpy(colors[i], sizeof(DvzColor), color, sizeof(DvzColor));

    if (dvz_visual_set_data(visual, "position", positions, 3) != 0 ||
        dvz_visual_set_data(visual, "color", colors, 3) != 0 ||
        dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_WBOIT) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        return NULL;
    }
    return visual;
}



/**
 * Add one unlit primitive quad to a panel.
 *
 * @param scene scene owner
 * @param panel destination panel
 * @param xmin minimum X coordinate
 * @param xmax maximum X coordinate
 * @param ymin minimum Y coordinate
 * @param ymax maximum Y coordinate
 * @param z scene depth coordinate
 * @param color per-vertex color
 * @param alpha_mode alpha mode for the visual
 * @param depth_test whether the visual should depth-test
 * @return created visual, or NULL on failure
 */
static DvzVisual* _app_primitive_add_quad(
    DvzScene* scene, DvzPanel* panel, float xmin, float xmax, float ymin, float ymax, float z,
    DvzColor color, DvzAlphaMode alpha_mode, bool depth_test)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        return NULL;

    float positions[6][3] = {
        {xmin, ymin, z},
        {xmax, ymin, z},
        {xmin, ymax, z},
        {xmax, ymin, z},
        {xmax, ymax, z},
        {xmin, ymax, z},
    };
    DvzColor colors[6] = {0};
    for (uint32_t i = 0; i < 6; i++)
        dvz_memcpy(colors[i], sizeof(DvzColor), color, sizeof(DvzColor));

    if (dvz_visual_set_data(visual, "position", positions, 6) != 0 ||
        dvz_visual_set_data(visual, "color", colors, 6) != 0 ||
        dvz_visual_set_alpha_mode(visual, alpha_mode) != 0 ||
        dvz_visual_set_depth_test(visual, depth_test) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        return NULL;
    }
    return visual;
}



/**
 * Render one source-over scene-occlusion case and capture the center pixel.
 *
 * @param scene_occlusion_enabled whether scene occlusion should be enabled on the panel
 * @param occluder_hidden whether the occluder visual is hidden
 * @param occluder_alpha alpha channel used by the visible occluder
 * @return captured RGB values, or skipped=true when no app context is available
 */
static AppSceneOcclusionCapture _app_source_over_scene_occlusion_capture_center(
    bool scene_occlusion_enabled, bool occluder_hidden, uint8_t occluder_alpha)
{
    AppSceneOcclusionCapture out = {0};
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return out;
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzColor red = {240, 20, 20, 220};
    DvzColor green = {20, 220, 40, occluder_alpha};
    DvzVisual* occluded = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.6f, red, DVZ_ALPHA_BLENDED, true);
    DvzVisual* occluder = _app_primitive_add_quad(
        scene, panel, -0.45f, 0.45f, -0.45f, 0.45f, 0.2f, green, DVZ_ALPHA_BLENDED, true);
    if (occluded == NULL || occluder == NULL ||
        dvz_visual_set_scene_occluded(occluded, true) != 0 ||
        dvz_visual_set_scene_occluder(occluder, true) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    if (scene_occlusion_enabled &&
        dvz_panel_set_scene_occlusion(
            panel, &(DvzSceneOcclusionDesc){
                       .enabled = true,
                       .depth_bias = 0.0f,
                       .soft_edge = 0.001f,
                       .hidden_alpha = 0.05f,
                   }) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    if (occluder_hidden)
        dvz_visual_set_visible(occluder, false);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    if (canvas == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        if (dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) != 0)
        {
            out.skipped = true;
            out.skip_reason = "app offscreen capture unavailable";
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return out;
        }
    }
    if (rgba != NULL && width == 64 && height == 64)
    {
        const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
        out.rgb[0] = center[0];
        out.rgb[1] = center[1];
        out.rgb[2] = center[2];
    }

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return out;
}



/**
 * Render two WBOIT layers and capture the center pixel.
 *
 * @param reverse_order whether the blue layer is added before the red layer
 * @return captured RGB values, or skipped=true when no app context is available
 */
static AppWboitCapture _app_wboit_capture_center(bool reverse_order)
{
    AppWboitCapture out = {0};
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return out;
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzColor red = {255, 0, 0, 128};
    DvzColor blue = {0, 0, 255, 128};
    DvzVisual* first =
        reverse_order ? _app_wboit_add_layer(scene, panel, blue) :
                        _app_wboit_add_layer(scene, panel, red);
    DvzVisual* second =
        reverse_order ? _app_wboit_add_layer(scene, panel, red) :
                        _app_wboit_add_layer(scene, panel, blue);
    if (first == NULL || second == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    if (canvas == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        if (dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) != 0)
        {
            out.skipped = true;
            out.skip_reason = "app offscreen capture unavailable";
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return out;
        }
    }
    if (rgba != NULL && width == 64 && height == 64)
    {
        const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
        out.rgb[0] = center[0];
        out.rgb[1] = center[1];
        out.rgb[2] = center[2];
    }
    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return out;
}



/**
 * Sum the RGB luminance of a captured image.
 *
 * @param rgba captured RGBA8 buffer
 * @param pixel_count number of pixels
 * @return RGB luminance sum
 */
static uint64_t _app_rgb_sum(const uint8_t* rgba, uint32_t pixel_count)
{
    ANN(rgba);
    uint64_t sum = 0;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        const uint8_t* px = &rgba[4 * i];
        sum += (uint64_t)px[0] + (uint64_t)px[1] + (uint64_t)px[2];
    }
    return sum;
}


/**
 * Sum RGB luminance over one captured image region.
 *
 * @param rgba captured RGBA8 buffer
 * @param width captured image width
 * @param height captured image height
 * @param x0 inclusive region x origin
 * @param y0 inclusive region y origin
 * @param x1 exclusive region x end
 * @param y1 exclusive region y end
 * @return RGB luminance sum for the region
 */
static uint64_t _app_rgb_region_sum(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t x0, uint32_t y0, uint32_t x1,
    uint32_t y1)
{
    ANN(rgba);
    if (x0 >= x1 || y0 >= y1 || x1 > width || y1 > height)
        return 0;

    uint64_t sum = 0;
    for (uint32_t y = y0; y < y1; y++)
    {
        for (uint32_t x = x0; x < x1; x++)
        {
            const uint8_t* px = _pixel_at(rgba, width, height, x, y);
            sum += (uint64_t)px[0] + (uint64_t)px[1] + (uint64_t)px[2];
        }
    }
    return sum;
}


/**
 * Sum one RGB channel over one captured image region.
 *
 * @param rgba captured RGBA8 buffer
 * @param width captured image width
 * @param height captured image height
 * @param x0 inclusive region x origin
 * @param y0 inclusive region y origin
 * @param x1 exclusive region x end
 * @param y1 exclusive region y end
 * @param channel RGB channel index
 * @return channel sum for the region
 */
static uint64_t _app_rgb_region_channel_sum(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t x0, uint32_t y0, uint32_t x1,
    uint32_t y1, uint32_t channel)
{
    ANN(rgba);
    if (channel >= 3 || x0 >= x1 || y0 >= y1 || x1 > width || y1 > height)
        return 0;

    uint64_t sum = 0;
    for (uint32_t y = y0; y < y1; y++)
    {
        for (uint32_t x = x0; x < x1; x++)
        {
            const uint8_t* px = _pixel_at(rgba, width, height, x, y);
            sum += (uint64_t)px[channel];
        }
    }
    return sum;
}


/**
 * Return a visual attribute index by name.
 *
 * @param visual the visual
 * @param name the attribute name
 * @return the attribute index, or -1 if absent
 */
static int _app_visual_attr_index(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}



/**
 * Convert glyph visual NDC bounds to a conservative pixel rectangle.
 *
 * @param visual the glyph visual
 * @param width capture width
 * @param height capture height
 * @param out_rect output x0, y0, x1, y1
 * @return whether bounds were resolved
 */
static bool _app_glyph_pixel_bounds(
    const DvzVisual* visual, uint32_t width, uint32_t height, uint32_t out_rect[4])
{
    ANN(visual);
    ANN(out_rect);
    int pos_idx = _app_visual_attr_index(visual, "position");
    if (pos_idx < 0 || visual->attrs[pos_idx].data == NULL || visual->attrs[pos_idx].item_count == 0)
        return false;

    const float(*positions)[3] = (const float(*)[3])visual->attrs[pos_idx].data;
    float min_x = +INFINITY;
    float min_y = +INFINITY;
    float max_x = -INFINITY;
    float max_y = -INFINITY;
    for (uint64_t i = 0; i < visual->attrs[pos_idx].item_count; i++)
    {
        float px = (positions[i][0] * 0.5f + 0.5f) * (float)width;
        float py = (1.0f - (positions[i][1] * 0.5f + 0.5f)) * (float)height;
        if (px < min_x)
            min_x = px;
        if (px > max_x)
            max_x = px;
        if (py < min_y)
            min_y = py;
        if (py > max_y)
            max_y = py;
    }
    if (!isfinite(min_x) || !isfinite(min_y) || !isfinite(max_x) || !isfinite(max_y))
        return false;

    int x0 = (int)floorf(min_x);
    int y0 = (int)floorf(min_y);
    int x1 = (int)ceilf(max_x);
    int y1 = (int)ceilf(max_y);
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > (int)width)
        x1 = (int)width;
    if (y1 > (int)height)
        y1 = (int)height;
    if (x0 >= x1 || y0 >= y1)
        return false;
    out_rect[0] = (uint32_t)x0;
    out_rect[1] = (uint32_t)y0;
    out_rect[2] = (uint32_t)x1;
    out_rect[3] = (uint32_t)y1;
    return true;
}


/**
 * Return whether a captured pixel belongs to the green text foreground.
 *
 * @param pixel captured RGBA8 pixel
 * @return whether the pixel is a bright green foreground sample
 */
static bool _app_text_green_pixel(const uint8_t* pixel)
{
    ANN(pixel);
    return pixel[1] > 140 && pixel[0] < 100 && pixel[2] < 100;
}



/**
 * Render the deterministic EDL point fixture and return its captured RGBA pixels.
 *
 * @param enabled whether EDL should be enabled for the panel
 * @return captured RGBA buffer, or skipped=true when no app context is available
 */
static AppRgbaCapture _app_edl_point_capture(bool enabled)
{
    AppRgbaCapture out = {0};
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return out;
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    float positions[6][3] = {
        {-0.24f, -0.18f, -0.30f},
        {+0.18f, -0.12f, +0.10f},
        {-0.05f, +0.18f, +0.36f},
        {+0.23f, +0.22f, -0.38f},
        {-0.30f, +0.16f, +0.08f},
        {+0.04f, -0.30f, +0.42f},
    };
    DvzColor colors[6] = {
        {255, 90, 80, 255},  {80, 220, 130, 255}, {80, 140, 255, 255},
        {240, 220, 80, 255}, {220, 80, 230, 255}, {80, 230, 230, 255},
    };
    float sizes[6] = {30.0f, 32.0f, 28.0f, 26.0f, 30.0f, 28.0f};
    if (dvz_visual_set_data(visual, "position", positions, 6) != 0 ||
        dvz_visual_set_data(visual, "color", colors, 6) != 0 ||
        dvz_visual_set_data(visual, "size", sizes, 6) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);
    if (enabled &&
        !dvz_panel_set_edl(
            panel, &(DvzEdlDesc){.radius = 2.0f, .strength = 90.0f, .depth_scale = 1.0f}))
    {
        dvz_scene_destroy(scene);
        return out;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    if (canvas == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }

    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (out.rgba != NULL)
            dvz_free(out.rgba);
        out.rgba = NULL;
        out.width = 0;
        out.height = 0;
        if (dvz_canvas_capture_rgba(canvas, &out.width, &out.height, &out.rgba) != 0)
        {
            out.skipped = true;
            out.skip_reason = "app offscreen capture unavailable";
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return out;
        }
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return out;
}


/**
 * Render a few app frames and return the last captured RGB luminance sum.
 *
 * @param app the offscreen app
 * @param canvas the app-window canvas
 * @param frame_count number of frames to render before returning
 * @param out_width captured width
 * @param out_height captured height
 * @param out_sum RGB luminance sum for the final frame
 * @return whether the capture succeeded
 */
static bool _app_capture_rgb_sum(
    DvzApp* app, DvzCanvas* canvas, uint32_t frame_count, uint32_t* out_width,
    uint32_t* out_height, uint64_t* out_sum)
{
    ANN(app);
    ANN(canvas);
    ANN(out_width);
    ANN(out_height);
    ANN(out_sum);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < frame_count; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        width = 0;
        height = 0;
        if (dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) != 0)
            return false;
        ANN(rgba);
    }

    *out_width = width;
    *out_height = height;
    *out_sum = _app_rgb_sum(rgba, width * height);
    dvz_free(rgba);
    return true;
}


#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
/**
 * Return an external-surface description for a GLFW-hosted app test.
 *
 * @param instance Vulkan instance used to create the surface
 * @param surface borrowed Vulkan surface handle
 * @param width framebuffer width in pixels
 * @param height framebuffer height in pixels
 * @return external surface description
 */
static DvzWindowExternalSurfaceInfo _app_glfw_surface_info(
    VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height)
{
    DvzWindowExternalSurfaceInfo info = {0};
    info.instance = instance;
    info.surface = surface;
    info.extent.width = width;
    info.extent.height = height;
    info.scale_x = 1.0f;
    info.scale_y = 1.0f;
    info.owned_by_datoviz = false;
    return info;
}



#endif



/**
 * Create a minimal scene/figure/panel used by app timer integration tests.
 *
 * @param out_figure destination for the created figure handle
 * @return scene handle, or NULL on failure
 */
static DvzScene* _app_timer_test_scene(DvzFigure** out_figure)
{
    ANN(out_figure);
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return NULL;

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return NULL;
    }
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
    {
        dvz_scene_destroy(scene);
        return NULL;
    }
    (void)panel;

    *out_figure = figure;
    return scene;
}



int test_app_offscreen(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    AT(dvz_app_vk_instance(app) != VK_NULL_HANDLE);
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    dvz_app_window_request_frame(win);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(dvz_app_window_emit_resize(win, 64, 64, 64, 64, 1.0f, 1.0f) == 0);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);

    /* Exercise host-driven and Datoviz-owned frame paths. */
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_render_once(app) == 0);
    dvz_app_run(app, 1);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_timer_advances_in_app_run(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 4.0);
    AppTimerProbe probe = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _app_timer_probe_callback, &probe);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_timer_advances_in_app_run skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 2);

    AT(probe.calls == 2);
    AC(probe.last_t, 0.25, EPS);
    AC(probe.last_dt, 0.25, EPS);
    AC(probe.total_dt, 0.25, EPS);
    AC(dvz_scene_clock_time(scene), 0.25, EPS);
    AC(dvz_scene_clock_dt(scene), 0.25, EPS);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_timer_advances_in_render_once(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 8.0);
    AppTimerProbe probe = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _app_timer_probe_callback, &probe);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_timer_advances_in_render_once skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);

    AT(probe.calls == 2);
    AC(probe.last_t, 0.125, EPS);
    AC(probe.last_dt, 0.125, EPS);
    AC(probe.total_dt, 0.125, EPS);
    AC(dvz_scene_clock_time(scene), 0.125, EPS);
    AC(dvz_scene_clock_dt(scene), 0.125, EPS);

    dvz_anim_stop(timer);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 2);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_render_enabled_gate(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 8.0);
    AppTimerProbe timer_probe = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _app_timer_probe_callback, &timer_probe);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_render_enabled_gate skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    AT(dvz_app_window_render_enabled(win));

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);

    dvz_app_window_set_render_enabled(win, false);
    AT(!dvz_app_window_render_enabled(win));
    AT(dvz_app_window_render_once(win) == 0);
    AT(request_probe.calls == 0);
    AT(timer_probe.calls == 0);
    AC(dvz_scene_clock_time(scene), 0.0, EPS);

    dvz_app_window_set_render_enabled(win, true);
    AT(dvz_app_window_render_enabled(win));
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(timer_probe.calls == 1);
    AC(dvz_scene_clock_time(scene), 0.0, EPS);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
int test_app_external_surface_release_waits(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    if (!dvz_window_glfw_init())
    {
        log_warn("test_app_external_surface_release_waits skipped: GLFW could not initialize");
        tst_skip(suite, "GLFW could not initialize");
        return 0;
    }

    uint32_t extension_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    if (extension_count == 0 || extensions == NULL)
    {
        log_warn(
            "test_app_external_surface_release_waits skipped: GLFW returned no Vulkan extensions");
        tst_skip(suite, "GLFW returned no Vulkan extensions");
        return 0;
    }

    DvzFigure* figure = NULL;
    DvzScene* scene = _app_timer_test_scene(&figure);
    AT(scene != NULL);
    ANN(figure);

    DvzAppConfig app_cfg = dvz_app_config();
    app_cfg.instance_extension_count = extension_count;
    app_cfg.instance_extensions = extensions;
    app_cfg.enable_canvas_extensions = true;
    app_cfg.enable_glfw_extensions = false;
    DvzApp* app = dvz_app_with_config(scene, &app_cfg);
    if (app == NULL)
    {
        log_warn("test_app_external_surface_release_waits skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    VkInstance instance = dvz_app_vk_instance(app);
    AT(instance != VK_NULL_HANDLE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* glfw_window =
        glfwCreateWindow(64, 64, "test_app_external_surface_release_waits", NULL, NULL);
    if (glfw_window == NULL)
    {
        log_warn("test_app_external_surface_release_waits skipped: GLFW window creation failed");
        tst_skip(suite, "GLFW window creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult surface_res = glfwCreateWindowSurface(instance, glfw_window, NULL, &surface);
    if (surface_res != VK_SUCCESS || surface == VK_NULL_HANDLE)
    {
        log_warn(
            "test_app_external_surface_release_waits skipped: surface creation failed (%d)",
            (int)surface_res);
        tst_skip(suite, "Vulkan surface creation failed");
        glfwDestroyWindow(glfw_window);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzWindowExternalSurfaceInfo surface_info =
        _app_glfw_surface_info(instance, surface, 64, 64);
    DvzAppWindow* win = dvz_app_window_external_surface(app, figure, &surface_info);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_app_window_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);

    AT_EXPECTED_LOG_STRICT(
        suite, LOG_WARN,
        dvz_app_window_release_external_surface(win) == DVZ_CANVAS_FRAME_WAIT_SURFACE);
    AT_EXPECTED_LOG_STRICT(
        suite, LOG_WARN, dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_WAIT_SURFACE);
    AT_EXPECTED_LOG_STRICT(suite, LOG_WARN, dvz_app_render_once(app) == DVZ_CANVAS_FRAME_WAIT_SURFACE);

    vkDestroySurfaceKHR(instance, surface, NULL);
    glfwDestroyWindow(glfw_window);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif


int test_app_offscreen_panel_three_visuals_all_drawn(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


/**
 * Ensure overlapping point visuals use depth testing in a normal non-EDL pass.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_point_depth_orders_overlap(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* near_visual = dvz_point(scene, 0);
    DvzVisual* far_visual = dvz_point(scene, 0);
    AT(near_visual != NULL);
    AT(far_visual != NULL);

    float near_pos[3] = {0.0f, 0.0f, 0.1f};
    float far_pos[3] = {0.0f, 0.0f, 0.8f};
    DvzColor near_color = {32, 64, 255, 255};
    DvzColor far_color = {255, 32, 32, 255};
    float size = 36.0f;

    AT(dvz_visual_set_data(near_visual, "position", near_pos, 1) == 0);
    AT(dvz_visual_set_data(near_visual, "color", &near_color, 1) == 0);
    AT(dvz_visual_set_data(near_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, near_visual, NULL) == 0);

    AT(dvz_visual_set_data(far_visual, "position", far_pos, 1) == 0);
    AT(dvz_visual_set_data(far_visual, "color", &far_color, 1) == 0);
    AT(dvz_visual_set_data(far_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, far_visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_depth_orders_overlap skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
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
 * Ensure point depth cueing darkens farther points in an offscreen render.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_point_depth_cue_darkens_far(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[2][3] = {{-0.45f, 0.0f, 0.0f}, {0.45f, 0.0f, 0.8f}};
    DvzColor colors[2] = {{255, 64, 64, 255}, {255, 64, 64, 255}};
    float sizes[2] = {20.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_visual_set_depth_cue(
           visual,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
               .falloff = DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL,
               .near_depth = 0.50f,
               .far_depth = 1.0f,
               .strength = 1.0f,
               .density = 3.0f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_depth_cue_darkens_far skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* near_px = _pixel_at(rgba, width, height, width / 4, height / 2);
    const uint8_t* far_px = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
    AT(near_px[0] > 180);
    AT(far_px[0] + 80 < near_px[0]);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_has_nonblank_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


/**
 * Ensure pixel visuals render nonblank square marks through the offscreen app path.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_app_offscreen_pixel_square_has_nonblank_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_pixel(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 0, 255, 255};
    float size = 18.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_pixel_square_has_nonblank_pixels skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
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

    uint32_t magenta_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[2] > 200)
            magenta_count++;
    }
    AT(magenta_count > 0);

    const uint8_t* corner = _pixel_at(rgba, width, height, 40, 40);
    AT(corner[0] > 200 && corner[2] > 200);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure panel EDL renders an offscreen point scene through the app runtime path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_points_edl_renders(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);
    float positions[4][3] = {
        {-0.25f, -0.15f, -0.20f},
        {+0.20f, -0.05f, +0.15f},
        {-0.05f, +0.20f, +0.35f},
        {+0.18f, +0.18f, -0.35f},
    };
    DvzColor colors[4] = {
        {255, 90, 80, 255},
        {80, 220, 130, 255},
        {80, 140, 255, 255},
        {240, 220, 80, 255},
    };
    float sizes[4] = {28.0f, 30.0f, 26.0f, 24.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);
    AT(dvz_panel_set_edl(
        panel, &(DvzEdlDesc){.radius = 2.0f, .strength = 65.0f, .depth_scale = 1.0f}));

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_points_edl_renders skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);
    AT(dvz_panel_set_edl(panel, NULL));
    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t lit_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* px = &rgba[4 * i];
        if (px[0] > 40 || px[1] > 40 || px[2] > 40)
            lit_count++;
    }
    AT(lit_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure EDL changes captured point pixels while it remains enabled.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_points_edl_changes_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppRgbaCapture disabled = _app_edl_point_capture(false);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_points_edl_changes_pixels skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppRgbaCapture enabled = _app_edl_point_capture(true);
    if (enabled.skipped)
    {
        log_warn("test_app_offscreen_points_edl_changes_pixels skipped: GPU context failed");
        dvz_free(disabled.rgba);
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }
    ANN(disabled.rgba);
    ANN(enabled.rgba);
    AT(disabled.width == 64);
    AT(disabled.height == 64);
    AT(enabled.width == disabled.width);
    AT(enabled.height == disabled.height);

    uint32_t changed_count = 0;
    uint32_t darkened_count = 0;
    const uint32_t pixel_count = disabled.width * disabled.height;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        const uint8_t* a = &disabled.rgba[4 * i];
        const uint8_t* b = &enabled.rgba[4 * i];
        const int lum0 = (int)a[0] + (int)a[1] + (int)a[2];
        const int lum1 = (int)b[0] + (int)b[1] + (int)b[2];
        if (abs(lum0 - lum1) > 8)
            changed_count++;
        if (lum0 > 80 && lum1 + 12 < lum0)
            darkened_count++;
    }
    AT(changed_count > 16);
    AT(darkened_count > 8);
    AT(_app_rgb_sum(enabled.rgba, pixel_count) < _app_rgb_sum(disabled.rgba, pixel_count));

    dvz_free(enabled.rgba);
    dvz_free(disabled.rgba);
    return 0;
}


/**
 * Ensure SSAO visibly darkens an offscreen mesh scene versus the same scene without SSAO.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_mesh_ssao_changes_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzColor back_color = {188, 196, 205, 255};
    DvzColor front_color = {224, 150, 92, 255};
    AppSsaoQuad back =
        _app_ssao_add_quad(scene, panel, -0.82f, +0.82f, -0.72f, +0.72f, 0.65f, back_color);
    AppSsaoQuad front =
        _app_ssao_add_quad(scene, panel, -0.22f, +0.52f, -0.28f, +0.46f, 0.25f, front_color);
    AT(back.visual != NULL);
    AT(front.visual != NULL);
    dvz_panel_set_background_color(panel, 0.03f, 0.035f, 0.045f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_mesh_ssao_changes_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);
    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 96);
    AT(height0 == 96);

    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 3.0f, .strength = 8.0f, .bias = 0.0f,
                            .sample_count = 16}));
    dvz_app_run(app, 1);
    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);
    AT(width1 == width0);
    AT(height1 == height0);

    uint32_t darkened_count = 0;
    uint32_t changed_count = 0;
    const uint32_t pixel_count = width0 * height0;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        const uint8_t* a = &rgba0[4 * i];
        const uint8_t* b = &rgba1[4 * i];
        int lum0 = (int)a[0] + (int)a[1] + (int)a[2];
        int lum1 = (int)b[0] + (int)b[1] + (int)b[2];
        if (lum0 != lum1)
            changed_count++;
        if (lum0 > 80 && lum1 + 24 < lum0)
            darkened_count++;
    }
    AT(changed_count > 0);
    AT(darkened_count > 8);
    AT(_app_rgb_sum(rgba1, pixel_count) < _app_rgb_sum(rgba0, pixel_count));

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure SSAO darkens an offscreen sphere and mesh contact scene.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_sphere_ssao_darkens_contact(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzColor back_color = {184, 192, 202, 255};
    AppSsaoQuad back =
        _app_ssao_add_quad(scene, panel, -0.86f, +0.86f, -0.74f, +0.70f, 0.70f, back_color);
    AT(back.visual != NULL);

    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);
    float positions[4][3] = {
        {-0.22f, -0.08f, 0.20f},
        {+0.10f, -0.06f, 0.24f},
        {-0.05f, +0.16f, 0.28f},
        {+0.30f, +0.10f, 0.22f},
    };
    DvzColor colors[4] = {
        {220, 95, 80, 255},
        {80, 190, 125, 255},
        {85, 130, 225, 255},
        {230, 190, 75, 255},
    };
    float radii[4] = {0.28f, 0.27f, 0.25f, 0.23f};
    AT(dvz_visual_set_data(sphere, "position", &positions[0][0], 4) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radii, 4) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.03f, 0.035f, 0.045f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_sphere_ssao_darkens_contact skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);
    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 96);
    AT(height0 == 96);

    AT(dvz_panel_set_ssao(
        panel, &(DvzSsaoDesc){.radius = 3.0f, .strength = 8.0f, .bias = 0.0f,
                              .sample_count = 16}));
    dvz_app_run(app, 1);
    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);
    AT(width1 == width0);
    AT(height1 == height0);

    uint32_t darkened_count = 0;
    uint32_t changed_count = 0;
    const uint32_t pixel_count = width0 * height0;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        const uint8_t* a = &rgba0[4 * i];
        const uint8_t* b = &rgba1[4 * i];
        int lum0 = (int)a[0] + (int)a[1] + (int)a[2];
        int lum1 = (int)b[0] + (int)b[1] + (int)b[2];
        if (lum0 != lum1)
            changed_count++;
        if (lum0 > 80 && lum1 + 24 < lum0)
            darkened_count++;
    }
    AT(changed_count > 0);
    AT(darkened_count > 8);
    AT(_app_rgb_sum(rgba1, pixel_count) < _app_rgb_sum(rgba0, pixel_count));

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_records_dvzr_frames(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 24.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_records_dvzr_frames skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    const char* path = "/tmp/dvz_app_offscreen_recording.dvzr";
    const char* old_record_fps = getenv("DVZ_DRP2_RECORD_FPS");
    bool had_record_fps = old_record_fps != NULL;
    char saved_record_fps[64] = {0};
    if (had_record_fps)
        dvz_strlcpy(saved_record_fps, old_record_fps, sizeof(saved_record_fps));
    AT(setenv("DVZ_DRP2_RECORD_FPS", "0", 1) == 0);
    int record_start = dvz_app_window_record_start(win, path);
    if (had_record_fps)
        (void)setenv("DVZ_DRP2_RECORD_FPS", saved_record_fps, 1);
    else
        (void)unsetenv("DVZ_DRP2_RECORD_FPS");
    AT(record_start == 0);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_window_record_stop(win) == 0);

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    ANN(recording);
    AT(dvz_drp2_recording_frame_count(recording) == 3);
    AT(dvz_drp2_recording_raw_fallback_count(recording) == 0);
    const DvzDrp2CommandStream* stream = dvz_drp2_recording_stream(recording);
    ANN(stream);
    AT(dvz_drp2_stream_count(stream) > 0);

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_recording_execute_all(recording, runtime);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_recording_close(recording);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_has_nonblank_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


/**
 * Ensure batched text renders visible pixels through the offscreen app path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_text_has_nonblank_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    AT(panel != NULL);

    DvzVisual* text = dvz_text(scene, 0);
    AT(text != NULL);
    const char* strings[1] = {"HI"};
    float positions[1][3] = {{8.0f, 8.0f, 0.0f}};
    float text_anchors[1][2] = {{0.0f, 0.0f}};
    float sizes[1] = {16.0f};
    float angles[1] = {0.0f};
    DvzColor colors[1] = {{0, 255, 0, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_text_has_nonblank_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t green_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &rgba[4 * i];
        if (pixel[1] > 120 && pixel[0] < 80 && pixel[2] < 80)
            green_count++;
    }
    AT(green_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}

/**
 * Ensure SDF-backed text renders visible pixels through the offscreen app path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_sdf_text_has_nonblank_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 256, 72, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    AT(panel != NULL);

    DvzVisual* text = dvz_text(scene, 0);
    AT(text != NULL);
    AT(dvz_text_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"UTF-8 fallback: A?B cafe? -> ?"};
    float positions[1][3] = {{6.0f, 12.0f, 0.0f}};
    float text_anchors[1][2] = {{0.0f, 0.0f}};
    float sizes[1] = {24.0f};
    float angles[1] = {0.0f};
    DvzColor colors[1] = {{0, 255, 0, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){.z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_sdf_text_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 256, 72);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 256);
    AT(height == 72);

    uint32_t green_count = 0;
    uint32_t green_in_bounds = 0;
    uint32_t isolated_green = 0;
    uint32_t bounds[4] = {0};
    bool has_bounds =
        text->text.glyph_visual != NULL &&
        _app_glyph_pixel_bounds(text->text.glyph_visual, width, height, bounds);
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &rgba[4 * i];
        if (_app_text_green_pixel(pixel))
            green_count++;
    }
    AT(green_count > 0);
    if (has_bounds)
    {
        for (uint32_t y = bounds[1]; y < bounds[3]; y++)
        {
            for (uint32_t x = bounds[0]; x < bounds[2]; x++)
            {
                const uint8_t* pixel = _pixel_at(rgba, width, height, x, y);
                if (!_app_text_green_pixel(pixel))
                    continue;
                green_in_bounds++;
                if (x == bounds[0] || y == bounds[1] || x + 1u >= bounds[2] || y + 1u >= bounds[3])
                    continue;
                bool connected = false;
                for (int32_t dy = -1; dy <= 1 && !connected; dy++)
                {
                    for (int32_t dx = -1; dx <= 1; dx++)
                    {
                        if (dx == 0 && dy == 0)
                            continue;
                        const uint8_t* neighbor = _pixel_at(
                            rgba, width, height, (uint32_t)((int32_t)x + dx),
                            (uint32_t)((int32_t)y + dy));
                        connected = connected || _app_text_green_pixel(neighbor);
                    }
                }
                if (!connected)
                    isolated_green++;
            }
        }
        uint32_t bounds_pixels = (bounds[2] - bounds[0]) * (bounds[3] - bounds[1]);
        AT(bounds_pixels > 0);
        AT(green_in_bounds * 10u < bounds_pixels * 7u);
        AT(green_in_bounds > 32u);
        AT(isolated_green * 10u < green_in_bounds);
    }

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_offscreen_image_field_partial_update_changes_region(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


/**
 * Ensure WBOIT transparent layers are stable when visual order changes.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_wboit_mesh_order_independent_layers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppWboitCapture forward = _app_wboit_capture_center(false);
    if (forward.skipped)
    {
        log_warn(
            "test_app_offscreen_wboit_mesh_order_independent_layers skipped: GPU context failed");
        tst_skip(suite, forward.skip_reason);
        return 0;
    }
    AppWboitCapture reverse = _app_wboit_capture_center(true);
    if (reverse.skipped)
    {
        log_warn(
            "test_app_offscreen_wboit_mesh_order_independent_layers skipped: GPU context failed");
        tst_skip(suite, reverse.skip_reason);
        return 0;
    }

    uint32_t diff = 0;
    for (uint32_t i = 0; i < 3; i++)
        diff += forward.rgb[i] > reverse.rgb[i] ? forward.rgb[i] - reverse.rgb[i] :
                                                  reverse.rgb[i] - forward.rgb[i];
    AT(diff <= 6);
    AT(forward.rgb[0] > 20 || forward.rgb[2] > 20);
    AT(reverse.rgb[0] > 20 || reverse.rgb[2] > 20);
    return 0;
}


/**
 * Ensure source-over transparency blends with background but respects opaque depth.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_source_over_mesh_depth_and_blend(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzColor background = {20, 30, 180, 255};
    DvzColor transparent = {240, 20, 20, 128};
    DvzColor occluder = {20, 220, 40, 255};
    DvzVisual* background_visual = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.8f, background, DVZ_ALPHA_OPAQUE, true);
    DvzVisual* transparent_visual = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.4f, transparent, DVZ_ALPHA_BLENDED,
        true);
    DvzVisual* occluder_visual = _app_primitive_add_quad(
        scene, panel, 0.15f, 0.75f, -0.45f, 0.45f, 0.1f, occluder, DVZ_ALPHA_OPAQUE, true);
    ANN(background_visual);
    ANN(transparent_visual);
    ANN(occluder_visual);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_source_over_mesh_depth_and_blend skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    for (uint32_t frame = 0; frame < 3; frame++)
        dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* blended = _pixel_at(rgba, width, height, width / 4, height / 2);
    const uint8_t* occluded = _pixel_at(rgba, width, height, (5 * width) / 8, height / 2);
    AT(blended[0] > background[0] + 60);
    AT(blended[2] > 70);
    AT(blended[1] < 80);
    AT(occluded[1] > 160);
    AT(occluded[1] > occluded[0] + 80);
    AT(occluded[1] > occluded[2] + 80);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure depth peeling renders two transparent layers and preserves opaque occlusion.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_depth_peel_mesh_two_layers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzColor background = {12, 12, 16, 255};
    DvzColor red = {255, 20, 20, 160};
    DvzColor blue = {20, 20, 255, 160};
    DvzColor occluder = {20, 220, 40, 255};
    DvzVisual* background_visual = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.9f, background, DVZ_ALPHA_OPAQUE, true);
    DvzVisual* red_visual = _app_primitive_add_quad(
        scene, panel, -0.95f, -0.10f, -0.95f, 0.95f, 0.5f, red, DVZ_ALPHA_DEPTH_PEEL,
        true);
    DvzVisual* blue_visual = _app_primitive_add_quad(
        scene, panel, -0.05f, 0.95f, -0.95f, 0.95f, 0.3f, blue, DVZ_ALPHA_DEPTH_PEEL,
        true);
    DvzVisual* occluder_visual = _app_primitive_add_quad(
        scene, panel, 0.15f, 0.75f, -0.45f, 0.45f, 0.1f, occluder, DVZ_ALPHA_OPAQUE, true);
    ANN(background_visual);
    ANN(red_visual);
    ANN(blue_visual);
    ANN(occluder_visual);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_depth_peel_mesh_two_layers skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    for (uint32_t frame = 0; frame < 3; frame++)
        dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint32_t region_pixels = 8 * 8;
    uint64_t red_region_r =
        _app_rgb_region_channel_sum(rgba, width, height, 12, 28, 20, 36, 0);
    uint64_t red_region_b =
        _app_rgb_region_channel_sum(rgba, width, height, 12, 28, 20, 36, 2);
    uint64_t blue_region_r =
        _app_rgb_region_channel_sum(rgba, width, height, 28, 28, 36, 36, 0);
    uint64_t blue_region_b =
        _app_rgb_region_channel_sum(rgba, width, height, 28, 28, 36, 36, 2);
    uint64_t occluded_region_r =
        _app_rgb_region_channel_sum(rgba, width, height, 40, 28, 48, 36, 0);
    uint64_t occluded_region_g =
        _app_rgb_region_channel_sum(rgba, width, height, 40, 28, 48, 36, 1);
    uint64_t occluded_region_b =
        _app_rgb_region_channel_sum(rgba, width, height, 40, 28, 48, 36, 2);

    AT(red_region_r > ((uint64_t)background[0] + 45) * region_pixels);
    AT(red_region_r > red_region_b + 30 * region_pixels);
    AT(blue_region_b > ((uint64_t)background[2] + 45) * region_pixels);
    AT(blue_region_b > blue_region_r + 30 * region_pixels);
    AT(occluded_region_g > 160 * region_pixels);
    AT(occluded_region_g > occluded_region_r + 80 * region_pixels);
    AT(occluded_region_g > occluded_region_b + 80 * region_pixels);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure hidden and zero-alpha scene occluders do not attenuate sampled visuals.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_scene_occlusion_hidden_alpha(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppSceneOcclusionCapture hidden =
        _app_source_over_scene_occlusion_capture_center(true, true, 255);
    if (hidden.skipped)
    {
        log_warn("test_app_offscreen_scene_occlusion_hidden_alpha skipped: GPU context failed");
        tst_skip(suite, hidden.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture zero =
        _app_source_over_scene_occlusion_capture_center(true, false, 0);
    if (zero.skipped)
    {
        log_warn("test_app_offscreen_scene_occlusion_hidden_alpha skipped: GPU context failed");
        tst_skip(suite, zero.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture positive =
        _app_source_over_scene_occlusion_capture_center(true, false, 64);
    if (positive.skipped)
    {
        log_warn("test_app_offscreen_scene_occlusion_hidden_alpha skipped: GPU context failed");
        tst_skip(suite, positive.skip_reason);
        return 0;
    }

    uint32_t hidden_sum =
        (uint32_t)hidden.rgb[0] + (uint32_t)hidden.rgb[1] + (uint32_t)hidden.rgb[2];
    uint32_t zero_sum = (uint32_t)zero.rgb[0] + (uint32_t)zero.rgb[1] + (uint32_t)zero.rgb[2];
    AT(hidden.rgb[0] > 140);
    AT(zero.rgb[0] > 140);
    AT(hidden_sum > 150);
    AT(zero_sum > 150);
    AT(hidden_sum > zero_sum ? hidden_sum - zero_sum <= 24 : zero_sum - hidden_sum <= 24);
    AT(hidden.rgb[0] > zero.rgb[0] ? hidden.rgb[0] - zero.rgb[0] <= 24
                                    : zero.rgb[0] - hidden.rgb[0] <= 24);
    AT(hidden.rgb[1] > zero.rgb[1] ? hidden.rgb[1] - zero.rgb[1] <= 24
                                    : zero.rgb[1] - hidden.rgb[1] <= 24);
    AT(positive.rgb[1] > zero.rgb[1] + 30);
    AT(positive.rgb[0] + 20 < zero.rgb[0]);
    return 0;
}


/**
 * Ensure source-over scene occlusion respects visible and hidden occluder policy.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_source_over_scene_occlusion_matrix(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppSceneOcclusionCapture disabled =
        _app_source_over_scene_occlusion_capture_center(false, false, 64);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_source_over_scene_occlusion_matrix skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture hidden =
        _app_source_over_scene_occlusion_capture_center(true, true, 255);
    if (hidden.skipped)
    {
        log_warn("test_app_offscreen_source_over_scene_occlusion_matrix skipped: GPU context failed");
        tst_skip(suite, hidden.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture enabled =
        _app_source_over_scene_occlusion_capture_center(true, false, 64);
    if (enabled.skipped)
    {
        log_warn("test_app_offscreen_source_over_scene_occlusion_matrix skipped: GPU context failed");
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }

    AT(disabled.rgb[0] > 120);
    AT(hidden.rgb[0] > 140);
    AT(disabled.rgb[1] > 50);
    AT(hidden.rgb[0] > disabled.rgb[0] + 30);
    AT(hidden.rgb[1] + 25 < disabled.rgb[1]);

    uint32_t diff = 0;
    for (uint32_t i = 0; i < 3; i++)
        diff += disabled.rgb[i] > enabled.rgb[i] ? disabled.rgb[i] - enabled.rgb[i] :
                                                   enabled.rgb[i] - disabled.rgb[i];
    AT(diff <= 6);
    return 0;
}


int test_app_offscreen_lit_primitive_depth_orders_overlap(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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
 * Ensure lit primitive depth cueing darkens farther geometry in an offscreen render.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_lit_primitive_depth_cue_darkens_far(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[12][3] = {
        {-0.9f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f}, {-0.1f, -0.8f, 0.0f},
        {-0.1f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f},
        {0.1f, -0.8f, 0.8f},  {0.1f, 0.8f, 0.8f},  {0.9f, -0.8f, 0.8f},
        {0.9f, -0.8f, 0.8f},  {0.1f, 0.8f, 0.8f},  {0.9f, 0.8f, 0.8f},
    };
    float normals[12][3];
    DvzColor colors[12];
    for (uint32_t i = 0; i < 12; i++)
    {
        normals[i][0] = 0.0f;
        normals[i][1] = 0.0f;
        normals[i][2] = 1.0f;
        colors[i][0] = 255;
        colors[i][1] = 48;
        colors[i][2] = 48;
        colors[i][3] = 255;
    }

    AT(dvz_visual_set_data(visual, "position", positions, 12) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 12) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 12) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);
    AT(dvz_visual_set_depth_cue(
           visual,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.50f,
               .far_depth = 0.95f,
               .strength = 1.0f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_lit_primitive_depth_cue_darkens_far skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
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

    const uint8_t* near_px = _pixel_at(rgba, width, height, width / 4, height / 2);
    const uint8_t* far_px = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
    AT(near_px[0] > 180);
    AT(far_px[0] + 80 < near_px[0]);
    AT(far_px[1] + 20 < near_px[1]);

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


int test_app_offscreen_mesh_renders_nonblank(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_offscreen_rotated_mesh_depth_orders_faces(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_offscreen_camera_arcball_mesh_renders_cube(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_offscreen_shared_field_mixed_runtime_updates(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_offscreen_retained_render_second_frame(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_offscreen_image_retained_render_second_frame(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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



/**
 * Ensure a reused offscreen app/runtime survives repeated resizes with mixed retained visuals.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_resize_reuses_runtime_with_mesh_and_image(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* image = dvz_image(scene, 0);
    AT(mesh != NULL);
    AT(image != NULL);

    float mesh_positions[4][3] = {
        {-0.9f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f},
        {-0.1f, -0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f},
    };
    DvzColor mesh_colors[4] = {
        {64, 255, 64, 255},
        {64, 255, 64, 255},
        {64, 255, 64, 255},
        {64, 255, 64, 255},
    };
    float mesh_normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)));

    AT(dvz_visual_set_data(mesh, "position", mesh_positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", mesh_colors, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_visual_set_primitive_shading(
           mesh,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    float image_positions[4][3] = {
        {0.1f, -0.8f, 0.0f}, {0.1f, 0.8f, 0.0f},
        {0.9f, -0.8f, 0.0f}, {0.9f, 0.8f, 0.0f},
    };
    float image_texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 3] = 255;
    }

    AT(dvz_visual_set_data(image, "position", image_positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", image_texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_resize_reuses_runtime_with_mesh_and_image skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    const uint32_t sizes[][2] = {
        {96, 64},
        {128, 72},
        {80, 96},
        {144, 80},
        {96, 64},
    };

    for (uint32_t frame = 0; frame < sizeof(sizes) / sizeof(sizes[0]); frame++)
    {
        uint32_t expected_width = sizes[frame][0];
        uint32_t expected_height = sizes[frame][1];
        AT(dvz_app_window_resize(win, expected_width, expected_height) == 0);
        AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);

        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == expected_width);
        AT(height == expected_height);

        const uint8_t* mesh_center = _pixel_at(rgba, width, height, width / 4, height / 2);
        AT(mesh_center[1] > 180);
        AT(mesh_center[0] < 140);
        AT(mesh_center[2] < 140);

        const uint8_t* image_center = _pixel_at(rgba, width, height, (3 * width) / 4, height / 2);
        AT(image_center[0] > 200);
        AT(image_center[1] < 80);
        AT(image_center[2] < 80);

        dvz_free(rgba);
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure app-owned request execution stays steady across repeated pick/probe frames.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_pick_probe_request_steady_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
    AT(panel != NULL);

    DvzVisual* points = dvz_point(scene, 0);
    AT(points != NULL);
    dvz_visual_set_pick_capabilities(points, DVZ_PICK_CAPABILITY_ITEM);
    float point_pos[1][3] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_pick_probe_request_steady_state skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    for (uint32_t frame = 0; frame < 8; frame++)
    {
        uint64_t pick_id = 100 + frame;
        uint64_t probe_id = 200 + frame;
        AT(dvz_panel_pick(panel, 32.0, 32.0, &(DvzPickRequest){.request_id = pick_id}) == 0);
        AT(dvz_panel_probe(panel, 32.0, 32.0, &(DvzProbeRequest){.request_id = probe_id}) == 0);
        AT(scene->pending_pick_count == 1);
        AT(scene->pending_probe_count == 1);

        AT(dvz_app_window_render_once(win) == DVZ_CANVAS_FRAME_READY);
        AT(scene->pending_pick_count == 0);
        AT(scene->pending_probe_count == 0);

        DvzPickResult pick = {0};
        DvzProbeResult probe = {0};
        AT(dvz_scene_poll_pick(scene, &pick));
        AT(dvz_scene_poll_probe(scene, &probe));
        AT(pick.hit);
        AT(pick.request_id == pick_id);
        AT(pick.resolved_target == DVZ_SCENE_TARGET_ITEM);
        AT(pick.resolved_id == 0);
        AT(probe.hit);
        AT(probe.request_id == probe_id);
        AT(probe.value_kind == DVZ_PROBE_VALUE_VEC4);
        AT(probe.vector[0] > 0.9);
        AT(probe.vector[1] < 0.1);
        AT(probe.vector[2] < 0.1);
        AT(probe.vector[3] > 0.9);

        AT(!dvz_scene_poll_pick(scene, &pick));
        AT(!dvz_scene_poll_probe(scene, &probe));
        AT(scene->pick_result_count == 0);
        AT(scene->probe_result_count == 0);
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



int test_app_offscreen_two_panel_points_light_both_halves(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_offscreen_clear_color(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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


int test_app_capture_rejects_wrong_dimensions(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_canvas_capture_rgba_into(canvas, 128, 128, buf, sizeof(buf)) != 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_capture_rejects_undersized_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
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
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_canvas_capture_rgba_into(canvas, 64, 64, buf, required - 1) != 0);
    dvz_free(buf);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure a retained volume visual renders a sampled 3D field into an offscreen app frame.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_slice_renders_field(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_slice_renders_field skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t white_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        white_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 220 && px[1] > 220 && px[2] > 220)
                white_count++;
        }
        dvz_free(rgba);
        if (white_count > (width * height) / 2)
            break;
    }
    AT(white_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure MIP volume rendering traverses the 3D field instead of sampling only the middle slice.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_mip_renders_bright_slice(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 4,
               });
    ANN(field);
    const uint8_t voxels[16] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        255, 255, 255, 255,
    };
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_mip_renders_bright_slice skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t white_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        white_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 220 && px[1] > 220 && px[2] > 220)
                white_count++;
        }
        dvz_free(rgba);
        if (white_count > (width * height) / 2)
            break;
    }
    AT(white_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure composite volume rendering accumulates scalar density through the 3D field.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_composite_renders_field(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 64) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_composite_renders_field skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t bright_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        bright_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 120 && px[1] > 120 && px[2] > 120)
                bright_count++;
        }
        dvz_free(rgba);
        if (bright_count > (width * height) / 2)
            break;
    }
    AT(bright_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Render the deterministic volume-occlusion fixture and return region sums.
 *
 * @param mode occlusion mode used by the fixture
 * @param perspective_camera whether to attach a perspective camera to the panel
 * @param clipped_occluder whether to restrict the volume occluder to one screen side
 * @return captured image sums, or skipped=true when no app context is available
 */
static AppVolumeOcclusionCapture _app_volume_occlusion_capture(
    AppVolumeOcclusionMode mode, bool perspective_camera, bool clipped_occluder)
{
    AppVolumeOcclusionCapture out = {0};
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return out;
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    if (perspective_camera)
    {
        DvzCameraDesc camera_desc = dvz_camera_desc();
        camera_desc.eye[0] = 0.5f;
        camera_desc.eye[1] = 0.5f;
        camera_desc.eye[2] = 3.0f;
        camera_desc.target[0] = 0.5f;
        camera_desc.target[1] = 0.5f;
        camera_desc.target[2] = 0.5f;
        camera_desc.fov_y = GLM_PI_4f;
        if (dvz_panel_set_camera(panel, &camera_desc) == NULL)
        {
            dvz_scene_destroy(scene);
            return out;
        }
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 4,
               });
    if (field == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    const uint8_t voxels[16] = {
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
    };
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}))
    {
        dvz_scene_destroy(scene);
        return out;
    }

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    if (volume == NULL || slice == NULL)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    double occluder_bounds_min[3] = {0.0, 0.0, 0.0};
    double occluder_bounds_max[3] = {0.52, 1.0, 1.0};
    if (!dvz_visual_set_field(volume, "field", field) ||
        !dvz_visual_set_field(slice, "field", field) ||
        dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) != 0 ||
        dvz_volume_set_step_count(volume, 16) != 0 || dvz_volume_set_opacity(volume, 0.15f) != 0 ||
        (clipped_occluder &&
         dvz_volume_set_bounds(volume, occluder_bounds_min, occluder_bounds_max) != 0) ||
        dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) != 0 ||
        dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) != 0 ||
        dvz_volume_set_slice_axis(slice, DVZ_VOLUME_AXIS_Z) != 0 ||
        dvz_volume_set_slice_position(slice, 0.90) != 0 ||
        dvz_volume_set_opacity(slice, 0.55f) != 0 ||
        dvz_visual_set_alpha_mode(slice, DVZ_ALPHA_BLENDED) != 0 ||
        dvz_visual_set_volume_occluded(slice, true) != 0 ||
        dvz_visual_set_scene_occluded(slice, true) != 0 ||
        dvz_visual_set_scene_occluder(volume, true) != 0 ||
        dvz_panel_add_visual(panel, volume, NULL) != 0 ||
        dvz_panel_add_visual(panel, slice, NULL) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    if (mode == APP_VOLUME_OCCLUSION_MODE_VOLUME &&
        dvz_panel_set_volume_occluder(
            panel, volume,
            &(DvzVolumeOcclusionDesc){
                .enabled = true,
                .alpha_threshold = 0.005f,
                .fade_distance = 0.02f,
                .occluded_alpha = 0.05f,
            }) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    if (mode == APP_VOLUME_OCCLUSION_MODE_SCENE &&
        dvz_panel_set_scene_occlusion(
            panel, &(DvzSceneOcclusionDesc){
                       .enabled = true,
                       .depth_bias = 0.0f,
                       .soft_edge = 0.02f,
                       .hidden_alpha = 0.05f,
                   }) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    if (canvas == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }

    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        out.width = 0;
        out.height = 0;
        if (dvz_canvas_capture_rgba(canvas, &out.width, &out.height, &rgba) != 0)
        {
            out.skipped = true;
            out.skip_reason = "app offscreen capture unavailable";
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return out;
        }
    }
    if (rgba != NULL && out.width == 64 && out.height == 64)
    {
        out.total_sum = _app_rgb_sum(rgba, out.width * out.height);
        out.left_sum = _app_rgb_region_sum(rgba, out.width, out.height, 12, 24, 24, 40);
        out.right_sum = _app_rgb_region_sum(rgba, out.width, out.height, 40, 24, 52, 40);
    }

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return out;
}


/**
 * Ensure volume-occluded slices render dimmer with an active volume occlusion pass.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_occlusion_slice_renders(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppVolumeOcclusionCapture disabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_DISABLED, false, false);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_volume_occlusion_slice_renders skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_VOLUME, false, false);
    if (enabled.skipped)
    {
        log_warn("test_app_offscreen_volume_occlusion_slice_renders skipped: GPU context failed");
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }

    AT(disabled.width == 64);
    AT(disabled.height == 64);
    AT(enabled.width == 64);
    AT(enabled.height == 64);
    AT(disabled.total_sum > 64 * 64 * 24);
    AT(enabled.total_sum > 64 * 64 * 24);
    AT(disabled.left_sum > 0);
    AT(disabled.right_sum > 0);
    AT(enabled.left_sum > 0);
    AT(enabled.right_sum > 0);
    AT(enabled.total_sum + 64ull * 64ull * 24ull < disabled.total_sum);
    AT(enabled.left_sum + 12ull * 16ull * 24ull < disabled.left_sum);
    AT(enabled.right_sum + 12ull * 16ull * 24ull < disabled.right_sum);
    return 0;
}


/**
 * Ensure volume occlusion affects only the screen region covered by a clipped occluder.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_occlusion_region_delta(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppVolumeOcclusionCapture disabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_DISABLED, false, true);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_volume_occlusion_region_delta skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_VOLUME, false, true);
    if (enabled.skipped)
    {
        log_warn("test_app_offscreen_volume_occlusion_region_delta skipped: GPU context failed");
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }

    AT(disabled.width == 64);
    AT(disabled.height == 64);
    AT(enabled.width == 64);
    AT(enabled.height == 64);
    AT(disabled.left_sum > 0);
    AT(disabled.right_sum > 0);
    AT(enabled.left_sum > 0);
    AT(enabled.right_sum > 0);

    uint64_t left_delta =
        disabled.left_sum > enabled.left_sum ? disabled.left_sum - enabled.left_sum : 0;
    uint64_t right_delta =
        disabled.right_sum > enabled.right_sum ? disabled.right_sum - enabled.right_sum : 0;
    uint64_t strong_delta = 12ull * 16ull * 24ull;
    uint64_t weak_delta = 12ull * 16ull * 8ull;
    AT(left_delta > strong_delta || right_delta > strong_delta);
    AT(left_delta < weak_delta || right_delta < weak_delta);
    return 0;
}


/**
 * Ensure volume occlusion remains visible when rendered through a perspective camera.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_occlusion_perspective_camera(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppVolumeOcclusionCapture disabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_DISABLED, true, false);
    if (disabled.skipped)
    {
        log_warn(
            "test_app_offscreen_volume_occlusion_perspective_camera skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_VOLUME, true, false);
    if (enabled.skipped)
    {
        log_warn(
            "test_app_offscreen_volume_occlusion_perspective_camera skipped: GPU context failed");
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }

    AT(disabled.width == 64);
    AT(disabled.height == 64);
    AT(enabled.width == 64);
    AT(enabled.height == 64);
    AT(disabled.total_sum > 64 * 64 * 24);
    AT(enabled.total_sum > 64 * 64 * 24);
    AT(enabled.total_sum + 64ull * 64ull * 12ull < disabled.total_sum);
    return 0;
}


/**
 * Ensure generic scene occlusion dims volume slices when the occluder is a volume.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_slice_scene_occlusion_dimming(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    AppVolumeOcclusionCapture disabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_DISABLED, false, false);
    if (disabled.skipped)
    {
        log_warn(
            "test_app_offscreen_volume_slice_scene_occlusion_dimming skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(APP_VOLUME_OCCLUSION_MODE_SCENE, false, false);
    if (enabled.skipped)
    {
        log_warn(
            "test_app_offscreen_volume_slice_scene_occlusion_dimming skipped: GPU context failed");
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }

    AT(disabled.width == 64);
    AT(disabled.height == 64);
    AT(enabled.width == 64);
    AT(enabled.height == 64);
    AT(disabled.total_sum > 64 * 64 * 24);
    AT(enabled.total_sum > 64 * 64 * 24);
    if (!(enabled.total_sum + 64ull * 64ull * 12ull < disabled.total_sum))
    {
        log_error(
            "volume slice scene occlusion dimming failed: disabled=%" PRIu64
            " enabled=%" PRIu64 " threshold=%" PRIu64,
            disabled.total_sum, enabled.total_sum, 64ull * 64ull * 12ull);
        return 1;
    }
    return 0;
}


/**
 * Ensure toggling a hidden mesh into a scene occluder keeps retained volume bindings valid.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 4,
               });
    ANN(field);
    const uint8_t voxels[16] = {
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
    };
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(volume);
    ANN(slice);
    ANN(mesh);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.1f},
        {+0.5f, -0.5f, 0.1f},
        {-0.5f, +0.5f, 0.1f},
        {+0.5f, +0.5f, 0.1f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[4] = {
        {255, 64, 64, 255},
        {255, 64, 64, 255},
        {255, 64, 64, 255},
        {255, 64, 64, 255},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_visual_set_field(slice, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_volume_set_opacity(volume, 0.15f) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_volume_set_slice_axis(slice, DVZ_VOLUME_AXIS_Z) == 0);
    AT(dvz_volume_set_slice_position(slice, 0.90) == 0);
    AT(dvz_volume_set_opacity(slice, 0.55f) == 0);
    AT(dvz_visual_set_alpha_mode(slice, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_volume_occluded(slice, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, false) == 0);
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_visual_set_depth_test(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluder(mesh, false) == 0);
    dvz_visual_set_visible(mesh, false);

    AT(dvz_panel_add_visual(
           panel, volume, &(DvzVisualAttachDesc){.z_layer = 0}) == 0);
    AT(dvz_panel_add_visual(
           panel, slice, &(DvzVisualAttachDesc){.z_layer = 1}) == 0);
    AT(dvz_panel_add_visual(
           panel, mesh, &(DvzVisualAttachDesc){.z_layer = 2}) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){
               .enabled = true,
               .alpha_threshold = 0.005f,
               .fade_distance = 0.02f,
               .occluded_alpha = 0.05f,
           }) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    if (win == NULL)
    {
        log_warn(
            "test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle skipped: app window "
            "failed");
        tst_skip(suite, "app window creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    tst_log_capture_begin(suite);
    int rc = dvz_app_window_render_once(win);
    AppLogCapture first = {0};
    _app_log_capture_from_suite(suite, &first);
    tst_log_capture_end(suite);
    AT(rc == DVZ_CANVAS_FRAME_READY);
    AT(first.error_count == 0);

    dvz_visual_set_visible(mesh, true);
    AT(dvz_visual_set_scene_occluder(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel, &(DvzSceneOcclusionDesc){
                      .enabled = true,
                      .depth_bias = 0.0005f,
                      .soft_edge = 0.02f,
                      .hidden_alpha = 0.05f,
                  }) == 0);

    tst_log_capture_begin(suite);
    rc = dvz_app_window_render_once(win);
    AppLogCapture second = {0};
    _app_log_capture_from_suite(suite, &second);
    tst_log_capture_end(suite);
    AT(rc == DVZ_CANVAS_FRAME_READY);
    AT(!second.sampled_bind_group_miss);
    AT(!second.contract_validation_failed);
    AT(second.error_count == 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure alpha-blended volume rays stop behind an opaque primitive depth buffer.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_depth_occluded_by_primitive(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* occluder = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(occluder);
    ANN(volume);

    float quad[6][3] = {
        {-0.65f, -0.65f, 0.1f}, {-0.65f, 0.65f, 0.1f}, {0.65f, -0.65f, 0.1f},
        {0.65f, -0.65f, 0.1f},  {-0.65f, 0.65f, 0.1f}, {0.65f, 0.65f, 0.1f},
    };
    DvzColor black[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        black[i][0] = 0;
        black[i][1] = 0;
        black[i][2] = 0;
        black[i][3] = 255;
    }
    AT(dvz_visual_set_data(occluder, "position", quad, 6) == 0);
    AT(dvz_visual_set_data(occluder, "color", black, 6) == 0);
    AT(dvz_panel_add_visual(panel, occluder, NULL) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 4,
               });
    ANN(field);
    const uint8_t voxels[16] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        255, 255, 255, 255,
    };
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.0f, 0.0f, 0.0f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_depth_occluded_by_primitive skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
    }
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    const uint8_t* corner = _pixel_at(rgba, width, height, width / 8, height / 8);
    AT(center[0] < 40 && center[1] < 40 && center[2] < 40);
    AT(corner[0] > 120 || corner[1] > 120 || corner[2] > 120);

    dvz_free(rgba);
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

    TST_MODULE(suite, "scene");
    TST_GROUP("app-offscreen");

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    TST_SCENE_APP_CASE(test_app_offscreen, TST_SCENE_APP_GPU_RES, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_timer_advances_in_app_run, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_timer_advances_in_render_once, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_render_enabled_gate, TST_SCENE_APP_GPU_RES, TST_ISOLATION_PROCESS);
#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
    TST_SCENE_APP_CASE(
        test_app_external_surface_release_waits, TST_SCENE_APP_GPU_RES | TST_RES_GLFW,
        TST_ISOLATION_PROCESS);
#endif
    TST_SCENE_APP_CASE(
        test_app_offscreen_panel_three_visuals_all_drawn, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_point_depth_orders_overlap, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_wboit_mesh_order_independent_layers, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_source_over_mesh_depth_and_blend, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_depth_peel_mesh_two_layers, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_scene_occlusion_hidden_alpha, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_source_over_scene_occlusion_matrix, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_point_depth_cue_darkens_far, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_has_nonblank_pixels, TST_SCENE_APP_GPU_RES, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_pixel_square_has_nonblank_pixels, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_points_edl_renders, TST_SCENE_APP_GPU_RES, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_points_edl_changes_pixels, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_mesh_ssao_changes_pixels, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_sphere_ssao_darkens_contact, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_records_dvzr_frames,
        TST_SCENE_APP_GPU_RES | TST_RES_FILESYSTEM | TST_RES_ENV, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_image_has_nonblank_pixels, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_text_has_nonblank_pixels, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_sdf_text_has_nonblank_pixels, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_image_field_partial_update_changes_region, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_lit_primitive_depth_orders_overlap, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_lit_primitive_depth_cue_darkens_far, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_mesh_renders_nonblank, TST_SCENE_APP_GPU_RES, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_rotated_mesh_depth_orders_faces, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_camera_arcball_mesh_renders_cube, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_shared_field_mixed_runtime_updates, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_retained_render_second_frame, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_image_retained_render_second_frame, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_resize_reuses_runtime_with_mesh_and_image, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_pick_probe_request_steady_state, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_two_panel_points_light_both_halves, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_clear_color, TST_SCENE_APP_GPU_RES, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_slice_renders_field, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_mip_renders_bright_slice, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_composite_renders_field, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_occlusion_slice_renders, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_occlusion_region_delta, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_occlusion_perspective_camera, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_slice_scene_occlusion_dimming, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle,
        TST_SCENE_APP_GPU_RES | TST_RES_LOG_CAPTURE, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_depth_occluded_by_primitive, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_capture_rejects_wrong_dimensions, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
    TST_SCENE_APP_CASE(
        test_app_capture_rejects_undersized_buffer, TST_SCENE_APP_GPU_RES,
        TST_ISOLATION_PROCESS);
#endif

    return 0;
}
