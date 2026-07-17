/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App — presentation layer                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_app.h"
#include "_status.h"
#include "_time_utils.h"
#include "_trace.h"
#include "datoviz/app.h"
#include "datoviz/app_interop.h"
#include "datoviz/ffi.h"
#include "datoviz/gui.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "../drp2/_stream.h"
#include "mutex_internal.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "core/figure_emit_internal.h"
#include "interaction/animation_internal.h"
#include "query/internal.h"
#include "visuals/_visual_internal.h"
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
#include "../gui/_gui.h"
#endif

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
#include <volk.h>
#include "datoviz/canvas.h"
#include "datoviz/drp2/recording.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_APP_MAX_VIEWS 16

/* Keep borrowed canvas targets out of the scene emitter's low transient id range. */
#define DVZ_APP_CANVAS_TARGET_BASE UINT64_C(0xF000000000000000)

#define DVZ_TRACE_COLOR_RESET "\x1b[0m"
#define DVZ_TRACE_COLOR_DIM   "\x1b[90m"
#define DVZ_TRACE_COLOR_GREEN "\x1b[32m"
#define DVZ_TRACE_COLOR_YELLOW "\x1b[33m"
#define DVZ_TRACE_COLOR_CYAN  "\x1b[36m"
#define DVZ_TRACE_COLOR_BLUE  "\x1b[34m"
#define DVZ_TRACE_COLOR_MAGENTA "\x1b[35m"
#define DVZ_TRACE_COLOR_RED   "\x1b[31m"
#define DVZ_TRACE_COLOR_BOLD  "\x1b[1m"
#define DVZ_APP_TRACE_LABEL_PRINT_SIZE ((int)(DVZ_DRP2_LABEL_SIZE - 1))

/* Keep developer DVZR recordings compact by default. Set DVZ_DRP2_RECORD_FPS<=0 to disable. */
#define DVZ_APP_DEFAULT_RECORD_FPS 30.0

#define DVZ_APP_CAPTURE_PATH_SIZE 1024
#define DVZ_APP_CAPTURE_BACKEND_SIZE 64
#define DVZ_APP_CAPTURE_DEFAULT_FPS 60.0
#define DVZ_APP_VIEW_POST_CAPACITY 64
#define DVZ_APP_MIN_LAYOUT_WIDTH  200u
#define DVZ_APP_MIN_LAYOUT_HEIGHT 200u
#define DVZ_APP_DEFAULT_REFERENCE_DPI 96.0



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzAppRuntimeFailure
{
    DvzDrp2ValidationCode code;
    DvzDrp2CommandType type;
    uint32_t command_index;
    uint32_t command_count;
} DvzAppRuntimeFailure;


typedef struct DvzViewPostItem
{
    DvzViewPostCallback callback;
    void* user_data;
} DvzViewPostItem;


typedef struct DvzResolvedViewDesc
{
    DvzViewDesc desc;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    float device_scale;
} DvzResolvedViewDesc;



struct DvzView
{
    DvzApp*    app;
    DvzFigure* figure;
    DvzViewKind kind;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzWindow* window;
    DvzCanvas* canvas;
#endif
    uint64_t target_id;
    bool is_interactive;
    bool render_enabled;
    uint64_t frame_index;
    DvzViewFrameCallback frame_callback;
    void* frame_user_data;
    DvzViewRequestFrameCallback request_frame_callback;
    void* request_frame_user_data;
    DvzMutex post_mutex;
    bool post_mutex_initialized;
    uint32_t post_count;
    DvzViewPostItem post_items[DVZ_APP_VIEW_POST_CAPACITY];
    DvzAppTraceSnapshot last_trace_snapshot;
    bool has_last_trace_snapshot;
    DvzAppRuntimeFailure last_runtime_failure;
    bool has_last_runtime_failure;
    uint32_t runtime_failure_repeat_count;
    DvzDrp2Recorder* recorder;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    DvzViewSizeDesc requested_size;
    DvzResolvedViewSize resolved_size;
    float device_scale_x;
    float device_scale_y;
    float render_scale;
    float user_scale;
    DvzClock recording_clock;
    double recording_fps;
    double recording_last_t_present;
    bool recording_target_created;
    bool recording_has_last_frame;
    DvzDrp2Recording* replay_recording;
    uint64_t replay_target_id;
    uint32_t replay_frame_index;
    DvzClock replay_clock;
    bool replay_clock_started;
    bool replay_paced;
    bool replay_loop;
    double replay_speed;
    bool fps_overlay_enabled;
    bool fps_valid;
    uint64_t fps_last_ns;
    uint64_t fps_sample_start_ns;
    uint32_t fps_sample_frames;
    uint32_t fps_last_sample_frames;
    double fps;
    double fps_frame_ms;
    double fps_last_sample_elapsed_s;
    bool frame_requested;
    bool dirty;
    uint64_t next_frame_ns;
    bool capture_dvzr_enabled;
    bool capture_video_enabled;
    bool capture_png_enabled;
    char capture_dvzr_path[DVZ_APP_CAPTURE_PATH_SIZE];
    char capture_video_path[DVZ_APP_CAPTURE_PATH_SIZE];
    char capture_png_path[DVZ_APP_CAPTURE_PATH_SIZE];
    char capture_video_backend[DVZ_APP_CAPTURE_BACKEND_SIZE];
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    DvzGui* gui;
#endif
};


struct DvzApp
{
    DvzScene* scene;
    DvzAppConfig config;
    bool stop_requested;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx*      gpu_ctx;
    DvzDrp2Runtime* runtime;
    DvzWindowHost*  window_host;
    bool owns_gpu_ctx;
    bool owns_runtime;
    bool owns_window_host;
    bool runtime_recovery_pending;
#endif
    uint32_t     view_count;
    DvzView views[DVZ_APP_MAX_VIEWS];
    DvzAppStatus status;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_APP_CONFIG_KNOWN_FLAGS 0u
#define DVZ_APP_RESOURCES_KNOWN_FLAGS 0u
#define DVZ_APP_CAPTURE_KNOWN_FLAGS                                                              \
    (DVZ_APP_CAPTURE_DVZR | DVZ_APP_CAPTURE_VIDEO | DVZ_APP_CAPTURE_PNG)
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
#define DVZ_APP_EXTERNAL_SURFACE_INFO_KNOWN_FLAGS 0u
#endif



static bool _app_config_validate(const DvzAppConfig* config)
{
    if (config == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(config, DvzAppConfig, DVZ_APP_CONFIG_KNOWN_FLAGS))
    {
        log_error("invalid DvzAppConfig ABI prologue");
        return false;
    }
    if (
        config->exit_policy != DVZ_APP_EXIT_WHEN_ALL_WINDOWS_CLOSED &&
        config->exit_policy != DVZ_APP_EXIT_WHEN_ANY_WINDOW_CLOSED &&
        config->exit_policy != DVZ_APP_EXIT_NEVER)
    {
        log_error("invalid DvzAppConfig exit_policy");
        return false;
    }
    return true;
}



static bool _app_resources_validate(const DvzAppResources* resources)
{
    if (resources == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(resources, DvzAppResources, DVZ_APP_RESOURCES_KNOWN_FLAGS))
    {
        log_error("invalid DvzAppResources ABI prologue");
        return false;
    }
    return true;
}



static bool _app_capture_config_validate(const DvzAppCaptureConfig* config)
{
    if (config == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(config, DvzAppCaptureConfig, DVZ_APP_CAPTURE_KNOWN_FLAGS))
    {
        log_error("invalid DvzAppCaptureConfig ABI prologue");
        return false;
    }
    return true;
}



static DvzFontDefaults _app_config_font_defaults(const DvzAppConfig* config)
{
    DvzFontDefaults defaults = dvz_font_defaults();
    if (config == NULL)
        return defaults;
    defaults.sans_path = config->font_sans_path;
    defaults.sans_family = config->font_sans_family;
    defaults.sans_style = config->font_sans_style;
    defaults.sans_face_index = config->font_sans_face_index;
    defaults.sans_font_flags = config->font_sans_font_flags;
    defaults.mono_path = config->font_mono_path;
    defaults.mono_family = config->font_mono_family;
    defaults.mono_style = config->font_mono_style;
    defaults.mono_face_index = config->font_mono_face_index;
    defaults.mono_font_flags = config->font_mono_font_flags;
    defaults.ui_size_px = config->font_ui_size_px;
    defaults.mono_size_px = config->font_mono_size_px;
    defaults.text_size_px = config->font_text_size_px;
    return defaults;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
static bool _app_external_surface_info_validate(const DvzWindowExternalSurfaceInfo* surface)
{
    if (surface == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(
            surface, DvzWindowExternalSurfaceInfo,
            DVZ_APP_EXTERNAL_SURFACE_INFO_KNOWN_FLAGS))
    {
        log_error("invalid DvzWindowExternalSurfaceInfo ABI prologue");
        return false;
    }
    return true;
}
#endif

/**
 * Return the app configuration before environment overrides are applied.
 *
 * @return default app configuration
 */
static DvzAppConfig _app_config_defaults(void)
{
    DvzAppConfig config = {DVZ_STRUCT_INIT_FIELDS(DvzAppConfig)};
    config.instance_extension_count = 0;
    config.instance_extensions = NULL;
    config.enable_canvas_extensions = false;
    config.enable_glfw_extensions = true;
    config.schedule_mode = DVZ_APP_SCHEDULE_ON_DEMAND;
    config.exit_policy = DVZ_APP_EXIT_WHEN_ALL_WINDOWS_CLOSED;
    config.fps_cap = 0.0;
    DvzFontDefaults fonts = dvz_font_defaults();
    config.font_sans_path = fonts.sans_path;
    config.font_sans_family = fonts.sans_family;
    config.font_sans_style = fonts.sans_style;
    config.font_sans_face_index = fonts.sans_face_index;
    config.font_sans_font_flags = fonts.sans_font_flags;
    config.font_mono_path = fonts.mono_path;
    config.font_mono_family = fonts.mono_family;
    config.font_mono_style = fonts.mono_style;
    config.font_mono_face_index = fonts.mono_face_index;
    config.font_mono_font_flags = fonts.mono_font_flags;
    config.font_ui_size_px = fonts.ui_size_px;
    config.font_mono_size_px = fonts.mono_size_px;
    config.font_text_size_px = fonts.text_size_px;
    return config;
}



/**
 * Apply the DVZ_APP_SCHEDULE environment override to an app configuration.
 *
 * @param config app configuration to mutate
 */
static void _app_config_apply_schedule_env(DvzAppConfig* config)
{
    ANN(config);
    const char* env = getenv("DVZ_APP_SCHEDULE");
    if (env == NULL || env[0] == '\0')
        return;

    if (strcmp(env, "on_demand") == 0)
        config->schedule_mode = DVZ_APP_SCHEDULE_ON_DEMAND;
    else if (strcmp(env, "continuous") == 0)
        config->schedule_mode = DVZ_APP_SCHEDULE_CONTINUOUS;
    else
        log_warn("ignoring DVZ_APP_SCHEDULE='%s' (expected on_demand|continuous)", env);
}



/**
 * Apply the DVZ_FPS_CAP environment override to an app configuration.
 *
 * @param config app configuration to mutate
 */
static void _app_config_apply_fps_cap_env(DvzAppConfig* config)
{
    ANN(config);
    const char* env = getenv("DVZ_FPS_CAP");
    if (env == NULL || env[0] == '\0')
        return;

    char* end = NULL;
    double fps = strtod(env, &end);
    if (end == env || *end != '\0' || fps <= 0)
    {
        log_warn("ignoring DVZ_FPS_CAP='%s' (expected positive FPS)", env);
        return;
    }
    config->fps_cap = fps;
}



/**
 * Apply supported app environment overrides to an app configuration.
 *
 * @param config app configuration to mutate
 */
static void _app_config_apply_env(DvzAppConfig* config)
{
    ANN(config);
    _app_config_apply_schedule_env(config);
    _app_config_apply_fps_cap_env(config);
}


/**
 * Return whether a capture token delimiter was found.
 *
 * @param c character to inspect
 * @return whether the character separates capture tokens
 */
static bool _app_capture_is_separator(char c)
{
    return c == ',' || c == ';' || c == '+' || c == ':' || c == '|' || c == ' ' || c == '\t' ||
           c == '\n' || c == '\r';
}



/**
 * Return a lowercase ASCII character.
 *
 * @param c input character
 * @return lowercase ASCII character when applicable
 */
static char _app_ascii_lower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}



/**
 * Return whether a token disables capture.
 *
 * @param token lowercase capture token
 * @return whether the token is a false-like value
 */
static bool _app_capture_token_is_false(const char* token)
{
    ANN(token);
    return strcmp(token, "0") == 0 || strcmp(token, "false") == 0 ||
           strcmp(token, "off") == 0 || strcmp(token, "none") == 0 ||
           strcmp(token, "no") == 0 || strcmp(token, "disable") == 0 ||
           strcmp(token, "disabled") == 0;
}



/**
 * Apply one capture token to a flag mask.
 *
 * @param token lowercase capture token
 * @param flags capture flags to update
 * @return whether the token was recognized
 */
static bool _app_capture_apply_token(const char* token, uint32_t* flags)
{
    ANN(token);
    ANN(flags);
    if (token[0] == '\0')
        return true;
    if (_app_capture_token_is_false(token))
    {
        *flags = DVZ_APP_CAPTURE_NONE;
        return true;
    }
    if (
        strcmp(token, "1") == 0 || strcmp(token, "true") == 0 ||
        strcmp(token, "on") == 0 || strcmp(token, "yes") == 0)
    {
        *flags |= DVZ_APP_CAPTURE_DVZR | DVZ_APP_CAPTURE_VIDEO;
        return true;
    }
    if (strcmp(token, "all") == 0)
    {
        *flags |= DVZ_APP_CAPTURE_DVZR | DVZ_APP_CAPTURE_VIDEO | DVZ_APP_CAPTURE_PNG;
        return true;
    }
    if (strcmp(token, "dvzr") == 0 || strcmp(token, "record") == 0)
    {
        *flags |= DVZ_APP_CAPTURE_DVZR;
        return true;
    }
    if (strcmp(token, "mp4") == 0 || strcmp(token, "video") == 0)
    {
        *flags |= DVZ_APP_CAPTURE_VIDEO;
        return true;
    }
    if (strcmp(token, "png") == 0 || strcmp(token, "screenshot") == 0)
    {
        *flags |= DVZ_APP_CAPTURE_PNG;
        return true;
    }
    return false;
}



/**
 * Parse the DVZ_CAPTURE token list into app capture flags.
 *
 * @param value environment variable value
 * @return parsed capture flags
 */
static uint32_t _app_capture_flags_from_env_value(const char* value)
{
    if (value == NULL || value[0] == '\0')
        return DVZ_APP_CAPTURE_NONE;

    uint32_t flags = DVZ_APP_CAPTURE_NONE;
    char token[32] = {0};
    size_t token_len = 0;
    bool valid = true;
    for (const char* p = value;; p++)
    {
        char c = *p;
        if (c == '\0' || _app_capture_is_separator(c))
        {
            token[token_len] = '\0';
            if (!_app_capture_apply_token(token, &flags))
            {
                log_warn("ignoring unknown DVZ_CAPTURE token '%s'", token);
                valid = false;
            }
            token_len = 0;
            if (c == '\0')
                break;
            continue;
        }
        if (token_len + 1 < sizeof(token))
            token[token_len++] = _app_ascii_lower(c);
        else
            valid = false;
    }

    if (!valid)
        log_warn("DVZ_CAPTURE='%s' was only partially understood", value);
    return flags;
}



/**
 * Parse the capture FPS environment override.
 *
 * @param fallback fallback FPS value
 * @return parsed FPS, or fallback on invalid input
 */
static double _app_capture_fps_from_env(double fallback)
{
    const char* env = getenv("DVZ_CAPTURE_FPS");
    if (env == NULL || env[0] == '\0')
        return fallback;

    char* end = NULL;
    double fps = strtod(env, &end);
    if (end == env || *end != '\0' || fps <= 0)
    {
        log_warn("ignoring DVZ_CAPTURE_FPS='%s' (expected positive FPS)", env);
        return fallback;
    }
    return fps;
}



/**
 * Parse the capture video mode environment override.
 *
 * @param fallback fallback video capture mode
 * @return parsed video capture mode
 */
static DvzVideoCaptureMode _app_capture_video_mode_from_env(DvzVideoCaptureMode fallback)
{
    const char* env = getenv("DVZ_CAPTURE_VIDEO_MODE");
    if (env == NULL || env[0] == '\0')
        return fallback;
    if (strcmp(env, "auto") == 0)
        return DVZ_VIDEO_CAPTURE_AUTO;
    if (strcmp(env, "external") == 0)
        return DVZ_VIDEO_CAPTURE_EXTERNAL;
    if (strcmp(env, "cpu") == 0 || strcmp(env, "cpu_readback") == 0)
        return DVZ_VIDEO_CAPTURE_CPU_READBACK;

    log_warn("ignoring DVZ_CAPTURE_VIDEO_MODE='%s' (expected auto|external|cpu)", env);
    return fallback;
}



/**
 * Build a capture output path.
 *
 * @param config capture configuration
 * @param extension output extension including the leading dot
 * @param out destination path buffer
 * @param out_size destination path buffer size
 * @return whether the path fit in the destination buffer
 */
static bool _app_capture_path(
    const DvzAppCaptureConfig* config, const char* extension, char* out, size_t out_size)
{
    ANN(config);
    ANN(extension);
    ANN(out);
    if (out_size == 0)
        return false;

    const char* directory =
        (config->directory != NULL && config->directory[0] != '\0') ? config->directory : ".";
    const char* basename =
        (config->basename != NULL && config->basename[0] != '\0') ? config->basename : "capture";

    int rc = 0;
    size_t dir_len = strlen(directory);
    if (strcmp(directory, ".") == 0)
        rc = dvz_snprintf(out, out_size, "%s%s", basename, extension);
    else if (dir_len > 0 && directory[dir_len - 1] == '/')
        rc = dvz_snprintf(out, out_size, "%s%s%s", directory, basename, extension);
    else
        rc = dvz_snprintf(out, out_size, "%s/%s%s", directory, basename, extension);

    return rc >= 0 && (size_t)rc < out_size;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE

/**
 * Return the largest sample count represented by Vulkan sample-count flags.
 *
 * @param flags Vulkan sample-count flag mask.
 * @return largest supported sample count, defaulting to one.
 */
static uint32_t _app_max_sample_count_from_flags(VkSampleCountFlags flags)
{
    if ((flags & VK_SAMPLE_COUNT_16_BIT) != 0)
        return 16;
    if ((flags & VK_SAMPLE_COUNT_8_BIT) != 0)
        return 8;
    if ((flags & VK_SAMPLE_COUNT_4_BIT) != 0)
        return 4;
    if ((flags & VK_SAMPLE_COUNT_2_BIT) != 0)
        return 2;
    return 1;
}



/**
 * Query the largest image sample count supported for one format and usage.
 *
 * @param physical_device Vulkan physical device.
 * @param format image format to query.
 * @param usage Vulkan image usage flags.
 * @param framebuffer_flags device framebuffer sample-count flags for the attachment class.
 * @return largest supported sample count, defaulting to one.
 */
static uint32_t _app_image_max_sample_count(
    VkPhysicalDevice physical_device, VkFormat format, VkImageUsageFlags usage,
    VkSampleCountFlags framebuffer_flags)
{
    if (physical_device == VK_NULL_HANDLE)
        return 1;

    VkImageFormatProperties props = {0};
    VkResult res = vkGetPhysicalDeviceImageFormatProperties(
        physical_device, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &props);
    if (res != VK_SUCCESS)
        return _app_max_sample_count_from_flags(framebuffer_flags);

    VkSampleCountFlags flags = props.sampleCounts & framebuffer_flags;
    return _app_max_sample_count_from_flags(flags);
}



/**
 * Return whether an environment flag is enabled.
 *
 * @param name environment variable name
 * @return whether the variable exists and is not an explicit false value
 */
static bool _app_env_flag_enabled(const char* name)
{
    ANN(name);
    const char* value = getenv(name);
    if (value == NULL || value[0] == '\0')
        return false;
    if (
        strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
        strcmp(value, "FALSE") == 0 || strcmp(value, "off") == 0 ||
        strcmp(value, "OFF") == 0 || strcmp(value, "no") == 0 ||
        strcmp(value, "NO") == 0)
    {
        return false;
    }
    return true;
}



/**
 * Return the app DVZR recording FPS cap from the environment.
 *
 * @return requested FPS cap, or 0/negative to disable frame skipping
 */
static double _app_record_fps_from_env(void)
{
    const char* env = getenv("DVZ_DRP2_RECORD_FPS");
    if (env == NULL || env[0] == '\0')
        return DVZ_APP_DEFAULT_RECORD_FPS;

    char* end = NULL;
    double fps = strtod(env, &end);
    if (end == env || fps < 0)
    {
        log_warn("ignoring DVZ_DRP2_RECORD_FPS='%s' (expected non-negative FPS)", env);
        return DVZ_APP_DEFAULT_RECORD_FPS;
    }
    return fps;
}


/**
 * Return whether a Vulkan format supports query render-target readback.
 *
 * @param physical_device Vulkan physical device
 * @param format Vulkan format to test
 * @return whether the format can be rendered and copied from optimal tiling
 */
static bool _app_format_supports_query_readback(VkPhysicalDevice physical_device, VkFormat format)
{
    if (physical_device == VK_NULL_HANDLE)
        return false;

    VkFormatProperties props = {0};
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
    const VkFormatFeatureFlags required =
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    return (props.optimalTilingFeatures & required) == required;
}



/**
 * Apply runtime-backed limits to app frame emission capabilities.
 *
 * @param app app owning the runtime GPU context
 * @param caps capabilities to update
 */
static bool _app_apply_runtime_caps(const DvzApp* app, DvzCapabilitySnapshot* caps)
{
    ANN(caps);
    if (app == NULL || app->gpu_ctx == NULL)
        return false;

    DvzInstance* instance = dvz_gpu_ctx_instance(app->gpu_ctx);
    uint32_t gpu_index = dvz_gpu_ctx_gpu_index(app->gpu_ctx);
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    if (instance == NULL ||
        !dvz_instance_gpu_handle(instance, gpu_index, &physical_device) ||
        physical_device == VK_NULL_HANDLE)
        return false;

    VkPhysicalDeviceVulkan13Properties props13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &props13,
    };
    vkGetPhysicalDeviceProperties2(physical_device, &props);
    if (props13.maxBufferSize > caps->max_buffer_size)
        caps->max_buffer_size = props13.maxBufferSize;
    if (props.properties.limits.maxColorAttachments > caps->max_color_attachments)
        caps->max_color_attachments = props.properties.limits.maxColorAttachments;
    caps->max_readback_size = caps->max_buffer_size;
    caps->supports_readback = true;
    caps->min_texture_copy_bytes_per_row_alignment =
        props.properties.limits.optimalBufferCopyRowPitchAlignment > UINT32_MAX
            ? UINT32_MAX
            : (uint32_t)props.properties.limits.optimalBufferCopyRowPitchAlignment;
    if (caps->min_texture_copy_bytes_per_row_alignment == 0)
        caps->min_texture_copy_bytes_per_row_alignment = 4;

    caps->render_target_format_r32uint =
        _app_format_supports_query_readback(physical_device, VK_FORMAT_R32_UINT);
    caps->render_target_format_rg32uint =
        _app_format_supports_query_readback(physical_device, VK_FORMAT_R32G32_UINT);
    caps->texture_format_r32uint = caps->render_target_format_r32uint;
    caps->texture_format_rg32uint = caps->render_target_format_rg32uint;
    caps->query_profile_u32_r32 = caps->supports_readback && caps->render_target_format_r32uint;
    caps->query_profile_u64_rg32 = caps->supports_readback && caps->render_target_format_rg32uint;
    caps->query_profile_u64_2xr32 =
        caps->supports_readback && caps->render_target_format_r32uint &&
        caps->max_color_attachments >= 2;

    caps->max_color_sample_count = _app_image_max_sample_count(
        physical_device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        props.properties.limits.framebufferColorSampleCounts);
    caps->max_depth_sample_count = _app_image_max_sample_count(
        physical_device, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        props.properties.limits.framebufferDepthSampleCounts);
    return true;
}



/**
 * Fill a scene-facing capability snapshot from the app runtime.
 *
 * @param app app owning the runtime GPU context
 * @param out output capability snapshot
 * @return whether the runtime-backed fields were available
 */
static bool _app_runtime_capabilities(const DvzApp* app, DvzCapabilitySnapshot* out)
{
    ANN(out);
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    const bool runtime_caps = _app_apply_runtime_caps(app, &caps);

    caps.shader_format_glsl = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    *out = caps;
    return runtime_caps;
}



/**
 * Update the per-window FPS estimator after a submitted frame.
 *
 * @param win view receiving the submitted frame
 * @param now current monotonic timestamp in nanoseconds
 */
static void _view_fps_update(DvzView* win, uint64_t now)
{
    ANN(win);
    if (now == 0)
        return;

    if (win->fps_last_ns != 0 && now > win->fps_last_ns)
    {
        uint64_t dt_ns = now - win->fps_last_ns;
        double dt_s = (double)dt_ns * 1e-9;
        if (dt_s > 0)
        {
            double instant_fps = 1.0 / dt_s;
            if (!win->fps_valid)
            {
                win->fps = instant_fps;
                win->fps_frame_ms = dt_s * 1e3;
                win->fps_valid = true;
            }
            else
            {
                const double alpha = 0.10;
                win->fps = win->fps + alpha * (instant_fps - win->fps);
                win->fps_frame_ms =
                    win->fps_frame_ms + alpha * ((dt_s * 1e3) - win->fps_frame_ms);
            }
        }
    }

    win->fps_last_ns = now;
    if (win->fps_sample_start_ns == 0)
        win->fps_sample_start_ns = now;
    win->fps_sample_frames++;

    uint64_t elapsed_ns = now - win->fps_sample_start_ns;
    if (elapsed_ns >= 1000000000ULL)
    {
        win->fps_last_sample_frames = win->fps_sample_frames;
        win->fps_last_sample_elapsed_s = (double)elapsed_ns * 1e-9;
        win->fps_sample_frames = 0;
        win->fps_sample_start_ns = now;
    }
}



/**
 * Mark a view as needing a submitted frame.
 *
 * @param win view to invalidate
 */
static void _view_mark_dirty(DvzView* win)
{
    ANN(win);
    win->dirty = true;
    win->frame_requested = true;
}


/**
 * Initialize the posted-callback queue for one view.
 *
 * @param win view receiving the queue
 * @return 0 on success, negative on failure
 */
static int _view_post_init(DvzView* win)
{
    ANN(win);
    if (win->post_mutex_initialized)
        return 0;
    if (dvz_mutex_init(&win->post_mutex) != 0)
        return -1;
    win->post_mutex_initialized = true;
    return 0;
}


/**
 * Destroy the posted-callback queue for one view.
 *
 * @param win view owning the queue
 */
static void _view_post_destroy(DvzView* win)
{
    ANN(win);
    if (!win->post_mutex_initialized)
        return;
    (void)dvz_mutex_lock(&win->post_mutex);
    win->post_count = 0;
    (void)dvz_mutex_unlock(&win->post_mutex);
    dvz_mutex_destroy(&win->post_mutex);
    win->post_mutex_initialized = false;
}


/**
 * Return whether one view has queued owner-thread callbacks.
 *
 * @param win view to inspect
 * @return whether at least one callback is queued
 */
static bool _view_has_posted_callbacks(DvzView* win)
{
    ANN(win);
    if (!win->post_mutex_initialized)
        return false;
    bool has_items = false;
    if (dvz_mutex_lock(&win->post_mutex) == 0)
    {
        has_items = win->post_count > 0;
        (void)dvz_mutex_unlock(&win->post_mutex);
    }
    return has_items;
}


/**
 * Drain queued callbacks on the current owner thread.
 *
 * @param win view owning the queue
 */
static void _view_post_drain(DvzView* win)
{
    ANN(win);
    if (!win->post_mutex_initialized)
        return;

    DvzViewPostItem items[DVZ_APP_VIEW_POST_CAPACITY] = {0};
    for (;;)
    {
        uint32_t count = 0;
        if (dvz_mutex_lock(&win->post_mutex) != 0)
            return;
        count = win->post_count;
        if (count > 0)
        {
            DvzSize bytes = (DvzSize)count * sizeof(DvzViewPostItem);
            dvz_memcpy(items, bytes, win->post_items, bytes);
            win->post_count = 0;
        }
        (void)dvz_mutex_unlock(&win->post_mutex);

        if (count == 0)
            break;
        for (uint32_t i = 0; i < count; i++)
        {
            if (items[i].callback != NULL)
                items[i].callback(win, items[i].user_data);
        }
    }
}


/**
 * Return a valid scale, replacing invalid values with one.
 *
 * @param scale input scale
 * @return positive finite scale
 */
static float _view_valid_scale(float scale)
{
    return isfinite(scale) && scale > 0.0f ? scale : 1.0f;
}



/**
 * Round a positive float pixel size to a nonzero integer.
 *
 * @param value input value
 * @return rounded integer size, or zero when unavailable
 */
static uint32_t _view_round_size(float value)
{
    if (!isfinite(value) || value <= 0.0f)
        return 0;
    if (value >= (float)UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)(value + 0.5f);
}


static uint32_t _view_round_size_d(double value)
{
    if (!isfinite(value) || value <= 0.0)
        return 0;
    if (value >= (double)UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)(value + 0.5);
}



static double _view_reference_dpi(double reference_dpi)
{
    return isfinite(reference_dpi) && reference_dpi > 0.0 ? reference_dpi :
                                                              DVZ_APP_DEFAULT_REFERENCE_DPI;
}



static double _view_valid_scale_d(double scale)
{
    return isfinite(scale) && scale > 0.0 ? scale : 1.0;
}



static bool _view_size_desc_active(const DvzViewSizeDesc* desc)
{
    return desc != NULL && isfinite(desc->width) && isfinite(desc->height) &&
           desc->width > 0.0 && desc->height > 0.0;
}



static DvzViewSizeDesc _view_desc_size_desc(const DvzViewDesc* desc)
{
    DvzViewSizeDesc size = {DVZ_STRUCT_INIT_FIELDS(DvzViewSizeDesc)};
    if (desc == NULL)
        return size;
    size.policy = desc->size_policy;
    size.width = desc->size_width;
    size.height = desc->size_height;
    size.reference_dpi = desc->size_reference_dpi;
    size.requested_device_scale = desc->size_requested_device_scale;
    size.monitor_dpi_x_override = desc->size_monitor_dpi_x_override;
    size.monitor_dpi_y_override = desc->size_monitor_dpi_y_override;
    size.strict_framebuffer_size = desc->size_strict_framebuffer_size;
    return size;
}


static void _view_desc_assign_size(DvzViewDesc* desc, const DvzViewSizeDesc* size)
{
    ANN(desc);
    ANN(size);
    desc->size_policy = size->policy;
    desc->size_width = size->width;
    desc->size_height = size->height;
    desc->size_reference_dpi = size->reference_dpi;
    desc->size_requested_device_scale = size->requested_device_scale;
    desc->size_monitor_dpi_x_override = size->monitor_dpi_x_override;
    desc->size_monitor_dpi_y_override = size->monitor_dpi_y_override;
    desc->size_strict_framebuffer_size = size->strict_framebuffer_size;
}



static DvzViewSizeDesc _view_size_desc(
    DvzViewSizePolicy policy, double width, double height, double reference_dpi)
{
    DvzViewSizeDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzViewSizeDesc)};
    desc.policy = policy;
    desc.width = width;
    desc.height = height;
    desc.reference_dpi = _view_reference_dpi(reference_dpi);
    return desc;
}



DvzViewSizeDesc dvz_view_size_desc_framebuffer_px(uint32_t width, uint32_t height)
{
    return _view_size_desc(
        DVZ_VIEW_SIZE_FRAMEBUFFER_PX, (double)width, (double)height,
        DVZ_APP_DEFAULT_REFERENCE_DPI);
}



DvzViewSizeDesc dvz_view_size_desc_host_logical_px(uint32_t width, uint32_t height)
{
    return _view_size_desc(
        DVZ_VIEW_SIZE_HOST_LOGICAL_PX, (double)width, (double)height,
        DVZ_APP_DEFAULT_REFERENCE_DPI);
}



DvzViewSizeDesc
dvz_view_size_desc_reference_px(double width, double height, double reference_dpi)
{
    return _view_size_desc(DVZ_VIEW_SIZE_REFERENCE_PX, width, height, reference_dpi);
}



DvzViewSizeDesc
dvz_view_size_desc_physical_mm(double width_mm, double height_mm, double reference_dpi)
{
    return _view_size_desc(DVZ_VIEW_SIZE_PHYSICAL_MM, width_mm, height_mm, reference_dpi);
}



static void _view_resolved_finalize(DvzResolvedViewSize* resolved)
{
    ANN(resolved);
    if (resolved->canvas_width_px <= 0.0)
        resolved->canvas_width_px = (double)resolved->host_logical_width;
    if (resolved->canvas_height_px <= 0.0)
        resolved->canvas_height_px = (double)resolved->host_logical_height;
    if (resolved->host_logical_width == 0)
        resolved->host_logical_width = _view_round_size_d(resolved->canvas_width_px);
    if (resolved->host_logical_height == 0)
        resolved->host_logical_height = _view_round_size_d(resolved->canvas_height_px);
    if (resolved->device_scale_x <= 0.0 || !isfinite(resolved->device_scale_x))
        resolved->device_scale_x = 1.0;
    if (resolved->device_scale_y <= 0.0 || !isfinite(resolved->device_scale_y))
        resolved->device_scale_y = 1.0;
    if (resolved->framebuffer_width == 0)
        resolved->framebuffer_width =
            _view_round_size_d((double)resolved->host_logical_width * resolved->device_scale_x);
    if (resolved->framebuffer_height == 0)
        resolved->framebuffer_height =
            _view_round_size_d((double)resolved->host_logical_height * resolved->device_scale_y);
    resolved->canvas_to_host_scale_x =
        resolved->canvas_width_px > 0.0 ?
            (double)resolved->host_logical_width / resolved->canvas_width_px :
            1.0;
    resolved->canvas_to_host_scale_y =
        resolved->canvas_height_px > 0.0 ?
            (double)resolved->host_logical_height / resolved->canvas_height_px :
            1.0;
    resolved->framebuffer_per_canvas_px_x =
        resolved->canvas_width_px > 0.0 ?
            (double)resolved->framebuffer_width / resolved->canvas_width_px :
            1.0;
    resolved->framebuffer_per_canvas_px_y =
        resolved->canvas_height_px > 0.0 ?
            (double)resolved->framebuffer_height / resolved->canvas_height_px :
            1.0;
    if (resolved->target_width_mm <= 0.0 && resolved->reference_dpi > 0.0)
        resolved->target_width_mm = resolved->canvas_width_px / resolved->reference_dpi * 25.4;
    if (resolved->target_height_mm <= 0.0 && resolved->reference_dpi > 0.0)
        resolved->target_height_mm = resolved->canvas_height_px / resolved->reference_dpi * 25.4;
    if (resolved->estimated_width_mm <= 0.0)
        resolved->estimated_width_mm = resolved->target_width_mm;
    if (resolved->estimated_height_mm <= 0.0)
        resolved->estimated_height_mm = resolved->target_height_mm;
}



DvzResolvedViewSize dvz_view_size_resolve(const DvzViewSizeDesc* desc, DvzViewKind kind)
{
    DvzViewSizeDesc size =
        _view_size_desc_active(desc) ? *desc : dvz_view_size_desc_host_logical_px(800, 600);
    if (size.struct_size == 0)
        size.struct_size = DVZ_STRUCT_SIZE(DvzViewSizeDesc);
    size.reference_dpi = _view_reference_dpi(size.reference_dpi);

    DvzResolvedViewSize resolved = {DVZ_STRUCT_INIT_FIELDS(DvzResolvedViewSize)};
    resolved.requested_policy = size.policy;
    resolved.requested_width = size.width;
    resolved.requested_height = size.height;
    resolved.reference_dpi = size.reference_dpi;
    resolved.device_scale_x = _view_valid_scale_d(size.requested_device_scale);
    resolved.device_scale_y = _view_valid_scale_d(size.requested_device_scale);
    resolved.physical_metrics_source =
        size.monitor_dpi_x_override > 0.0 || size.monitor_dpi_y_override > 0.0 ?
            DVZ_PHYSICAL_METRICS_USER_OVERRIDE :
            DVZ_PHYSICAL_METRICS_NONE;
    resolved.exactness =
        kind == DVZ_VIEW_OFFSCREEN || size.policy == DVZ_VIEW_SIZE_FRAMEBUFFER_PX ?
            DVZ_RESOLVED_EXACT :
            DVZ_RESOLVED_APPROXIMATE;

    switch (size.policy)
    {
    case DVZ_VIEW_SIZE_FRAMEBUFFER_PX:
        resolved.framebuffer_width = _view_round_size_d(size.width);
        resolved.framebuffer_height = _view_round_size_d(size.height);
        resolved.canvas_width_px = size.width;
        resolved.canvas_height_px = size.height;
        resolved.host_logical_width =
            _view_round_size_d(size.width / resolved.device_scale_x);
        resolved.host_logical_height =
            _view_round_size_d(size.height / resolved.device_scale_y);
        break;
    case DVZ_VIEW_SIZE_REFERENCE_PX:
        resolved.canvas_width_px = size.width;
        resolved.canvas_height_px = size.height;
        resolved.target_width_mm = size.width / size.reference_dpi * 25.4;
        resolved.target_height_mm = size.height / size.reference_dpi * 25.4;
        if (resolved.physical_metrics_source == DVZ_PHYSICAL_METRICS_USER_OVERRIDE)
        {
            double dpi_x = size.monitor_dpi_x_override > 0.0 ? size.monitor_dpi_x_override :
                                                                size.monitor_dpi_y_override;
            double dpi_y = size.monitor_dpi_y_override > 0.0 ? size.monitor_dpi_y_override :
                                                                size.monitor_dpi_x_override;
            resolved.framebuffer_width = _view_round_size_d(size.width / size.reference_dpi * dpi_x);
            resolved.framebuffer_height =
                _view_round_size_d(size.height / size.reference_dpi * dpi_y);
            resolved.host_logical_width =
                _view_round_size_d((double)resolved.framebuffer_width / resolved.device_scale_x);
            resolved.host_logical_height =
                _view_round_size_d((double)resolved.framebuffer_height / resolved.device_scale_y);
        }
        else
        {
            resolved.host_logical_width = _view_round_size_d(size.width);
            resolved.host_logical_height = _view_round_size_d(size.height);
        }
        break;
    case DVZ_VIEW_SIZE_PHYSICAL_MM:
        resolved.target_width_mm = size.width;
        resolved.target_height_mm = size.height;
        resolved.canvas_width_px = size.width / 25.4 * size.reference_dpi;
        resolved.canvas_height_px = size.height / 25.4 * size.reference_dpi;
        if (resolved.physical_metrics_source == DVZ_PHYSICAL_METRICS_USER_OVERRIDE)
        {
            double dpi_x = size.monitor_dpi_x_override > 0.0 ? size.monitor_dpi_x_override :
                                                                size.monitor_dpi_y_override;
            double dpi_y = size.monitor_dpi_y_override > 0.0 ? size.monitor_dpi_y_override :
                                                                size.monitor_dpi_x_override;
            resolved.framebuffer_width = _view_round_size_d(size.width / 25.4 * dpi_x);
            resolved.framebuffer_height = _view_round_size_d(size.height / 25.4 * dpi_y);
            resolved.host_logical_width =
                _view_round_size_d((double)resolved.framebuffer_width / resolved.device_scale_x);
            resolved.host_logical_height =
                _view_round_size_d((double)resolved.framebuffer_height / resolved.device_scale_y);
        }
        else
        {
            resolved.host_logical_width = _view_round_size_d(resolved.canvas_width_px);
            resolved.host_logical_height = _view_round_size_d(resolved.canvas_height_px);
        }
        break;
    case DVZ_VIEW_SIZE_HOST_LOGICAL_PX:
    default:
        resolved.host_logical_width = _view_round_size_d(size.width);
        resolved.host_logical_height = _view_round_size_d(size.height);
        resolved.canvas_width_px = size.width;
        resolved.canvas_height_px = size.height;
        break;
    }

    _view_resolved_finalize(&resolved);
    if (resolved.physical_metrics_source == DVZ_PHYSICAL_METRICS_USER_OVERRIDE)
    {
        double dpi_x = size.monitor_dpi_x_override > 0.0 ? size.monitor_dpi_x_override :
                                                            size.monitor_dpi_y_override;
        double dpi_y = size.monitor_dpi_y_override > 0.0 ? size.monitor_dpi_y_override :
                                                            size.monitor_dpi_x_override;
        if (dpi_x > 0.0)
            resolved.estimated_width_mm = (double)resolved.framebuffer_width / dpi_x * 25.4;
        if (dpi_y > 0.0)
            resolved.estimated_height_mm = (double)resolved.framebuffer_height / dpi_y * 25.4;
    }
    return resolved;
}



static void _view_resolved_apply_actual(
    DvzResolvedViewSize* resolved, uint32_t logical_width, uint32_t logical_height,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float device_scale_x,
    float device_scale_y)
{
    ANN(resolved);
    resolved->host_logical_width = logical_width;
    resolved->host_logical_height = logical_height;
    resolved->framebuffer_width = framebuffer_width;
    resolved->framebuffer_height = framebuffer_height;
    resolved->device_scale_x = _view_valid_scale_d((double)device_scale_x);
    resolved->device_scale_y = _view_valid_scale_d((double)device_scale_y);
    resolved->canvas_width_px = (double)logical_width;
    resolved->canvas_height_px = (double)logical_height;
    _view_resolved_finalize(resolved);
}



/**
 * Clamp a view size for retained figure layout resolution.
 *
 * @param value literal view logical size
 * @param minimum minimum retained layout size
 * @return clamped layout size
 */
static uint32_t _view_layout_dimension(uint32_t value, uint32_t minimum)
{
    if (value == 0)
        return 0;
    return value < minimum ? minimum : value;
}



/**
 * Return whether a figure uses retained layout machinery that needs a minimum solve size.
 *
 * @param figure figure to inspect
 * @return whether app/view sizing should clamp retained layout dimensions
 */
static bool _view_figure_needs_layout_clamp(const DvzFigure* figure)
{
    ANN(figure);
    if (figure->grid_count > 0)
        return true;
    return false;
}



/**
 * Synchronize a figure with the clamped retained layout size for one view.
 *
 * @param win view owning the figure
 */
static void _view_sync_figure_layout_size(DvzView* win)
{
    ANN(win);
    if (win->figure == NULL || win->logical_width == 0 || win->logical_height == 0)
        return;

    uint32_t layout_width = win->logical_width;
    uint32_t layout_height = win->logical_height;
    if (_view_figure_needs_layout_clamp(win->figure))
    {
        layout_width = _view_layout_dimension(layout_width, DVZ_APP_MIN_LAYOUT_WIDTH);
        layout_height = _view_layout_dimension(layout_height, DVZ_APP_MIN_LAYOUT_HEIGHT);
    }
    dvz_figure_resize(win->figure, layout_width, layout_height);
}



/**
 * Update cached logical, framebuffer, and scale state for one view.
 *
 * @param win view to update
 * @param logical_width logical width, or zero to derive it
 * @param logical_height logical height, or zero to derive it
 * @param framebuffer_width physical framebuffer width, or zero when unknown
 * @param framebuffer_height physical framebuffer height, or zero when unknown
 * @param device_scale_x horizontal device scale
 * @param device_scale_y vertical device scale
 */
static void _view_update_size_state(
    DvzView* win, uint32_t logical_width, uint32_t logical_height, uint32_t framebuffer_width,
    uint32_t framebuffer_height, float device_scale_x, float device_scale_y)
{
    ANN(win);
    device_scale_x = _view_valid_scale(device_scale_x);
    device_scale_y = _view_valid_scale(device_scale_y);

    if (logical_width == 0 && framebuffer_width > 0)
        logical_width = _view_round_size((float)framebuffer_width / device_scale_x);
    if (logical_height == 0 && framebuffer_height > 0)
        logical_height = _view_round_size((float)framebuffer_height / device_scale_y);
    if (framebuffer_width == 0 && logical_width > 0)
        framebuffer_width = _view_round_size((float)logical_width * device_scale_x);
    if (framebuffer_height == 0 && logical_height > 0)
        framebuffer_height = _view_round_size((float)logical_height * device_scale_y);

    win->logical_width = logical_width;
    win->logical_height = logical_height;
    win->framebuffer_width = framebuffer_width;
    win->framebuffer_height = framebuffer_height;
    win->device_scale_x = device_scale_x;
    win->device_scale_y = device_scale_y;
    if (win->render_scale <= 0.0f || !isfinite(win->render_scale))
        win->render_scale = 1.0f;
    if (win->user_scale <= 0.0f || !isfinite(win->user_scale))
        win->user_scale = 1.0f;

    DvzViewSizeDesc size = win->requested_size;
    if (!_view_size_desc_active(&size))
        size = dvz_view_size_desc_host_logical_px(logical_width, logical_height);
    DvzResolvedViewSize resolved = dvz_view_size_resolve(&size, win->kind);
    _view_resolved_apply_actual(
        &resolved, logical_width, logical_height, framebuffer_width, framebuffer_height,
        device_scale_x, device_scale_y);
    win->resolved_size = resolved;
}



/**
 * Return the current surface scale for one view.
 *
 * @param win view to query
 * @param out_x output horizontal scale
 * @param out_y output vertical scale
 */
static void _view_surface_scale(const DvzView* win, float* out_x, float* out_y)
{
    ANN(win);
    ANN(out_x);
    ANN(out_y);
    *out_x = win->device_scale_x > 0.0f ? win->device_scale_x : 1.0f;
    *out_y = win->device_scale_y > 0.0f ? win->device_scale_y : 1.0f;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window != NULL && dvz_window_backend_type(win->window) != DVZ_BACKEND_OFFSCREEN)
    {
        const DvzWindowSurface* surface = dvz_window_surface(win->window);
        if (surface != NULL)
        {
            *out_x = _view_valid_scale(surface->scale_x);
            *out_y = _view_valid_scale(surface->scale_y);
        }
    }
#endif
}



/**
 * Refresh cached size state from the latest frame, surface, and resize event.
 *
 * @param win view to refresh
 * @param frame current canvas frame, or NULL when unavailable
 */
static void _view_refresh_size_state(DvzView* win, const DvzStreamFrame* frame)
{
    ANN(win);
    uint32_t framebuffer_width = win->framebuffer_width;
    uint32_t framebuffer_height = win->framebuffer_height;
    uint32_t logical_width = win->logical_width;
    uint32_t logical_height = win->logical_height;
    float device_scale_x = 1.0f;
    float device_scale_y = 1.0f;
    _view_surface_scale(win, &device_scale_x, &device_scale_y);

    if (frame != NULL)
    {
        framebuffer_width = frame->extent.width;
        framebuffer_height = frame->extent.height;
    }

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window != NULL && dvz_window_backend_type(win->window) != DVZ_BACKEND_OFFSCREEN)
    {
        const DvzWindowMetrics* metrics = dvz_window_metrics(win->window);
        if (metrics != NULL)
        {
            if (metrics->surface_size.width > 0 && metrics->surface_size.height > 0)
            {
                framebuffer_width = metrics->surface_size.width;
                framebuffer_height = metrics->surface_size.height;
            }
            if (metrics->logical_size.width > 0 && metrics->logical_size.height > 0)
            {
                logical_width = metrics->logical_size.width;
                logical_height = metrics->logical_size.height;
            }
            device_scale_x = _view_valid_scale(metrics->device_scale.x);
            device_scale_y = _view_valid_scale(metrics->device_scale.y);
        }
    }

    if (win->canvas != NULL)
    {
        DvzInputRouter* router = dvz_canvas_input(win->canvas);
        DvzInputResizeEvent resize = {0};
        if (router != NULL && dvz_input_router_last_resize(router, &resize))
        {
            if (resize.framebuffer_width > 0 && resize.framebuffer_height > 0)
            {
                framebuffer_width = resize.framebuffer_width;
                framebuffer_height = resize.framebuffer_height;
            }
            const bool surface_controls_scale =
                win->window == NULL || dvz_window_backend_type(win->window) != DVZ_BACKEND_OFFSCREEN;
            if (surface_controls_scale && resize.window_width > 0 && resize.window_height > 0)
            {
                logical_width = resize.window_width;
                logical_height = resize.window_height;
            }
            if (surface_controls_scale)
            {
                device_scale_x = _view_valid_scale(resize.content_scale_x);
                device_scale_y = _view_valid_scale(resize.content_scale_y);
            }
        }
    }
#endif

    if (logical_width == 0 && win->figure != NULL && win->figure->width > 0)
        logical_width = win->figure->width;
    if (logical_height == 0 && win->figure != NULL && win->figure->height > 0)
        logical_height = win->figure->height;
    _view_update_size_state(
        win, logical_width, logical_height, framebuffer_width, framebuffer_height, device_scale_x,
        device_scale_y);
}



/**
 * Forward input-router events into the view scheduler state.
 *
 * @param router input router that emitted the event
 * @param event input event
 * @param user_data view pointer
 */
static void
_view_input_event(DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    (void)router;
    if (event == NULL)
        return;
    DvzView* win = (DvzView*)user_data;
    if (win == NULL)
        return;
    _view_mark_dirty(win);
}



/**
 * Subscribe a view to input events that invalidate rendered content.
 *
 * @param win view receiving input events
 */
static void _view_subscribe_input(DvzView* win)
{
    ANN(win);
    if (win->canvas == NULL)
        return;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router != NULL)
        dvz_input_subscribe_event(router, _view_input_event, win);
}



/**
 * Return the scheduler timestamp in nanoseconds.
 *
 * @return current timestamp in nanoseconds
 */
static uint64_t _app_scheduler_now_ns(void)
{
    uint64_t now = dvz_time_monotonic_ns();
    return now != 0 ? now : dvz_input_timestamp_ns();
}



/**
 * Poll the built-in window host for pending events.
 *
 * @param app app owning the window host
 */
static void _app_host_poll(DvzApp* app)
{
    ANN(app);
    if (app->window_host != NULL)
        dvz_window_host_poll(app->window_host);
}



/**
 * Wait for window events through the host backend.
 *
 * @param app app owning the window host
 */
static void _app_host_wait(DvzApp* app)
{
    ANN(app);
    if (app->window_host != NULL)
        dvz_window_host_wait(app->window_host);
}



/**
 * Wait until a scheduler deadline, then poll window events.
 *
 * @param app app owning the window host
 * @param deadline_ns absolute scheduler deadline in nanoseconds
 */
static void _app_host_wait_until(DvzApp* app, uint64_t deadline_ns)
{
    ANN(app);
    uint64_t now = _app_scheduler_now_ns();
    if (deadline_ns > now)
    {
        uint64_t delay_ns = deadline_ns - now;
        if (app->window_host != NULL)
            dvz_window_host_wait_timeout(app->window_host, (double)delay_ns * 1e-9);
        return;
    }
    _app_host_poll(app);
}


/**
 * Return whether one interactive view has received a native close request.
 *
 * @param win view to inspect
 * @return whether the view is closed for scheduling purposes
 */
static bool _view_close_requested(DvzView* win)
{
    ANN(win);
    return win->is_interactive && win->window != NULL && dvz_window_should_close(win->window);
}



/**
 * Return whether at least one interactive view is still open.
 *
 * @param app app to inspect
 * @return whether the interactive loop should continue
 */
static bool _app_any_interactive_window_open(DvzApp* app)
{
    ANN(app);
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (win->is_interactive && win->window != NULL && !_view_close_requested(win))
            return true;
    }
    return false;
}


/**
 * Return whether any interactive view has received a native close request.
 *
 * @param app app to inspect
 * @return whether at least one interactive view was closed
 */
static bool _app_any_interactive_window_closed(DvzApp* app)
{
    ANN(app);
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (_view_close_requested(win))
            return true;
    }
    return false;
}


/**
 * Return whether the interactive app loop should terminate.
 *
 * @param app app to inspect
 * @return whether dvz_app_run(app, 0) should return
 */
static bool _app_should_exit(DvzApp* app)
{
    ANN(app);
    if (app->stop_requested)
        return true;
    switch (app->config.exit_policy)
    {
    case DVZ_APP_EXIT_WHEN_ALL_WINDOWS_CLOSED:
        return !_app_any_interactive_window_open(app);
    case DVZ_APP_EXIT_WHEN_ANY_WINDOW_CLOSED:
        return _app_any_interactive_window_closed(app);
    case DVZ_APP_EXIT_NEVER:
        return false;
    default:
        return !_app_any_interactive_window_open(app);
    }
}


/**
 * Disconnect all current figure panels from their input router before the view router is destroyed.
 *
 * @param win the view
 */
static void _view_disconnect_figure_panels(DvzView* win)
{
    if (win == NULL || win->figure == NULL)
        return;
    for (uint32_t i = 0; i < win->figure->panel_count; i++)
        (void)dvz_panel_connect_input(&win->figure->panels[i], NULL);
}


/**
 * Release runtime resources for one closed interactive view.
 *
 * @param win view to close
 */
static void _view_close_runtime_resources(DvzView* win)
{
    ANN(win);
    _view_disconnect_figure_panels(win);
    if (win->app != NULL && win->app->gpu_ctx != NULL)
    {
        DvzDevice* device = dvz_gpu_ctx_device(win->app->gpu_ctx);
        if (device != NULL)
            dvz_device_wait(device);
    }
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
    {
        _dvz_gui_destroy(win->gui);
        win->gui = NULL;
    }
#endif
    if (win->canvas != NULL)
    {
        dvz_canvas_destroy(win->canvas);
        win->canvas = NULL;
    }
    if (win->recorder != NULL)
    {
        (void)dvz_drp2_recorder_close(win->recorder);
        win->recorder = NULL;
    }
    if (win->replay_recording != NULL)
    {
        dvz_drp2_recording_close(win->replay_recording);
        win->replay_recording = NULL;
    }
    if (win->window != NULL)
    {
        dvz_window_destroy(win->window);
        win->window = NULL;
    }
    win->render_enabled = false;
    win->is_interactive = false;
    win->dirty = false;
    win->frame_requested = false;
    win->next_frame_ns = 0;
}


/**
 * Destroy native windows that have received a close request.
 *
 * @param app app to inspect
 * @return whether any views were reaped
 */
static bool _app_reap_closed_views(DvzApp* app)
{
    ANN(app);
    bool reaped = false;
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (_view_close_requested(win))
        {
            _view_close_runtime_resources(win);
            reaped = true;
        }
    }
    return reaped;
}



/**
 * Return whether the app has active continuous rendering work.
 *
 * @param app app to inspect
 * @return whether continuous scheduling should be active
 */
static bool _app_has_continuous_work(DvzApp* app)
{
    ANN(app);
    if (app->config.schedule_mode == DVZ_APP_SCHEDULE_CONTINUOUS)
        return true;
    if (app->scene != NULL && dvz_scene_has_active_animations(app->scene))
        return true;
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (_view_close_requested(win))
            continue;
        if (win->render_enabled && win->frame_callback != NULL)
            return true;
        if (!win->render_enabled || win->replay_recording == NULL)
            continue;
        uint32_t frame_count = dvz_drp2_recording_frame_count(win->replay_recording);
        if (frame_count > 0 && (win->replay_loop || win->replay_frame_index < frame_count))
            return true;
    }
    return false;
}



/**
 * Return whether one view's figure has queued scene queries.
 *
 * @param win view to inspect
 * @return whether query work is pending for the figure
 */
static bool _view_has_pending_requests(DvzView* win)
{
    ANN(win);
    if (win->app == NULL || win->app->scene == NULL || win->figure == NULL)
        return false;
    DvzScene* scene = win->app->scene;
    for (uint32_t i = 0; i < scene->pending_query_count; i++)
    {
        DvzPendingQueryRequest* pending = &scene->pending_queries[i];
        if (pending->panel != NULL && pending->panel->figure == win->figure)
            return true;
    }
    return false;
}



/**
 * Return whether one view has retained scene work waiting for a frame.
 *
 * @param win view to inspect
 * @return whether the window's figure has dirty scene state
 */
static bool _view_has_pending_scene_work(DvzView* win)
{
    ANN(win);
    return _scene_figure_has_pending_render_work(win->figure);
}



/**
 * Return whether any enabled window has pending invalidated work.
 *
 * @param app app to inspect
 * @return whether at least one window is dirty or has a frame request
 */
static bool _app_has_pending_windows(DvzApp* app)
{
    ANN(app);
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (_view_close_requested(win))
            continue;
        if (
            win->render_enabled &&
            (win->dirty || win->frame_requested || _view_has_pending_requests(win) ||
             _view_has_pending_scene_work(win)))
            return true;
    }
    return false;
}



/**
 * Return the earliest continuous-frame deadline among enabled windows.
 *
 * @param app app to inspect
 * @return earliest frame deadline, or zero when no deadline is set
 */
static uint64_t _app_next_continuous_deadline(DvzApp* app)
{
    ANN(app);
    uint64_t deadline = 0;
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (!win->render_enabled || win->next_frame_ns == 0)
            continue;
        if (_view_close_requested(win))
            continue;
        if (deadline == 0 || win->next_frame_ns < deadline)
            deadline = win->next_frame_ns;
    }
    return deadline;
}



/**
 * Return whether one view should render on this scheduler tick.
 *
 * @param win view to inspect
 * @param continuous whether continuous scheduling is active
 * @param now current scheduler timestamp in nanoseconds
 * @return whether the view should render
 */
static bool _view_should_render(DvzView* win, bool continuous, uint64_t now)
{
    ANN(win);
    if (!win->render_enabled)
        return false;
    if (win->canvas == NULL)
        return false;
    if (_view_close_requested(win))
        return false;
    if (continuous)
    {
        if (win->next_frame_ns != 0 && now < win->next_frame_ns)
            return false;
    }
    if (win->dirty || win->frame_requested)
        return true;
    if (_view_has_posted_callbacks(win))
        return true;
    if (_view_has_pending_requests(win))
        return true;
    if (_view_has_pending_scene_work(win))
        return true;
    if (continuous)
        return win->next_frame_ns == 0 || now >= win->next_frame_ns;
    return false;
}


/**
 * Return whether one view should render on this scheduler tick.
 *
 * @param win view to inspect
 * @param continuous whether continuous scheduling is active
 * @param now current scheduler timestamp in nanoseconds
 * @return whether the view should render
 */
bool _dvz_view_scheduler_should_render(DvzView* win, bool continuous, uint64_t now)
{
    return _view_should_render(win, continuous, now);
}


/**
 * Return whether the app has active continuous rendering work.
 *
 * @param app app to inspect
 * @return whether continuous scheduling should be active
 */
bool _dvz_app_has_continuous_work(DvzApp* app)
{
    return _app_has_continuous_work(app);
}


/**
 * Forward scene request-frame notifications to the view that owns the figure.
 *
 * @param figure figure requesting a frame
 * @param user_data app pointer
 */
static void _app_scene_request_frame(DvzFigure* figure, void* user_data)
{
    DvzApp* app = (DvzApp*)user_data;
    if (app == NULL || figure == NULL)
        return;
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        if (win->figure == figure)
            dvz_view_request_frame(win);
    }
}



/**
 * Update one view's next continuous-frame deadline after a submitted frame.
 *
 * @param win view to update
 * @param now current scheduler timestamp in nanoseconds
 * @param fps_cap positive FPS cap, or zero for unlimited
 */
static void _view_update_deadline(DvzView* win, uint64_t now, double fps_cap)
{
    ANN(win);
    if (fps_cap <= 0)
    {
        win->next_frame_ns = 0;
        return;
    }

    double period = 1000000000.0 / fps_cap;
    uint64_t period_ns = period > (double)UINT64_MAX ? UINT64_MAX : (uint64_t)period;
    if (period_ns == 0)
        period_ns = 1;
    if (win->next_frame_ns > 0 && UINT64_MAX - win->next_frame_ns >= period_ns)
        win->next_frame_ns += period_ns;
    else if (UINT64_MAX - now >= period_ns)
        win->next_frame_ns = now + period_ns;
    else
        win->next_frame_ns = UINT64_MAX;
    if (win->next_frame_ns <= now)
        win->next_frame_ns = UINT64_MAX - now >= period_ns ? now + period_ns : UINT64_MAX;
}



/**
 * Add Vulkan instance extensions required by an external host.
 *
 * @param gpu_cfg GPU-context configuration receiving extension names
 * @param count number of extension names
 * @param extensions extension-name array, or NULL when count is zero
 * @return true on success, false on invalid input
 */
static bool _app_gpu_config_add_instance_extensions(
    DvzGpuCtxConfig* gpu_cfg, uint32_t count, const char* const* extensions)
{
    ANN(gpu_cfg);
    if (count == 0)
        return true;
    if (extensions == NULL)
    {
        log_error("app config requires an extension-name array when extension count is nonzero");
        return false;
    }
    if (count > 16)
    {
        log_error("app config supports at most 16 Vulkan instance extensions");
        return false;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        if (extensions[i] == NULL)
        {
            log_error("app config contains a NULL Vulkan instance extension name");
            return false;
        }
        dvz_gpu_ctx_config_add_instance_extension(gpu_cfg, extensions[i]);
    }
    return true;
}



/**
 * Add GLFW surface extensions when the default app configuration asks for them.
 *
 * @param app partially initialized app with a window host
 * @param gpu_cfg GPU-context configuration receiving extension names
 * @param config app configuration
 */
static void _app_gpu_config_add_glfw_extensions(
    DvzApp* app, DvzGpuCtxConfig* gpu_cfg, const DvzAppConfig* config)
{
    ANN(app);
    ANN(gpu_cfg);
    ANN(config);
#if DVZ_HAS_GLFW
    if (!config->enable_glfw_extensions)
        return;
    /* If GLFW is available, add surface extensions so the same instance supports windowed mode. */
    if (dvz_window_glfw_init())
    {
        uint32_t ext_count =
            dvz_window_host_required_extension_count(app->window_host, DVZ_BACKEND_GLFW);
        if (ext_count > 0)
        {
            const char* extensions[16] = {0};
            int written = dvz_window_host_required_extensions(
                app->window_host, DVZ_BACKEND_GLFW, ext_count, extensions);
            if (written == (int)ext_count)
            {
                for (uint32_t i = 0; i < ext_count; i++)
                    dvz_gpu_ctx_config_add_instance_extension(gpu_cfg, extensions[i]);
                dvz_gpu_ctx_config_enable_canvas_extensions(gpu_cfg, true);
            }
        }
    }
#else
    (void)app;
    (void)gpu_cfg;
    (void)config;
#endif
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
/**
 * Return whether a borrowed runtime can execute with a borrowed GPU context.
 *
 * @param runtime borrowed DRP2 runtime
 * @param gpu_ctx borrowed GPU context
 * @return true when the runtime uses the GPU context's device and allocator
 */
static bool _app_runtime_matches_gpu_ctx(DvzDrp2Runtime* runtime, DvzGpuCtx* gpu_ctx)
{
    ANN(runtime);
    ANN(gpu_ctx);

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_get_config(runtime);
    if (cfg.semantic_only)
        return false;
    return cfg.device == dvz_gpu_ctx_device(gpu_ctx) &&
           cfg.allocator == dvz_gpu_ctx_alloc(gpu_ctx);
}



/**
 * Destroy top-level app resources according to the app ownership flags.
 *
 * @param app app whose top-level resources should be released or detached
 */
static void _app_destroy_resources(DvzApp* app)
{
    ANN(app);

    if (app->runtime != NULL)
    {
        if (app->owns_runtime)
            dvz_drp2_runtime_destroy(app->runtime);
        app->runtime = NULL;
    }
    app->owns_runtime = false;

    if (app->gpu_ctx != NULL)
    {
        if (app->owns_gpu_ctx)
            dvz_gpu_ctx_destroy(app->gpu_ctx);
        app->gpu_ctx = NULL;
    }
    app->owns_gpu_ctx = false;

    if (app->window_host != NULL)
    {
        if (app->owns_window_host)
            dvz_window_host_destroy(app->window_host);
        app->window_host = NULL;
    }
    app->owns_window_host = false;
}
#endif



/**
 * Return a readable label for one DRP2 command type.
 *
 * @param type command type enum value
 * @return static command label
 */
static const char* _trace_command_name(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        return "HelloRenderer";
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        return "RendererHelloReply";
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return "CreateBuffer";
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return "DestroyBuffer";
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return "CreateTexture";
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return "DestroyTexture";
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return "CreateShaderModule";
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return "DestroyShaderModule";
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return "CreateRenderPipeline";
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return "DestroyRenderPipeline";
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return "CreateComputePipeline";
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return "DestroyComputePipeline";
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return "CreateSampler";
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return "CreateBindGroupLayout";
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return "CreateBindGroup";
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return "DestroyBindGroupLayout";
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return "DestroyBindGroup";
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return "WriteBuffer";
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return "WriteTexture";
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return "BeginCommandEncoder";
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return "BeginRenderPass";
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return "BeginComputePass";
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        return "SetViewport";
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        return "SetScissor";
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return "SetPipeline";
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        return "SetBindGroup";
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return "SetVertexBuffer";
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return "SetIndexBuffer";
    case DVZ_DRP2_COMMAND_DRAW:
        return "Draw";
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        return "DrawIndexed";
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return "EndRenderPass";
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return "DispatchWorkgroups";
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        return "EndComputePass";
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return "CopyBufferToBuffer";
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return "CopyBufferToTexture";
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return "CopyTextureToBuffer";
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return "CopyTextureToTexture";
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        return "FinishCommandEncoder";
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        return "QueueSubmit";
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return "QueueSubmitReply";
    case DVZ_DRP2_COMMAND_NONE:
    default:
        return "None";
    }
}


/**
 * Return the display prefix used for one DRP2 command type in human trace mode.
 *
 * @param type command type enum value
 * @return one-character semantic prefix
 */
static char _trace_command_prefix(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return '+';
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return '-';
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return '~';
    default:
        return '=';
    }
}


/**
 * Return whether DRP2 trace color should be emitted on stderr.
 *
 * @return whether ANSI color should be used
 */
static bool _trace_color_enabled(void)
{
    const char* env = getenv("DVZ_DRP2_TRACE_COLOR");
    if (env != NULL)
    {
        if (strcmp(env, "0") == 0 || strcmp(env, "false") == 0 || strcmp(env, "FALSE") == 0 ||
            strcmp(env, "off") == 0 || strcmp(env, "OFF") == 0)
            return false;
        if (strcmp(env, "1") == 0 || strcmp(env, "true") == 0 || strcmp(env, "TRUE") == 0 ||
            strcmp(env, "on") == 0 || strcmp(env, "ON") == 0)
            return true;
    }

    if (getenv("NO_COLOR") != NULL)
        return false;

    return true;
}


/**
 * Return the ANSI color for one command category.
 *
 * @param type command type enum value
 * @return static ANSI color string
 */
static const char* _trace_command_color(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return DVZ_TRACE_COLOR_GREEN;
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return DVZ_TRACE_COLOR_RED;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return DVZ_TRACE_COLOR_YELLOW;
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return DVZ_TRACE_COLOR_CYAN;
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return DVZ_TRACE_COLOR_BLUE;
    case DVZ_DRP2_COMMAND_DRAW:
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return DVZ_TRACE_COLOR_MAGENTA;
    default:
        return DVZ_TRACE_COLOR_DIM;
    }
}


/**
 * Format a DRP2 id with an optional stream debug label.
 *
 * @param stream command stream carrying optional labels
 * @param id numeric DRP2 id
 * @param out output string buffer
 * @param out_size output buffer size
 */
static void _trace_format_id(
    const DvzDrp2CommandStream* stream, uint64_t id, char* out, size_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    const char* label = dvz_drp2_stream_label(stream, id);
    if (label != NULL)
    {
        dvz_snprintf(
            out, out_size, "%" PRIu64 "(%.*s)", id, DVZ_APP_TRACE_LABEL_PRINT_SIZE, label);
    }
    else
        dvz_snprintf(out, out_size, "%" PRIu64, id);
    out[out_size - 1] = '\0';
}


/**
 * Print one detailed command line when the command carries useful trace payload.
 *
 * @param stream command stream carrying optional labels
 * @param command command to print
 * @param index command index
 * @param include_transient_ids whether to include per-stream pass ids
 * @return true if a line was printed
 */
static bool _app_trace_print_command_detail(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command, uint32_t index,
    bool include_transient_ids)
{
    ANN(command);
#define TRACE_ID(name, value)                                                                       \
    char name[DVZ_DRP2_LABEL_SIZE + 64];                                                            \
    _trace_format_id(stream, (value), name, sizeof(name))

    DvzDrp2CommandType type = command->type;
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        dvz_fprintf(
            stderr, "  %03u = HelloRenderer name=%.*s\n", index,
            DVZ_APP_TRACE_LABEL_PRINT_SIZE, command->u.handshake.name);
        return true;
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        dvz_fprintf(
            stderr, "  %03u = RendererHelloReply name=%.*s\n", index,
            DVZ_APP_TRACE_LABEL_PRINT_SIZE, command->u.handshake.name);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    {
        TRACE_ID(id, command->u.create_buffer.id);
        dvz_fprintf(
            stderr, "  %03u + CreateBuffer id=%s size=%" PRIu64 " usage=0x%" PRIx32 "\n",
            index, id, command->u.create_buffer.size,
            command->u.create_buffer.usage);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    {
        TRACE_ID(id, command->u.destroy_buffer.buffer_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyBuffer id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    {
        TRACE_ID(id, command->u.create_texture.id);
        dvz_fprintf(
            stderr, "  %03u + CreateTexture id=%s size=(%" PRIu32 ",%" PRIu32
                    ",%" PRIu32 ") usage=0x%" PRIx32 " samples=%" PRIu32 "\n",
            index, id, command->u.create_texture.width,
            command->u.create_texture.height, command->u.create_texture.depth,
            command->u.create_texture.usage,
            command->u.create_texture.sample_count != 0 ? command->u.create_texture.sample_count :
                                                          1);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    {
        TRACE_ID(id, command->u.destroy_texture.texture_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyTexture id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    {
        TRACE_ID(id, command->u.create_shader_module.id);
        dvz_fprintf(
            stderr, "  %03u + CreateShaderModule id=%s stage=%.*s format=%.*s"
                    " code=%s spirv_size=%" PRIu64 "\n",
            index, id, DVZ_APP_TRACE_LABEL_PRINT_SIZE, command->u.create_shader_module.stage,
            DVZ_APP_TRACE_LABEL_PRINT_SIZE, command->u.create_shader_module.format,
            command->u.create_shader_module.code != NULL ? "yes" : "no",
            command->u.create_shader_module.spirv_size);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    {
        TRACE_ID(id, command->u.destroy_shader_module.shader_module_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyShaderModule id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    {
        TRACE_ID(id, command->u.create_render_pipeline.id);
        TRACE_ID(vs, command->u.create_render_pipeline.vertex_shader_module_id);
        TRACE_ID(fs, command->u.create_render_pipeline.fragment_shader_module_id);
        dvz_fprintf(
            stderr, "  %03u + CreateRenderPipeline id=%s vs=%s fs=%s"
                    " vslots=%" PRIu32 " bgls=%" PRIu32
                    " depth=%s write=%s compare=%" PRIu32 " topology=%" PRIu32
                    " samples=%" PRIu32 " a2c=%s bindings=%" PRIu32 " attrs=%" PRIu32 "\n",
            index, id, vs, fs,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.bind_group_layout_count,
            command->u.create_render_pipeline.has_depth_attachment ? "yes" : "no",
            command->u.create_render_pipeline.depth_write_enabled ? "yes" : "no",
            command->u.create_render_pipeline.depth_compare_op,
            command->u.create_render_pipeline.topology,
            command->u.create_render_pipeline.sample_count != 0 ?
                command->u.create_render_pipeline.sample_count :
                1,
            command->u.create_render_pipeline.alpha_to_coverage_enabled ? "yes" : "no",
            command->u.create_render_pipeline.binding_count,
            command->u.create_render_pipeline.attr_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    {
        TRACE_ID(id, command->u.destroy_render_pipeline.render_pipeline_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyRenderPipeline id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u + CreateComputePipeline id=%" PRIu64 " shader=%" PRIu64
                    " bgls=%" PRIu32 "\n",
            index, command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id,
            command->u.create_compute_pipeline.bind_group_layout_count);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u - DestroyComputePipeline id=%" PRIu64 "\n", index,
            command->u.destroy_compute_pipeline.compute_pipeline_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    {
        TRACE_ID(id, command->u.create_sampler.id);
        dvz_fprintf(
            stderr, "  %03u + CreateSampler id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    {
        TRACE_ID(id, command->u.create_bind_group_layout.id);
        dvz_fprintf(
            stderr, "  %03u + CreateBindGroupLayout id=%s entries=%" PRIu32 "\n",
            index, id,
            command->u.create_bind_group_layout.entry_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
    {
        TRACE_ID(id, command->u.create_bind_group.id);
        TRACE_ID(layout, command->u.create_bind_group.bind_group_layout_id);
        dvz_fprintf(
            stderr, "  %03u + CreateBindGroup id=%s layout=%s entries=%" PRIu32 "\n",
            index, id, layout,
            command->u.create_bind_group.entry_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        dvz_fprintf(
            stderr, "  %03u - DestroyBindGroupLayout id=%" PRIu64 "\n", index,
            command->u.destroy_bind_group_layout.bind_group_layout_id);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        dvz_fprintf(
            stderr, "  %03u - DestroyBindGroup id=%" PRIu64 "\n", index,
            command->u.destroy_bind_group.bind_group_id);
        return true;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    {
        TRACE_ID(buffer, command->u.write_buffer.buffer_id);
        dvz_fprintf(
            stderr, "  %03u ~ WriteBuffer buffer=%s offset=%" PRIu64
                    " size=%" PRIu64 "\n",
            index, buffer, command->u.write_buffer.offset,
            command->u.write_buffer.size);
        return true;
    }
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    {
        TRACE_ID(texture, command->u.write_texture.texture_id);
        dvz_fprintf(
            stderr, "  %03u ~ WriteTexture texture=%s origin=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32 ",%" PRIu32 ",%" PRIu32
                    ") bytes_per_row=%" PRIu32 "\n",
            index, texture, command->u.write_texture.origin_x,
            command->u.write_texture.origin_y, command->u.write_texture.origin_z,
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth, command->u.write_texture.bytes_per_row);
        return true;
    }
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        dvz_fprintf(
            stderr, "  %03u = BeginCommandEncoder id=%" PRIu64 "\n", index,
            command->u.begin_command_encoder.id);
        return true;
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
    {
        TRACE_ID(id, command->u.begin_render_pass.id);
        TRACE_ID(encoder, command->u.begin_render_pass.encoder_id);
        TRACE_ID(target, command->u.begin_render_pass.texture_id);
        TRACE_ID(depth, command->u.begin_render_pass.depth_texture_id);
        dvz_fprintf(
            stderr, "  %03u = BeginRenderPass id=%s encoder=%s"
                    " target=%s clear=%s depth=%s depth_target=%s clear_depth=%.3g"
                    " viewport=(%.3g,%.3g %.3gx%.3g)\n",
            index, id, encoder, target,
            command->u.begin_render_pass.clear ? "yes" : "load",
            command->u.begin_render_pass.has_depth_attachment ? "yes" : "no",
            depth,
            (double)command->u.begin_render_pass.clear_depth,
            (double)command->u.begin_render_pass.viewport[0],
            (double)command->u.begin_render_pass.viewport[1],
            (double)command->u.begin_render_pass.viewport[2],
            (double)command->u.begin_render_pass.viewport[3]);
        return true;
    }
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        dvz_fprintf(
            stderr, "  %03u = BeginComputePass id=%" PRIu64 " encoder=%" PRIu64 "\n",
            index, command->u.begin_compute_pass.id, command->u.begin_compute_pass.encoder_id);
        return true;
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        dvz_fprintf(
            stderr, "  %03u = SetViewport pass=%" PRIu64
                    " viewport=(%.3g,%.3g %.3gx%.3g)\n",
            index, command->u.set_viewport.pass_id,
            (double)command->u.set_viewport.viewport[0],
            (double)command->u.set_viewport.viewport[1],
            (double)command->u.set_viewport.viewport[2],
            (double)command->u.set_viewport.viewport[3]);
        return true;
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        dvz_fprintf(
            stderr, "  %03u = SetScissor pass=%" PRIu64
                    " scissor=(%.3g,%.3g %.3gx%.3g)\n",
            index, command->u.set_scissor.pass_id,
            (double)command->u.set_scissor.scissor[0],
            (double)command->u.set_scissor.scissor[1],
            (double)command->u.set_scissor.scissor[2],
            (double)command->u.set_scissor.scissor[3]);
        return true;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u ~ CopyBufferToBuffer encoder=%" PRIu64
                        " src=%" PRIu64 ":%" PRIu64 " dst=%" PRIu64 ":%" PRIu64
                        " size=%" PRIu64 "\n",
                index, command->u.copy_buffer_to_buffer.encoder_id,
                command->u.copy_buffer_to_buffer.src_buffer_id,
                command->u.copy_buffer_to_buffer.src_offset,
                command->u.copy_buffer_to_buffer.dst_buffer_id,
                command->u.copy_buffer_to_buffer.dst_offset,
                command->u.copy_buffer_to_buffer.size);
        else
            dvz_fprintf(
                stderr, "  %03u ~ CopyBufferToBuffer src=%" PRIu64 ":%" PRIu64
                        " dst=%" PRIu64 ":%" PRIu64 " size=%" PRIu64 "\n",
                index, command->u.copy_buffer_to_buffer.src_buffer_id,
                command->u.copy_buffer_to_buffer.src_offset,
                command->u.copy_buffer_to_buffer.dst_buffer_id,
                command->u.copy_buffer_to_buffer.dst_offset,
                command->u.copy_buffer_to_buffer.size);
        return true;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u ~ CopyBufferToTexture encoder=%" PRIu64
                    " buffer=%" PRIu64 ":%" PRIu64 " texture=%" PRIu64
                    " origin=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ") bytes_per_row=%" PRIu32 "\n",
            index, command->u.copy_buffer_to_texture.encoder_id,
            command->u.copy_buffer_to_texture.src_buffer_id,
            command->u.copy_buffer_to_texture.src_offset,
            command->u.copy_buffer_to_texture.dst_texture_id,
            command->u.copy_buffer_to_texture.dst_origin_x,
            command->u.copy_buffer_to_texture.dst_origin_y,
            command->u.copy_buffer_to_texture.dst_origin_z,
            command->u.copy_buffer_to_texture.width,
            command->u.copy_buffer_to_texture.height,
            command->u.copy_buffer_to_texture.depth,
            command->u.copy_buffer_to_texture.bytes_per_row);
        return true;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u ~ CopyTextureToBuffer encoder=%" PRIu64
                        " texture=%" PRIu64 " buffer=%" PRIu64 ":%" PRIu64
                        " size=(%" PRIu32 ",%" PRIu32 ") bytes_per_row=%" PRIu32 "\n",
                index, command->u.copy_texture_to_buffer.encoder_id,
                command->u.copy_texture_to_buffer.src_texture_id,
                command->u.copy_texture_to_buffer.dst_buffer_id,
                command->u.copy_texture_to_buffer.dst_offset,
                command->u.copy_texture_to_buffer.width,
                command->u.copy_texture_to_buffer.height,
                command->u.copy_texture_to_buffer.bytes_per_row);
        else
            dvz_fprintf(
                stderr, "  %03u ~ CopyTextureToBuffer texture=%" PRIu64
                        " buffer=%" PRIu64 ":%" PRIu64 " size=(%" PRIu32 ",%" PRIu32
                        ") bytes_per_row=%" PRIu32 "\n",
                index, command->u.copy_texture_to_buffer.src_texture_id,
                command->u.copy_texture_to_buffer.dst_buffer_id,
                command->u.copy_texture_to_buffer.dst_offset,
                command->u.copy_texture_to_buffer.width,
                command->u.copy_texture_to_buffer.height,
                command->u.copy_texture_to_buffer.bytes_per_row);
        return true;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u ~ CopyTextureToTexture encoder=%" PRIu64
                    " src=%" PRIu64 " dst=%" PRIu64
                    " origin=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ")\n",
            index, command->u.copy_texture_to_texture.encoder_id,
            command->u.copy_texture_to_texture.src_texture_id,
            command->u.copy_texture_to_texture.dst_texture_id,
            command->u.copy_texture_to_texture.dst_origin_x,
            command->u.copy_texture_to_texture.dst_origin_y,
            command->u.copy_texture_to_texture.dst_origin_z,
            command->u.copy_texture_to_texture.width,
            command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth);
        return true;
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
    {
        TRACE_ID(pipeline, command->u.set_pipeline.pipeline_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_pipeline.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetPipeline pass=%s pipeline=%s\n", index, pass, pipeline);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetPipeline pipeline=%s\n", index, pipeline);
        return true;
    }
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
    {
        TRACE_ID(bind_group, command->u.set_bind_group.bind_group_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_bind_group.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetBindGroup pass=%s slot=%" PRIu32
                        " bind_group=%s offsets=%" PRIu32 "\n",
                index, pass, command->u.set_bind_group.slot, bind_group,
                command->u.set_bind_group.dynamic_offset_count);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetBindGroup slot=%" PRIu32 " bind_group=%s offsets=%" PRIu32
                        "\n",
                index, command->u.set_bind_group.slot, bind_group,
                command->u.set_bind_group.dynamic_offset_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
    {
        TRACE_ID(buffer, command->u.set_vertex_buffer.buffer_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_vertex_buffer.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetVertexBuffer pass=%s slot=%" PRIu32
                        " buffer=%s offset=%" PRIu64 "\n",
                index, pass, command->u.set_vertex_buffer.slot, buffer,
                command->u.set_vertex_buffer.offset);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetVertexBuffer slot=%" PRIu32
                        " buffer=%s offset=%" PRIu64 "\n",
                index, command->u.set_vertex_buffer.slot, buffer,
                command->u.set_vertex_buffer.offset);
        return true;
    }
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
    {
        TRACE_ID(buffer, command->u.set_index_buffer.buffer_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_index_buffer.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetIndexBuffer pass=%s"
                        " buffer=%s format=%.*s offset=%" PRIu64 "\n",
                index, pass, buffer, DVZ_APP_TRACE_LABEL_PRINT_SIZE,
                command->u.set_index_buffer.index_format,
                command->u.set_index_buffer.offset);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetIndexBuffer buffer=%s"
                        " format=%.*s offset=%" PRIu64 "\n",
                index, buffer, DVZ_APP_TRACE_LABEL_PRINT_SIZE,
                command->u.set_index_buffer.index_format,
                command->u.set_index_buffer.offset);
        return true;
    }
    case DVZ_DRP2_COMMAND_DRAW:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = Draw pass=%" PRIu64 " vertices=%" PRIu32
                        " first=%" PRIu32 " instances=%" PRIu32 "\n",
                index, command->u.draw.pass_id, command->u.draw.vertex_count,
                command->u.draw.first_vertex, command->u.draw.instance_count);
        else
            dvz_fprintf(
                stderr, "  %03u = Draw vertices=%" PRIu32
                        " first=%" PRIu32 " instances=%" PRIu32 "\n",
                index, command->u.draw.vertex_count, command->u.draw.first_vertex,
                command->u.draw.instance_count);
        return true;
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = DrawIndexed pass=%" PRIu64 " indices=%" PRIu32
                        " first=%" PRIu32 " base=%" PRId32 "\n",
                index, command->u.draw_indexed.pass_id, command->u.draw_indexed.index_count,
                command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex);
        else
            dvz_fprintf(
                stderr, "  %03u = DrawIndexed indices=%" PRIu32
                        " first=%" PRIu32 " base=%" PRId32 "\n",
                index, command->u.draw_indexed.index_count,
                command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex);
        return true;
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        dvz_fprintf(
            stderr, "  %03u = EndRenderPass pass=%" PRIu64 "\n", index,
            command->u.end_render_pass.pass_id);
        return true;
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        dvz_fprintf(
            stderr, "  %03u = DispatchWorkgroups pass=%" PRIu64 " groups=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ")\n",
            index, command->u.dispatch.pass_id, command->u.dispatch.x,
            command->u.dispatch.y, command->u.dispatch.z);
        return true;
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        dvz_fprintf(
            stderr, "  %03u = EndComputePass pass=%" PRIu64 "\n", index,
            command->u.end_compute_pass.pass_id);
        return true;
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        dvz_fprintf(
            stderr, "  %03u = FinishCommandEncoder encoder=%" PRIu64
                    " command_buffer=%" PRIu64 "\n",
            index, command->u.finish_command_encoder.encoder_id,
            command->u.finish_command_encoder.command_buffer_id);
        return true;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        dvz_fprintf(
            stderr, "  %03u = QueueSubmit command_buffer=%" PRIu64
                    " submission=%" PRIu64 " readback=%s buffer=%" PRIu64
                    " offset=%" PRIu64 " size=%" PRIu64 " data=%s\n",
            index, command->u.queue_submit.command_buffer_id,
            command->u.queue_submit.submission_id,
            command->u.queue_submit.has_readback ? "yes" : "no",
            command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
            command->u.queue_submit.size,
            command->u.queue_submit.data_base64[0] != '\0' ? "yes" : "no");
        return true;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        dvz_fprintf(
            stderr, "  %03u = QueueSubmitReply submission=%" PRIu64
                    " readback=%s buffer=%" PRIu64 " offset=%" PRIu64
                    " size=%" PRIu64 " data=%s\n",
            index, command->u.queue_submit.submission_id,
            command->u.queue_submit.has_readback ? "yes" : "no",
            command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
            command->u.queue_submit.size,
            command->u.queue_submit.data_base64[0] != '\0' ? "yes" : "no");
        return true;
    case DVZ_DRP2_COMMAND_NONE:
        dvz_fprintf(stderr, "  %03u = None\n", index);
        return true;
    default:
        break;
    }
#undef TRACE_ID
    return false;
}


/**
 * Print an expanded human-readable dump for one artifact stream snapshot.
 *
 * @param stream artifact-owned command stream snapshot
 * @param frame_index 0-based frame index for the owning window
 * @param label short trace label for the frame header
 */
static void _app_trace_stream_full(
    const DvzDrp2CommandStream* stream, uint64_t frame_index, const char* label)
{
    ANN(stream);
    ANN(label);
    uint32_t command_count = dvz_drp2_stream_count(stream);
    bool use_color = _trace_color_enabled();
    dvz_fprintf(
        stderr, "\n%sframe %08" PRIu64 " | %s | %u cmds%s\n",
        use_color ? DVZ_TRACE_COLOR_BOLD : "", frame_index, label, command_count,
        use_color ? DVZ_TRACE_COLOR_RESET : "");
    for (uint32_t i = 0; i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
            continue;
        DvzDrp2CommandType type = dvz_drp2_command_type(command);
        if (use_color)
            dvz_fprintf(stderr, "%s", _trace_command_color(type));
        if (!_app_trace_print_command_detail(stream, command, i, true))
        {
            dvz_fprintf(
                stderr, "  %03u %c %s\n", i, _trace_command_prefix(type),
                _trace_command_name(type));
        }
    }
    if (use_color)
        dvz_fprintf(stderr, "%s", DVZ_TRACE_COLOR_RESET);
}


/**
 * Print or refresh the live DRP2 trace for one frame artifact stream snapshot.
 *
 * In normal mode, changed frames print one expanded human-readable command list while unchanged
 * frames rewrite one in-place status line without scrolling. In full mode, every frame prints an
 * expanded command list.
 *
 * @param win view owning the trace state
 * @param stream the artifact-owned command stream snapshot
 */
static void _app_trace_stream(DvzView* win, const DvzDrp2CommandStream* stream)
{
    ANN(win);
    ANN(stream);
    const char* trace_env = getenv("DVZ_DRP2_TRACE");
    DvzAppTraceMode mode = _dvz_app_trace_mode_from_env(trace_env);
    if (mode == DVZ_APP_TRACE_NONE)
        return;

    uint32_t command_count = dvz_drp2_stream_count(stream);

    DvzAppTraceSnapshot snapshot;
    _dvz_app_trace_snapshot_init(&snapshot);
    if (!_dvz_app_trace_snapshot_build(&snapshot, stream))
    {
        log_error("failed to build normalized DRP2 trace snapshot");
        return;
    }

    bool changed = !win->has_last_trace_snapshot ||
                   !_dvz_app_trace_snapshot_equal(&win->last_trace_snapshot, &snapshot);
    DvzAppTracePlan plan =
        _dvz_app_trace_plan(mode, win->app->status.line_open, changed);

    if (plan.prepend_newline)
        _dvz_app_status_break_line(&win->app->status);

    if (mode == DVZ_APP_TRACE_FULL)
    {
        _app_trace_stream_full(stream, win->frame_index, "full");
        _dvz_app_status_trace(
            &win->app->status, win->frame_index, command_count, snapshot.count, true);
        _dvz_app_status_render(&win->app->status);
    }
    else
    {
        if (plan.event_kind == DVZ_APP_TRACE_EVENT_CHANGED)
        {
            _app_trace_stream_full(stream, win->frame_index, "changed");
        }
        _dvz_app_status_trace(
            &win->app->status, win->frame_index, command_count, snapshot.count, changed);
        _dvz_app_status_render(&win->app->status);
    }
    _dvz_app_trace_snapshot_destroy(&win->last_trace_snapshot);
    win->last_trace_snapshot = snapshot;
    win->has_last_trace_snapshot = true;
}

#endif



/*************************************************************************************************/
/*  Draw callback                                                                                */
/*************************************************************************************************/

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE

/**
 * Synchronize the figure size with the current output before emitting a frame.
 *
 * @param win view being drawn
 * @param frame canvas frame attached to the DRP2 runtime
 */
static void _app_sync_figure_size(DvzView* win, const DvzStreamFrame* frame)
{
    ANN(win);
    ANN(win->figure);
    ANN(frame);

    _view_refresh_size_state(win, frame);
    _view_sync_figure_layout_size(win);
}



/**
 * Return a frame-local scope key for mutable app-frame runtime intermediates.
 *
 * @param frame canvas frame attached to the DRP2 runtime
 * @return scope id based on the borrowed command buffer
 */
static uint64_t _app_frame_runtime_scope(const DvzStreamFrame* frame)
{
    ANN(frame);
    uint64_t scope = (uint64_t)(uintptr_t)frame->command_buffer;
    return scope != 0 ? scope : UINT64_C(1);
}



/**
 * Return a readable label for one DRP2 validation code.
 *
 * @param code validation result code
 * @return static validation label
 */
static const char* _app_validation_name(DvzDrp2ValidationCode code)
{
    switch (code)
    {
    case DVZ_DRP2_VALIDATION_OK:
        return "OK";
    case DVZ_DRP2_VALIDATION_INVALID_ARGUMENT:
        return "InvalidArgument";
    case DVZ_DRP2_VALIDATION_INVALID_STATE:
        return "InvalidState";
    case DVZ_DRP2_VALIDATION_OUT_OF_RANGE:
        return "OutOfRange";
    case DVZ_DRP2_VALIDATION_USAGE:
        return "Usage";
    default:
        return "Unknown";
    }
}



/**
 * Return the primary object id referenced by one command.
 *
 * @param command DRP2 command
 * @return the command's most useful object id, or zero when none applies
 */
static uint64_t _app_command_primary_id(const DvzDrp2Command* command)
{
    if (command == NULL)
        return 0;

    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return command->u.create_buffer.id;
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return command->u.destroy_buffer.buffer_id;
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return command->u.create_texture.id;
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return command->u.destroy_texture.texture_id;
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return command->u.create_shader_module.id;
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return command->u.destroy_shader_module.shader_module_id;
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return command->u.create_render_pipeline.id;
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return command->u.destroy_render_pipeline.render_pipeline_id;
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return command->u.create_compute_pipeline.id;
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return command->u.destroy_compute_pipeline.compute_pipeline_id;
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return command->u.create_sampler.id;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return command->u.create_bind_group_layout.id;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return command->u.create_bind_group.id;
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return command->u.destroy_bind_group_layout.bind_group_layout_id;
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return command->u.destroy_bind_group.bind_group_id;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return command->u.write_buffer.buffer_id;
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return command->u.write_texture.texture_id;
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return command->u.begin_command_encoder.id;
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return command->u.begin_render_pass.id;
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return command->u.begin_compute_pass.id;
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return command->u.set_pipeline.pipeline_id;
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        return command->u.set_bind_group.bind_group_id;
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return command->u.set_vertex_buffer.buffer_id;
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return command->u.set_index_buffer.buffer_id;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return command->u.copy_texture_to_buffer.src_texture_id;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return command->u.copy_buffer_to_texture.dst_texture_id;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return command->u.copy_texture_to_texture.dst_texture_id;
    default:
        return 0;
    }
}



/**
 * Return whether two app runtime failure signatures match.
 *
 * @param a first failure signature
 * @param b second failure signature
 * @return whether both signatures describe the same repeated runtime failure
 */
static bool
_app_runtime_failure_equal(const DvzAppRuntimeFailure* a, const DvzAppRuntimeFailure* b)
{
    ANN(a);
    ANN(b);
    return a->code == b->code && a->type == b->type &&
           a->command_index == b->command_index && a->command_count == b->command_count;
}



/**
 * Clear repeated runtime-failure state after a successful frame.
 *
 * @param win view carrying failure state
 * @param context short recovery context
 */
static void _app_runtime_failure_reset(DvzView* win, const char* context)
{
    ANN(win);
    ANN(context);
    if (win->has_last_runtime_failure && win->runtime_failure_repeat_count > 0)
    {
        log_warn(
            "%s recovered; suppressed %" PRIu32 " repeated runtime failure%s", context,
            win->runtime_failure_repeat_count,
            win->runtime_failure_repeat_count == 1 ? "" : "s");
    }
    win->has_last_runtime_failure = false;
    win->runtime_failure_repeat_count = 0;
}


static void _app_runtime_recovery_defer(DvzApp* app)
{
    ANN(app);
    app->runtime_recovery_pending = true;
    for (uint32_t i = 0; i < app->view_count; i++)
        dvz_view_request_frame(&app->views[i]);
}


static bool _app_runtime_recovery_apply(DvzApp* app)
{
    ANN(app);
    if (!app->runtime_recovery_pending)
        return true;
    if (app->runtime != NULL)
        dvz_drp2_runtime_reset(app->runtime);
    if (!_scene_runtime_emitter_reset(app->scene))
    {
        log_error("_app_draw failed to reset scene runtime emitter after runtime failure");
        return false;
    }
    app->runtime_recovery_pending = false;
    return true;
}



/**
 * Report one failed DRP2 runtime execution, suppressing exact repeats.
 *
 * @param win view carrying failure state
 * @param prefix short failure context
 * @param stream artifact-owned command stream snapshot
 * @param result the failed validation result
 */
static void _app_log_runtime_failure(
    DvzView* win, const char* prefix, const DvzDrp2CommandStream* stream,
    DvzDrp2ValidationResult result)
{
    ANN(win);
    ANN(prefix);
    ANN(stream);

    DvzDrp2CommandType type = DVZ_DRP2_COMMAND_NONE;
    const DvzDrp2Command* failed = dvz_drp2_stream_get(stream, result.command_index);
    if (failed != NULL)
        type = dvz_drp2_command_type(failed);
    uint32_t command_count = dvz_drp2_stream_count(stream);
    uint64_t id = _app_command_primary_id(failed);
    DvzAppRuntimeFailure signature = {
        .code = result.code,
        .type = type,
        .command_index = result.command_index,
        .command_count = command_count,
    };

    if (win->has_last_runtime_failure &&
        _app_runtime_failure_equal(&win->last_runtime_failure, &signature))
    {
        if (win->runtime_failure_repeat_count < UINT32_MAX)
            win->runtime_failure_repeat_count++;
        return;
    }

    if (win->has_last_runtime_failure && win->runtime_failure_repeat_count > 0)
    {
        log_warn(
            "%s: suppressed %" PRIu32 " repeated runtime failure%s", prefix,
            win->runtime_failure_repeat_count,
            win->runtime_failure_repeat_count == 1 ? "" : "s");
    }

    char id_text[DVZ_DRP2_LABEL_SIZE + 64];
    if (id != 0)
        _trace_format_id(stream, id, id_text, sizeof(id_text));
    else
        dvz_snprintf(id_text, sizeof(id_text), "0");

    log_error(
        "%s: code=%d (%s) command=%" PRIu32 "/%" PRIu32 " type=%d (%s) id=%s",
        prefix, (int)result.code, _app_validation_name(result.code), result.command_index,
        command_count, (int)type, _trace_command_name(type), id_text);
    if (failed != NULL)
        (void)_app_trace_print_command_detail(stream, failed, result.command_index, true);

    win->last_runtime_failure = signature;
    win->has_last_runtime_failure = true;
    win->runtime_failure_repeat_count = 0;
}



/**
 * Return whether a frame callback may safely run mutation-oriented user code.
 *
 * @param win view owning the callback
 * @return true when scene mutation may run after artifact emission
 */
static bool _app_frame_callback_allowed(DvzView* win)
{
    ANN(win);
    if (win->app == NULL || win->app->scene == NULL)
        return true;
    return _scene_visual_mutation_allowed(win->app->scene, "run app frame callback");
}



/**
 * Append one app frame artifact stream snapshot to the active DVZR recorder.
 *
 * @param win view owning the recorder
 * @param frame borrowed stream frame for target dimensions
 * @param stream artifact-owned scene stream snapshot
 */
static void _app_record_stream(
    DvzView* win, const DvzStreamFrame* frame, const DvzDrp2CommandStream* stream)
{
    ANN(win);
    ANN(frame);
    ANN(stream);
    if (win->recorder == NULL)
        return;

    double t_present = dvz_clock_get(&win->recording_clock);
    if (
        win->recording_fps > 0 && win->recording_has_last_frame &&
        t_present - win->recording_last_t_present < 1.0 / win->recording_fps)
    {
        return;
    }

    if (!win->recording_target_created)
    {
        DvzDrp2CommandStream* setup = dvz_drp2_stream();
        if (setup == NULL ||
            !dvz_drp2_stream_hello_renderer(setup, "app-recording") ||
            !dvz_drp2_stream_renderer_hello_reply(setup, "datoviz-drp2-runtime") ||
            !dvz_drp2_stream_create_texture_2d_format_usage(
                setup, win->target_id, frame->extent.width, frame->extent.height,
                (DvzFormat)frame->color_format,
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) ||
            !dvz_drp2_recorder_write_stream(win->recorder, t_present, setup))
        {
            log_error("_app_draw failed to append DVZR target setup stream");
            dvz_drp2_stream_destroy(setup);
            return;
        }
        dvz_drp2_stream_destroy(setup);
        win->recording_target_created = true;
    }

    const DvzDrp2CommandStream* recorded = stream;
    DvzDrp2CommandStream filtered = {0};
    if (dvz_drp2_stream_count(stream) >= 2 &&
        dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) ==
            DVZ_DRP2_COMMAND_HELLO_RENDERER &&
        dvz_drp2_command_type(dvz_drp2_stream_get(stream, 1)) ==
            DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY)
    {
        filtered.count = dvz_drp2_stream_count(stream) - 2;
        filtered.commands = &stream->commands[2];
        recorded = &filtered;
    }

    if (!dvz_drp2_recorder_write_stream(win->recorder, t_present, recorded))
        log_error("_app_draw failed to append DRP2 stream to DVZR recording");
    else
    {
        win->recording_last_t_present = t_present;
        win->recording_has_last_frame = true;
    }
}



/**
 * Return whether a replay command creates or destroys the synthetic recorded frame target.
 *
 * @param command replay command
 * @param target_id recorded app target id
 * @return whether the command should be skipped for live replay
 */
static bool _app_replay_command_is_synthetic_target(
    const DvzDrp2Command* command, uint64_t target_id)
{
    ANN(command);
    if (target_id == 0)
        return false;

    if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE &&
        command->u.create_texture.id == target_id)
    {
        return true;
    }
    if (command->type == DVZ_DRP2_COMMAND_DESTROY_TEXTURE &&
        command->u.destroy_texture.texture_id == target_id)
    {
        return true;
    }
    return false;
}



/**
 * Remove synthetic target setup commands from a mutable per-frame replay stream.
 *
 * @param stream mutable frame command stream
 * @param target_id recorded app target id
 */
static void _app_replay_filter_synthetic_target(
    DvzDrp2CommandStream* stream, uint64_t target_id)
{
    if (stream == NULL || stream->commands == NULL || target_id == 0)
        return;

    uint32_t write = 0;
    for (uint32_t read = 0; read < stream->count; read++)
    {
        if (_app_replay_command_is_synthetic_target(&stream->commands[read], target_id))
            continue;
        if (write != read)
        {
            dvz_memmove(
                &stream->commands[write], sizeof(DvzDrp2Command),
                &stream->commands[read], sizeof(DvzDrp2Command));
        }
        write++;
    }
    stream->count = write;
}



/**
 * Return whether a replay stream contains commands that draw into the attached frame target.
 *
 * @param stream replay frame command stream
 * @return whether the stream contains at least one draw command
 */
static bool _app_replay_stream_has_draw(const DvzDrp2CommandStream* stream)
{
    if (stream == NULL || stream->commands == NULL)
        return false;

    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2CommandType type = stream->commands[i].type;
        if (type == DVZ_DRP2_COMMAND_DRAW || type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            return true;
    }
    return false;
}



/**
 * Find the app recording target id in a loaded recording.
 *
 * @param recording loaded recording
 * @param out_target_id output target id
 * @return whether a renderable target id was found
 */
static bool _app_replay_find_target_id(
    const DvzDrp2Recording* recording, uint64_t* out_target_id)
{
    ANN(out_target_id);
    *out_target_id = 0;
    const DvzDrp2CommandStream* stream = dvz_drp2_recording_stream(recording);
    if (stream == NULL)
        return false;

    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL || command->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        if ((command->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) == 0)
            continue;
        *out_target_id = command->u.create_texture.id;
        return *out_target_id != 0;
    }
    return false;
}



/**
 * Reset live replay timing and runtime state.
 *
 * @param win replay view
 */
static void _app_replay_restart(DvzView* win)
{
    ANN(win);
    if (win->app != NULL && win->app->runtime != NULL)
        dvz_drp2_runtime_reset(win->app->runtime);
    win->replay_frame_index = 0;
    win->replay_clock = dvz_clock();
    win->replay_clock_started = true;
}



/**
 * Sleep until a recorded frame timestamp should be presented.
 *
 * @param win replay view
 * @param frame recorded frame metadata
 */
static void _app_replay_pace(DvzView* win, const DvzDrp2RecordedFrame* frame)
{
    ANN(win);
    if (!win->replay_paced || frame == NULL || frame->t_present <= 0)
        return;
    if (!win->replay_clock_started)
    {
        win->replay_clock = dvz_clock();
        win->replay_clock_started = true;
    }

    double speed = win->replay_speed > 0 ? win->replay_speed : 1.0;
    double target = frame->t_present / speed;
    double now = dvz_clock_get(&win->replay_clock);
    double delay = target - now;
    if (delay <= 0)
        return;

    double delay_us = delay * 1000000.0;
    int sleep_us = delay_us > (double)INT32_MAX ? INT32_MAX : (int)delay_us;
    dvz_sleep_us(sleep_us);
}



/**
 * Replay one recorded frame into the current live canvas frame.
 *
 * @param win replay view
 * @param frame borrowed canvas frame
 */
static void _app_draw_replay(DvzView* win, const DvzStreamFrame* frame)
{
    ANN(win);
    ANN(frame);
    if (win->app == NULL || win->app->runtime == NULL || win->replay_recording == NULL)
        return;

    uint32_t frame_count = dvz_drp2_recording_frame_count(win->replay_recording);
    if (frame_count == 0)
        return;

    bool drew_frame = false;
    uint32_t processed = 0;
    while (!drew_frame && processed < frame_count)
    {
        if (win->replay_frame_index >= frame_count)
        {
            if (!win->replay_loop)
            {
                win->render_enabled = false;
                return;
            }
            _app_replay_restart(win);
        }

        const DvzDrp2RecordedFrame* recorded =
            dvz_drp2_recording_frame(win->replay_recording, win->replay_frame_index);
        _app_replay_pace(win, recorded);

        if (!dvz_drp2_runtime_attach_frame_target(
                win->app->runtime, win->replay_target_id, frame))
        {
            log_error("_app_draw_replay failed to attach canvas frame target");
            win->render_enabled = false;
            return;
        }

        DvzDrp2CommandStream* stream =
            dvz_drp2_recording_frame_stream(win->replay_recording, win->replay_frame_index);
        if (stream == NULL)
        {
            log_error("_app_draw_replay failed to load frame stream");
            win->render_enabled = false;
            return;
        }
        _app_replay_filter_synthetic_target(stream, win->replay_target_id);
        const bool has_draw = _app_replay_stream_has_draw(stream);

        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(win->app->runtime, stream);
        if (!result.ok)
        {
            _app_log_runtime_failure(
                win, "_app_draw_replay runtime execution failed", stream, result);
            win->render_enabled = false;
            dvz_drp2_stream_destroy(stream);
            return;
        }

        _app_runtime_failure_reset(win, "_app_draw_replay runtime execution");
        win->replay_frame_index++;
        processed++;
        if (has_draw)
        {
            win->frame_index++;
            drew_frame = true;
        }
        dvz_drp2_stream_destroy(stream);
    }

    if (!drew_frame)
        win->render_enabled = false;
}



/**
 * Apply the DVZ_PRESENT_MODE environment override to a present canvas configuration.
 *
 * @param ccfg canvas configuration to mutate
 */
static void _app_canvas_config_apply_present_mode_env(DvzCanvasConfig* ccfg)
{
    ANN(ccfg);
    /* DVZ_PRESENT_MODE: fifo (default, vsync), mailbox (vsync+latest), immediate (no vsync). */
    const char* pm_env = getenv("DVZ_PRESENT_MODE");
    if (pm_env != NULL)
    {
        if (strcmp(pm_env, "immediate") == 0)
            ccfg->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else if (strcmp(pm_env, "mailbox") == 0)
            ccfg->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (strcmp(pm_env, "fifo") == 0)
            ccfg->present_mode = VK_PRESENT_MODE_FIFO_KHR;
        else
            log_warn("ignoring DVZ_PRESENT_MODE='%s' (expected fifo|mailbox|immediate)", pm_env);
    }
}



#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
static void _app_render_gui_frame(DvzView* win, const DvzStreamFrame* frame)
{
    ANN(win);
    ANN(frame);
    if (win->gui != NULL)
        _dvz_gui_render_frame(win->gui, frame);
}



/**
 * Render one GUI viewport source view synchronously during strict GUI viewport resolution.
 *
 * @param source offscreen source view
 * @param user_data parent app
 * @return 0 on success, negative on failure
 */
static int _app_resolve_gui_viewport(DvzView* source, void* user_data)
{
    ANN(source);
    DvzApp* app = (DvzApp*)user_data;
    ANN(app);
    dvz_view_set_render_enabled(source, true);
    int rc = dvz_view_render_once(source);
    if (rc != DVZ_CANVAS_FRAME_READY)
        return -1;
    DvzDevice* device = dvz_gpu_ctx_device(app->gpu_ctx);
    if (device != NULL)
        dvz_device_wait(device);
    return 0;
}
#endif



static void _app_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    DvzView* win = (DvzView*)user_data;
    ANN(win);
    DvzApp* app = win->app;
    ANN(app);

    if (win->replay_recording != NULL)
    {
        _app_draw_replay(win, frame);
        return;
    }

    if (!_app_runtime_recovery_apply(app))
        return;

#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
    {
        _dvz_gui_begin_frame(win->gui, win, frame);
        if (win->fps_overlay_enabled && win->fps_valid)
        {
            _dvz_gui_fps_overlay(
                win->gui, win->fps, win->fps_frame_ms, win->fps_last_sample_frames,
                win->fps_last_sample_elapsed_s);
        }
        if (!_dvz_gui_resolve_viewports(win->gui, _app_resolve_gui_viewport, app))
        {
            log_error("_app_draw failed to resolve strict GUI viewports");
            return;
        }
    }
#endif

    _dvz_scene_animations_step(app->scene, dvz_input_timestamp_ns());
    _app_sync_figure_size(win, frame);
    bool fly_active = _dvz_figure_fly_update(win->figure, app->scene->clock.dt);

    /* Attach the canvas frame to the reserved DRP2 texture ID. */
    if (!dvz_drp2_runtime_attach_frame_target(app->runtime, win->target_id, frame))
    {
        log_error("_app_draw failed to attach canvas frame target");
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
        _app_render_gui_frame(win, frame);
#endif
        return;
    }

    /* Emit one frame artifact with the canvas as external color target. */
    DvzCapabilitySnapshot caps = {0};
    (void)_app_runtime_capabilities(app, &caps);
    caps.max_color_attachments = 3;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format         = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.color_pipeline        = dvz_figure_color_pipeline(win->figure);
    cfg.external_color_target = true;
    cfg.color_target_id       = win->target_id;
    cfg.color_target_format   = (DvzFormat)frame->color_format;
    cfg.target_width          = frame->extent.width;
    cfg.target_height         = frame->extent.height;
    cfg.device_scale_x        = win->device_scale_x > 0.0f ? win->device_scale_x : 1.0f;
    cfg.device_scale_y        = win->device_scale_y > 0.0f ? win->device_scale_y : 1.0f;
    cfg.render_scale          = win->render_scale > 0.0f ? win->render_scale : 1.0f;
    cfg.user_scale            = win->user_scale > 0.0f ? win->user_scale : 1.0f;
    cfg.runtime_resource_scope_id = _app_frame_runtime_scope(frame);
    cfg.clear_color[0]        = 0.05f;
    cfg.clear_color[1]        = 0.05f;
    cfg.clear_color[2]        = 0.08f;
    cfg.clear_color[3]        = 1.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzSceneFrameArtifact* artifact =
        dvz_figure_emit_frame(win->figure, &caps, &report, &cfg);
    if (artifact == NULL)
    {
        uint32_t n = dvz_diagnostic_report_count(&report);
        for (uint32_t i = 0; i < n; i++)
            log_error("_app_draw emit failed: %s", dvz_diagnostic_report_get(&report, i));
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
        _app_render_gui_frame(win, frame);
#endif
        return;
    }

    const DvzDrp2CommandStream* stream = dvz_scene_frame_artifact_stream(artifact);
    if (
        dvz_scene_frame_artifact_status(artifact) != DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK ||
        stream == NULL)
    {
        log_error("_app_draw failed to create scene frame artifact");
        dvz_scene_frame_artifact_destroy(artifact);
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
        _app_render_gui_frame(win, frame);
#endif
        return;
    }

    _app_trace_stream(win, stream);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(app->runtime, stream);
    if (!result.ok)
    {
        _app_log_runtime_failure(win, "_app_draw runtime execution failed", stream, result);
        _app_runtime_recovery_defer(app);
    }
    else
    {
        _app_runtime_failure_reset(win, "_app_draw runtime execution");
        (void)dvz_figure_process_queries(win->figure, app->runtime, &caps);
    }

    if (result.ok)
        _app_record_stream(win, frame, stream);
    dvz_scene_frame_artifact_destroy(artifact);

#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    _app_render_gui_frame(win, frame);
#endif

    if (win->frame_callback != NULL && _app_frame_callback_allowed(win))
        win->frame_callback(win, win->frame_user_data);
    if (_view_has_pending_scene_work(win))
        dvz_view_request_frame(win);
    if (fly_active)
        dvz_view_request_frame(win);
    win->frame_index++;
}

#endif



/*************************************************************************************************/
/*  App lifecycle                                                                                */
/*************************************************************************************************/

DvzAppConfig dvz_app_config(void)
{
    DvzAppConfig config = _app_config_defaults();
    _app_config_apply_env(&config);
    return config;
}



DvzAppResources dvz_app_resources(void)
{
    DvzAppResources resources = {DVZ_STRUCT_INIT_FIELDS(DvzAppResources)};
    return resources;
}



DvzApp* dvz_app(DvzScene* scene)
{
    return dvz_app_with_config(scene, NULL);
}



DvzApp* dvz_app_with_config(DvzScene* scene, const DvzAppConfig* config)
{
    return dvz_app_with_resources(scene, config, NULL);
}



/**
 * Create an app bound to a scene with optional borrowed host resources.
 *
 * @param scene scene borrowed by the app
 * @param config optional app configuration
 * @param resources optional borrowed resource bundle
 * @return the app, or NULL on failure
 */
DvzApp*
dvz_app_with_resources(DvzScene* scene, const DvzAppConfig* config, const DvzAppResources* resources)
{
    ANN(scene);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (!_app_config_validate(config) || !_app_resources_validate(resources))
        return NULL;

    DvzAppConfig resolved = config != NULL ? *config : _app_config_defaults();
    _app_config_apply_env(&resolved);
    const DvzAppResources empty_resources = dvz_app_resources();
    const DvzAppResources* res = resources != NULL ? resources : &empty_resources;
    if (res->runtime != NULL && res->gpu_ctx == NULL)
    {
        log_error("dvz_app_with_resources() requires gpu_ctx when runtime is provided");
        return NULL;
    }
    if (res->runtime != NULL && !_app_runtime_matches_gpu_ctx(res->runtime, res->gpu_ctx))
    {
        log_error("dvz_app_with_resources() received an incompatible runtime/gpu_ctx pair");
        return NULL;
    }
    if (res->gpu_ctx != NULL &&
        (resolved.instance_extension_count > 0 || resolved.enable_canvas_extensions ||
         resolved.enable_glfw_extensions))
    {
        log_warn("dvz_app_with_resources() ignores app GPU-extension config for borrowed gpu_ctx");
    }

    DvzApp* app = (DvzApp*)dvz_calloc(1, sizeof(DvzApp));
    if (app == NULL)
        return NULL;
    app->scene = scene;
    app->config = resolved;
    DvzFontDefaults fonts = _app_config_font_defaults(&resolved);
    dvz_scene_set_font_defaults(scene, &fonts);
    _dvz_app_status_init(&app->status);

    /* Window host first — needed to query GLFW surface extensions before building the instance. */
    if (res->window_host != NULL)
    {
        app->window_host = res->window_host;
        app->owns_window_host = false;
    }
    else
    {
        app->window_host = dvz_window_host();
        app->owns_window_host = app->window_host != NULL;
    }
    if (app->window_host == NULL)
    {
        dvz_free(app);
        return NULL;
    }

    if (res->gpu_ctx != NULL)
    {
        app->gpu_ctx = res->gpu_ctx;
        app->owns_gpu_ctx = false;
    }
    else
    {
        /* GPU context — request independent blending, dynamic rendering, synchronization2, and
         * timeline semaphores. */
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

        if (!_app_gpu_config_add_instance_extensions(
                &gpu_cfg, resolved.instance_extension_count, resolved.instance_extensions))
        {
            _app_destroy_resources(app);
            dvz_free(app);
            return NULL;
        }
        if (resolved.enable_canvas_extensions)
            dvz_gpu_ctx_config_enable_canvas_extensions(&gpu_cfg, true);
        _app_gpu_config_add_glfw_extensions(app, &gpu_cfg, &resolved);

        app->gpu_ctx = dvz_gpu_ctx(&gpu_cfg);
        app->owns_gpu_ctx = app->gpu_ctx != NULL;
        if (app->gpu_ctx == NULL)
        {
            _app_destroy_resources(app);
            dvz_free(app);
            return NULL;
        }
    }

    if (res->runtime != NULL)
    {
        app->runtime = res->runtime;
        app->owns_runtime = false;
    }
    else
    {
        /* DRP2 runtime backed by vklite. */
        DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
            dvz_gpu_ctx_device(app->gpu_ctx), dvz_gpu_ctx_alloc(app->gpu_ctx));
        app->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
        app->owns_runtime = app->runtime != NULL;
        if (app->runtime == NULL)
        {
            _app_destroy_resources(app);
            dvz_free(app);
            return NULL;
        }
    }
    if (!_scene_add_request_frame_callback(app->scene, _app_scene_request_frame, app))
    {
        _app_destroy_resources(app);
        dvz_free(app);
        return NULL;
    }

    return app;
#else
    (void)scene;
    return NULL;
#endif
}



/**
 * Return the borrowed Vulkan instance handle owned by the app.
 *
 * @param app app to query
 * @return Vulkan instance handle, or VK_NULL_HANDLE when unavailable
 */
VkInstance dvz_app_vk_instance(DvzApp* app)
{
    ANN(app);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->gpu_ctx == NULL)
        return VK_NULL_HANDLE;
    DvzInstance* instance = dvz_gpu_ctx_instance(app->gpu_ctx);
    if (instance == NULL)
        return VK_NULL_HANDLE;
    return dvz_instance_handle(instance);
#else
    return VK_NULL_HANDLE;
#endif
}


void dvz_app_destroy(DvzApp* app)
{
    if (app == NULL)
        return;

    if (app->scene != NULL)
        _scene_remove_request_frame_callback(app->scene, _app_scene_request_frame, app);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    _dvz_app_status_finish(&app->status);
    if (app->gpu_ctx != NULL)
    {
        DvzDevice* device = dvz_gpu_ctx_device(app->gpu_ctx);
        if (device != NULL)
            dvz_device_wait(device);
    }
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        DvzView* win = &app->views[i];
        _view_disconnect_figure_panels(win);
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
        if (win->gui != NULL)
        {
            _dvz_gui_destroy(win->gui);
            win->gui = NULL;
        }
#endif
        if (win->canvas != NULL)
        {
            dvz_canvas_destroy(win->canvas);
            win->canvas = NULL;
        }
        if (win->recorder != NULL)
        {
            (void)dvz_drp2_recorder_close(win->recorder);
            win->recorder = NULL;
        }
        if (win->replay_recording != NULL)
        {
            dvz_drp2_recording_close(win->replay_recording);
            win->replay_recording = NULL;
        }
        if (win->window != NULL)
        {
            dvz_window_destroy(win->window);
            win->window = NULL;
        }
        _dvz_app_trace_snapshot_destroy(&win->last_trace_snapshot);
        win->has_last_trace_snapshot = false;
        _view_post_destroy(win);
    }
    if (app->scene != NULL)
        _scene_request_executor_destroy(&app->scene->query_executor);
    if (app->owns_runtime && app->scene != NULL && !_scene_runtime_emitter_reset(app->scene))
        log_error("dvz_app_destroy() failed to reset the scene runtime emitter");
    _app_destroy_resources(app);
#endif

    dvz_free(app);
}


DvzResult dvz_app_stop(DvzApp* app)
{
    if (app == NULL)
        return DVZ_ERROR;
    app->stop_requested = true;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->window_host != NULL)
    {
        for (uint32_t i = 0; i < app->view_count; i++)
        {
            DvzView* win = &app->views[i];
            if (win->window != NULL)
                dvz_window_host_request_frame(app->window_host, win->window);
        }
    }
#endif
    return DVZ_OK;
}


bool dvz_app_should_stop(const DvzApp* app)
{
    ANN(app);
    return app->stop_requested;
}


bool dvz_app_should_exit(DvzApp* app)
{
    ANN(app);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return _app_should_exit(app);
#else
    return true;
#endif
}


/**
 * Destroy native windows that have received a close request.
 *
 * @param app app to inspect
 * @return whether any views were reaped
 */
bool dvz_app_reap_closed_views(DvzApp* app)
{
    ANN(app);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return _app_reap_closed_views(app);
#else
    return false;
#endif
}



/*************************************************************************************************/
/*  View management                                                                            */
/*************************************************************************************************/

/**
 * Connect all current figure panels to a view's input router.
 *
 * @param win the view
 */
static void _view_connect_figure_panels(DvzView* win)
{
    if (win == NULL || win->figure == NULL)
        return;
    for (uint32_t i = 0; i < win->figure->panel_count; i++)
        (void)dvz_view_connect_panel(win, &win->figure->panels[i]);
}


static DvzView*
_view_create_offscreen(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height);


static DvzView* _view_create_glfw(
    DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height, const char* title,
    bool has_position, int32_t x, int32_t y);


static DvzView* _view_create_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface);


/**
 * Return default view descriptor values.
 *
 * @param kind view kind
 * @return initialized descriptor
 */
DvzViewDesc dvz_view_desc(DvzViewKind kind)
{
    DvzViewDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzViewDesc)};
    desc.kind = kind;
    desc.device_scale = 1.0f;
    desc.user_scale = 1.0f;
    desc.render_scale = 1.0f;
    return desc;
}



/**
 * Resolve implicit descriptor dimensions and scales.
 *
 * @param src source descriptor
 * @param figure figure used for fallback logical dimensions
 * @param out resolved descriptor
 */
static void _view_desc_resolve(
    const DvzViewDesc* src, const DvzFigure* figure, DvzResolvedViewDesc* out)
{
    ANN(out);
    DvzViewDesc desc = src != NULL ? *src : dvz_view_desc(DVZ_VIEW_OFFSCREEN);
    if (desc.struct_size == 0)
        desc.struct_size = DVZ_STRUCT_SIZE(DvzViewDesc);

    desc.device_scale = _view_valid_scale(desc.device_scale);
    desc.user_scale = _view_valid_scale(desc.user_scale);
    desc.render_scale = _view_valid_scale(desc.render_scale);
    float device_scale = desc.device_scale;
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;

    DvzViewSizeDesc size = _view_desc_size_desc(&desc);
    if (_view_size_desc_active(&size))
    {
        if (size.requested_device_scale <= 0.0 || !isfinite(size.requested_device_scale))
        {
            size.requested_device_scale = desc.device_scale;
            desc.size_requested_device_scale = size.requested_device_scale;
        }
        DvzResolvedViewSize resolved = dvz_view_size_resolve(&size, desc.kind);
        logical_width = resolved.host_logical_width;
        logical_height = resolved.host_logical_height;
        framebuffer_width = resolved.framebuffer_width;
        framebuffer_height = resolved.framebuffer_height;
        device_scale = (float)(0.5 * (resolved.device_scale_x + resolved.device_scale_y));
    }

    if (logical_width == 0 && framebuffer_width > 0)
        logical_width = _view_round_size((float)framebuffer_width / device_scale);
    if (logical_height == 0 && framebuffer_height > 0)
        logical_height = _view_round_size((float)framebuffer_height / device_scale);

    if (logical_width == 0 && figure != NULL && figure->width > 0)
        logical_width = figure->width;
    if (logical_height == 0 && figure != NULL && figure->height > 0)
        logical_height = figure->height;

    if (logical_width == 0)
        logical_width = 800;
    if (logical_height == 0)
        logical_height = 600;

    if (framebuffer_width == 0)
        framebuffer_width = _view_round_size((float)logical_width * device_scale);
    if (framebuffer_height == 0)
        framebuffer_height = _view_round_size((float)logical_height * device_scale);

    *out = (DvzResolvedViewDesc){
        .desc = desc,
        .logical_width = logical_width,
        .logical_height = logical_height,
        .framebuffer_width = framebuffer_width,
        .framebuffer_height = framebuffer_height,
        .device_scale = device_scale,
    };
}



/**
 * Apply descriptor scale state after view construction.
 *
 * @param win view to update
 * @param desc resolved descriptor
 */
static void _view_apply_desc_state(DvzView* win, const DvzResolvedViewDesc* resolved)
{
    ANN(win);
    ANN(resolved);
    win->requested_size = _view_desc_size_desc(&resolved->desc);
    win->user_scale = _view_valid_scale(resolved->desc.user_scale);
    win->render_scale = _view_valid_scale(resolved->desc.render_scale);
    _view_update_size_state(
        win, resolved->logical_width, resolved->logical_height, resolved->framebuffer_width,
        resolved->framebuffer_height, resolved->device_scale, resolved->device_scale);
    _view_sync_figure_layout_size(win);
}


/**
 * Apply descriptor scale fields without overriding backend-observed view dimensions.
 *
 * @param win view to update
 * @param desc resolved descriptor
 */
static void _view_apply_desc_scales(DvzView* win, const DvzResolvedViewDesc* resolved)
{
    ANN(win);
    ANN(resolved);
    win->requested_size = _view_desc_size_desc(&resolved->desc);
    win->user_scale = _view_valid_scale(resolved->desc.user_scale);
    win->render_scale = _view_valid_scale(resolved->desc.render_scale);
}



/**
 * Create a view from a resolved descriptor.
 *
 * @param app app
 * @param figure figure
 * @param desc view descriptor
 * @return view, or NULL
 */
DvzView* dvz_view(DvzApp* app, DvzFigure* figure, const DvzViewDesc* desc)
{
    ANN(app);
    ANN(figure);

    DvzResolvedViewDesc resolved = {0};
    _view_desc_resolve(desc, figure, &resolved);

    DvzView* win = NULL;
    switch (resolved.desc.kind)
    {
    case DVZ_VIEW_OFFSCREEN:
        win = _view_create_offscreen(
            app, figure, resolved.framebuffer_width, resolved.framebuffer_height);
        break;
    case DVZ_VIEW_WINDOW:
        win = _view_create_glfw(
            app, figure, resolved.logical_width, resolved.logical_height, resolved.desc.title,
            resolved.desc.has_position, resolved.desc.x, resolved.desc.y);
        break;
    case DVZ_VIEW_EXTERNAL_SURFACE:
        log_error("use dvz_view_external_surface() to create external-surface views");
        return NULL;
    default:
        return NULL;
    }
    if (win == NULL)
        return NULL;

    if (resolved.desc.kind == DVZ_VIEW_WINDOW)
    {
        _view_apply_desc_scales(win, &resolved);
        _view_refresh_size_state(win, NULL);
        _view_sync_figure_layout_size(win);
    }
    else
    {
        _view_apply_desc_state(win, &resolved);
    }
    return win;
}



static DvzView*
_view_create_offscreen(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->view_count >= DVZ_APP_MAX_VIEWS)
        return NULL;

    /* Create the offscreen window. */
    DvzWindowConfig wcfg = dvz_window_config();
    wcfg.width           = width;
    wcfg.height          = height;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_OFFSCREEN, &wcfg);
    if (window == NULL)
        return NULL;

    /* Create an offscreen canvas. */
    DvzCanvasConfig ccfg = dvz_canvas_config();
    ccfg.window          = window;
    ccfg.device          = dvz_gpu_ctx_device(app->gpu_ctx);
    ccfg.render_mode     = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    if (dvz_figure_color_pipeline(figure) == DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND)
        ccfg.color_format = VK_FORMAT_R8G8B8A8_UNORM;
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzView* win = &app->views[app->view_count];
    if (_view_post_init(win) != 0)
    {
        dvz_canvas_destroy(canvas);
        dvz_window_destroy(window);
        return NULL;
    }
    win->app           = app;
    win->figure        = figure;
    win->kind          = DVZ_VIEW_OFFSCREEN;
    win->window        = window;
    win->canvas        = canvas;
    win->target_id     = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->view_count;
    win->render_enabled = true;
    _view_update_size_state(win, width, height, width, height, 1.0f, 1.0f);
    _view_mark_dirty(win);
    app->view_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    _view_subscribe_input(win);
    _view_connect_figure_panels(win);
    return win;
#else
    (void)width;
    (void)height;
    return NULL;
#endif
}



static DvzView*
_view_create_glfw(
    DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height, const char* title,
    bool has_position, int32_t x, int32_t y)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE && DVZ_HAS_GLFW
    if (app->view_count >= DVZ_APP_MAX_VIEWS)
        return NULL;

    DvzWindowConfig wcfg = dvz_window_config();
    wcfg.width  = width;
    wcfg.height = height;
    if (title != NULL)
        wcfg.title = title;
    wcfg.has_position = has_position;
    wcfg.x = x;
    wcfg.y = y;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_GLFW, &wcfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
    {
        if (window != NULL)
            dvz_window_destroy(window);
        return NULL;
    }

    /* Poll once so the initial resize event sets the surface extent. */
    dvz_window_host_poll(app->window_host);

    DvzCanvasConfig ccfg = dvz_canvas_config();
    ccfg.window = window;
    ccfg.device = dvz_gpu_ctx_device(app->gpu_ctx);
    /* render_mode defaults to DVZ_CANVAS_RENDER_MODE_PRESENT */
    if (dvz_figure_color_pipeline(figure) == DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND)
        ccfg.color_format = VK_FORMAT_R8G8B8A8_UNORM;
    _app_canvas_config_apply_present_mode_env(&ccfg);
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzView* win = &app->views[app->view_count];
    if (_view_post_init(win) != 0)
    {
        dvz_canvas_destroy(canvas);
        dvz_window_destroy(window);
        return NULL;
    }
    win->app            = app;
    win->figure         = figure;
    win->kind           = DVZ_VIEW_WINDOW;
    win->window         = window;
    win->canvas         = canvas;
    win->target_id      = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->view_count;
    win->is_interactive = true;
    win->render_enabled = true;
    win->fps_overlay_enabled = _app_env_flag_enabled("DVZ_FPS_OVERLAY");
    _view_refresh_size_state(win, NULL);
    if (win->logical_width == 0 || win->logical_height == 0)
        _view_update_size_state(win, width, height, 0, 0, 1.0f, 1.0f);
    _view_mark_dirty(win);
    app->view_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    _view_subscribe_input(win);
    _view_connect_figure_panels(win);
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->fps_overlay_enabled && dvz_view_gui(win, NULL) == NULL)
        log_warn("DVZ_FPS_OVERLAY is enabled but the Dear ImGui overlay could not be created");
#else
    if (win->fps_overlay_enabled)
        log_warn("DVZ_FPS_OVERLAY is enabled but Datoviz was built without GUI support");
#endif
    return win;
#else
    (void)width;
    (void)height;
    (void)title;
    (void)has_position;
    (void)x;
    (void)y;
    return NULL;
#endif
}



/**
 * Create a view around an externally-owned Vulkan surface.
 *
 * @param app app that owns the rendering runtime
 * @param figure figure rendered into the surface
 * @param surface external surface description
 * @return view handle, or NULL on failure
 */
static DvzView* _view_create_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (!_app_external_surface_info_validate(surface))
        return NULL;
    if (app->view_count >= DVZ_APP_MAX_VIEWS)
        return NULL;

    DvzWindowConfig wcfg = dvz_window_config();
    wcfg.width = surface->extent.width;
    wcfg.height = surface->extent.height;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_WRAP, &wcfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_WRAP)
    {
        if (window != NULL)
            dvz_window_destroy(window);
        return NULL;
    }
    if (dvz_window_wrap_attach_surface(window, surface) != 0)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzCanvasConfig ccfg = dvz_canvas_config();
    ccfg.window = window;
    ccfg.device = dvz_gpu_ctx_device(app->gpu_ctx);
    _app_canvas_config_apply_present_mode_env(&ccfg);
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzView* win = &app->views[app->view_count];
    if (_view_post_init(win) != 0)
    {
        dvz_canvas_destroy(canvas);
        dvz_window_destroy(window);
        return NULL;
    }
    win->app        = app;
    win->figure     = figure;
    win->kind       = DVZ_VIEW_EXTERNAL_SURFACE;
    win->window     = window;
    win->canvas     = canvas;
    win->target_id  = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->view_count;
    win->render_enabled = true;
    _view_refresh_size_state(win, NULL);
    _view_mark_dirty(win);
    app->view_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    _view_subscribe_input(win);
    _view_connect_figure_panels(win);
    return win;
#else
    return NULL;
#endif
}


/**
 * Create an offscreen view with exact framebuffer pixels.
 *
 * @param app app that owns the rendering runtime
 * @param figure figure rendered into the view
 * @param width framebuffer width in pixels
 * @param height framebuffer height in pixels
 * @return view handle, or NULL on failure
 */
DvzView*
dvz_view_offscreen(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height)
{
    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
    DvzViewSizeDesc size = dvz_view_size_desc_framebuffer_px(width, height);
    _view_desc_assign_size(&desc, &size);
    return dvz_view(app, figure, &desc);
}



/**
 * Create an interactive native-window view.
 *
 * @param app app that owns the rendering runtime
 * @param figure figure rendered into the view
 * @param width logical window width in pixels
 * @param height logical window height in pixels
 * @param title window title
 * @return view handle, or NULL on failure
 */
DvzView* dvz_view_window(
    DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height, const char* title)
{
    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_WINDOW);
    DvzViewSizeDesc size = dvz_view_size_desc_host_logical_px(width, height);
    _view_desc_assign_size(&desc, &size);
    desc.title = title;
    return dvz_view(app, figure, &desc);
}



/**
 * Create a hosted external-surface view.
 *
 * @param app app that owns the rendering runtime
 * @param figure figure rendered into the view
 * @param surface external surface description
 * @return view handle, or NULL on failure
 */
DvzView* dvz_view_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface)
{
    return _view_create_external_surface(app, figure, surface);
}


/**
 * Create a view around an externally-owned Vulkan surface from FFI-friendly handles.
 *
 * @param app app that owns the rendering runtime
 * @param figure figure rendered into the surface
 * @param instance borrowed Vulkan instance handle
 * @param surface borrowed Vulkan surface handle value
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 * @param owned_by_datoviz whether Datoviz should destroy the surface
 * @return view handle, or NULL on failure
 */
DvzView* dvz_ffi_view_external_surface(
    DvzApp* app, DvzFigure* figure, void* instance, uint64_t surface,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float scale_x, float scale_y,
    bool owned_by_datoviz)
{
    DvzWindowExternalSurfaceInfo info = dvz_window_external_surface_info();
    info.instance = (VkInstance)instance;
    info.surface = (VkSurfaceKHR)(uintptr_t)surface;
    info.extent.width = framebuffer_width;
    info.extent.height = framebuffer_height;
    info.scale_x = scale_x;
    info.scale_y = scale_y;
    info.owned_by_datoviz = owned_by_datoviz;
    return dvz_view_external_surface(app, figure, &info);
}



/*************************************************************************************************/
/*  Window accessors                                                                             */
/*************************************************************************************************/

/**
 * Return the content scale currently cached for a view.
 *
 * @param win view to query
 * @return horizontal content scale, or 1 when unavailable
 */
static float _view_content_scale(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL)
        return 1.0f;
    const DvzWindowSurface* surface = dvz_window_surface(win->window);
    if (surface == NULL || surface->scale_x <= 0.0f)
        return 1.0f;
    return surface->scale_x;
#else
    return 1.0f;
#endif
}



/**
 * Update the external Vulkan surface associated with a hosted view.
 *
 * @param win hosted view
 * @param surface external surface description
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_update_external_surface(
    DvzView* win, const DvzWindowExternalSurfaceInfo* surface)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (!_app_external_surface_info_validate(surface))
        return -1;
    if (win->window == NULL || dvz_window_backend_type(win->window) != DVZ_BACKEND_WRAP)
        return -1;
    int rc = dvz_window_wrap_update_surface(win->window, surface);
    if (rc == 0)
        dvz_view_request_frame(win);
    return rc;
#else
    return -1;
#endif
}


/**
 * Update the external Vulkan surface associated with a hosted view from FFI-friendly handles.
 *
 * @param win hosted view
 * @param instance borrowed Vulkan instance handle, or NULL for surface loss
 * @param surface borrowed Vulkan surface handle value, or zero for surface loss
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 * @param owned_by_datoviz whether Datoviz should destroy the surface
 * @return 0 on success, negative on error
 */
DvzResult dvz_ffi_view_update_external_surface(
    DvzView* win, void* instance, uint64_t surface, uint32_t framebuffer_width,
    uint32_t framebuffer_height, float scale_x, float scale_y, bool owned_by_datoviz)
{
    DvzWindowExternalSurfaceInfo info = dvz_window_external_surface_info();
    info.instance = (VkInstance)instance;
    info.surface = (VkSurfaceKHR)(uintptr_t)surface;
    info.extent.width = framebuffer_width;
    info.extent.height = framebuffer_height;
    info.scale_x = scale_x;
    info.scale_y = scale_y;
    info.owned_by_datoviz = owned_by_datoviz;
    return dvz_view_update_external_surface(win, &info);
}



/**
 * Release a hosted external Vulkan surface before the host destroys it.
 *
 * @param win hosted view
 * @return DVZ_CANVAS_FRAME_WAIT_SURFACE on clean release, or negative on error
 */
int dvz_view_release_external_surface(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL || dvz_window_backend_type(win->window) != DVZ_BACKEND_WRAP)
        return -1;

    dvz_view_set_request_frame_callback(win, NULL, NULL);

    DvzWindowExternalSurfaceInfo surface = dvz_window_external_surface_info();
    surface.scale_x = 1.0f;
    surface.scale_y = 1.0f;
    if (dvz_view_update_external_surface(win, &surface) != 0)
        return -1;
    return dvz_view_render_once(win);
#else
    return -1;
#endif
}



/**
 * Emit a hosted resize event for a view.
 *
 * @param win view receiving the event
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param content_scale_x horizontal content scale
 * @param content_scale_y vertical content scale
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_emit_resize(
    DvzView* win, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL)
        return -1;
    dvz_window_backend_emit_resize(
        win->window, framebuffer_width, framebuffer_height, window_width, window_height,
        content_scale_x, content_scale_y);
    _view_update_size_state(
        win, window_width, window_height, framebuffer_width, framebuffer_height, content_scale_x,
        content_scale_y);
    _view_sync_figure_layout_size(win);
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Emit a hosted pointer position/button event for a view.
 *
 * @param win view receiving the event
 * @param type pointer event type
 * @param x pointer x position in logical host-window coordinates
 * @param y pointer y position in logical host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param button pointer button
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_emit_pointer(
    DvzView* win, DvzPointerEventType type, float x, float y, float window_width,
    float window_height, DvzPointerButton button, int mods)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router == NULL)
        return -1;
    dvz_pointer_emit_position(
        router, type, x, y, window_width, window_height, button, mods,
        _view_content_scale(win), dvz_input_timestamp_ns(),
        win->window != NULL ? dvz_window_user_data(win->window) : NULL);
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Emit a hosted pointer wheel event for a view.
 *
 * @param win view receiving the event
 * @param x pointer x position in logical host-window coordinates
 * @param y pointer y position in logical host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param dx horizontal wheel delta
 * @param dy vertical wheel delta
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_emit_wheel(
    DvzView* win, float x, float y, float window_width, float window_height, float dx,
    float dy, int mods)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router == NULL)
        return -1;
    dvz_pointer_emit_wheel(
        router, x, y, window_width, window_height, dx, dy, mods, _view_content_scale(win),
        dvz_input_timestamp_ns(), win->window != NULL ? dvz_window_user_data(win->window) : NULL);
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Emit a hosted keyboard event for a view.
 *
 * @param win view receiving the event
 * @param type keyboard event type
 * @param key Datoviz key code
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
DvzResult
dvz_view_emit_key(DvzView* win, DvzKeyboardEventType type, DvzKeyCode key, int mods)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router == NULL)
        return -1;
    dvz_keyboard_emit(
        router, type, key, mods, win->window != NULL ? dvz_window_user_data(win->window) : NULL);
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}




struct DvzCanvas* dvz_view_canvas(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return win->canvas;
#else
    return NULL;
#endif
}


struct DvzInputRouter* dvz_view_input(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return win->canvas ? dvz_canvas_input(win->canvas) : NULL;
#else
    return NULL;
#endif
}


/**
 * Return the current device scale for a view.
 *
 * @param win the view
 * @return physical pixels per logical pixel, or 1 when unavailable
 */
float dvz_view_device_scale(const DvzView* win)
{
    ANN(win);
    const float sx = win->device_scale_x > 0.0f ? win->device_scale_x : 1.0f;
    const float sy = win->device_scale_y > 0.0f ? win->device_scale_y : 1.0f;
    return 0.5f * (sx + sy);
}



/**
 * Return the current two-axis device scale for a view.
 *
 * @param win the view
 * @return physical pixels per logical pixel along X and Y
 */
DvzScaleXY dvz_view_device_scale_xy(const DvzView* win)
{
    ANN(win);
    return (DvzScaleXY){
        .x = win->device_scale_x > 0.0f ? win->device_scale_x : 1.0f,
        .y = win->device_scale_y > 0.0f ? win->device_scale_y : 1.0f,
    };
}



/**
 * Return the current view size in an explicit size space.
 *
 * @param win the view
 * @param space requested size space
 * @return size in the requested space, or zero on invalid input
 */
DvzExtent dvz_view_size(const DvzView* win, DvzSizeSpace space)
{
    ANN(win);
    switch (space)
    {
    case DVZ_SIZE_LOGICAL:
        return (DvzExtent){.width = win->logical_width, .height = win->logical_height};
    case DVZ_SIZE_NATIVE:
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
        if (win->window != NULL)
        {
            const DvzWindowMetrics* metrics = dvz_window_metrics(win->window);
            if (metrics != NULL && metrics->native_size.width > 0 && metrics->native_size.height > 0)
                return metrics->native_size;
        }
#endif
        return (DvzExtent){.width = win->logical_width, .height = win->logical_height};
    case DVZ_SIZE_SURFACE:
        return (DvzExtent){.width = win->framebuffer_width, .height = win->framebuffer_height};
    case DVZ_SIZE_RENDER:
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
        if (win->window != NULL)
        {
            const DvzWindowMetrics* metrics = dvz_window_metrics(win->window);
            if (metrics != NULL && metrics->render_size.width > 0 && metrics->render_size.height > 0)
                return metrics->render_size;
        }
#endif
        return (DvzExtent){.width = win->framebuffer_width, .height = win->framebuffer_height};
    default:
        return (DvzExtent){0};
    }
}



DvzResolvedViewSize dvz_view_resolved_size(const DvzView* win)
{
    if (win == NULL)
        return (DvzResolvedViewSize){0};
    return win->resolved_size;
}



/**
 * Return the current logical view size.
 *
 * @param win the view
 * @param out_width output logical width, may be NULL
 * @param out_height output logical height, may be NULL
 */
void dvz_view_logical_size(const DvzView* win, uint32_t* out_width, uint32_t* out_height)
{
    ANN(win);
    if (out_width != NULL)
        *out_width = win->logical_width;
    if (out_height != NULL)
        *out_height = win->logical_height;
}



/**
 * Return the current framebuffer view size.
 *
 * @param win the view
 * @param out_width output framebuffer width, may be NULL
 * @param out_height output framebuffer height, may be NULL
 */
void dvz_view_framebuffer_size(const DvzView* win, uint32_t* out_width, uint32_t* out_height)
{
    ANN(win);
    if (out_width != NULL)
        *out_width = win->framebuffer_width;
    if (out_height != NULL)
        *out_height = win->framebuffer_height;
}



bool dvz_view_capabilities(const DvzView* win, DvzCapabilitySnapshot* out)
{
    if (win == NULL || out == NULL)
        return false;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return _app_runtime_capabilities(win->app, out);
#else
    return false;
#endif
}



/**
 * Return the current render scale for a view.
 *
 * @param win the view
 * @return render scale, defaulting to 1
 */
float dvz_view_render_scale(const DvzView* win)
{
    ANN(win);
    return win->render_scale > 0.0f && isfinite(win->render_scale) ? win->render_scale : 1.0f;
}



/**
 * Return the current user scale for UI-like scene quantities.
 *
 * @param win the view
 * @return user scale, defaulting to 1
 */
float dvz_view_user_scale(const DvzView* win)
{
    ANN(win);
    return win->user_scale > 0.0f && isfinite(win->user_scale) ? win->user_scale : 1.0f;
}



/**
 * Set the user scale for UI-like scene quantities.
 *
 * @param win the view
 * @param scale positive user scale
 */
DvzResult dvz_view_set_user_scale(DvzView* win, float scale)
{
    if (win == NULL)
        return DVZ_ERROR;
    if (!isfinite(scale) || scale <= 0.0f)
    {
        log_error("view user scale must be positive and finite");
        return DVZ_ERROR;
    }
    if (win->user_scale == scale)
        return DVZ_OK;
    win->user_scale = scale;
    _view_mark_dirty(win);
    return DVZ_OK;
}


/**
 * Connect a panel's bound controllers to a view input router.
 *
 * @param win the view
 * @param panel the panel
 * @return 0 on success, -1 on validation error
 */
DvzResult dvz_view_connect_panel(DvzView* win, DvzPanel* panel)
{
    if (win == NULL || panel == NULL)
        return -1;

    DvzInputRouter* router = dvz_view_input(win);
    if (router == NULL)
        return -1;

    return dvz_panel_connect_input(panel, router);
}



/**
 * Bind a controller to a panel and connect the panel to a view input router.
 *
 * @param win the view
 * @param panel the panel
 * @param controller the scene-owned controller
 * @param dims dimension mask
 * @return 0 on success, -1 on validation error
 */
DvzResult dvz_view_bind_controller(
    DvzView* win, DvzPanel* panel, DvzController* controller, DvzDimMask dims)
{
    if (win == NULL || panel == NULL || controller == NULL)
        return -1;

    DvzInputRouter* router = dvz_view_input(win);
    if (router == NULL)
        return -1;

    if (dvz_panel_bind_controller(panel, controller, dims) != 0)
        return -1;

    return dvz_panel_connect_input(panel, router);
}



/**
 * Create, bind, and connect a panzoom controller for one panel.
 *
 * @param win the view
 * @param panel the panel
 * @param desc panzoom descriptor, or NULL for defaults
 * @return the panzoom payload, or NULL on validation error
 */
DvzPanzoom*
dvz_view_panzoom(DvzView* win, DvzPanel* panel, const DvzPanzoomDesc* desc)
{
    if (win == NULL || panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (dvz_view_input(win) == NULL)
        return NULL;

    DvzController* controller = dvz_panzoom(panel->figure->scene, desc);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    if (panzoom == NULL)
        return NULL;
    if (dvz_view_bind_controller(win, panel, controller, DVZ_DIM_MASK_XY) != 0)
        return NULL;
    return panzoom;
}



/**
 * Create, bind, and connect an arcball controller for one panel.
 *
 * @param win the view
 * @param panel the panel
 * @param desc arcball descriptor, or NULL for defaults
 * @return the arcball payload, or NULL on validation error
 */
DvzArcball*
dvz_view_arcball(DvzView* win, DvzPanel* panel, const DvzArcballDesc* desc)
{
    if (win == NULL || panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (dvz_view_input(win) == NULL)
        return NULL;

    DvzController* controller = dvz_arcball(panel->figure->scene, desc);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_view_bind_controller(win, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    return arcball;
}



/**
 * Create, bind, and connect a fly controller for one panel.
 *
 * @param win the view
 * @param panel the panel
 * @param desc fly descriptor, or NULL for defaults
 * @return the fly payload, or NULL on validation error
 */
DvzFly* dvz_view_fly(DvzView* win, DvzPanel* panel, const DvzFlyDesc* desc)
{
    if (win == NULL || panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (dvz_view_input(win) == NULL)
        return NULL;

    DvzController* controller = dvz_fly(panel->figure->scene, desc);
    DvzFly* fly = dvz_controller_fly(controller);
    if (fly == NULL)
        return NULL;
    if (dvz_view_bind_controller(win, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    return fly;
}



/**
 * Create, bind, and connect a turntable controller for one panel.
 *
 * @param win the view
 * @param panel the panel
 * @param desc turntable descriptor, or NULL for defaults
 * @return the turntable payload, or NULL on validation error
 */
DvzTurntable* dvz_view_turntable(
    DvzView* win, DvzPanel* panel, const DvzTurntableDesc* desc)
{
    if (win == NULL || panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (dvz_view_input(win) == NULL)
        return NULL;

    DvzController* controller = dvz_turntable(panel->figure->scene, desc);
    DvzTurntable* turntable = dvz_controller_turntable(controller);
    if (turntable == NULL)
        return NULL;
    if (dvz_view_bind_controller(win, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    return turntable;
}



DvzResult dvz_view_capture_png(DvzView* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return dvz_canvas_capture_png(win->canvas, path);
#else
    return -1;
#endif
}


/**
 * Return the default app capture configuration.
 *
 * @return default capture configuration
 */
DvzAppCaptureConfig dvz_app_capture_config(void)
{
    DvzAppCaptureConfig config = {
        .struct_size = DVZ_STRUCT_SIZE(DvzAppCaptureConfig),
        .flags = DVZ_APP_CAPTURE_NONE,
        .directory = ".",
        .basename = "capture",
        .fps = DVZ_APP_CAPTURE_DEFAULT_FPS,
        .video_backend = "auto",
        .video_capture_mode = DVZ_VIDEO_CAPTURE_AUTO,
    };
    return config;
}



/**
 * Return an app capture configuration from environment variables.
 *
 * @param basename fallback output basename
 * @return environment-derived capture configuration
 */
DvzAppCaptureConfig dvz_app_capture_config_from_env(const char* basename)
{
    DvzAppCaptureConfig config = dvz_app_capture_config();
    if (basename != NULL && basename[0] != '\0')
        config.basename = basename;

    const char* capture = getenv("DVZ_CAPTURE");
    config.flags = _app_capture_flags_from_env_value(capture);

    const char* directory = getenv("DVZ_CAPTURE_DIR");
    if (directory != NULL && directory[0] != '\0')
        config.directory = directory;

    const char* env_basename = getenv("DVZ_CAPTURE_BASENAME");
    if (env_basename != NULL && env_basename[0] != '\0')
        config.basename = env_basename;

    config.fps = _app_capture_fps_from_env(config.fps);

    const char* backend = getenv("DVZ_CAPTURE_VIDEO_BACKEND");
    if (backend != NULL && backend[0] != '\0')
        config.video_backend = backend;

    config.video_capture_mode = _app_capture_video_mode_from_env(config.video_capture_mode);
    return config;
}



/**
 * Start configured view captures.
 *
 * @param win the view
 * @param config capture configuration
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_capture_start(DvzView* win, const DvzAppCaptureConfig* config)
{
    ANN(win);
    if (!_app_capture_config_validate(config))
        return -1;

    DvzAppCaptureConfig resolved = config != NULL ? *config : dvz_app_capture_config();
    if (resolved.flags == DVZ_APP_CAPTURE_NONE)
        return 0;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    if (win->capture_dvzr_enabled || win->capture_video_enabled || win->capture_png_enabled)
        return -1;

    if ((resolved.flags & DVZ_APP_CAPTURE_DVZR) != 0)
    {
        if (!_app_capture_path(
                &resolved, ".dvzr", win->capture_dvzr_path, sizeof(win->capture_dvzr_path)))
        {
            log_error("DVZR capture path is too long");
            return -1;
        }
        if (dvz_view_record_start(win, win->capture_dvzr_path) != 0)
            return -1;
        win->capture_dvzr_enabled = true;
    }

    if ((resolved.flags & DVZ_APP_CAPTURE_VIDEO) != 0)
    {
        if (!_app_capture_path(
                &resolved, ".mp4", win->capture_video_path, sizeof(win->capture_video_path)))
        {
            log_error("video capture path is too long");
            if (win->capture_dvzr_enabled)
                (void)dvz_view_record_stop(win);
            win->capture_dvzr_enabled = false;
            return -1;
        }

        DvzVideoSinkConfig video = dvz_video_sink_config();
        const char* backend = resolved.video_backend != NULL ? resolved.video_backend : "auto";
        int backend_rc = dvz_snprintf(
            win->capture_video_backend, sizeof(win->capture_video_backend), "%s", backend);
        if (backend_rc < 0 || (size_t)backend_rc >= sizeof(win->capture_video_backend))
        {
            log_error("video capture backend name is too long");
            if (win->capture_dvzr_enabled)
                (void)dvz_view_record_stop(win);
            win->capture_dvzr_enabled = false;
            return -1;
        }
        if (resolved.fps > (double)UINT32_MAX)
        {
            log_error("video capture FPS is too large");
            if (win->capture_dvzr_enabled)
                (void)dvz_view_record_stop(win);
            win->capture_dvzr_enabled = false;
            return -1;
        }
        video.encoder.backend = win->capture_video_backend;
        video.encoder.width = win->figure != NULL ? win->figure->width : 0;
        video.encoder.height = win->figure != NULL ? win->figure->height : 0;
        video.encoder.fps = resolved.fps > 0 ? (uint32_t)(resolved.fps + 0.5) : 60;
        video.encoder.mp4_path = win->capture_video_path;
        video.capture_mode = resolved.video_capture_mode;

        if (dvz_canvas_configure_video_sink(win->canvas, true, &video) != 0)
        {
            if (win->capture_dvzr_enabled)
                (void)dvz_view_record_stop(win);
            win->capture_dvzr_enabled = false;
            return -1;
        }
        win->capture_video_enabled = true;
    }

    if ((resolved.flags & DVZ_APP_CAPTURE_PNG) != 0)
    {
        if (!_app_capture_path(
                &resolved, ".png", win->capture_png_path, sizeof(win->capture_png_path)))
        {
            log_error("PNG capture path is too long");
            (void)dvz_view_capture_stop(win);
            return -1;
        }
        win->capture_png_enabled = true;
    }

    return 0;
#else
    return -1;
#endif
}



/**
 * Start view captures from environment variables.
 *
 * @param win the view
 * @param basename fallback output basename
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_capture_from_env(DvzView* win, const char* basename)
{
    ANN(win);
    DvzAppCaptureConfig config = dvz_app_capture_config_from_env(basename);
    return dvz_view_capture_start(win, &config);
}



/**
 * Stop active view captures.
 *
 * @param win the view
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_capture_stop(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    int rc = 0;
    if (win->capture_png_enabled)
    {
        if (dvz_view_capture_png(win, win->capture_png_path) != 0)
            rc = -1;
        else
            dvz_fprintf(stdout, "datoviz: saved %s\n", win->capture_png_path);
        win->capture_png_enabled = false;
    }

    if (win->capture_video_enabled)
    {
        if (dvz_canvas_configure_video_sink(win->canvas, false, NULL) != 0)
            rc = -1;
        else
            dvz_fprintf(stdout, "datoviz: saved %s\n", win->capture_video_path);
        win->capture_video_enabled = false;
    }

    if (win->capture_dvzr_enabled)
    {
        if (dvz_view_record_stop(win) != 0)
            rc = -1;
        else
            dvz_fprintf(stdout, "datoviz: saved %s\n", win->capture_dvzr_path);
        win->capture_dvzr_enabled = false;
    }
    return rc;
#else
    return -1;
#endif
}


/**
 * Start view DVZR recording.
 *
 * @param win the view
 * @param path output recording directory path
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_record_start(DvzView* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->recorder != NULL)
        return -1;
    DvzDrp2RecordingInfo info = dvz_drp2_recording_info();
    info.width = win->figure != NULL ? win->figure->width : 0;
    info.height = win->figure != NULL ? win->figure->height : 0;
    info.duration_s = 0.0;
    info.t_present = 0.0;
    info.fps_cap = _app_record_fps_from_env();
    info.backend_hint = "app";
    win->recorder = dvz_drp2_recorder_open(path, &info);
    if (win->recorder == NULL)
        return -1;
    win->recording_clock = dvz_clock();
    win->recording_fps = info.fps_cap;
    win->recording_last_t_present = 0.0;
    win->recording_target_created = false;
    win->recording_has_last_frame = false;
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Stop view DVZR recording.
 *
 * @param win the view
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_record_stop(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->recorder == NULL)
        return -1;
    bool ok = dvz_drp2_recorder_close(win->recorder);
    win->recorder = NULL;
    win->recording_target_created = false;
    win->recording_has_last_frame = false;
    dvz_view_request_frame(win);
    return ok ? 0 : -1;
#else
    return -1;
#endif
}



/**
 * Start view DVZR live replay.
 *
 * @param win the view
 * @param path input recording directory path
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_replay_start(DvzView* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->app == NULL || win->app->runtime == NULL || win->replay_recording != NULL)
        return -1;

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    if (recording == NULL)
        return -1;

    uint64_t target_id = 0;
    if (!_app_replay_find_target_id(recording, &target_id))
    {
        dvz_drp2_recording_close(recording);
        return -1;
    }

    win->replay_recording = recording;
    win->replay_target_id = target_id;
    win->replay_frame_index = 0;
    win->replay_clock = dvz_clock();
    win->replay_clock_started = true;
    win->replay_paced = true;
    win->replay_loop = false;
    win->replay_speed = 1.0;
    win->render_enabled = true;
    dvz_drp2_runtime_reset(win->app->runtime);
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Stop view DVZR live replay.
 *
 * @param win the view
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_replay_stop(DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->replay_recording == NULL)
        return -1;
    dvz_drp2_recording_close(win->replay_recording);
    win->replay_recording = NULL;
    win->replay_target_id = 0;
    win->replay_frame_index = 0;
    win->replay_clock_started = false;
    dvz_view_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Enable or disable timestamp-paced live replay.
 *
 * @param win the view
 * @param paced whether replay waits for recorded timestamps
 */
DvzResult dvz_view_replay_set_paced(DvzView* win, bool paced)
{
    if (win == NULL)
        return DVZ_ERROR;
    win->replay_paced = paced;
    return dvz_view_request_frame(win);
}



/**
 * Set the live replay speed multiplier.
 *
 * @param win the view
 * @param speed speed multiplier
 */
DvzResult dvz_view_replay_set_speed(DvzView* win, double speed)
{
    if (win == NULL)
        return DVZ_ERROR;
    if (!(speed > 0))
        return DVZ_ERROR;
    win->replay_speed = speed;
    return dvz_view_request_frame(win);
}



/**
 * Enable or disable live replay looping.
 *
 * @param win the view
 * @param loop whether replay should loop
 */
DvzResult dvz_view_replay_set_loop(DvzView* win, bool loop)
{
    if (win == NULL)
        return DVZ_ERROR;
    win->replay_loop = loop;
    return dvz_view_request_frame(win);
}



/**
 * Return the active live replay frame count.
 *
 * @param win the view
 * @return replay frame count, or 0 when no replay is active
 */
uint32_t dvz_view_replay_frame_count(const DvzView* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->replay_recording == NULL)
        return 0;
    return dvz_drp2_recording_frame_count(win->replay_recording);
#else
    return 0;
#endif
}



/**
 * Resize a view's logical and framebuffer extent.
 *
 * @param win the view
 * @param width width in pixels
 * @param height height in pixels
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_resize(DvzView* win, uint32_t width, uint32_t height)
{
    return dvz_view_resize_scaled(win, width, height, 1.0f);
}


/**
 * Resize a view's logical and framebuffer extent with an explicit device scale.
 *
 * @param win the view
 * @param logical_width logical width in pixels
 * @param logical_height logical height in pixels
 * @param device_scale physical pixels per logical pixel
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_resize_scaled(
    DvzView* win, uint32_t logical_width, uint32_t logical_height, float device_scale)
{
    return dvz_view_resize_scaled_xy(
        win, logical_width, logical_height, device_scale, device_scale);
}


/**
 * Resize a view's logical and framebuffer extent with explicit per-axis device scale.
 *
 * @param win the view
 * @param logical_width logical width in pixels
 * @param logical_height logical height in pixels
 * @param device_scale_x physical pixels per logical pixel along X
 * @param device_scale_y physical pixels per logical pixel along Y
 * @return 0 on success, negative on error
 */
DvzResult dvz_view_resize_scaled_xy(
    DvzView* win, uint32_t logical_width, uint32_t logical_height, float device_scale_x,
    float device_scale_y)
{
    ANN(win);
    if (logical_width == 0 || logical_height == 0)
        return -1;
    device_scale_x = _view_valid_scale(device_scale_x);
    device_scale_y = _view_valid_scale(device_scale_y);
    uint32_t framebuffer_width = _view_round_size((float)logical_width * device_scale_x);
    uint32_t framebuffer_height = _view_round_size((float)logical_height * device_scale_y);
    if (framebuffer_width == 0 || framebuffer_height == 0)
        return -1;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL || win->figure == NULL)
        return -1;
    dvz_window_backend_emit_resize(
        win->window, framebuffer_width, framebuffer_height, logical_width, logical_height,
        device_scale_x, device_scale_y);
    _view_update_size_state(
        win, logical_width, logical_height, framebuffer_width, framebuffer_height, device_scale_x,
        device_scale_y);
    _view_sync_figure_layout_size(win);
    dvz_view_request_frame(win);
    return 0;
#else
    (void)logical_width;
    (void)logical_height;
    (void)device_scale_x;
    (void)device_scale_y;
    return -1;
#endif
}


/**
 * Enable or disable rendering for a view.
 *
 * @param win view to update
 * @param enabled whether rendering should be enabled
 */
DvzResult dvz_view_set_render_enabled(DvzView* win, bool enabled)
{
    if (win == NULL)
        return DVZ_ERROR;
    win->render_enabled = enabled;
    _view_mark_dirty(win);
    return DVZ_OK;
}



/**
 * Return whether rendering is enabled for a view.
 *
 * @param win view to query
 * @return whether rendering is enabled
 */
bool dvz_view_render_enabled(const DvzView* win)
{
    ANN(win);
    return win->render_enabled;
}



/**
 * Request that the host schedules another frame for a view.
 *
 * @param win view requesting a frame
 */
DvzResult dvz_view_request_frame(DvzView* win)
{
    if (win == NULL)
        return DVZ_ERROR;
    _view_mark_dirty(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->app != NULL && win->app->window_host != NULL && win->window != NULL)
        dvz_window_host_request_frame(win->app->window_host, win->window);
#endif
    if (win->request_frame_callback != NULL)
        win->request_frame_callback(win, win->request_frame_user_data);
    return DVZ_OK;
}



/**
 * Wake the host scheduler for a view.
 *
 * @param win view requesting scheduler attention
 */
DvzResult dvz_view_wake(DvzView* win)
{
    if (win == NULL)
        return DVZ_ERROR;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->app != NULL && win->app->window_host != NULL && win->window != NULL)
        dvz_window_host_request_frame(win->app->window_host, win->window);
#endif
    if (win->request_frame_callback != NULL)
        win->request_frame_callback(win, win->request_frame_user_data);
    return DVZ_OK;
}



/**
 * Post a callback to run on the view owner thread.
 *
 * @param win view owning the callback queue
 * @param callback callback to run
 * @param user_data opaque pointer forwarded to the callback
 * @return 0 on success, negative on invalid input or overflow
 */
DvzResult dvz_view_post(DvzView* win, DvzViewPostCallback callback, void* user_data)
{
    if (win == NULL || callback == NULL)
        return -1;
    if (!win->post_mutex_initialized && _view_post_init(win) != 0)
        return -1;

    if (dvz_mutex_lock(&win->post_mutex) != 0)
        return -1;
    if (win->post_count >= DVZ_APP_VIEW_POST_CAPACITY)
    {
        (void)dvz_mutex_unlock(&win->post_mutex);
        log_error("dvz_view_post() queue overflow");
        return -1;
    }
    win->post_items[win->post_count++] =
        (DvzViewPostItem){.callback = callback, .user_data = user_data};
    (void)dvz_mutex_unlock(&win->post_mutex);

    dvz_view_wake(win);
    return 0;
}



/**
 * Register a callback invoked whenever Datoviz requests another frame.
 *
 * @param win view receiving the callback
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
DvzResult dvz_view_set_request_frame_callback(
    DvzView* win, DvzViewRequestFrameCallback callback, void* user_data)
{
    if (win == NULL)
        return DVZ_ERROR;
    win->request_frame_callback = callback;
    win->request_frame_user_data = user_data;
    return DVZ_OK;
}



DvzResult dvz_view_set_frame_callback(
    DvzView* win, DvzViewFrameCallback callback, void* user_data)
{
    if (win == NULL)
        return DVZ_ERROR;
    win->frame_callback = callback;
    win->frame_user_data = user_data;
    return DVZ_OK;
}



/*************************************************************************************************/
/*  GUI                                                                                          */
/*************************************************************************************************/

DvzGui* dvz_view_gui(DvzView* win, const DvzGuiConfig* config)
{
    ANN(win);
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
        return win->gui;
    if (win->app == NULL || win->app->gpu_ctx == NULL || win->window == NULL)
        return NULL;
    if (!_dvz_gui_config_validate(config))
        return NULL;
    DvzGuiConfig resolved = config != NULL ? *config : dvz_gui_config();
    DvzFontDefaults fonts = _app_config_font_defaults(&win->app->config);
    win->gui = _dvz_gui_create(win->app, win->app->gpu_ctx, win, win->window, &resolved, &fonts);
    if (win->gui != NULL)
        dvz_view_request_frame(win);
    return win->gui;
#else
    (void)config;
    return NULL;
#endif
}



DvzResult dvz_view_set_gui_callback(
    DvzView* win, DvzGuiCallback callback, void* user_data)
{
    if (win == NULL)
        return DVZ_ERROR;
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui == NULL)
        return DVZ_ERROR;
    _dvz_gui_set_callback(win->gui, callback, user_data);
    return dvz_view_request_frame(win);
#else
    (void)callback;
    (void)user_data;
    return DVZ_ERROR;
#endif
}



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

/**
 * Render one frame for a single view.
 *
 * @param win view to render
 * @return canvas frame status or negative on error
 */
int dvz_view_render_once(DvzView* win)
{
    ANN(win);
    _view_post_drain(win);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (!win->render_enabled)
        return 0;
    if (win->canvas == NULL)
        return -1;
    if (_view_close_requested(win))
        return 0;

    bool dirty_before = win->dirty;
    bool requested_before = win->frame_requested;
    win->dirty = false;
    win->frame_requested = false;

    int rc = dvz_canvas_frame(win->canvas);
    if (rc == DVZ_CANVAS_FRAME_READY)
    {
        if (dvz_canvas_submit(win->canvas) != 0)
        {
            win->dirty = win->dirty || dirty_before;
            win->frame_requested = win->frame_requested || requested_before;
            return -1;
        }
        _view_fps_update(win, dvz_input_timestamp_ns());
        if (win->app != NULL && win->app->scene != NULL &&
            dvz_scene_has_active_animations(win->app->scene))
        {
            dvz_view_request_frame(win);
        }
        if (win->replay_recording != NULL)
        {
            uint32_t frame_count = dvz_drp2_recording_frame_count(win->replay_recording);
            if (frame_count > 0 && (win->replay_loop || win->replay_frame_index < frame_count))
                dvz_view_request_frame(win);
        }
    }
    else
    {
        win->dirty = win->dirty || dirty_before;
        win->frame_requested = win->frame_requested || requested_before;
    }
    return rc;
#else
    return -1;
#endif
}



/**
 * Render one frame for every view without polling events.
 *
 * @param app app whose windows should render
 * @return 0 on success, wait-surface status, or negative on error
 */
int dvz_app_render_once(DvzApp* app)
{
    ANN(app);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    int result = 0;
    for (uint32_t i = 0; i < app->view_count; i++)
    {
        int rc = dvz_view_render_once(&app->views[i]);
        if (rc < 0)
            result = -1;
        else if (result == 0 && rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            result = DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    return result;
#else
    return -1;
#endif
}



void dvz_app_run(DvzApp* app, uint32_t frame_count)
{
    ANN(app);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    /* FPS counter — opt in via DVZ_FPS=1 (default on for interactive runs to aid profiling). */
    const char* fps_env = getenv("DVZ_FPS");
    bool fps_enabled = (fps_env == NULL) ? (frame_count == 0)
                                         : (strcmp(fps_env, "0") != 0);
    uint64_t fps_window_start = fps_enabled ? dvz_input_timestamp_ns() : 0;
    uint32_t fps_window_frames = 0;

    if (frame_count == 0)
    {
        /* Interactive mode: loop until the configured app exit policy or dvz_app_stop() trips. */
        for (;;)
        {
            if (_app_should_exit(app))
                break;
            bool continuous = _app_has_continuous_work(app);
            bool pending = _app_has_pending_windows(app);
            if (continuous)
            {
                uint64_t deadline =
                    app->config.fps_cap > 0 ? _app_next_continuous_deadline(app) : 0;
                if (deadline > 0)
                    _app_host_wait_until(app, deadline);
                else
                    _app_host_poll(app);
            }
            else if (!pending)
                _app_host_wait(app);
            else
                _app_host_poll(app);

            if (_app_should_exit(app))
            {
                _app_reap_closed_views(app);
                break;
            }
            _app_reap_closed_views(app);
            if (_app_should_exit(app))
                break;

            continuous = _app_has_continuous_work(app);
            uint64_t now = _app_scheduler_now_ns();
            for (uint32_t i = 0; i < app->view_count; i++)
            {
                DvzView* win = &app->views[i];
                if (!_view_should_render(win, continuous, now))
                    continue;
                int rc = dvz_view_render_once(win);
                if (continuous && app->config.fps_cap > 0 && rc == DVZ_CANVAS_FRAME_READY)
                    _view_update_deadline(win, _app_scheduler_now_ns(), app->config.fps_cap);
                if (rc == DVZ_CANVAS_FRAME_READY && fps_enabled)
                    fps_window_frames++;
            }
            if (fps_enabled)
            {
                now = dvz_input_timestamp_ns();
                uint64_t elapsed_ns = now - fps_window_start;
                if (elapsed_ns >= 1000000000ULL)
                {
                    double fps = (double)fps_window_frames * 1e9 / (double)elapsed_ns;
                    _dvz_app_status_fps(
                        &app->status, fps, fps_window_frames, (double)elapsed_ns / 1e9);
                    _dvz_app_status_render(&app->status);
                    fps_window_start = now;
                    fps_window_frames = 0;
                }
            }
        }
    }
    else
    {
        for (uint32_t f = 0; f < frame_count; f++)
        {
            if (app->stop_requested)
                break;
            dvz_window_host_poll(app->window_host);
            for (uint32_t i = 0; i < app->view_count; i++)
            {
                DvzView* win = &app->views[i];
                if (_view_close_requested(win))
                    continue;
                (void)dvz_view_render_once(win);
            }
        }
    }
    DvzDevice* device = dvz_gpu_ctx_device(app->gpu_ctx);
    if (device != NULL)
        dvz_device_wait(device);
    _dvz_app_status_finish(&app->status);
#else
    (void)frame_count;
#endif
}
