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
#include "_scene.h"
#include "_technique.h"
#include "../../app/_app.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/ffi.h"
#include "datoviz/scene.h"
#include "text/internal.h"
#include "text/text_internal.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_APP_GPU_RES (TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN)

#define TST_SCENE_APP_GPU_FIXTURE "scene-app-gpu"

#define TST_SCENE_APP_CASE(test, resource_flags, isolation_mode)                                  \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = (resource_flags);                                                   \
        _tst_desc.isolation = (isolation_mode);                                                   \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_APP_SHARED_CASE(test)                                                           \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_SCENE_APP_GPU_RES;                                              \
        _tst_desc.isolation = TST_ISOLATION_SERIAL;                                               \
        _tst_desc.fixture = TST_SCENE_APP_GPU_FIXTURE;                                            \
        _tst_desc.fixture_scope = TST_FIXTURE_SCOPE_PROCESS;                                      \
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

/**
 * Apply a Phong material while preserving the current visual alpha mode.
 *
 * @param visual the visual
 * @param light_direction material light direction
 * @param ambient ambient coefficient
 * @param diffuse diffuse coefficient
 * @param specular specular coefficient
 * @param shininess shininess exponent
 * @return 0 on success, -1 on error
 */
static int _test_set_phong_material(
    DvzVisual* visual, const float light_direction[3], float ambient, float diffuse,
    float specular, float shininess)
{
    ANN(visual);
    ANN(light_direction);
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.alpha_mode = dvz_visual_alpha_mode(visual);
    material.light_direction[0] = light_direction[0];
    material.light_direction[1] = light_direction[1];
    material.light_direction[2] = light_direction[2];
    material.phong.ambient = ambient;
    material.phong.diffuse = diffuse;
    material.phong.specular = specular;
    material.phong.shininess = shininess;
    return dvz_visual_set_material(visual, &material);
}



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
    DvzView* last_window;
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


typedef struct
{
    DvzGpuCtx* gpu_ctx;
    DvzDrp2Runtime* runtime;
    DvzWindowHost* window_host;
    bool available;
    const char* skip_reason;
} DvzTestGpuFixture;


typedef enum
{
    APP_VOLUME_OCCLUSION_MODE_DISABLED,
    APP_VOLUME_OCCLUSION_MODE_VOLUME,
    APP_VOLUME_OCCLUSION_MODE_SCENE,
} AppVolumeOcclusionMode;



/**
 * Return an app config that does not request extra instance extensions from borrowed GPU contexts.
 *
 * @return app config with GPU-extension requests disabled
 */
static DvzAppConfig _app_test_resource_config(void)
{
    DvzAppConfig config = dvz_app_config();
    config.instance_extension_count = 0;
    config.instance_extensions = NULL;
    config.enable_canvas_extensions = false;
    config.enable_glfw_extensions = false;
    return config;
}



/**
 * Create a GPU context with the same feature baseline as the default app path.
 *
 * @return GPU context, or NULL when Vulkan setup is unavailable
 */
static DvzGpuCtx* _app_test_gpu_ctx(void)
{
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    return dvz_gpu_ctx(&gpu_cfg);
}



/**
 * Create the process-scoped GPU fixture used by shared offscreen app tests.
 *
 * @param suite active test suite
 * @param worker_index scheduler worker index
 * @return fixture state
 */
static void* _app_gpu_fixture_create(TstSuite* suite, uint32_t worker_index)
{
    (void)suite;
    (void)worker_index;

    DvzTestGpuFixture* fixture = (DvzTestGpuFixture*)dvz_calloc(1, sizeof(DvzTestGpuFixture));
    ANN(fixture);

    fixture->window_host = dvz_window_host();
    if (fixture->window_host == NULL)
    {
        fixture->skip_reason = "window host creation failed";
        return fixture;
    }

    fixture->gpu_ctx = _app_test_gpu_ctx();
    if (fixture->gpu_ctx == NULL)
    {
        fixture->skip_reason = "GPU context creation failed";
        return fixture;
    }

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
        dvz_gpu_ctx_device(fixture->gpu_ctx), dvz_gpu_ctx_alloc(fixture->gpu_ctx));
    fixture->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (fixture->runtime == NULL)
    {
        fixture->skip_reason = "DRP2 runtime creation failed";
        return fixture;
    }

    fixture->available = true;
    return fixture;
}



/**
 * Destroy a process-scoped GPU fixture used by shared offscreen app tests.
 *
 * @param fixture_ptr fixture state
 */
static void _app_gpu_fixture_destroy(void* fixture_ptr)
{
    DvzTestGpuFixture* fixture = (DvzTestGpuFixture*)fixture_ptr;
    if (fixture == NULL)
        return;
    if (fixture->gpu_ctx != NULL)
    {
        /* Other app paths may have loaded Volk against a later transient instance. */
        DvzInstance* instance = dvz_gpu_ctx_instance(fixture->gpu_ctx);
        if (instance != NULL && dvz_instance_handle(instance) != VK_NULL_HANDLE)
            volkLoadInstance(dvz_instance_handle(instance));
    }
    if (fixture->runtime != NULL)
    {
        dvz_drp2_runtime_destroy(fixture->runtime);
        fixture->runtime = NULL;
    }
    if (fixture->gpu_ctx != NULL)
    {
        dvz_gpu_ctx_destroy(fixture->gpu_ctx);
        fixture->gpu_ctx = NULL;
    }
    if (fixture->window_host != NULL)
    {
        dvz_window_host_destroy(fixture->window_host);
        fixture->window_host = NULL;
    }
    dvz_free(fixture);
}



/**
 * Create an app using the current test fixture when the case requested one.
 *
 * @param suite active test context
 * @param scene scene borrowed by the app
 * @return app, or NULL when setup is unavailable
 */
static DvzApp* _app_test_create(TstContext* suite, DvzScene* scene)
{
    ANN(suite);
    ANN(scene);

    DvzTestGpuFixture* fixture =
        (DvzTestGpuFixture*)tst_context_fixture(suite, TST_SCENE_APP_GPU_FIXTURE);
    if (fixture == NULL)
    {
        return dvz_app(scene);
    }
    if (!fixture->available)
    {
        tst_skip(suite, fixture->skip_reason != NULL ? fixture->skip_reason : "GPU fixture unavailable");
        return NULL;
    }

    dvz_drp2_runtime_reset(fixture->runtime);
    DvzAppResources resources = dvz_app_resources();
    resources.gpu_ctx = fixture->gpu_ctx;
    resources.runtime = fixture->runtime;
    resources.window_host = fixture->window_host;
    DvzAppConfig config = _app_test_resource_config();
    DvzApp* app = dvz_app_with_resources(scene, &config, &resources);
    if (app == NULL)
    {
        tst_skip(suite, "app creation from shared GPU fixture failed");
    }
    return app;
}



/**
 * Record one app-driven timer callback.
 *
 * @param animation animation handle
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time
 * @param tick timer tick index
 * @param user_data timer probe storage
 */
static void _app_timer_probe_callback(
    DvzAnimation* animation, double t, double dt, uint64_t tick, void* user_data)
{
    (void)animation;
    (void)tick;
    AppTimerProbe* probe = (AppTimerProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_t = t;
    probe->last_dt = dt;
    probe->total_dt += dt;
}


/**
 * Record one view request-frame callback.
 *
 * @param win view requesting a frame
 * @param user_data request-frame probe storage
 */
static void _app_request_frame_probe_callback(DvzView* win, void* user_data)
{
    AppRequestFrameProbe* probe = (AppRequestFrameProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_window = win;
}


/**
 * Accept one view frame callback without mutating test state.
 *
 * @param win view that completed a frame
 * @param user_data unused callback user data
 */
static void _app_empty_frame_callback(DvzView* win, void* user_data)
{
    (void)win;
    (void)user_data;
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

    vec3 positions[4] = {
        {xmin, ymin, z},
        {xmax, ymin, z},
        {xmin, ymax, z},
        {xmax, ymax, z},
    };
    DvzColor colors[4] = {0};
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    for (uint32_t i = 0; i < 4; i++)
        colors[i] = color;

    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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

    vec3 positions[3] = {
        {-0.9f, -0.9f, 0.0f},
        {0.9f, -0.9f, 0.0f},
        {0.0f, 0.9f, 0.0f},
    };
    DvzColor colors[3] = {0};
    for (uint32_t i = 0; i < 3; i++)
        colors[i] = color;

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

    vec3 positions[6] = {
        {xmin, ymin, z},
        {xmax, ymin, z},
        {xmin, ymax, z},
        {xmax, ymin, z},
        {xmax, ymax, z},
        {xmin, ymax, z},
    };
    DvzColor colors[6] = {0};
    for (uint32_t i = 0; i < 6; i++)
        colors[i] = color;

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
 * @param suite test context used for shared app resources
 * @param scene_occlusion_enabled whether scene occlusion should be enabled on the panel
 * @param occluder_hidden whether the occluder visual is hidden
 * @param occluder_alpha alpha channel used by the visible occluder
 * @return captured RGB values, or skipped=true when no app context is available
 */
static AppSceneOcclusionCapture _app_source_over_scene_occlusion_capture_center(
    TstContext* suite, bool scene_occlusion_enabled, bool occluder_hidden, uint8_t occluder_alpha)
{
    ANN(suite);
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

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
            panel, &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_view_canvas(win);
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
 * @param suite test context used for shared app resources
 * @param reverse_order whether the blue layer is added before the red layer
 * @return captured RGB values, or skipped=true when no app context is available
 */
static AppWboitCapture _app_wboit_capture_center(TstContext* suite, bool reverse_order)
{
    ANN(suite);
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    DvzVisualDataView position_view = {0};
    if (dvz_visual_data(visual, "position", &position_view) != 0 ||
        position_view.item_size != 3 * sizeof(float))
    {
        return false;
    }

    const float* positions = position_view.data;
    const float* bounds = NULL;
    DvzVisualDataView bounds_view = {0};
    if (dvz_visual_data(visual, "bounds", &bounds_view) == 0 &&
        bounds_view.item_count == position_view.item_count &&
        bounds_view.item_size == 4 * sizeof(float))
    {
        bounds = bounds_view.data;
    }

    const float* angles = NULL;
    DvzVisualDataView angle_view = {0};
    if (dvz_visual_data(visual, "angle", &angle_view) == 0 &&
        angle_view.item_count == position_view.item_count && angle_view.item_size == sizeof(float))
    {
        angles = angle_view.data;
    }
    float min_x = +INFINITY;
    float min_y = +INFINITY;
    float max_x = -INFINITY;
    float max_y = -INFINITY;
    for (uint64_t i = 0; i < position_view.item_count; i++)
    {
        uint64_t pos_offset = 3 * i;
        float px = (positions[pos_offset + 0] * 0.5f + 0.5f) * (float)width;
        float py = (1.0f - (positions[pos_offset + 1] * 0.5f + 0.5f)) * (float)height;
        if (bounds != NULL)
        {
            float c = angles != NULL ? cosf(angles[i]) : 1.0f;
            float s = angles != NULL ? sinf(angles[i]) : 0.0f;
            uint64_t bounds_offset = 4 * i;
            float x0 = bounds[bounds_offset + 0], y0 = bounds[bounds_offset + 1];
            float x1 = bounds[bounds_offset + 2], y1 = bounds[bounds_offset + 3];
            vec2 corners[4] = {{x0, y0}, {x0, y1}, {x1, y0}, {x1, y1}};
            for (uint32_t k = 0; k < 4; k++)
            {
                float x = px + c * corners[k][0] - s * corners[k][1];
                float y = py + s * corners[k][0] + c * corners[k][1];
                if (x < min_x)
                    min_x = x;
                if (x > max_x)
                    max_x = x;
                if (y < min_y)
                    min_y = y;
                if (y > max_y)
                    max_y = y;
            }
        }
        else
        {
            if (px < min_x)
                min_x = px;
            if (px > max_x)
                max_x = px;
            if (py < min_y)
                min_y = py;
            if (py > max_y)
                max_y = py;
        }
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
 * @param suite test context used for shared app resources
 * @param enabled whether EDL should be enabled for the panel
 * @return captured RGBA buffer, or skipped=true when no app context is available
 */
static AppRgbaCapture _app_edl_point_capture(TstContext* suite, bool enabled)
{
    ANN(suite);
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
    vec3 positions[6] = {
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));
    if (enabled &&
        !dvz_panel_set_edl(
            panel, &(DvzEdlDesc){DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 90.0f, .depth_scale = 1.0f}))
    {
        dvz_scene_destroy(scene);
        return out;
    }

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_view_canvas(win);
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
 * @param canvas the view canvas
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

    vec3 positions[3] = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 20.0f, 15.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Create app and offscreen window */
    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    AT(dvz_app_vk_instance(app) != VK_NULL_HANDLE);
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_view_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    dvz_view_request_frame(win);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(dvz_view_emit_resize(win, 64, 64, 64, 64, 1.0f, 1.0f) == 0);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);

    /* Exercise host-driven and Datoviz-owned frame paths. */
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_app_render_once(app) == 0);
    dvz_app_run(app, 1);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_view_capabilities(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_view_capabilities skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    DvzCapabilitySnapshot caps = {0};
    AT(!dvz_view_capabilities(NULL, &caps));
    AT(!dvz_view_capabilities(win, NULL));
    AT(dvz_view_capabilities(win, &caps));
    AT(caps.struct_size == sizeof(DvzCapabilitySnapshot));
    AT(caps.max_buffer_size > 0);
    AT(caps.max_texture_dimension_2d > 0);
    AT(caps.max_readback_size > 0);
    AT(caps.min_texture_copy_bytes_per_row_alignment > 0);
    AT(caps.shader_format_glsl);
    AT(caps.supports_readback);
    AT(!caps.query_profile_u32_r32 || caps.render_target_format_r32uint);
    AT(!caps.query_profile_u64_rg32 || caps.render_target_format_rg32uint);
    AT(!caps.query_profile_u64_2xr32 ||
       (caps.render_target_format_r32uint && caps.max_color_attachments >= 2));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_app_view_desc_offscreen_scale(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 80, 60, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_view_desc_offscreen_scale skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
    desc.logical_width = 80;
    desc.logical_height = 60;
    desc.device_scale = 2.0f;
    desc.user_scale = 1.5f;
    desc.render_scale = 1.25f;
    DvzView* win = dvz_view(app, figure, &desc);
    AT(win != NULL);

    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    dvz_view_logical_size(win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(win, &framebuffer_width, &framebuffer_height);
    AT(logical_width == 80);
    AT(logical_height == 60);
    AT(framebuffer_width == 160);
    AT(framebuffer_height == 120);
    AT(fabsf(dvz_view_device_scale(win) - 2.0f) < 1e-6f);
    AT(fabsf(dvz_view_user_scale(win) - 1.5f) < 1e-6f);
    AT(fabsf(dvz_view_render_scale(win) - 1.25f) < 1e-6f);

    uint32_t figure_width = 0;
    uint32_t figure_height = 0;
    dvz_figure_size(figure, &figure_width, &figure_height);
    AT(figure_width == 80);
    AT(figure_height == 60);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    uint32_t capture_width = 0;
    uint32_t capture_height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &capture_width, &capture_height, &rgba) == 0);
    AT(capture_width == 160);
    AT(capture_height == 120);
    dvz_free(rgba);

    dvz_view_set_user_scale(win, 2.0f);
    AT(fabsf(dvz_view_user_scale(win) - 2.0f) < 1e-6f);
    dvz_view_logical_size(win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(win, &framebuffer_width, &framebuffer_height);
    AT(logical_width == 80);
    AT(logical_height == 60);
    AT(framebuffer_width == 160);
    AT(framebuffer_height == 120);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



int test_app_view_desc_offscreen_exact_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 0, 0, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_view_desc_offscreen_exact_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
    desc.framebuffer_width = 96;
    desc.framebuffer_height = 64;
    DvzView* win = dvz_view(app, figure, &desc);
    AT(win != NULL);

    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    dvz_view_logical_size(win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(win, &framebuffer_width, &framebuffer_height);
    AT(logical_width == 96);
    AT(logical_height == 64);
    AT(framebuffer_width == 96);
    AT(framebuffer_height == 64);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



int test_app_offscreen_small_view_clamps_layout(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 320, 240, 0);
    AT(figure != NULL);

    DvzGrid* grid = dvz_figure_grid(figure, 1, 1);
    AT(grid != NULL);
    DvzPanel* panel = dvz_grid_panel(grid, 0, 0);
    AT(panel != NULL);
    AT(dvz_panel_set_reserve(
        panel, &(DvzPanelReserve){.left_px = 140.0f, .right_px = 80.0f, .bottom_px = 90.0f,
                                        .top_px = 45.0f}));

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 255, 255, 255}};
    float sizes[1] = {16.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_small_view_clamps_layout skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
    desc.logical_width = 120;
    desc.logical_height = 90;
    DvzView* win = dvz_view(app, figure, &desc);
    AT(win != NULL);

    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    uint32_t figure_width = 0;
    uint32_t figure_height = 0;
    dvz_view_logical_size(win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(win, &framebuffer_width, &framebuffer_height);
    dvz_figure_size(figure, &figure_width, &figure_height);
    AT(logical_width == 120);
    AT(logical_height == 90);
    AT(framebuffer_width == 120);
    AT(framebuffer_height == 90);
    AT(figure_width == 200);
    AT(figure_height == 200);

    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    uint32_t capture_width = 0;
    uint32_t capture_height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(dvz_view_canvas(win), &capture_width, &capture_height, &rgba) == 0);
    AT(capture_width == 120);
    AT(capture_height == 90);
    dvz_free(rgba);

    AT(dvz_view_resize(win, 80, 80) == 0);
    dvz_view_logical_size(win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(win, &framebuffer_width, &framebuffer_height);
    dvz_figure_size(figure, &figure_width, &figure_height);
    AT(logical_width == 80);
    AT(logical_height == 80);
    AT(framebuffer_width == 80);
    AT(framebuffer_height == 80);
    AT(figure_width == 200);
    AT(figure_height == 200);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure on-demand scheduling sees retained scene mutations without an explicit frame request.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_scheduler_sees_scene_dirty_without_request(
    TstContext* suite, const TstCase* item)
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

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float size[1] = {8.0f};
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_scheduler_sees_scene_dirty_without_request skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    AT(_dvz_view_scheduler_should_render(win, false, 0));
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    if (_dvz_view_scheduler_should_render(win, false, 0))
        AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(!_dvz_view_scheduler_should_render(win, false, 0));

    AppRequestFrameProbe request_probe = {0};
    dvz_view_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);

    float updated_size[1] = {16.0f};
    AT(dvz_visual_set_data(visual, "size", updated_size, 1) == 0);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(_dvz_view_scheduler_should_render(win, false, 0));
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(!_dvz_view_scheduler_should_render(win, false, 0));

    dvz_visual_set_visible(visual, false);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);
    AT(_dvz_view_scheduler_should_render(win, false, 0));
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(!_dvz_view_scheduler_should_render(win, false, 0));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure on-demand app runs treat frame callbacks as continuous work.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_frame_callback_enables_continuous_scheduler(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_frame_callback_enables_continuous_scheduler skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    AT(!_dvz_app_has_continuous_work(app));
    dvz_view_set_frame_callback(win, _app_empty_frame_callback, NULL);
    AT(_dvz_app_has_continuous_work(app));
    dvz_view_set_frame_callback(win, NULL, NULL);
    AT(!_dvz_app_has_continuous_work(app));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure query requests notify hosted view repaint callbacks.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_query_requests_notify_hosted_callback(
    TstContext* suite, const TstCase* item)
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
    dvz_visual_set_query_capabilities(points, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
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
    AT(dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = -1}) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_query_requests_notify_hosted_callback skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    if (_dvz_view_scheduler_should_render(win, false, 0))
        AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(!_dvz_view_scheduler_should_render(win, false, 0));

    AppRequestFrameProbe request_probe = {0};
    dvz_view_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);

    AT(dvz_panel_query(panel, 32.0, 32.0, &(DvzQueryRequest){DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest), .request_id = 1}) == 0);
    AT(request_probe.calls == 1);
    AT(request_probe.last_window == win);
    AT(_dvz_view_scheduler_should_render(win, false, 0));

    AT(dvz_panel_query(panel, 32.0, 32.0, &(DvzQueryRequest){DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest), .request_id = 2}) == 0);
    AT(request_probe.calls == 2);
    AT(request_probe.last_window == win);
    AT(_dvz_view_scheduler_should_render(win, false, 0));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure multiple apps sharing one scene receive independent request-frame notifications.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_shared_scene_request_frame_subscribers(
    TstContext* suite, const TstCase* item)
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

    DvzApp* app1 = _app_test_create(suite, scene);
    if (app1 == NULL)
    {
        log_warn(
            "test_app_offscreen_shared_scene_request_frame_subscribers skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win1 = dvz_view_offscreen(app1, figure, 64, 64);
    AT(win1 != NULL);

    DvzApp* app2 = dvz_app(scene);
    if (app2 == NULL)
    {
        log_warn(
            "test_app_offscreen_shared_scene_request_frame_subscribers skipped: second GPU "
            "context failed");
        tst_skip(suite, "second GPU context failed");
        dvz_app_destroy(app1);
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win2 = dvz_view_offscreen(app2, figure, 64, 64);
    AT(win2 != NULL);

    AppRequestFrameProbe probe1 = {0};
    AppRequestFrameProbe probe2 = {0};
    dvz_view_set_request_frame_callback(win1, _app_request_frame_probe_callback, &probe1);
    dvz_view_set_request_frame_callback(win2, _app_request_frame_probe_callback, &probe2);

    AT(dvz_panel_query(panel, 32.0, 32.0, &(DvzQueryRequest){DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest), .request_id = 1}) == 0);
    AT(probe1.calls == 1);
    AT(probe1.last_window == win1);
    AT(probe2.calls == 1);
    AT(probe2.last_window == win2);

    dvz_app_destroy(app1);
    AT(dvz_panel_query(panel, 32.0, 32.0, &(DvzQueryRequest){DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest), .request_id = 2}) == 0);
    AT(probe1.calls == 1);
    AT(probe2.calls == 2);
    AT(probe2.last_window == win2);

    dvz_app_destroy(app2);
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.callback = _app_timer_probe_callback;
    timer_desc.user_data = &probe;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_timer_advances_in_app_run skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.callback = _app_timer_probe_callback;
    timer_desc.user_data = &probe;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_timer_advances_in_render_once skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_view_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls >= 1);
    AT(request_probe.last_window == win);
    uint32_t calls_after_first_frame = request_probe.calls;
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls > calls_after_first_frame);
    AT(request_probe.last_window == win);

    AT(probe.calls == 2);
    AC(probe.last_t, 0.125, EPS);
    AC(probe.last_dt, 0.125, EPS);
    AC(probe.total_dt, 0.125, EPS);
    AC(dvz_scene_clock_time(scene), 0.125, EPS);
    AC(dvz_scene_clock_dt(scene), 0.125, EPS);

    dvz_anim_stop(timer);
    uint32_t calls_after_active_frames = request_probe.calls;
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls == calls_after_active_frames);
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.callback = _app_timer_probe_callback;
    timer_desc.user_data = &timer_probe;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_render_enabled_gate skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    AT(dvz_view_render_enabled(win));

    AppRequestFrameProbe request_probe = {0};
    dvz_view_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);

    dvz_view_set_render_enabled(win, false);
    AT(!dvz_view_render_enabled(win));
    AT(dvz_view_render_once(win) == 0);
    AT(request_probe.calls == 0);
    AT(timer_probe.calls == 0);
    AC(dvz_scene_clock_time(scene), 0.0, EPS);

    dvz_view_set_render_enabled(win, true);
    AT(dvz_view_render_enabled(win));
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(request_probe.calls >= 1);
    AT(request_probe.last_window == win);
    AT(timer_probe.calls == 1);
    AC(dvz_scene_clock_time(scene), 0.0, EPS);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_view_panel_panzoom_helper(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_view_panel_panzoom_helper skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    ANN(panzoom);

    DvzController* controller_x = dvz_panel_controller(panel, DVZ_DIM_X);
    DvzController* controller_y = dvz_panel_controller(panel, DVZ_DIM_Y);
    DvzController* controller_z = dvz_panel_controller(panel, DVZ_DIM_Z);
    AT(controller_x != NULL);
    AT(controller_x == controller_y);
    AT(controller_z == NULL);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


int test_view_connects_prebound_panel_controller(TstContext* suite, const TstCase* item)
{
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);
    AC(panzoom->zoom[0], 1.0f, EPS);
    AC(panzoom->zoom[1], 1.0f, EPS);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_view_connects_prebound_panel_controller skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);

    AT(dvz_view_emit_wheel(win, 32.0f, 32.0f, 64.0f, 64.0f, 0.0f, 1.0f, 0) == 0);
    AT(panzoom->zoom[0] > 1.0f);
    AT(panzoom->zoom[1] > 1.0f);

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

    DvzView* win = dvz_ffi_view_external_surface(
        app, figure, (void*)instance, (uint64_t)(uintptr_t)surface, 64, 64, 1.0f, 1.0f, false);
    AT(win != NULL);

    AppRequestFrameProbe request_probe = {0};
    dvz_view_set_request_frame_callback(win, _app_request_frame_probe_callback, &request_probe);
    AT(dvz_ffi_view_update_external_surface(
           win, (void*)instance, (uint64_t)(uintptr_t)surface, 64, 64, 1.0f, 1.0f, false) == 0);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);

    AT(dvz_view_release_external_surface(win) == DVZ_CANVAS_FRAME_WAIT_SURFACE);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_WAIT_SURFACE);
    AT(dvz_app_render_once(app) == DVZ_CANVAS_FRAME_WAIT_SURFACE);

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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_panel_three_visuals_all_drawn skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_depth_orders_overlap skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    vec3 positions[2] = {{-0.45f, 0.0f, 0.0f}, {0.45f, 0.0f, 0.8f}};
    DvzColor colors[2] = {{255, 64, 64, 255}, {255, 64, 64, 255}};
    float sizes[2] = {20.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_visual_set_depth_cue(
           visual,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_depth_cue_darkens_far skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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


/**
 * Ensure the default point shader's analytic edge coverage reaches the color target.
 *
 * This catches regressions where the shader computes fractional edge alpha but the point draw uses
 * a non-blended opaque pipeline, producing a binary/aliased circle.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_point_default_edge_has_fractional_pixels(
    TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_rgba(0, 0, 0, 255));

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float size[1] = {28.0f};
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_point_default_edge_has_fractional_pixels skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t bright_count = 0;
    uint32_t fractional_count = 0;
    for (uint32_t y = 16; y < 48; y++)
    {
        for (uint32_t x = 16; x < 48; x++)
        {
            const uint8_t* px = _pixel_at(rgba, width, height, x, y);
            if (px[0] > 240 && px[1] > 240 && px[2] > 240)
                bright_count++;
            if (
                px[0] > 8 && px[0] < 240 && abs((int)px[0] - (int)px[1]) <= 3 &&
                abs((int)px[0] - (int)px[2]) <= 3)
                fractional_count++;
        }
    }
    AT(bright_count > 100);
    AT(fractional_count >= 8);

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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_has_nonblank_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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
 * Ensure a sharp stroked path join contributes pixels at the join center.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_path_join_has_no_center_gap(TstContext* suite, const TstCase* item)
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
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.75f, -0.45f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.75f, -0.45f, 0.0f},
    };
    DvzColor colors[3] = {
        {255, 0, 0, 255},
        {255, 0, 0, 255},
        {255, 0, 0, 255},
    };
    float widths[3] = {20.0f, 20.0f, 20.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", widths, 3) == 0);
    AT(dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_path_join_has_no_center_gap skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);
    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint8_t* center = _pixel_at(rgba, width, height, width / 2, height / 2);
    AT(center[0] > 120);
    AT(center[0] > center[1] + 40);
    AT(center[0] > center[2] + 40);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Count red-dominant captured pixels.
 *
 * @param rgba captured RGBA8 pixels
 * @param pixel_count number of pixels
 * @return red-dominant pixel count
 */
static void _app_red_dominant_pixel_stats(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t* out_count,
    uint64_t* out_signature)
{
    ANN(rgba);
    ANN(out_count);
    ANN(out_signature);
    uint32_t count = 0;
    uint64_t signature = 0;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            const uint32_t i = y * width + x;
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 80 && px[0] > px[1] + 40 && px[0] > px[2] + 40)
            {
                count++;
                signature += (uint64_t)(x + 1u) * 1315423911ull + (uint64_t)(y + 1u) * 2654435761ull;
            }
        }
    }
    *out_count = count;
    *out_signature = signature;
}



/**
 * Render one sharp path join and count its red pixels.
 *
 * @param suite the test suite
 * @param join path join mode
 * @param out_count output red-dominant pixel count
 * @return 0 on success, -1 on skipped GPU setup
 */
static int _app_render_path_join_stats(
    TstContext* suite, DvzPathJoin join, uint32_t* out_count, uint64_t* out_signature)
{
    ANN(suite);
    ANN(out_count);
    ANN(out_signature);
    *out_count = 0;
    *out_signature = 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 128, 128, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);
    vec3 positions[5] = {
        {-0.82f, -0.56f, 0.0f},
        {-0.30f, 0.48f, 0.0f},
        {0.0f, -0.54f, 0.0f},
        {0.30f, 0.48f, 0.0f},
        {0.82f, -0.56f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 0, 0, 255},
        {255, 0, 0, 255},
        {255, 0, 0, 255},
        {255, 0, 0, 255},
        {255, 0, 0, 255},
    };
    float widths[5] = {26.0f, 26.0f, 26.0f, 26.0f, 26.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", widths, 5) == 0);
    const float miter_limit = join == DVZ_PATH_JOIN_MITER ? 20.0f : 4.0f;
    AT(dvz_path_set_join(visual, join, miter_limit) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        dvz_scene_destroy(scene);
        return -1;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 128, 128);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);
    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 128);
    _app_red_dominant_pixel_stats(rgba, width, height, out_count, out_signature);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure path join modes have distinct stable silhouettes.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_path_join_modes_are_ordered(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    uint32_t miter_count = 0;
    uint32_t round_count = 0;
    uint32_t bevel_count = 0;
    uint64_t miter_sig = 0;
    uint64_t round_sig = 0;
    uint64_t bevel_sig = 0;
    if (_app_render_path_join_stats(suite, DVZ_PATH_JOIN_MITER, &miter_count, &miter_sig) != 0 ||
        _app_render_path_join_stats(suite, DVZ_PATH_JOIN_ROUND, &round_count, &round_sig) != 0 ||
        _app_render_path_join_stats(suite, DVZ_PATH_JOIN_BEVEL, &bevel_count, &bevel_sig) != 0)
    {
        log_warn("path join mode silhouette test skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    AT(miter_count > 0);
    AT(round_count > 0);
    AT(bevel_count > 0);
    AT(miter_count != round_count || miter_sig != round_sig);
    AT(miter_count != bevel_count || miter_sig != bevel_sig);
    AT(round_count != bevel_count || round_sig != bevel_sig);
    return 0;
}


/**
 * Count red-dominant pixels in a square capture neighborhood.
 *
 * @param rgba captured RGBA8 pixels
 * @param width capture width
 * @param height capture height
 * @param cx center x pixel coordinate
 * @param cy center y pixel coordinate
 * @param radius square half-size in pixels
 * @return red-dominant pixel count
 */
static uint32_t _app_red_neighborhood_count(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t cx, uint32_t cy,
    uint32_t radius)
{
    ANN(rgba);
    uint32_t count = 0;
    const uint32_t x0 = cx > radius ? cx - radius : 0;
    const uint32_t y0 = cy > radius ? cy - radius : 0;
    const uint32_t x1 = cx + radius < width ? cx + radius : width - 1u;
    const uint32_t y1 = cy + radius < height ? cy + radius : height - 1u;
    for (uint32_t y = y0; y <= y1; y++)
    {
        for (uint32_t x = x0; x <= x1; x++)
        {
            const uint8_t* px = _pixel_at(rgba, width, height, x, y);
            if (px[0] > 100 && px[0] > px[1] + 45 && px[0] > px[2] + 45)
                count++;
        }
    }
    return count;
}



/**
 * Ensure a closed sharp star ring draws pixels at the repeated-endpoint seam.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_path_closed_star_seam_has_pixels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 128, 128, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);
    enum
    {
        STAR_POINT_COUNT = 11,
    };
    const float tau = 6.28318530718f;
    vec3 positions[STAR_POINT_COUNT] = {{0}};
    DvzColor colors[STAR_POINT_COUNT] = {{0}};
    float widths[STAR_POINT_COUNT] = {0};
    for (uint32_t i = 0; i < STAR_POINT_COUNT; i++)
    {
        const uint32_t k = i % 10u;
        const float radius = (k % 2u) == 0u ? 0.56f : 0.17f;
        const float a = -0.25f * tau + tau * (float)k / 10.0f;
        positions[i][0] = radius * cosf(a);
        positions[i][1] = radius * sinf(a);
        positions[i][2] = 0.0f;
        colors[i] = (DvzColor){255, 0, 0, 255};
        widths[i] = 30.0f;
    }
    uint32_t subpath = STAR_POINT_COUNT;
    AT(dvz_visual_set_data(visual, "position", positions, STAR_POINT_COUNT) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, STAR_POINT_COUNT) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", widths, STAR_POINT_COUNT) == 0);
    AT(dvz_path_set_subpaths(visual, 1, &subpath) == 0);
    AT(dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_path_closed_star_seam_has_pixels skipped: GPU context creation "
            "failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 128, 128);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);
    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 128);

    const uint32_t x = width / 2u;
    const uint32_t y_a = (uint32_t)((1.0f - positions[0][1]) * 0.5f * (float)(height - 1u));
    const uint32_t y_b = (uint32_t)((1.0f + positions[0][1]) * 0.5f * (float)(height - 1u));
    const uint32_t seam_count_a = _app_red_neighborhood_count(rgba, width, height, x, y_a, 5);
    const uint32_t seam_count_b = _app_red_neighborhood_count(rgba, width, height, x, y_b, 5);
    AT(seam_count_a > 0 || seam_count_b > 0);

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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_pixel_square_has_nonblank_pixels skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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
    vec3 positions[4] = {
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));
    AT(dvz_panel_set_edl(
        panel, &(DvzEdlDesc){DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 65.0f, .depth_scale = 1.0f}));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_points_edl_renders skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    AppRgbaCapture disabled = _app_edl_point_capture(suite, false);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_points_edl_changes_pixels skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppRgbaCapture enabled = _app_edl_point_capture(suite, true);
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.03f, 0.035f, 0.045f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_mesh_ssao_changes_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 96, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 3.0f, .strength = 8.0f, .bias = 0.0f,
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
    vec3 positions[4] = {
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
    AT(dvz_visual_set_data(sphere, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radii, 4) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.03f, 0.035f, 0.045f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_sphere_ssao_darkens_contact skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 96, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);
    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 96);
    AT(height0 == 96);

    AT(dvz_panel_set_ssao(
        panel, &(DvzSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSsaoDesc), .radius = 3.0f, .strength = 8.0f, .bias = 0.0f,
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
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    const char* path = "/tmp/dvz_app_offscreen_recording.dvzr";
    const char* old_record_fps = getenv("DVZ_DRP2_RECORD_FPS");
    bool had_record_fps = old_record_fps != NULL;
    char saved_record_fps[64] = {0};
    if (had_record_fps)
        dvz_strlcpy(saved_record_fps, old_record_fps, sizeof(saved_record_fps));
    AT(tst_setenv("DVZ_DRP2_RECORD_FPS", "0") == 0);
    int record_start = dvz_view_record_start(win, path);
    if (had_record_fps)
        (void)tst_setenv("DVZ_DRP2_RECORD_FPS", saved_record_fps);
    else
        (void)tst_unsetenv("DVZ_DRP2_RECORD_FPS");
    AT(record_start == 0);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_view_record_stop(win) == 0);

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

    DvzFigure* replay_figure = dvz_figure(scene, 64, 64, 0);
    AT(replay_figure != NULL);
    DvzView* replay = dvz_view_offscreen(app, replay_figure, 64, 64);
    AT(replay != NULL);
    AT(dvz_view_replay_start(replay, path) == 0);
    dvz_view_replay_set_paced(replay, false);
    AT(dvz_view_replay_frame_count(replay) == 3);
    AT(dvz_view_render_once(replay) == DVZ_CANVAS_FRAME_READY);

    DvzCanvas* replay_canvas = dvz_view_canvas(replay);
    ANN(replay_canvas);
    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(replay_canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);
    uint32_t yellow_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200)
            yellow_count++;
    }
    AT(yellow_count > 0);
    dvz_free(rgba);
    AT(dvz_view_replay_stop(replay) == 0);

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
    vec3 positions[4] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    vec2 texcoords[4] = {
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_has_nonblank_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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
 * Ensure a scene-generated colorbar renders visible ramp and text pixels offscreen.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_colorbar_has_visible_ramp_and_labels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 256, 192, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    AT(panel != NULL);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS, .label = "Intensity"});
    AT(scale != NULL);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    AT(colormap != NULL);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    vec3 positions[4] = {
        {-0.95f, -0.95f, 0.0f}, {-0.95f, 0.95f, 0.0f},
        {0.95f, -0.95f, 0.0f},  {0.95f, 0.95f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    int rc = dvz_visual_set_data(image, "position", positions, 4);
    AT(rc == 0);
    rc = dvz_visual_set_data(image, "texcoords", texcoords, 4);
    AT(rc == 0);
    rc = dvz_visual_set_scale(image, "color", scale);
    AT(rc == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 8,
                   .height = 8,
                   .depth = 1,
               });
    AT(field != NULL);
    float values[8 * 8] = {0};
    for (uint32_t y = 0; y < 8; y++)
    {
        for (uint32_t x = 0; x < 8; x++)
            values[y * 8 + x] = (float)x / 7.0f;
    }
    bool ok = dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = values,
                   .bytes_per_row = 8 * sizeof(float),
                   .rows_per_image = 8,
               });
    AT(ok);
    ok = dvz_visual_set_field(image, "field", field);
    AT(ok);
    rc = dvz_panel_add_visual(panel, image, NULL);
    AT(rc == 0);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Intensity",
        });
    AT(colorbar != NULL);
    dvz_colorbar_set_format(colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 1});
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_colorbar_has_visible_ramp_and_labels skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 256, 192);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
    AT(canvas != NULL);

    for (uint32_t frame = 0; frame < 3; frame++)
        dvz_app_run(app, 1);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    rc = dvz_canvas_capture_rgba(canvas, &width, &height, &rgba);
    AT(rc == 0);
    AT(rgba != NULL);
    AT(width == 256);
    AT(height == 192);

    uint32_t red_count = 0;
    uint32_t blue_count = 0;
    uint32_t light_count = 0;
    uint32_t dark_count = 0;
    for (uint32_t y = 12; y < height - 12; y++)
    {
        for (uint32_t x = width / 2; x < width - 4; x++)
        {
            const uint8_t* px = _pixel_at(rgba, width, height, x, y);
            if (px[0] > 150 && px[1] < 120 && px[2] < 120)
                red_count++;
            if (px[2] > 150 && px[0] < 120 && px[1] < 120)
                blue_count++;
            if (px[0] > 150 && px[1] > 150 && px[2] > 150)
                light_count++;
            if (px[0] < 30 && px[1] < 30 && px[2] < 30)
                dark_count++;
        }
    }
    AT(red_count > 8);
    AT(blue_count > 8);
    AT(light_count > 8);
    AT(dark_count > 8);

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

    DvzVisual* text = _scene_text_visual(scene, 0);
    AT(text != NULL);
    const char* strings[1] = {"HI"};
    vec3 positions[1] = {{8.0f, 8.0f, 0.0f}};
    vec2 text_anchors[1] = {{0.0f, 0.0f}};
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
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_text_has_nonblank_pixels skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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

    DvzVisual* text = _scene_text_visual(scene, 0);
    AT(text != NULL);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"UTF-8 fallback: A?B cafe? -> ?"};
    vec3 positions[1] = {{6.0f, 12.0f, 0.0f}};
    vec2 text_anchors[1] = {{0.0f, 0.0f}};
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
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_sdf_text_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 256, 72);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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
    uint32_t sparse_green = 0;
    uint32_t bounds[4] = {0};
    bool has_bounds =
        _visual_family_state(text)->text.glyph_visual != NULL &&
        _app_glyph_pixel_bounds(_visual_family_state(text)->text.glyph_visual, width, height, bounds);
    AT(has_bounds);
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
                uint32_t neighbor_count = 0;
                for (int32_t dy = -1; dy <= 1; dy++)
                {
                    for (int32_t dx = -1; dx <= 1; dx++)
                    {
                        if (dx == 0 && dy == 0)
                            continue;
                        const uint8_t* neighbor = _pixel_at(
                            rgba, width, height, (uint32_t)((int32_t)x + dx),
                            (uint32_t)((int32_t)y + dy));
                        if (_app_text_green_pixel(neighbor))
                        {
                            connected = true;
                            neighbor_count++;
                        }
                    }
                }
                if (!connected)
                    isolated_green++;
                if (neighbor_count <= 2u)
                    sparse_green++;
            }
        }
        uint32_t bounds_pixels = (bounds[2] - bounds[0]) * (bounds[3] - bounds[1]);
        AT(bounds_pixels > 0);
        AT(green_in_bounds * 10u < bounds_pixels * 7u);
        AT(green_in_bounds > 32u);
        AT(isolated_green * 10u < green_in_bounds);
        AT(sparse_green * 15u < green_in_bounds);
    }

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure CPU-rasterized text blocks render through the image visual offscreen path.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_text_block_raster_has_nonblank_pixels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    AT(panel != NULL);

    DvzTextBlock block = {0};
    _scene_text_block_init(&block, "Rich <b>block</b> <i>card</i>");
    AT(_scene_text_block_parse(&block) == 0);
    AT(_scene_text_block_measure(
           &block,
           &(DvzTextBlockLayout){
               .scene = scene,
               .max_width_px = 96.0f,
               .font_size_px = 12.0f,
               .line_height_px = 15.0f,
               .padding_px = {3.0f, 3.0f},
           }) == 0);
    AT(_scene_text_block_rasterize(
           &block,
           &(DvzTextBlockRasterDesc){
               .scene = scene,
               .text_color = {0, 255, 0, 255},
               .background_color = {0, 0, 0, 0},
           }) == 0);
    AT(_scene_text_block_realize_image(
           &block, panel,
           &(DvzTextBlockImageDesc){
               .position_px = {12.0f, 12.0f, 0.0f},
               .extent_px = {(float)block.raster_width, (float)block.raster_height},
               .anchor = {-1.0f, -1.0f},
               .pixel_space = true,
               .z_layer = 1,
               .controller_mode = DVZ_CONTROLLER_FIXED,
           }) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_text_block_raster_has_nonblank_pixels skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        _scene_text_block_destroy(&block);
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 128, 96);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 128);
    AT(height == 96);

    uint32_t green_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &rgba[4 * i];
        if (_app_text_green_pixel(pixel))
            green_count++;
    }
    AT(green_count > 0);

    dvz_free(rgba);
    _scene_text_block_destroy(&block);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure public rich overlay cards render their background and image-backed text.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_overlay_rich_card_has_visible_pixels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 256, 160, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    AT(panel != NULL);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    AT(overlay != NULL);
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = dvz_color_rgba(180, 20, 20, 255);
    style.padding_px[0] = 10.0f;
    style.padding_px[1] = 8.0f;
    style.min_width_px = 120.0f;
    DvzOverlayCard* card = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "fallback",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT,
            .offset_px = {12.0f, 10.0f},
            .style = &style,
        });
    AT(card != NULL);
    AT(dvz_overlay_card_set_rich_text(
           card,
           &(DvzOverlayRichTextDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayRichTextDesc),
               .source = "<b>Rich card</b> visible",
               .max_width_px = 112.0f,
               .char_width_px = 7.0f,
               .line_height_px = 14.0f,
               .scale = 2.0f,
               .text_color = {0, 255, 0, 255},
               .background_color = {0, 0, 0, 0},
           }) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_overlay_rich_card_has_visible_pixels skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 256, 160);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 256);
    AT(height == 160);

    uint32_t red_count = 0;
    uint32_t green_count = 0;
    for (uint32_t y = height / 2u; y < height; y++)
    {
        for (uint32_t x = width / 2u; x < width; x++)
        {
            const uint8_t* pixel = _pixel_at(rgba, width, height, x, y);
            if (pixel[0] > 120 && pixel[1] < 80 && pixel[2] < 80)
                red_count++;
            if (_app_text_green_pixel(pixel))
                green_count++;
        }
    }
    AT(red_count > 100u);
    AT(green_count > 8u);

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

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
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
    vec3 positions[4] = {
        {-0.95f, -0.95f, 0.0f}, {-0.95f, 0.95f, 0.0f},
        {0.95f, -0.95f, 0.0f},  {0.95f, 0.95f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "color", scale) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_image_field_partial_update_changes_region skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
        &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
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

    AppWboitCapture forward = _app_wboit_capture_center(suite, false);
    if (forward.skipped)
    {
        log_warn(
            "test_app_offscreen_wboit_mesh_order_independent_layers skipped: GPU context failed");
        tst_skip(suite, forward.skip_reason);
        return 0;
    }
    AppWboitCapture reverse = _app_wboit_capture_center(suite, true);
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_source_over_mesh_depth_and_blend skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(blended[0] > background.r + 60);
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

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
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    AT(red_region_r > ((uint64_t)background.r + 45) * region_pixels);
    AT(red_region_r > red_region_b + 30 * region_pixels);
    AT(blue_region_b > ((uint64_t)background.b + 45) * region_pixels);
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
 * Ensure depth peeling accumulates a third layer between nearest and farthest transparent quads.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_depth_peel_mesh_three_layers(TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzColor background = {0, 0, 0, 255};
    DvzColor red = {255, 0, 0, 128};
    DvzColor green = {0, 255, 0, 128};
    DvzColor blue = {0, 0, 255, 128};
    DvzVisual* background_visual = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.9f, background, DVZ_ALPHA_OPAQUE, true);
    DvzVisual* red_visual = _app_primitive_add_quad(
        scene, panel, -0.75f, 0.75f, -0.75f, 0.75f, 0.2f, red, DVZ_ALPHA_DEPTH_PEEL, true);
    DvzVisual* green_visual = _app_primitive_add_quad(
        scene, panel, -0.75f, 0.75f, -0.75f, 0.75f, 0.45f, green, DVZ_ALPHA_DEPTH_PEEL, true);
    DvzVisual* blue_visual = _app_primitive_add_quad(
        scene, panel, -0.75f, 0.75f, -0.75f, 0.75f, 0.7f, blue, DVZ_ALPHA_DEPTH_PEEL, true);
    ANN(background_visual);
    ANN(red_visual);
    ANN(green_visual);
    ANN(blue_visual);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_depth_peel_mesh_three_layers skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    for (uint32_t frame = 0; frame < 3; frame++)
        dvz_app_run(app, 1);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    const uint32_t region_pixels = 12 * 12;
    uint64_t region_r = _app_rgb_region_channel_sum(rgba, width, height, 26, 26, 38, 38, 0);
    uint64_t region_g = _app_rgb_region_channel_sum(rgba, width, height, 26, 26, 38, 38, 1);
    uint64_t region_b = _app_rgb_region_channel_sum(rgba, width, height, 26, 26, 38, 38, 2);

    AT(region_r > 100 * region_pixels);
    AT(region_g > 45 * region_pixels);
    AT(region_b > 20 * region_pixels);
    AT(region_r > region_g);
    AT(region_g > region_b);

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
        _app_source_over_scene_occlusion_capture_center(suite, true, true, 255);
    if (hidden.skipped)
    {
        log_warn("test_app_offscreen_scene_occlusion_hidden_alpha skipped: GPU context failed");
        tst_skip(suite, hidden.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture zero =
        _app_source_over_scene_occlusion_capture_center(suite, true, false, 0);
    if (zero.skipped)
    {
        log_warn("test_app_offscreen_scene_occlusion_hidden_alpha skipped: GPU context failed");
        tst_skip(suite, zero.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture positive =
        _app_source_over_scene_occlusion_capture_center(suite, true, false, 64);
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
        _app_source_over_scene_occlusion_capture_center(suite, false, false, 64);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_source_over_scene_occlusion_matrix skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture hidden =
        _app_source_over_scene_occlusion_capture_center(suite, true, true, 255);
    if (hidden.skipped)
    {
        log_warn("test_app_offscreen_source_over_scene_occlusion_matrix skipped: GPU context failed");
        tst_skip(suite, hidden.skip_reason);
        return 0;
    }
    AppSceneOcclusionCapture enabled =
        _app_source_over_scene_occlusion_capture_center(suite, true, false, 64);
    if (enabled.skipped)
    {
        log_warn("test_app_offscreen_source_over_scene_occlusion_matrix skipped: GPU context failed");
        tst_skip(suite, enabled.skip_reason);
        return 0;
    }

    AT(disabled.rgb[0] > 120);
    AT(hidden.rgb[0] > 140);
    AT(disabled.rgb[1] > 50);
    AT(hidden.rgb[0] > disabled.rgb[0] + 24);
    AT(hidden.rgb[1] + 25 < disabled.rgb[1]);

    uint32_t diff = 0;
    for (uint32_t i = 0; i < 3; i++)
        diff += disabled.rgb[i] > enabled.rgb[i] ? disabled.rgb[i] - enabled.rgb[i] :
                                                   enabled.rgb[i] - disabled.rgb[i];
    bool matches_disabled = diff <= 6;
    bool visibly_occluded = enabled.rgb[1] > 45 && enabled.rgb[0] + 40 < disabled.rgb[0];
    AT(matches_disabled || visibly_occluded);
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

    vec3 near_positions[6] = {
        {-0.9f, -0.9f, 0.1f}, {-0.9f, 0.9f, 0.1f},  {0.9f, -0.9f, 0.1f},
        {0.9f, -0.9f, 0.1f},  {-0.9f, 0.9f, 0.1f},  {0.9f, 0.9f, 0.1f},
    };
    vec3 far_positions[6] = {
        {-0.9f, -0.9f, 0.8f}, {-0.9f, 0.9f, 0.8f},  {0.9f, -0.9f, 0.8f},
        {0.9f, -0.9f, 0.8f},  {-0.9f, 0.9f, 0.8f},  {0.9f, 0.9f, 0.8f},
    };
    vec3 normals[6] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzColor near_colors[6];
    DvzColor far_colors[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        near_colors[i] = dvz_color_rgba(32, 64, 255, 255);
        far_colors[i] = dvz_color_rgba(255, 32, 32, 255);
    }

    AT(dvz_visual_set_data(near_visual, "position", near_positions, 6) == 0);
    AT(dvz_visual_set_data(near_visual, "color", near_colors, 6) == 0);
    AT(dvz_visual_set_data(near_visual, "normal", normals, 6) == 0);
    AT(dvz_panel_add_visual(panel, near_visual, NULL) == 0);
    AT(_test_set_phong_material(
           near_visual, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);

    AT(dvz_visual_set_data(far_visual, "position", far_positions, 6) == 0);
    AT(dvz_visual_set_data(far_visual, "color", far_colors, 6) == 0);
    AT(dvz_visual_set_data(far_visual, "normal", normals, 6) == 0);
    AT(dvz_panel_add_visual(panel, far_visual, NULL) == 0);
    AT(_test_set_phong_material(
           far_visual, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_lit_primitive_depth_orders_overlap skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    vec3 positions[12] = {
        {-0.9f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f}, {-0.1f, -0.8f, 0.0f},
        {-0.1f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f},
        {0.1f, -0.8f, 0.8f},  {0.1f, 0.8f, 0.8f},  {0.9f, -0.8f, 0.8f},
        {0.9f, -0.8f, 0.8f},  {0.1f, 0.8f, 0.8f},  {0.9f, 0.8f, 0.8f},
    };
    vec3 normals[12];
    DvzColor colors[12];
    for (uint32_t i = 0; i < 12; i++)
    {
        normals[i][0] = 0.0f;
        normals[i][1] = 0.0f;
        normals[i][2] = 1.0f;
        colors[i] = dvz_color_rgba(255, 48, 48, 255);
    }

    AT(dvz_visual_set_data(visual, "position", positions, 12) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 12) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 12) == 0);
    AT(_test_set_phong_material(
           visual, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);
    AT(dvz_visual_set_depth_cue(
           visual,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.50f,
               .far_depth = 0.95f,
               .strength = 1.0f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_lit_primitive_depth_cue_darkens_far skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 64, 64, 255},
        {64, 255, 64, 255},
        {64, 64, 255, 255},
        {255, 224, 64, 255},
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
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(_test_set_phong_material(
           visual, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_mesh_renders_nonblank skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    vec3 positions[24], DvzColor colors[24], vec3 normals[24], DvzIndex indices[36])
{
    const float s = 0.58f;
    const vec3 face_positions[6][4] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const vec3 face_normals[6] = {
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
            colors[vertex] = face_colors[face];
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
    vec3 positions[24], DvzColor colors[24], vec3 normals[24], DvzIndex indices[36])
{
    const float s = 0.58f;
    const vec3 face_positions[6][4] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const vec3 face_normals[6] = {
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
            colors[vertex] = face_colors[face];
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

    vec3 positions[24] = {0};
    DvzColor colors[24] = {0};
    vec3 normals[24] = {0};
    DvzIndex indices[36] = {0};
    _rotated_mesh_build_cube(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
    AT(_test_set_phong_material(
           visual, (float[3]){0.35f, 0.55f, 0.75f}, 0.25f, 0.85f, 0.25f, 32.0f) ==
       0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_rotated_mesh_depth_orders_faces skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 128, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    camera_desc.view.eye[2] = 3.0f;
    camera_desc.projection.fov_y = GLM_PI_4f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);
    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    ANN(arcball_controller);
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0);
    dvz_arcball_initial(arcball, (vec3){+0.6f, -1.2f, +3.0f});

    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[24] = {0};
    DvzColor colors[24] = {0};
    vec3 normals[24] = {0};
    DvzIndex indices[36] = {0};
    _mesh_build_cube_object_space(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
    AT(_test_set_phong_material(
           visual, (float[3]){0.35f, 0.55f, 0.75f}, 0.25f, 0.85f, 0.25f, 32.0f) ==
       0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_camera_arcball_mesh_renders_cube skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 128, 96);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
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
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));

    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    vec3 left_positions[4] = {
        {-1.0f, -0.95f, 0.0f}, {-1.0f, 0.95f, 0.0f},
        {0.0f, -0.95f, 0.0f},  {0.0f, 0.95f, 0.0f},
    };
    vec3 right_positions[4] = {
        {0.0f, -0.95f, 0.0f}, {0.0f, 0.95f, 0.0f},
        {1.0f, -0.95f, 0.0f}, {1.0f, 0.95f, 0.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", left_positions, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "color", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", right_positions, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "color", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_shared_field_mixed_runtime_updates skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 96, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
        &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_retained_render_second_frame skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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

    vec3 positions[4] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    vec2 texcoords[4] = {
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_retained_render_second_frame skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));

    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* image = dvz_image(scene, 0);
    AT(mesh != NULL);
    AT(image != NULL);

    vec3 mesh_positions[4] = {
        {-0.9f, -0.8f, 0.0f}, {-0.9f, 0.8f, 0.0f},
        {-0.1f, -0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f},
    };
    DvzColor mesh_colors[4] = {
        {64, 255, 64, 255},
        {64, 255, 64, 255},
        {64, 255, 64, 255},
        {64, 255, 64, 255},
    };
    vec3 mesh_normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)));

    AT(dvz_visual_set_data(mesh, "position", mesh_positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", mesh_colors, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(_test_set_phong_material(
           mesh, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    vec3 image_positions[4] = {
        {0.1f, -0.8f, 0.0f}, {0.1f, 0.8f, 0.0f},
        {0.9f, -0.8f, 0.0f}, {0.9f, 0.8f, 0.0f},
    };
    vec2 image_texcoords[4] = {
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_resize_reuses_runtime_with_mesh_and_image skipped: GPU context "
            "creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 96, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
        AT(dvz_view_resize(win, expected_width, expected_height) == 0);
        AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);

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
 * Ensure app-owned request execution stays steady across repeated query frames.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_query_request_steady_state(TstContext* suite, const TstCase* item)
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
    dvz_visual_set_query_capabilities(points, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_size[1] = {24.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "size", point_size, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
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
    AT(dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = -1}) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_query_request_steady_state skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    for (uint32_t frame = 0; frame < 8; frame++)
    {
        uint64_t first_id = 100 + frame;
        uint64_t second_id = 200 + frame;
        AT(dvz_panel_query(
               panel, 32.0, 32.0,
               &(DvzQueryRequest){DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest), .request_id = first_id, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
        AT(dvz_panel_query(
               panel, 32.0, 32.0,
               &(DvzQueryRequest){DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest), .request_id = second_id, .target = DVZ_SCENE_TARGET_ITEM}) == 0);
        AT(scene->pending_query_count == 2);

        AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
        AT(scene->pending_query_count == 0);

        DvzQueryResult first = {0};
        DvzQueryResult second = {0};
        AT(dvz_scene_poll_query(scene, &first));
        AT(dvz_scene_poll_query(scene, &second));
        AT(first.hit);
        AT(second.hit);
        AT(first.request_id == first_id || first.request_id == second_id);
        AT(second.request_id == first_id || second.request_id == second_id);
        AT(first.request_id != second.request_id);
        AT(first.resolved_target == DVZ_SCENE_TARGET_ITEM);
        AT(second.resolved_target == DVZ_SCENE_TARGET_ITEM);
        AT(first.resolved_id == 0);
        AT(second.resolved_id == 0);

        AT(!dvz_scene_poll_query(scene, &second));
        AT(scene->query_result_count == 0);
    }

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Exercise the first public C surface expected by a GSP adapter.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_gsp_first_slice_smoke(TstContext* suite, const TstCase* item)
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

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        pixels[4 * i + 0] = 220;
        pixels[4 * i + 1] = 40;
        pixels[4 * i + 2] = 40;
        pixels[4 * i + 3] = 255;
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, image,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = -1}) ==
       0);

    DvzVisual* points = dvz_point(scene, 0);
    AT(points != NULL);
    dvz_visual_set_query_capabilities(points, DVZ_QUERY_CAPABILITY_ITEM);
    vec3 point_pos[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor point_color[1] = {{255, 255, 0, 255}};
    float point_diameter[1] = {20.0f};
    AT(dvz_visual_set_data(points, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(points, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(points, "diameter_px", point_diameter, 1) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    const DvzId scene_id = dvz_scene_id(scene);
    const DvzId figure_id = dvz_figure_id(figure);
    const DvzId panel_id = dvz_panel_id(panel);
    const DvzId image_id = dvz_visual_id(image);
    const DvzId points_id = dvz_visual_id(points);
    AT(scene_id != DVZ_ID_NONE);
    AT(figure_id != DVZ_ID_NONE);
    AT(panel_id != DVZ_ID_NONE);
    AT(image_id != DVZ_ID_NONE);
    AT(points_id != DVZ_ID_NONE);
    AT(image_id != points_id);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_gsp_first_slice_smoke skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_view_capture_png(win, "/tmp/dvz_gsp_first_slice_smoke.png") == 0);

    DvzQueryRequest point_request = dvz_query_request();
    point_request.request_id = 1001;
    point_request.target = DVZ_SCENE_TARGET_ITEM;
    DvzQueryRequest image_request = dvz_query_request();
    image_request.request_id = 1002;
    image_request.target = DVZ_SCENE_TARGET_PIXEL;
    AT(dvz_panel_query(panel, 32.0, 32.0, &point_request) == 0);
    AT(dvz_panel_query(panel, 8.0, 8.0, &image_request) == 0);

    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);

    DvzQueryResult first = {0};
    DvzQueryResult second = {0};
    AT(dvz_scene_poll_query(scene, &first));
    AT(dvz_scene_poll_query(scene, &second));
    AT(!dvz_scene_poll_query(scene, &second));

    DvzQueryResult* point_result = first.request_id == point_request.request_id ? &first : &second;
    DvzQueryResult* image_result = first.request_id == image_request.request_id ? &first : &second;
    AT(point_result != image_result);

    AT(point_result->hit);
    AT(point_result->scene_id == scene_id);
    AT(point_result->figure_id == figure_id);
    AT(point_result->panel_id == panel_id);
    AT(point_result->visual_id == points_id);
    AT(point_result->visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT);
    AT(point_result->resolved_target == DVZ_SCENE_TARGET_ITEM);
    AT(point_result->item_id == 0);

    AT(image_result->hit);
    AT(image_result->scene_id == scene_id);
    AT(image_result->figure_id == figure_id);
    AT(image_result->panel_id == panel_id);
    AT(image_result->visual_id == image_id);
    AT(image_result->visual_family == DVZ_SCENE_VISUAL_FAMILY_IMAGE);
    AT(image_result->resolved_target == DVZ_SCENE_TARGET_PIXEL);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



static uint8_t _test_pixel_luma(const uint8_t* pixel)
{
    ANN(pixel);
    return (uint8_t)(((uint32_t)pixel[0] + (uint32_t)pixel[1] + (uint32_t)pixel[2]) / 3);
}



static bool _test_pixel_red_point(const uint8_t* pixel)
{
    ANN(pixel);
    return pixel[0] > 160 && pixel[0] > pixel[1] + 80 && pixel[0] > pixel[2] + 80;
}



static bool _test_pixel_pale_halo(const uint8_t* pixel)
{
    ANN(pixel);
    return pixel[0] > 120 && pixel[1] > 120 && pixel[2] > 120;
}



/**
 * Render the GSP checker/image + filled point overlay case and assert that nearest sampling and
 * zero-stroke point styling are visible in readback pixels.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_gsp_image_nearest_point_no_stroke_smoke(
    TstContext* suite, const TstCase* item)
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

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    AT(dvz_image_set_sampling(image, DVZ_IMAGE_SAMPLING_NEAREST) == 0);
    vec3 image_pos[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    DvzColor pixels[4 * 4] = {0};
    for (uint32_t y = 0; y < 4; y++)
    {
        for (uint32_t x = 0; x < 4; x++)
        {
            const bool central_dark = (x == 1 || x == 2) && (y == 1 || y == 2);
            const bool dark = central_dark || ((x + y) % 2 == 0);
            pixels[y * 4 + x] = dark ? (DvzColor){8, 8, 8, 255}
                                     : (DvzColor){245, 245, 245, 255};
        }
    }
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, image,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = -1}) ==
       0);

    DvzVisual* point = dvz_point(scene, 0);
    AT(point != NULL);
    vec3 point_pos[1] = {{-0.25f, -0.25f, 0.0f}};
    DvzColor point_color[1] = {{255, 0, 0, 255}};
    float diameter_px[1] = {8.0f};
    DvzPointStyleDesc style = dvz_point_style_desc();
    style.stroke_width_px = 0.0f;
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.edge_color = (DvzColor){255, 255, 255, 255};
    AT(dvz_point_set_style(point, &style) == 0);
    AT(dvz_visual_set_data(point, "position", point_pos, 1) == 0);
    AT(dvz_visual_set_data(point, "color", point_color, 1) == 0);
    AT(dvz_visual_set_data(point, "diameter_px", diameter_px, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_gsp_image_nearest_point_no_stroke_smoke skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    AT(dvz_view_render_once(win) == DVZ_CANVAS_FRAME_READY);
    AT(dvz_view_capture_png(win, "/tmp/dvz_gsp_image_nearest_point_no_stroke_smoke.png") == 0);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);
    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    uint32_t checker_dark = 0;
    uint32_t checker_light = 0;
    uint32_t checker_mid = 0;
    const uint32_t cell_centers[4] = {8, 24, 40, 56};
    for (uint32_t y_idx = 0; y_idx < 4; y_idx++)
    {
        for (uint32_t x_idx = 0; x_idx < 4; x_idx++)
        {
            const uint32_t x = cell_centers[x_idx];
            const uint32_t y = cell_centers[y_idx];
            const uint8_t* pixel = &rgba[4 * (y * width + x)];
            if (_test_pixel_red_point(pixel))
                continue;

            const uint8_t luma = _test_pixel_luma(pixel);
            if (luma < 35)
                checker_dark++;
            else if (luma > 210)
                checker_light++;
            else
                checker_mid++;
        }
    }
    AT(checker_dark >= 8);
    AT(checker_light >= 2);
    AT(checker_mid == 0);

    uint32_t red_count = 0;
    uint32_t min_x = width, min_y = height, max_x = 0, max_y = 0;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            const uint8_t* pixel = &rgba[4 * (y * width + x)];
            if (!_test_pixel_red_point(pixel))
                continue;
            red_count++;
            if (x < min_x)
                min_x = x;
            if (y < min_y)
                min_y = y;
            if (x > max_x)
                max_x = x;
            if (y > max_y)
                max_y = y;
        }
    }
    AT(red_count >= 20);
    AT(max_x > min_x);
    AT(max_y > min_y);

    const uint32_t ring_min_x = min_x > 3 ? min_x - 3 : 0;
    const uint32_t ring_min_y = min_y > 3 ? min_y - 3 : 0;
    const uint32_t ring_max_x = max_x + 3 < width ? max_x + 3 : width - 1;
    const uint32_t ring_max_y = max_y + 3 < height ? max_y + 3 : height - 1;
    uint32_t pale_ring = 0;
    for (uint32_t y = ring_min_y; y <= ring_max_y; y++)
    {
        for (uint32_t x = ring_min_x; x <= ring_max_x; x++)
        {
            if (x >= min_x && x <= max_x && y >= min_y && y <= max_y)
                continue;
            const uint8_t* pixel = &rgba[4 * (y * width + x)];
            if (_test_pixel_pale_halo(pixel))
                pale_ring++;
        }
    }
    AT(pale_ring == 0);

    dvz_free(rgba);
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_two_panel_points_light_both_halves skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 96, 64);
    AT(win != NULL);

    DvzCanvas* canvas = dvz_view_canvas(win);
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_clear_color skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);

    /* Default clear color is linear (0.05, 0.05, 0.08, 1.0), encoded by the SRGB target. */
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* px = &rgba[4 * i];
        if (px[0] > 96 || px[1] > 96 || px[2] > 112)
            bright_count++;
    }
    AT(bright_count == 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure semantic mid-gray RGBA8 colors read back as standard sRGB u8 screenshots.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_midgray_srgb_readback(TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzColor midgray = {128, 128, 128, 255};
    DvzVisual* visual = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.0f, midgray, DVZ_ALPHA_OPAQUE, false);
    AT(visual != NULL);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_midgray_srgb_readback skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(center[0] >= 108 && center[0] <= 148);
    AT(center[1] >= 108 && center[1] <= 148);
    AT(center[2] >= 108 && center[2] <= 148);
    AT(center[0] > 80 && center[1] > 80 && center[2] > 80);
    AT(center[0] < 168 && center[1] < 168 && center[2] < 168);
    AT(center[3] >= 240);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure legacy display-space colors are not encoded again by an SRGB offscreen target.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_legacy_srgb_blend_readback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_APP_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    dvz_figure_set_color_pipeline(figure, DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzColor display_red = {230, 57, 70, 255};
    DvzVisual* visual = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.0f, display_red, DVZ_ALPHA_OPAQUE,
        false);
    AT(visual != NULL);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_legacy_srgb_blend_readback skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(center[0] >= 215 && center[0] <= 245);
    AT(center[1] >= 45 && center[1] <= 75);
    AT(center[2] >= 55 && center[2] <= 85);
    AT(center[1] < 100);
    AT(center[3] >= 240);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure explicit linear-color RGBA8 fields are not decoded as sRGB textures.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_linear_color_field_not_decoded(TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    vec3 positions[4] = {
        {-0.95f, -0.95f, 0.0f},
        {-0.95f, 0.95f, 0.0f},
        {0.95f, -0.95f, 0.0f},
        {0.95f, 0.95f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .color_role = DVZ_COLOR_ROLE_LINEAR_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    DvzColor pixels[4] = {
        {128, 128, 128, 255},
        {128, 128, 128, 255},
        {128, 128, 128, 255},
        {128, 128, 128, 255},
    };
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = pixels,
                   .bytes_per_row = 2 * sizeof(DvzColor),
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzColor expected = dvz_color_from_linear(
        dvz_colorf(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_linear_color_field_not_decoded skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(abs((int)center[0] - (int)expected.r) <= 16);
    AT(abs((int)center[1] - (int)expected.g) <= 16);
    AT(abs((int)center[2] - (int)expected.b) <= 16);
    AT(center[0] > 160 && center[1] > 160 && center[2] > 160);
    AT(center[3] >= 240);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure source-over alpha compositing uses linear RGB over a nonblack background.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_alpha_over_nonblack_linear(TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzColor dst = {64, 64, 64, 255};
    DvzColor src = {192, 32, 32, 128};
    DvzVisual* background = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.0f, dst, DVZ_ALPHA_OPAQUE, false);
    DvzVisual* foreground = _app_primitive_add_quad(
        scene, panel, -0.75f, 0.75f, -0.75f, 0.75f, 0.0f, src, DVZ_ALPHA_BLENDED, false);
    AT(background != NULL);
    AT(foreground != NULL);

    DvzColorf dst_linear = dvz_color_to_linear(dst);
    DvzColorf src_linear = dvz_color_to_linear(src);
    float alpha = src_linear.a;
    DvzColor expected = dvz_color_from_linear(dvz_colorf(
        src_linear.r * alpha + dst_linear.r * (1.0f - alpha),
        src_linear.g * alpha + dst_linear.g * (1.0f - alpha),
        src_linear.b * alpha + dst_linear.b * (1.0f - alpha),
        1.0f));

    uint8_t naive_r = (uint8_t)((uint32_t)src.r * src.a / 255u +
                                (uint32_t)dst.r * (255u - src.a) / 255u);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_alpha_over_nonblack_linear skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(abs((int)center[0] - (int)expected.r) <= 12);
    AT(abs((int)center[1] - (int)expected.g) <= 12);
    AT(abs((int)center[2] - (int)expected.b) <= 12);
    AT(abs((int)center[0] - (int)naive_r) > 10);
    AT(center[3] >= 240);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure custom colormap RGBA8 entries are decoded before linear alpha blending.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_app_offscreen_colormap_srgb_lut_linear_blend(TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzColor dst = {64, 64, 64, 255};
    DvzColor src = {192, 32, 32, 128};
    DvzVisual* background = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.0f, dst, DVZ_ALPHA_OPAQUE, false);
    AT(background != NULL);

    DvzScale* scale =
        dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                             .kind = DVZ_SCALE_CONTINUOUS});
    AT(scale != NULL);
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColor colors[2] = {src, src};
    DvzColormap* colormap = dvz_colormap_custom(scene, "srgb-linear-blend", colors, 2);
    AT(colormap != NULL);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    AT(image != NULL);
    vec3 positions[4] = {
        {-0.75f, -0.75f, 0.0f},
        {-0.75f, 0.75f, 0.0f},
        {0.75f, -0.75f, 0.0f},
        {0.75f, 0.75f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    float values[4 * 4];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 0.5f;

    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "color", scale) == 0);
    AT(dvz_visual_set_texture_f32(image, values, 4, 4) == 0);
    AT(dvz_visual_set_alpha_mode(image, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzColorf dst_linear = dvz_color_to_linear(dst);
    DvzColorf src_linear = dvz_color_to_linear(src);
    float alpha = src_linear.a;
    DvzColor expected = dvz_color_from_linear(dvz_colorf(
        src_linear.r * alpha + dst_linear.r * (1.0f - alpha),
        src_linear.g * alpha + dst_linear.g * (1.0f - alpha),
        src_linear.b * alpha + dst_linear.b * (1.0f - alpha),
        1.0f));

    uint8_t naive_r = (uint8_t)((uint32_t)src.r * src.a / 255u +
                                (uint32_t)dst.r * (255u - src.a) / 255u);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_colormap_srgb_lut_linear_blend skipped: GPU context creation "
            "failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(abs((int)center[0] - (int)expected.r) <= 12);
    AT(abs((int)center[1] - (int)expected.g) <= 12);
    AT(abs((int)center[2] - (int)expected.b) <= 12);
    AT(abs((int)center[0] - (int)naive_r) > 10);
    AT(center[3] >= 240);

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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_wrong_dimensions skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_undersized_buffer skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_view_canvas(win);
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
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_slice_renders_field skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
 * Ensure RGBA8 volume color fields are decoded before source-over blending.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_rgba_srgb_linear_blend(TstContext* suite, const TstCase* item)
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzColor dst = {64, 64, 64, 255};
    DvzColor src = {192, 32, 32, 128};
    DvzVisual* background = _app_primitive_add_quad(
        scene, panel, -0.95f, 0.95f, -0.95f, 0.95f, 0.0f, dst, DVZ_ALPHA_OPAQUE, false);
    AT(background != NULL);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    DvzColor voxels[8] = {0};
    for (uint32_t i = 0; i < 8; i++)
        voxels[i] = src;
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = voxels,
                   .bytes_per_row = 2 * sizeof(DvzColor),
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzColorf dst_linear = dvz_color_to_linear(dst);
    DvzColorf src_linear = dvz_color_to_linear(src);
    float alpha = src_linear.a;
    DvzColor expected = dvz_color_from_linear(dvz_colorf(
        src_linear.r * alpha + dst_linear.r * (1.0f - alpha),
        src_linear.g * alpha + dst_linear.g * (1.0f - alpha),
        src_linear.b * alpha + dst_linear.b * (1.0f - alpha),
        1.0f));
    uint8_t naive_r = (uint8_t)((uint32_t)src.r * src.a / 255u +
                                (uint32_t)dst.r * (255u - src.a) / 255u);

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_rgba_srgb_linear_blend skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    AT(abs((int)center[0] - (int)expected.r) <= 16);
    AT(abs((int)center[1] - (int)expected.g) <= 16);
    AT(abs((int)center[2] - (int)expected.b) <= 16);
    AT(abs((int)center[0] - (int)naive_r) > 10);
    AT(center[3] >= 240);

    dvz_free(rgba);
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
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_mip_renders_bright_slice skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 64) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_composite_renders_field skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
 * Ensure composite label-volume rendering displays categorical colors.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_label_composite_renders_category(
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

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R16_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint16_t labels[8] = {2, 2, 2, 2, 2, 2, 2, 2};
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
            .data = labels,
            .bytes_per_row = 2 * sizeof(uint16_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory category = {
        .category_id = 2,
        .order = 0,
        .label = "green",
        .color = {0, 255, 0, 255},
    };
    AT(dvz_scale_set_categories(scale, &category, 1));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_volume_label_composite_renders_category skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    uint32_t green_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        green_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[1] > 180 && px[0] < 80 && px[2] < 80)
                green_count++;
        }
        dvz_free(rgba);
        if (green_count > (width * height) / 2)
            break;
    }
    AT(green_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure composite label-volume rendering displays sparse categorical ids through the lookup buffer.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_app_offscreen_volume_label_composite_renders_sparse_category(
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

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint32_t labels[8] = {70000, 70000, 70000, 70000, 70000, 70000, 70000, 70000};
    AT(dvz_sampled_field_set_data(
        field,
        &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
            .data = labels,
            .bytes_per_row = 2 * sizeof(uint32_t),
            .rows_per_image = 2,
        }));
    AT(dvz_visual_set_field(volume, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory category = {
        .category_id = 70000,
        .order = 0,
        .label = "magenta",
        .color = {255, 0, 255, 255},
    };
    AT(dvz_scale_set_categories(scale, &category, 1));
    AT(dvz_visual_set_scale(volume, "labels", scale) == 0);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_volume_label_composite_renders_sparse_category skipped: GPU "
            "context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
    ANN(canvas);

    uint32_t magenta_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        magenta_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            const uint8_t* px = &rgba[4 * i];
            if (px[0] > 180 && px[2] > 180 && px[1] < 80)
                magenta_count++;
        }
        dvz_free(rgba);
        if (magenta_count > (width * height) / 2)
            break;
    }
    AT(magenta_count > 64 * 64 / 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Render the deterministic volume-occlusion fixture and return region sums.
 *
 * @param suite test context used for shared app resources
 * @param mode occlusion mode used by the fixture
 * @param perspective_camera whether to attach a perspective camera to the panel
 * @param clipped_occluder whether to restrict the volume occluder to one screen side
 * @return captured image sums, or skipped=true when no app context is available
 */
static AppVolumeOcclusionCapture _app_volume_occlusion_capture(
    TstContext* suite, AppVolumeOcclusionMode mode, bool perspective_camera, bool clipped_occluder)
{
    ANN(suite);
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
        camera_desc.view.eye[0] = 0.5f;
        camera_desc.view.eye[1] = 0.5f;
        camera_desc.view.eye[2] = 3.0f;
        camera_desc.view.target[0] = 0.5f;
        camera_desc.view.target[1] = 0.5f;
        camera_desc.view.target[2] = 0.5f;
        camera_desc.projection.fov_y = GLM_PI_4f;
        if (dvz_panel_set_camera(panel, &camera_desc) == NULL)
        {
            dvz_scene_destroy(scene);
            return out;
        }
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}))
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
            &(DvzVolumeOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
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
            panel, &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
                       .enabled = true,
                       .depth_bias = 0.0f,
                       .soft_edge = 0.02f,
                       .hidden_alpha = 0.05f,
                   }) != 0)
    {
        dvz_scene_destroy(scene);
        return out;
    }
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_scene_destroy(scene);
        return out;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    if (win == NULL)
    {
        out.skipped = true;
        out.skip_reason = "app offscreen capture unavailable";
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return out;
    }
    DvzCanvas* canvas = dvz_view_canvas(win);
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
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_DISABLED, false, false);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_volume_occlusion_slice_renders skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_VOLUME, false, false);
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
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_DISABLED, false, true);
    if (disabled.skipped)
    {
        log_warn("test_app_offscreen_volume_occlusion_region_delta skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_VOLUME, false, true);
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
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_DISABLED, true, false);
    if (disabled.skipped)
    {
        log_warn(
            "test_app_offscreen_volume_occlusion_perspective_camera skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_VOLUME, true, false);
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
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_DISABLED, false, false);
    if (disabled.skipped)
    {
        log_warn(
            "test_app_offscreen_volume_slice_scene_occlusion_dimming skipped: GPU context failed");
        tst_skip(suite, disabled.skip_reason);
        return 0;
    }
    AppVolumeOcclusionCapture enabled =
        _app_volume_occlusion_capture(suite, APP_VOLUME_OCCLUSION_MODE_SCENE, false, false);
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
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(volume);
    ANN(slice);
    ANN(mesh);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.1f},
        {+0.5f, -0.5f, 0.1f},
        {-0.5f, +0.5f, 0.1f},
        {+0.5f, +0.5f, 0.1f},
    };
    vec3 normals[4] = {
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
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
           panel, volume, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 0}) == 0);
    AT(dvz_panel_add_visual(
           panel, slice, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) == 0);
    AT(dvz_panel_add_visual(
           panel, mesh, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2}) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
               .enabled = true,
               .alpha_threshold = 0.005f,
               .fade_distance = 0.02f,
               .occluded_alpha = 0.05f,
           }) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = _app_test_create(suite, scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle skipped: GPU context "
            "failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    if (win == NULL)
    {
        log_warn(
            "test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle skipped: view "
            "failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    tst_log_capture_begin(suite);
    int rc = dvz_view_render_once(win);
    AppLogCapture first = {0};
    _app_log_capture_from_suite(suite, &first);
    tst_log_capture_end(suite);
    AT(rc == DVZ_CANVAS_FRAME_READY);
    AT(first.error_count == 0);

    dvz_visual_set_visible(mesh, true);
    AT(dvz_visual_set_scene_occluder(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel, &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
                      .enabled = true,
                      .depth_bias = 0.0005f,
                      .soft_edge = 0.02f,
                      .hidden_alpha = 0.05f,
                  }) == 0);

    tst_log_capture_begin(suite);
    rc = dvz_view_render_once(win);
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

    vec3 quad[6] = {
        {-0.65f, -0.65f, 0.1f}, {-0.65f, 0.65f, 0.1f}, {0.65f, -0.65f, 0.1f},
        {0.65f, -0.65f, 0.1f},  {-0.65f, 0.65f, 0.1f}, {0.65f, 0.65f, 0.1f},
    };
    DvzColor black[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        black[i] = dvz_color_rgb(0, 0, 0);
    }
    AT(dvz_visual_set_data(occluder, "position", quad, 6) == 0);
    AT(dvz_visual_set_data(occluder, "color", black, 6) == 0);
    AT(dvz_panel_add_visual(panel, occluder, NULL) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 1.0f));

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_volume_depth_occluded_by_primitive skipped: GPU context failed");
        tst_skip(suite, "GPU context failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzView* win = dvz_view_offscreen(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_view_canvas(win);
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
    tst_suite_register_fixture(
        suite, TST_SCENE_APP_GPU_FIXTURE, TST_FIXTURE_SCOPE_PROCESS, _app_gpu_fixture_create,
        _app_gpu_fixture_destroy);

    TST_SCENE_APP_SHARED_CASE(test_app_offscreen);
    TST_SCENE_APP_SHARED_CASE(test_app_view_capabilities);
    TST_SCENE_APP_SHARED_CASE(test_app_view_desc_offscreen_scale);
    TST_SCENE_APP_SHARED_CASE(test_app_view_desc_offscreen_exact_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_small_view_clamps_layout);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_scheduler_sees_scene_dirty_without_request);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_frame_callback_enables_continuous_scheduler);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_query_requests_notify_hosted_callback);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_shared_scene_request_frame_subscribers);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_timer_advances_in_app_run);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_timer_advances_in_render_once);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_render_enabled_gate);
    TST_SCENE_APP_SHARED_CASE(test_view_panel_panzoom_helper);
    TST_SCENE_APP_SHARED_CASE(test_view_connects_prebound_panel_controller);
#if defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
    TST_SCENE_APP_CASE(
        test_app_external_surface_release_waits, TST_SCENE_APP_GPU_RES | TST_RES_GLFW,
        TST_ISOLATION_PROCESS);
#endif
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_panel_three_visuals_all_drawn);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_point_depth_orders_overlap);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_wboit_mesh_order_independent_layers);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_source_over_mesh_depth_and_blend);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_depth_peel_mesh_two_layers);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_depth_peel_mesh_three_layers);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_scene_occlusion_hidden_alpha);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_source_over_scene_occlusion_matrix);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_point_depth_cue_darkens_far);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_point_default_edge_has_fractional_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_path_join_has_no_center_gap);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_path_join_modes_are_ordered);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_path_closed_star_seam_has_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_pixel_square_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_points_edl_renders);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_points_edl_changes_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_mesh_ssao_changes_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_sphere_ssao_darkens_contact);
    TST_SCENE_APP_CASE(
        test_app_offscreen_records_dvzr_frames,
        TST_SCENE_APP_GPU_RES | TST_RES_FILESYSTEM | TST_RES_ENV, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_image_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_colorbar_has_visible_ramp_and_labels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_text_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_sdf_text_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_text_block_raster_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_overlay_rich_card_has_visible_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_image_field_partial_update_changes_region);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_lit_primitive_depth_orders_overlap);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_lit_primitive_depth_cue_darkens_far);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_mesh_renders_nonblank);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_rotated_mesh_depth_orders_faces);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_camera_arcball_mesh_renders_cube);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_shared_field_mixed_runtime_updates);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_retained_render_second_frame);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_image_retained_render_second_frame);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_resize_reuses_runtime_with_mesh_and_image);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_query_request_steady_state);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_gsp_first_slice_smoke);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_gsp_image_nearest_point_no_stroke_smoke);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_two_panel_points_light_both_halves);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_clear_color);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_midgray_srgb_readback);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_legacy_srgb_blend_readback);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_linear_color_field_not_decoded);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_alpha_over_nonblack_linear);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_colormap_srgb_lut_linear_blend);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_slice_renders_field);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_rgba_srgb_linear_blend);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_mip_renders_bright_slice);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_composite_renders_field);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_label_composite_renders_category);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_label_composite_renders_sparse_category);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_occlusion_slice_renders);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_occlusion_region_delta);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_occlusion_perspective_camera);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_slice_scene_occlusion_dimming);
    TST_SCENE_APP_CASE(
        test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle,
        TST_SCENE_APP_GPU_RES | TST_RES_LOG_CAPTURE, TST_ISOLATION_PROCESS);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_depth_occluded_by_primitive);
    TST_SCENE_APP_SHARED_CASE(test_app_capture_rejects_wrong_dimensions);
    TST_SCENE_APP_SHARED_CASE(test_app_capture_rejects_undersized_buffer);

    TST_GROUP("app-offscreen-shared");
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_clear_color);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_pixel_square_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_image_has_nonblank_pixels);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_mesh_renders_nonblank);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_panel_three_visuals_all_drawn);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_two_panel_points_light_both_halves);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_point_depth_orders_overlap);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_point_depth_cue_darkens_far);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_lit_primitive_depth_orders_overlap);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_lit_primitive_depth_cue_darkens_far);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_slice_renders_field);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_mip_renders_bright_slice);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_composite_renders_field);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_label_composite_renders_category);
    TST_SCENE_APP_SHARED_CASE(test_app_offscreen_volume_label_composite_renders_sparse_category);
#endif

    return 0;
}
