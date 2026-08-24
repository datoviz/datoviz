/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Dear ImGui overlay                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/gui.h"
#include "_gui.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <volk.h>

#include "_assertions.h"
#include "_log.h"
#include "datoviz/canvas.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/input/pointer.h"
#include "datoviz/window/backend.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "_alloc.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "imgui_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GUI_VIEWPORT_DEFAULT_INITIAL_WIDTH 640u
#define DVZ_GUI_VIEWPORT_DEFAULT_INITIAL_HEIGHT 480u
#define DVZ_GUI_VIEWPORT_DEFAULT_MIN_WIDTH 32u
#define DVZ_GUI_VIEWPORT_DEFAULT_MIN_HEIGHT 32u
#define DVZ_GUI_VIEWPORT_DEFAULT_RESIZE_STEP 8u
#define DVZ_GUI_VIEWPORT_DEFAULT_RESIZE_DELAY_FRAMES 2u
#define DVZ_GUI_VIEWPORT_RETIRED_TEXTURE_CAPACITY 64u
#define DVZ_GUI_DEFAULT_WINDOW_WIDTH 200u
#define DVZ_GUI_DEFAULT_DOCK_RATIO 0.25f
#define DVZ_GUI_MAX_DOCKED_WINDOWS 64u
#define DVZ_GUI_CONFIG_KNOWN_FLAGS 0u
#define DVZ_GUI_VIEWPORT_CONFIG_KNOWN_FLAGS 0u
#define DVZ_FONT_DEFAULTS_KNOWN_FLAGS 0u
#define DVZ_FONT_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Datoviz C API shims                                                                          */
/*************************************************************************************************/

extern "C" {
typedef struct DvzDevice DvzDevice;
typedef struct DvzGpuCtx DvzGpuCtx;
typedef struct DvzInstance DvzInstance;
typedef struct DvzQueue DvzQueue;

typedef enum DvzQueueRole
{
    DVZ_QUEUE_MAIN,
    DVZ_QUEUE_COMPUTE,
    DVZ_QUEUE_TRANSFER,
    DVZ_QUEUE_VIDEO_ENCODE,
    DVZ_QUEUE_VIDEO_DECODE,
    DVZ_QUEUE_COUNT,
} DvzQueueRole;

DvzInstance* dvz_gpu_ctx_instance(DvzGpuCtx* ctx);
DvzDevice* dvz_gpu_ctx_device(DvzGpuCtx* ctx);
DvzQueue* dvz_gpu_ctx_queue(DvzGpuCtx* ctx, DvzQueueRole role);
VkInstance dvz_instance_handle(DvzInstance* instance);
VkPhysicalDevice dvz_device_physical_device(DvzDevice* device);
VkDevice dvz_device_handle(DvzDevice* device);
uint32_t dvz_queue_family(DvzQueue* queue);
VkQueue dvz_queue_handle(DvzQueue* queue);
}



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGui
{
    DvzApp* app;
    DvzView* view;
    DvzGpuCtx* gpu_ctx;
    DvzDevice* device;
    DvzQueue* queue;
    DvzWindow* window;
    GLFWwindow* glfw_window;
    ImGuiContext* context;
    DvzGuiConfig config;
    DvzFontDefaults font_defaults;
    DvzGuiCallback callback;
    void* callback_user_data;
    DvzGuiViewport* viewports;
    DvzGuiViewport* keyboard_viewport;
    ImGuiID dockspace_id;
    ImGuiID dockspace_main_id;
    ImGuiID dock_nodes[4];
    ImGuiID docked_window_hashes[DVZ_GUI_MAX_DOCKED_WINDOWS];
    uint32_t docked_window_hash_count;
    int glfw_mods;
    ImFont* font_regular;
    ImFont* font_mono;
    VkFormat color_format;
    bool glfw_initialized;
    bool vulkan_initialized;
    bool failed;
    bool had_active_item;
    bool had_open_popup;
};


struct DvzGuiViewport
{
    DvzGui* gui;
    DvzFigure* figure;
    DvzView* source;
    DvzCanvas* canvas;
    DvzGuiViewportConfig config;
    VkSampler sampler;
    VkDescriptorSet texture;
    VkDescriptorSet retired_textures[DVZ_GUI_VIEWPORT_RETIRED_TEXTURE_CAPACITY];
    uint32_t retired_texture_count;
    VkImage image;
    VkImageView image_view;
    VkExtent2D extent;
    uint64_t resource_generation;
    bool image_valid;
    uint32_t requested_width;
    uint32_t requested_height;
    uint32_t requested_framebuffer_width;
    uint32_t requested_framebuffer_height;
    uint32_t pending_width;
    uint32_t pending_height;
    uint32_t pending_stable_frames;
    uint32_t stale_frame_count;
    uint32_t frame_request_width;
    uint32_t frame_request_height;
    uint32_t frame_request_framebuffer_width;
    uint32_t frame_request_framebuffer_height;
    ImDrawList* frame_draw_list;
    ImVec2 frame_image_min;
    ImVec2 frame_image_max;
    bool owns_source;
    bool visible;
    bool frame_visible;
    bool frame_resolved;
    bool has_frame;
    bool texture_dirty;
    bool keyboard_focused;
    bool input_capturing;
    int input_button;
    DvzPointerButton input_dvz_button;
    bool mouse_valid;
    bool mouse_hovered;
    float mouse_pos[2];
    float mouse_size[2];
    DvzGuiViewport* next;
};



/*************************************************************************************************/
/*  Function prototypes                                                                         */
/*************************************************************************************************/

static void _gui_request_frame(DvzGui* gui);
static bool _gui_update_followup_frame_state(DvzGui* gui);
static bool _gui_viewport_ensure_texture(DvzGuiViewport* viewport);
static bool _gui_viewport_display_drawable(const DvzGuiViewport* viewport);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Report a Vulkan error produced by Dear ImGui.
 *
 * @param err Vulkan result
 */
static void _gui_check_vk_result(VkResult err)
{
    if (err != VK_SUCCESS)
        log_error("Dear ImGui Vulkan backend error: %d", (int)err);
}



/**
 * Set this GUI context as Dear ImGui's current context.
 *
 * @param gui the GUI overlay
 */
static void _gui_set_current(DvzGui* gui)
{
    ANN(gui);
    ANN(gui->context);
    ImGui::SetCurrentContext(gui->context);
}



static int _gui_dock_slot_index(DvzGuiDockSlot slot)
{
    switch (slot)
    {
    case DVZ_GUI_DOCK_SLOT_LEFT:
        return 0;
    case DVZ_GUI_DOCK_SLOT_RIGHT:
        return 1;
    case DVZ_GUI_DOCK_SLOT_TOP:
        return 2;
    case DVZ_GUI_DOCK_SLOT_BOTTOM:
        return 3;
    default:
        return -1;
    }
}



static ImGuiDir _gui_dock_slot_dir(DvzGuiDockSlot slot)
{
    switch (slot)
    {
    case DVZ_GUI_DOCK_SLOT_RIGHT:
        return ImGuiDir_Right;
    case DVZ_GUI_DOCK_SLOT_TOP:
        return ImGuiDir_Up;
    case DVZ_GUI_DOCK_SLOT_BOTTOM:
        return ImGuiDir_Down;
    case DVZ_GUI_DOCK_SLOT_LEFT:
    default:
        return ImGuiDir_Left;
    }
}



static float _gui_dock_slot_ratio(DvzGuiDockSlot slot, float size_px, ImVec2 viewport_size)
{
    const float axis =
        (slot == DVZ_GUI_DOCK_SLOT_LEFT || slot == DVZ_GUI_DOCK_SLOT_RIGHT) ? viewport_size.x :
                                                                              viewport_size.y;
    float ratio = DVZ_GUI_DEFAULT_DOCK_RATIO;
    if (size_px > 0.0f && isfinite(size_px) && axis > 0.0f)
        ratio = size_px / axis;
    if (ratio < 0.05f)
        ratio = 0.05f;
    if (ratio > 0.9f)
        ratio = 0.9f;
    return ratio;
}



static bool _gui_dock_window_hash_seen(const DvzGui* gui, ImGuiID hash)
{
    ANN(gui);
    for (uint32_t i = 0; i < gui->docked_window_hash_count; i++)
    {
        if (gui->docked_window_hashes[i] == hash)
            return true;
    }
    return false;
}



static bool _gui_dock_window_hash_add(DvzGui* gui, ImGuiID hash)
{
    ANN(gui);
    if (_gui_dock_window_hash_seen(gui, hash))
        return true;
    if (gui->docked_window_hash_count >= DVZ_GUI_MAX_DOCKED_WINDOWS)
        return false;
    gui->docked_window_hashes[gui->docked_window_hash_count++] = hash;
    return true;
}



static ImGuiID _gui_dock_node_for_slot(DvzGui* gui, DvzGuiDockSlot slot, float size_px)
{
    ANN(gui);
    const int index = _gui_dock_slot_index(slot);
    if (index < 0)
        return 0;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport == NULL)
        return 0;

    if (gui->dockspace_id == 0)
        gui->dockspace_id = ImGui::GetID("DatovizDockSpace");

    if (gui->dockspace_main_id == 0)
    {
        if (ImGui::DockBuilderGetNode(gui->dockspace_id) == NULL)
        {
            ImGui::DockBuilderAddNode(
                gui->dockspace_id,
                ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
        }
        else
        {
            ImGui::DockBuilderRemoveNodeChildNodes(gui->dockspace_id);
        }
        ImGui::DockBuilderSetNodePos(gui->dockspace_id, viewport->Pos);
        ImGui::DockBuilderSetNodeSize(gui->dockspace_id, viewport->Size);
        gui->dockspace_main_id = gui->dockspace_id;
    }

    if (gui->dock_nodes[index] == 0)
    {
        ImGuiID dock_id = 0;
        ImGuiID main_id = 0;
        ImGui::DockBuilderSplitNode(
            gui->dockspace_main_id, _gui_dock_slot_dir(slot),
            _gui_dock_slot_ratio(slot, size_px, viewport->Size), &dock_id, &main_id);
        gui->dock_nodes[index] = dock_id;
        gui->dockspace_main_id = main_id;
        ImGui::DockBuilderFinish(gui->dockspace_id);
    }

    return gui->dock_nodes[index];
}



/**
 * Return a usable font size.
 *
 * @param size requested font size
 * @param fallback fallback font size
 * @return requested font size, or fallback when the request is invalid
 */
static float _gui_font_size(float size, float fallback)
{
    return size > 0 ? size : fallback;
}



static bool _gui_font_defaults_validate(const DvzFontDefaults* defaults)
{
    if (defaults == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(defaults, DvzFontDefaults, DVZ_FONT_DEFAULTS_KNOWN_FLAGS))
    {
        log_error("invalid DvzFontDefaults ABI prologue");
        return false;
    }
    return true;
}



bool _dvz_gui_config_validate(const DvzGuiConfig* config)
{
    if (config == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(config, DvzGuiConfig, DVZ_GUI_CONFIG_KNOWN_FLAGS))
    {
        log_error("invalid DvzGuiConfig ABI prologue");
        return false;
    }
    return true;
}



static bool _gui_viewport_config_validate(const DvzGuiViewportConfig* config)
{
    if (config == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(config, DvzGuiViewportConfig, DVZ_GUI_VIEWPORT_CONFIG_KNOWN_FLAGS))
    {
        log_error("invalid DvzGuiViewportConfig ABI prologue");
        return false;
    }
    return true;
}



/**
 * Return a resize dimension snapped to viewport policy.
 *
 * @param value requested floating-point size
 * @param minimum minimum accepted size
 * @param step resize quantization step
 * @return snapped non-zero size
 */
static uint32_t _gui_viewport_dimension(float value, uint32_t minimum, uint32_t step)
{
    uint32_t out = value > 0 ? (uint32_t)(value + 0.5f) : minimum;
    if (out < minimum)
        out = minimum;
    if (step > 1)
    {
        out = ((out + step / 2) / step) * step;
        if (out < minimum)
            out = minimum;
    }
    return out;
}



static uint32_t _gui_viewport_scale_dimension(uint32_t value, float scale)
{
    if (value == 0)
        return 0;
    if (scale <= 0.0f || !isfinite(scale))
        scale = 1.0f;
    uint32_t out = (uint32_t)roundf((float)value * scale);
    return out > 0 ? out : 1u;
}



static float _gui_viewport_device_scale(const DvzGuiViewport* viewport)
{
    ANN(viewport);
    if (viewport->gui != NULL && viewport->gui->view != NULL)
        return dvz_view_device_scale(viewport->gui->view);
    return 1.0f;
}



/**
 * Return a GUI viewport configuration with zero fields replaced by defaults.
 *
 * @param config optional user configuration
 * @return normalized viewport configuration
 */
static DvzGuiViewportConfig _gui_viewport_config_normalize(const DvzGuiViewportConfig* config)
{
    DvzGuiViewportConfig out = config != NULL ? *config : dvz_gui_viewport_config();
    if (out.initial_width == 0)
        out.initial_width = DVZ_GUI_VIEWPORT_DEFAULT_INITIAL_WIDTH;
    if (out.initial_height == 0)
        out.initial_height = DVZ_GUI_VIEWPORT_DEFAULT_INITIAL_HEIGHT;
    if (out.min_width == 0)
        out.min_width = DVZ_GUI_VIEWPORT_DEFAULT_MIN_WIDTH;
    if (out.min_height == 0)
        out.min_height = DVZ_GUI_VIEWPORT_DEFAULT_MIN_HEIGHT;
    if (out.resize_step == 0)
        out.resize_step = DVZ_GUI_VIEWPORT_DEFAULT_RESIZE_STEP;
    return out;
}


/**
 * Return Datoviz keyboard modifier bits from the current ImGui IO state.
 *
 * @param io Dear ImGui IO state
 * @return Datoviz keyboard modifier bit mask
 */
static int _gui_mods_from_io(const ImGuiIO& io)
{
    int mods = DVZ_KEY_MODIFIER_NONE;
    if (io.KeyShift)
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (io.KeyCtrl)
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (io.KeyAlt)
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (io.KeySuper)
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}



/**
 * Convert a Datoviz 8-bit RGBA color to normalized float channels.
 *
 * @param color input Datoviz color
 * @param out output RGBA channels in [0, 1]
 */
static void _gui_color_to_float(const DvzColor color, float out[4])
{
    ANN(out);
    out[0] = (float)color.r / 255.0f;
    out[1] = (float)color.g / 255.0f;
    out[2] = (float)color.b / 255.0f;
    out[3] = (float)color.a / 255.0f;
}



/**
 * Convert normalized float RGBA channels to a Datoviz 8-bit color.
 *
 * @param rgba input RGBA channels
 * @param out output Datoviz color
 */
static void _gui_color_from_float(const float rgba[4], DvzColor* out)
{
    ANN(rgba);
    ANN(out);
    *out = dvz_color_from_unit(rgba[0], rgba[1], rgba[2], rgba[3]);
}



/**
 * Translate a GLFW key action to a Datoviz keyboard event type.
 *
 * @param action GLFW key action
 * @return Datoviz keyboard event type
 */
static DvzKeyboardEventType _gui_key_event_type(int action)
{
    if (action == GLFW_PRESS)
        return DVZ_KEYBOARD_EVENT_PRESS;
    if (action == GLFW_RELEASE)
        return DVZ_KEYBOARD_EVENT_RELEASE;
    if (action == GLFW_REPEAT)
        return DVZ_KEYBOARD_EVENT_REPEAT;
    return DVZ_KEYBOARD_EVENT_NONE;
}



/**
 * Update whether the source view should render.
 *
 * @param viewport GUI viewport
 * @param visible whether the viewport window was visible this frame
 */
static void _gui_viewport_set_visible(DvzGuiViewport* viewport, bool visible)
{
    ANN(viewport);
    viewport->visible = visible;
    if (viewport->source == NULL)
        return;

    const bool render_hidden =
        (viewport->config.viewport_flags & DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN) != 0;
    const bool enabled = visible || !viewport->has_frame || render_hidden;
    dvz_view_set_render_enabled(viewport->source, enabled);
}


/**
 * Clear the current ImGui-frame presentation request for a viewport.
 *
 * @param viewport GUI viewport
 */
static void _gui_viewport_reset_frame_request(DvzGuiViewport* viewport)
{
    ANN(viewport);
    viewport->frame_visible = false;
    viewport->frame_resolved = false;
    viewport->frame_request_width = 0;
    viewport->frame_request_height = 0;
    viewport->frame_request_framebuffer_width = 0;
    viewport->frame_request_framebuffer_height = 0;
    viewport->frame_draw_list = NULL;
    viewport->frame_image_min = ImVec2(0.0f, 0.0f);
    viewport->frame_image_max = ImVec2(0.0f, 0.0f);
}



/**
 * Return whether the currently published source image exactly satisfies this frame's request.
 *
 * @param viewport GUI viewport
 * @return whether the live image matches the requested framebuffer extent
 */
static bool _gui_viewport_frame_matches_request(const DvzGuiViewport* viewport)
{
    ANN(viewport);
    if (!viewport->frame_visible)
        return false;
    if (!viewport->has_frame || !viewport->image_valid)
        return false;
    if (viewport->frame_request_framebuffer_width == 0 ||
        viewport->frame_request_framebuffer_height == 0)
    {
        return false;
    }
    return viewport->extent.width == viewport->frame_request_framebuffer_width &&
           viewport->extent.height == viewport->frame_request_framebuffer_height;
}



/**
 * Draw a strictly resolved viewport image into the recorded ImGui item rect.
 *
 * @param viewport GUI viewport
 */
static void _gui_viewport_draw_resolved(DvzGuiViewport* viewport)
{
    ANN(viewport);
    if (!viewport->frame_visible || viewport->frame_draw_list == NULL)
        return;
    if (!_gui_viewport_frame_matches_request(viewport))
        return;
    if (!_gui_viewport_display_drawable(viewport) || !_gui_viewport_ensure_texture(viewport))
        return;

    viewport->frame_draw_list->AddImage(
        (ImTextureID)viewport->texture, viewport->frame_image_min, viewport->frame_image_max,
        ImVec2(0, 0), ImVec2(1, 1));
    viewport->frame_resolved = true;
}



/**
 * Load Datoviz's ImGui fonts into the current font atlas.
 *
 * @param gui the GUI overlay
 */
static void _gui_load_fonts(DvzGui* gui)
{
    ANN(gui);
    _gui_set_current(gui);

    ImGuiIO& io = ImGui::GetIO();
    DvzFontDefaults defaults = gui->font_defaults;
    DvzFontDefaults fallback_defaults = dvz_font_defaults();
    if (defaults.ui_size_px <= 0.0f)
        defaults.ui_size_px = fallback_defaults.ui_size_px;
    if (defaults.mono_size_px <= 0.0f)
        defaults.mono_size_px = fallback_defaults.mono_size_px;

    const float font_size = _gui_font_size(defaults.ui_size_px, fallback_defaults.ui_size_px);
    const float mono_font_size =
        _gui_font_size(defaults.mono_size_px, fallback_defaults.mono_size_px);

    ImFontConfig regular_config = {};
    regular_config.OversampleH = 2;
    regular_config.OversampleV = 1;
    if (defaults.sans_path != NULL && defaults.sans_path[0] != '\0')
        gui->font_regular = io.Fonts->AddFontFromFileTTF(
            defaults.sans_path, font_size, &regular_config, io.Fonts->GetGlyphRangesDefault());
    if (gui->font_regular == NULL)
    {
        ImFontConfig embedded_config = regular_config;
        embedded_config.FontDataOwnedByAtlas = false;
        DvzSize size = 0;
        const unsigned char* bytes = dvz_resource_font("SourceSans3_Regular", &size);
        if (bytes != NULL && size > 0 && size <= INT_MAX)
            gui->font_regular = io.Fonts->AddFontFromMemoryTTF(
                const_cast<void*>((const void*)bytes), (int)size, font_size, &embedded_config,
                io.Fonts->GetGlyphRangesDefault());
    }
    if (gui->font_regular == NULL)
    {
        log_error("Dear ImGui failed to load the embedded Source Sans 3 font");
        gui->font_regular = io.Fonts->AddFontDefault();
    }

    // Merge only bounded scientific ranges into the regular UI font.
    static const ImWchar scientific_ranges[] = {
        0x0370, 0x03FF, // Greek and Coptic
        0x1D00, 0x1D7F, // phonetic extensions and superscript modifiers
        0x2070, 0x209F, // superscripts and subscripts
        0x2100, 0x214F, // letterlike symbols
        0x2190, 0x21FF, // arrows
        0x2200, 0x22FF, // mathematical operators
        0x2300, 0x23FF, // technical symbols
        0x25A0, 0x25FF, // geometric shapes
        0x27C0, 0x27FF, // supplemental mathematical operators
        0x2900, 0x297F, // supplemental arrows
        0x2980, 0x29FF, // miscellaneous mathematical symbols
        0x2A00, 0x2AFF, // supplemental mathematical operators
        0,
    };
    ImFontConfig math_config = {};
    math_config.MergeMode = true;
    math_config.OversampleH = 2;
    math_config.OversampleV = 1;
    math_config.FontDataOwnedByAtlas = false;
    DvzSize math_size = 0;
    const unsigned char* math_bytes = dvz_resource_font("NotoSansMath_Regular", &math_size);
    if (gui->font_regular != NULL && math_bytes != NULL && math_size > 0 && math_size <= INT_MAX)
        io.Fonts->AddFontFromMemoryTTF(
            const_cast<void*>((const void*)math_bytes), (int)math_size, font_size, &math_config, scientific_ranges);

    ImFontConfig mono_config = {};
    mono_config.OversampleH = 2;
    mono_config.OversampleV = 1;
    if (defaults.mono_path != NULL && defaults.mono_path[0] != '\0')
        gui->font_mono = io.Fonts->AddFontFromFileTTF(
            defaults.mono_path, mono_font_size, &mono_config, io.Fonts->GetGlyphRangesDefault());
    if (gui->font_mono == NULL)
    {
        ImFontConfig embedded_config = mono_config;
        embedded_config.FontDataOwnedByAtlas = false;
        DvzSize size = 0;
        const unsigned char* bytes = dvz_resource_font("SourceCodePro_Regular", &size);
        if (bytes != NULL && size > 0 && size <= INT_MAX)
            gui->font_mono = io.Fonts->AddFontFromMemoryTTF(
                const_cast<void*>((const void*)bytes), (int)size, mono_font_size, &embedded_config,
                io.Fonts->GetGlyphRangesDefault());
    }
    if (gui->font_mono == NULL)
        log_error("Dear ImGui failed to load the embedded Source Code Pro font");

    if (gui->font_regular != NULL)
        io.FontDefault = gui->font_regular;
}



/**
 * Attach a GUI viewport to the overlay-owned list.
 *
 * @param gui the GUI overlay
 * @param viewport the viewport to attach
 */
static void _gui_viewport_attach(DvzGui* gui, DvzGuiViewport* viewport)
{
    ANN(gui);
    ANN(viewport);
    viewport->next = gui->viewports;
    gui->viewports = viewport;
}



/**
 * Detach a GUI viewport from the overlay-owned list.
 *
 * @param gui the GUI overlay
 * @param viewport the viewport to detach
 */
static void _gui_viewport_detach(DvzGui* gui, DvzGuiViewport* viewport)
{
    ANN(gui);
    ANN(viewport);
    DvzGuiViewport* prev = NULL;
    DvzGuiViewport* cur = gui->viewports;
    while (cur != NULL)
    {
        if (cur == viewport)
        {
            if (prev != NULL)
                prev->next = cur->next;
            else
                gui->viewports = cur->next;
            viewport->next = NULL;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}



/**
 * Create the sampler used when displaying Datoviz images through ImGui.
 *
 * @param viewport GUI viewport receiving the sampler
 * @return whether the sampler was created
 */
static bool _gui_viewport_create_sampler(DvzGuiViewport* viewport)
{
    ANN(viewport);
    ANN(viewport->gui);
    VkDevice device = dvz_device_handle(viewport->gui->device);
    if (device == VK_NULL_HANDLE)
        return false;

    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;
    info.maxAnisotropy = 1.0f;
    VkResult res = vkCreateSampler(device, &info, NULL, &viewport->sampler);
    if (res != VK_SUCCESS)
    {
        log_error("Dear ImGui Datoviz viewport sampler creation failed: %d", (int)res);
        viewport->sampler = VK_NULL_HANDLE;
        return false;
    }
    return true;
}



static void _gui_viewport_free_texture(DvzGui* gui, VkDescriptorSet texture)
{
    if (gui == NULL || !gui->vulkan_initialized || texture == VK_NULL_HANDLE)
        return;
    _gui_set_current(gui);
    ImGui_ImplVulkan_RemoveTexture(texture);
}



static void _gui_viewport_collect_retired_textures(DvzGuiViewport* viewport, bool wait_idle)
{
    ANN(viewport);
    if (viewport->retired_texture_count == 0)
        return;

    DvzGui* gui = viewport->gui;
    if (gui != NULL && wait_idle)
    {
        VkDevice device = dvz_device_handle(gui->device);
        if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);
    }
    for (uint32_t i = 0; i < viewport->retired_texture_count; i++)
        _gui_viewport_free_texture(gui, viewport->retired_textures[i]);
    viewport->retired_texture_count = 0;
}



/**
 * Retire the current ImGui descriptor for a viewport image.
 *
 * Submitted GUI command buffers may still reference the descriptor, so actual descriptor-set
 * freeing is deferred until a known idle point.
 *
 * @param viewport GUI viewport
 */
static void _gui_viewport_retire_texture(DvzGuiViewport* viewport)
{
    ANN(viewport);
    if (viewport->texture == VK_NULL_HANDLE)
        return;
    if (viewport->retired_texture_count >= DVZ_GUI_VIEWPORT_RETIRED_TEXTURE_CAPACITY)
        _gui_viewport_collect_retired_textures(viewport, true);
    if (viewport->retired_texture_count < DVZ_GUI_VIEWPORT_RETIRED_TEXTURE_CAPACITY)
        viewport->retired_textures[viewport->retired_texture_count++] = viewport->texture;
    viewport->texture = VK_NULL_HANDLE;
}



/**
 * Ensure the ImGui descriptor points at the current Datoviz source image view.
 *
 * @param viewport GUI viewport
 * @return whether a texture descriptor is ready
 */
static bool _gui_viewport_ensure_texture(DvzGuiViewport* viewport)
{
    ANN(viewport);
    DvzGui* gui = viewport->gui;
    ANN(gui);
    if (!gui->vulkan_initialized || viewport->image_view == VK_NULL_HANDLE ||
        viewport->sampler == VK_NULL_HANDLE)
    {
        return false;
    }
    if (!viewport->texture_dirty && viewport->texture != VK_NULL_HANDLE)
        return true;

    _gui_viewport_retire_texture(viewport);
    _gui_set_current(gui);
    viewport->texture = ImGui_ImplVulkan_AddTexture(
        viewport->sampler, viewport->image_view, VK_IMAGE_LAYOUT_GENERAL);
    viewport->texture_dirty = viewport->texture == VK_NULL_HANDLE;
    return viewport->texture != VK_NULL_HANDLE;
}



/**
 * Return whether a source frame matches the committed viewport source size.
 *
 * @param viewport GUI viewport
 * @param extent source framebuffer extent
 * @return whether the frame may become the displayed viewport image
 */
static bool _gui_viewport_frame_matches_committed(
    const DvzGuiViewport* viewport, const VkExtent2D extent)
{
    ANN(viewport);
    if (extent.width == 0 || extent.height == 0)
        return false;
    if (viewport->requested_framebuffer_width == 0 || viewport->requested_framebuffer_height == 0)
        return true;
    return extent.width == viewport->requested_framebuffer_width &&
           extent.height == viewport->requested_framebuffer_height;
}



/**
 * Return whether the last accepted source frame can be displayed in the current ImGui image item.
 *
 * @param viewport GUI viewport
 * @param width snapped content width for this ImGui frame
 * @param height snapped content height for this ImGui frame
 * @return whether the texture can be drawn without stretching a stale source frame
 */
static bool
_gui_viewport_display_ready(const DvzGuiViewport* viewport, uint32_t width, uint32_t height)
{
    ANN(viewport);
    if (!viewport->has_frame)
        return false;
    if (width == 0 || height == 0)
        return false;
    if (!_gui_viewport_frame_matches_committed(viewport, viewport->extent))
        return false;
    if (viewport->requested_width != width || viewport->requested_height != height)
        return viewport->pending_width == width && viewport->pending_height == height;
    return true;
}


/**
 * Return whether the viewport has a drawable source texture for the current ImGui frame.
 *
 * @param viewport GUI viewport
 * @return whether a live-image frame may be drawn
 */
static bool _gui_viewport_display_drawable(const DvzGuiViewport* viewport)
{
    ANN(viewport);
    return viewport->has_frame && viewport->image_valid && viewport->image_view != VK_NULL_HANDLE &&
           viewport->extent.width > 0 && viewport->extent.height > 0;
}



/**
 * Receive a live source-canvas image after submission.
 *
 * @param frame live image metadata
 * @param user_data GUI viewport
 * @return 0 on success
 */
static int _gui_viewport_live_image_callback(
    const DvzCanvasLiveImageFrame* frame, void* user_data)
{
    ANN(frame);
    DvzGuiViewport* viewport = (DvzGuiViewport*)user_data;
    ANN(viewport);
    if (!frame->image_valid || frame->image_view == VK_NULL_HANDLE || frame->extent.width == 0 ||
        frame->extent.height == 0 || frame->resource_generation == 0)
    {
        return 0;
    }
    if (
        viewport->image_view != frame->image_view ||
        viewport->resource_generation != frame->resource_generation)
    {
        _gui_viewport_retire_texture(viewport);
        viewport->image_view = frame->image_view;
        viewport->image = frame->image;
        viewport->resource_generation = frame->resource_generation;
        viewport->texture_dirty = true;
    }
    viewport->extent = frame->extent;
    viewport->image_valid = true;
    viewport->has_frame = true;
    viewport->stale_frame_count = _gui_viewport_frame_matches_committed(viewport, frame->extent) ?
                                      0 :
                                      viewport->stale_frame_count + 1;
    _gui_request_frame(viewport->gui);
    return 0;
}



/**
 * Rebuild the source live-image stream after a viewport resize.
 *
 * @param viewport GUI viewport
 * @param width new source width
 * @param height new source height
 */
static void _gui_viewport_resize_source(DvzGuiViewport* viewport, uint32_t width, uint32_t height)
{
    ANN(viewport);
    float scale = _gui_viewport_device_scale(viewport);
    uint32_t framebuffer_width = _gui_viewport_scale_dimension(width, scale);
    uint32_t framebuffer_height = _gui_viewport_scale_dimension(height, scale);
    if (
        viewport->requested_width == width && viewport->requested_height == height &&
        viewport->requested_framebuffer_width == framebuffer_width &&
        viewport->requested_framebuffer_height == framebuffer_height)
    {
        return;
    }
    if (dvz_view_resize_scaled(viewport->source, width, height, scale) != 0)
        return;
    viewport->requested_width = width;
    viewport->requested_height = height;
    viewport->requested_framebuffer_width = framebuffer_width;
    viewport->requested_framebuffer_height = framebuffer_height;
    viewport->stale_frame_count = 0;
}



/**
 * Forward ImGui item input to the source view router.
 *
 * @param viewport GUI viewport
 * @param image_min top-left image position in ImGui coordinates
 * @param size displayed image size
 */
static void _gui_viewport_forward_input(
    DvzGuiViewport* viewport, ImVec2 image_min, ImVec2 size)
{
    ANN(viewport);
    if (viewport->source == NULL || dvz_view_input(viewport->source) == NULL ||
        size.x <= 0 || size.y <= 0)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    float x = io.MousePos.x - image_min.x;
    float y = io.MousePos.y - image_min.y;
    const float window_x = size.x;
    const float window_y = size.y;
    const int mods = _gui_mods_from_io(io);

    if ((hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) || active)
    {
        viewport->keyboard_focused = true;
        if (viewport->gui != NULL)
            viewport->gui->keyboard_viewport = viewport;
    }

    const DvzPointerButton buttons[3] = {
        DVZ_POINTER_BUTTON_LEFT,
        DVZ_POINTER_BUTTON_RIGHT,
        DVZ_POINTER_BUTTON_MIDDLE,
    };
    for (int i = 0; i < 3; i++)
    {
        if (ImGui::IsMouseClicked(i) && hovered)
        {
            viewport->input_capturing = true;
            viewport->input_button = i;
            viewport->input_dvz_button = buttons[i];
            (void)dvz_view_emit_pointer(
                viewport->source, DVZ_POINTER_EVENT_PRESS, x, y, window_x, window_y, buttons[i],
                mods);
        }
    }

    if (!hovered && !viewport->input_capturing)
        return;

    (void)dvz_view_emit_pointer(
        viewport->source, DVZ_POINTER_EVENT_MOVE, x, y, window_x, window_y,
        DVZ_POINTER_BUTTON_NONE, mods);
    if (hovered && (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f))
    {
        (void)dvz_view_emit_wheel(
            viewport->source, x, y, window_x, window_y, io.MouseWheelH, io.MouseWheel, mods);
    }

    if (
        viewport->input_capturing &&
        (ImGui::IsMouseReleased(viewport->input_button) || !io.MouseDown[viewport->input_button]))
    {
        (void)dvz_view_emit_pointer(
            viewport->source, DVZ_POINTER_EVENT_RELEASE, x, y, window_x, window_y,
            viewport->input_dvz_button, mods);
        viewport->input_capturing = false;
        viewport->input_button = 0;
        viewport->input_dvz_button = DVZ_POINTER_BUTTON_NONE;
    }
}


/**
 * Update the public mouse state for a rendered GUI viewport image.
 *
 * @param viewport GUI viewport
 * @param image_min top-left image position in ImGui coordinates
 * @param size displayed image size
 */
static void _gui_viewport_update_mouse(
    DvzGuiViewport* viewport, ImVec2 image_min, ImVec2 size)
{
    ANN(viewport);
    ImGuiIO& io = ImGui::GetIO();
    const bool hovered = ImGui::IsItemHovered();
    viewport->mouse_valid = size.x > 0.0f && size.y > 0.0f;
    viewport->mouse_hovered = hovered;
    viewport->mouse_pos[0] = io.MousePos.x - image_min.x;
    viewport->mouse_pos[1] = io.MousePos.y - image_min.y;
    viewport->mouse_size[0] = size.x;
    viewport->mouse_size[1] = size.y;
}



/**
 * Destroy a GUI viewport, optionally unlinking it from the owning overlay.
 *
 * @param viewport GUI viewport
 * @param detach whether to detach from the overlay-owned list
 */
static void _gui_viewport_destroy(DvzGuiViewport* viewport, bool detach)
{
    if (viewport == NULL)
        return;
    DvzGui* gui = viewport->gui;
    if (gui != NULL)
    {
        _gui_set_current(gui);
        if (detach)
            _gui_viewport_detach(gui, viewport);
        if (gui->keyboard_viewport == viewport)
            gui->keyboard_viewport = NULL;
    }
    if (viewport->canvas != NULL)
        (void)dvz_canvas_configure_live_image_sink(viewport->canvas, false, NULL);
    if (viewport->source != NULL && viewport->owns_source)
        dvz_view_set_render_enabled(viewport->source, false);
    _gui_viewport_retire_texture(viewport);
    _gui_viewport_collect_retired_textures(viewport, true);
    if (gui != NULL && viewport->sampler != VK_NULL_HANDLE)
    {
        VkDevice device = dvz_device_handle(gui->device);
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);
            vkDestroySampler(device, viewport->sampler, NULL);
        }
    }
    dvz_free(viewport);
}



/**
 * Return whether ImGui currently wants to capture mouse input.
 *
 * @param gui the GUI overlay
 * @return whether mouse input should be consumed before Datoviz routing
 */
static bool _gui_want_capture_mouse(DvzGui* gui)
{
    _gui_set_current(gui);
    return ImGui::GetIO().WantCaptureMouse;
}



/**
 * Return whether ImGui currently wants to capture keyboard input.
 *
 * @param gui the GUI overlay
 * @return whether keyboard input should be consumed before Datoviz routing
 */
static bool _gui_want_capture_keyboard(DvzGui* gui)
{
    _gui_set_current(gui);
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantTextInput;
}


/**
 * Forward a GLFW key event to the currently keyboard-focused GUI viewport.
 *
 * @param gui the GUI overlay
 * @param key GLFW key
 * @param action GLFW action
 * @param mods GLFW modifier mask
 * @return whether the key was forwarded and should be consumed
 */
static bool _gui_viewport_forward_key(DvzGui* gui, int key, int action, int mods)
{
    ANN(gui);
    DvzGuiViewport* viewport = gui->keyboard_viewport;
    if (viewport == NULL || viewport->source == NULL || !viewport->keyboard_focused)
        return false;

    DvzKeyboardEventType type = _gui_key_event_type(action);
    if (type == DVZ_KEYBOARD_EVENT_NONE)
        return false;

    return dvz_view_emit_key(viewport->source, type, (DvzKeyCode)key, mods) == 0;
}



static uint32_t _gui_utf8_encode(uint32_t codepoint, char out[4])
{
    if (codepoint <= 0x7fu)
    {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7ffu)
    {
        out[0] = (char)(0xc0u | (codepoint >> 6));
        out[1] = (char)(0x80u | (codepoint & 0x3fu));
        return 2;
    }
    if (codepoint >= 0xd800u && codepoint <= 0xdfffu)
        return 0;
    if (codepoint <= 0xffffu)
    {
        out[0] = (char)(0xe0u | (codepoint >> 12));
        out[1] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
        out[2] = (char)(0x80u | (codepoint & 0x3fu));
        return 3;
    }
    if (codepoint <= 0x10ffffu)
    {
        out[0] = (char)(0xf0u | (codepoint >> 18));
        out[1] = (char)(0x80u | ((codepoint >> 12) & 0x3fu));
        out[2] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
        out[3] = (char)(0x80u | (codepoint & 0x3fu));
        return 4;
    }
    return 0;
}



static bool _gui_viewport_forward_text(DvzGui* gui, uint32_t codepoint)
{
    ANN(gui);
    DvzGuiViewport* viewport = gui->keyboard_viewport;
    if (viewport == NULL || viewport->source == NULL || !viewport->keyboard_focused)
        return false;
    char utf8[4] = {};
    uint32_t byte_size = _gui_utf8_encode(codepoint, utf8);
    return byte_size > 0 &&
           dvz_view_emit_text(viewport->source, utf8, byte_size, gui->glfw_mods) == DVZ_OK;
}



/**
 * Request a view frame after GUI input changes ImGui state.
 *
 * @param gui the GUI overlay
 */
static void _gui_request_frame(DvzGui* gui)
{
    if (gui == NULL || gui->view == NULL)
        return;
    dvz_view_request_frame(gui->view);
}



/**
 * Update tracked ImGui interaction state and report whether another frame is needed.
 *
 * @param gui the GUI overlay
 * @return whether the current GUI state transition needs a follow-up frame
 */
static bool _gui_update_followup_frame_state(DvzGui* gui)
{
    ANN(gui);
    _gui_set_current(gui);

    ImGuiIO& io = ImGui::GetIO();
    bool active_item = ImGui::IsAnyItemActive() || io.WantTextInput;
    bool open_popup = ImGui::IsPopupOpen((const char*)NULL, ImGuiPopupFlags_AnyPopup);
    bool request_frame = active_item != gui->had_active_item || open_popup != gui->had_open_popup;

    gui->had_active_item = active_item;
    gui->had_open_popup = open_popup;
    return request_frame;
}



/**
 * Forward a raw GLFW cursor event to ImGui.
 *
 * @param window Datoviz window
 * @param x cursor x position
 * @param y cursor y position
 * @param user_data GUI pointer
 * @return whether Datoviz should consume the event
 */
static bool _gui_glfw_cursor_pos(DvzWindow* window, double x, double y, void* user_data)
{
    (void)window;
    DvzGui* gui = (DvzGui*)user_data;
    ANN(gui);
    _gui_set_current(gui);
    ImGui_ImplGlfw_CursorPosCallback(gui->glfw_window, x, y);
    _gui_request_frame(gui);
    return _gui_want_capture_mouse(gui);
}



/**
 * Forward a raw GLFW mouse-button event to ImGui.
 *
 * @param window Datoviz window
 * @param button GLFW button
 * @param action GLFW action
 * @param mods GLFW modifier mask
 * @param user_data GUI pointer
 * @return whether Datoviz should consume the event
 */
static bool
_gui_glfw_mouse_button(DvzWindow* window, int button, int action, int mods, void* user_data)
{
    (void)window;
    DvzGui* gui = (DvzGui*)user_data;
    ANN(gui);
    _gui_set_current(gui);
    ImGui_ImplGlfw_MouseButtonCallback(gui->glfw_window, button, action, mods);
    _gui_request_frame(gui);
    return _gui_want_capture_mouse(gui);
}



/**
 * Forward a raw GLFW scroll event to ImGui.
 *
 * @param window Datoviz window
 * @param xoffset horizontal offset
 * @param yoffset vertical offset
 * @param user_data GUI pointer
 * @return whether Datoviz should consume the event
 */
static bool _gui_glfw_scroll(DvzWindow* window, double xoffset, double yoffset, void* user_data)
{
    (void)window;
    DvzGui* gui = (DvzGui*)user_data;
    ANN(gui);
    _gui_set_current(gui);
    ImGui_ImplGlfw_ScrollCallback(gui->glfw_window, xoffset, yoffset);
    _gui_request_frame(gui);
    return _gui_want_capture_mouse(gui);
}



/**
 * Forward a raw GLFW keyboard event to ImGui.
 *
 * @param window Datoviz window
 * @param key GLFW key
 * @param scancode GLFW scancode
 * @param action GLFW action
 * @param mods GLFW modifier mask
 * @param user_data GUI pointer
 * @return whether Datoviz should consume the event
 */
static bool
_gui_glfw_key(DvzWindow* window, int key, int scancode, int action, int mods, void* user_data)
{
    (void)window;
    DvzGui* gui = (DvzGui*)user_data;
    ANN(gui);
    gui->glfw_mods = mods;
    _gui_set_current(gui);
    ImGui_ImplGlfw_KeyCallback(gui->glfw_window, key, scancode, action, mods);
    _gui_request_frame(gui);
    bool capture = _gui_want_capture_keyboard(gui);
    if (!capture)
        capture = _gui_viewport_forward_key(gui, key, action, mods);
    return capture;
}



/**
 * Forward a raw GLFW character event to ImGui.
 *
 * @param window Datoviz window
 * @param codepoint Unicode codepoint
 * @param user_data GUI pointer
 * @return whether Datoviz should consume the event
 */
static bool _gui_glfw_char(DvzWindow* window, uint32_t codepoint, void* user_data)
{
    (void)window;
    DvzGui* gui = (DvzGui*)user_data;
    ANN(gui);
    _gui_set_current(gui);
    ImGui_ImplGlfw_CharCallback(gui->glfw_window, codepoint);
    _gui_request_frame(gui);
    if (_gui_want_capture_keyboard(gui))
        return true;
    return _gui_viewport_forward_text(gui, codepoint);
}



/**
 * Ensure the Vulkan backend is initialized for the canvas frame format.
 *
 * @param gui the GUI overlay
 * @param frame current canvas frame
 * @return whether initialization is available
 */
static bool _gui_ensure_vulkan(DvzGui* gui, const DvzStreamFrame* frame)
{
    ANN(gui);
    ANN(frame);
    if (gui->failed)
        return false;
    if (gui->vulkan_initialized)
        return true;

    if (frame->color_format == VK_FORMAT_UNDEFINED || frame->image_view == VK_NULL_HANDLE ||
        frame->command_buffer == VK_NULL_HANDLE)
    {
        log_error("Dear ImGui Vulkan init failed: invalid canvas frame");
        gui->failed = true;
        return false;
    }

    DvzInstance* instance = dvz_gpu_ctx_instance(gui->gpu_ctx);
    ANN(instance);
    VkInstance vk_instance = dvz_instance_handle(instance);
    VkPhysicalDevice physical_device = dvz_device_physical_device(gui->device);
    VkDevice device = dvz_device_handle(gui->device);
    if (vk_instance == VK_NULL_HANDLE || physical_device == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE || gui->queue == NULL)
    {
        log_error("Dear ImGui Vulkan init failed: missing Vulkan handles");
        gui->failed = true;
        return false;
    }

    gui->color_format = frame->color_format;
    VkPipelineRenderingCreateInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &gui->color_format;

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = vk_instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device;
    init_info.QueueFamily = dvz_queue_family(gui->queue);
    init_info.Queue = dvz_queue_handle(gui->queue);
    init_info.DescriptorPoolSize = 1024;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = rendering_info;
    init_info.CheckVkResultFn = _gui_check_vk_result;
    init_info.MinAllocationSize = 1024 * 1024;

    _gui_set_current(gui);
    if (!ImGui_ImplVulkan_Init(&init_info))
    {
        log_error("Dear ImGui Vulkan init failed");
        gui->failed = true;
        return false;
    }
    gui->vulkan_initialized = true;
    return true;
}



/**
 * Submit the full-window docking space.
 *
 * @param gui the GUI overlay
 */
static void _gui_submit_dockspace(DvzGui* gui)
{
    ANN(gui);
    if ((gui->config.gui_flags & DVZ_GUI_FLAGS_DOCKSPACE) == 0)
        return;
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    gui->dockspace_id = ImGui::GetID("DatovizDockSpace");
    ImGui::DockSpaceOverViewport(gui->dockspace_id, ImGui::GetMainViewport(), flags);
}



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

/**
 * Create an ImGui overlay.
 *
 * @param app app that owns any GUI-hosted offscreen windows
 * @param gpu_ctx GPU context borrowed from the app
 * @param view view to invalidate after GUI input
 * @param window GLFW window borrowed from the view
 * @param config optional GUI configuration
 * @param font_defaults resolved app font defaults
 * @return created GUI overlay, or NULL
 */
DvzGui* _dvz_gui_create(
    DvzApp* app, DvzGpuCtx* gpu_ctx, DvzView* view, DvzWindow* window,
    const DvzGuiConfig* config, const DvzFontDefaults* font_defaults)
{
    ANN(app);
    ANN(gpu_ctx);
    ANN(window);
    if (!_dvz_gui_config_validate(config) || !_gui_font_defaults_validate(font_defaults))
        return NULL;

    GLFWwindow* glfw_window = (GLFWwindow*)dvz_window_backend_handle(window);
    if (glfw_window == NULL)
    {
        log_error("Dear ImGui overlay requires a GLFW window");
        return NULL;
    }

    DvzGui* gui = (DvzGui*)dvz_calloc(1, sizeof(DvzGui));
    if (gui == NULL)
        return NULL;

    gui->app = app;
    gui->view = view;
    gui->gpu_ctx = gpu_ctx;
    gui->device = dvz_gpu_ctx_device(gpu_ctx);
    gui->queue = dvz_gpu_ctx_queue(gpu_ctx, DVZ_QUEUE_MAIN);
    gui->window = window;
    gui->glfw_window = glfw_window;
    gui->config = config != NULL ? *config : dvz_gui_config();
    gui->font_defaults = *font_defaults;
    gui->context = ImGui::CreateContext(NULL);
    if (gui->context == NULL)
    {
        dvz_free(gui);
        return NULL;
    }

    _gui_set_current(gui);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = gui->config.ini_path;
    _gui_load_fonts(gui);
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForVulkan(glfw_window, false))
    {
        log_error("Dear ImGui GLFW init failed");
        ImGui::DestroyContext(gui->context);
        dvz_free(gui);
        return NULL;
    }
    gui->glfw_initialized = true;

    DvzWindowGlfwInputCallbacks callbacks = {};
    callbacks.cursor_pos = _gui_glfw_cursor_pos;
    callbacks.mouse_button = _gui_glfw_mouse_button;
    callbacks.scroll = _gui_glfw_scroll;
    callbacks.key = _gui_glfw_key;
    callbacks.character = _gui_glfw_char;
    dvz_window_glfw_set_input_callbacks(window, &callbacks, gui);

    return gui;
}


DvzFontDefaults _dvz_gui_font_defaults(const DvzGui* gui)
{
    if (gui == NULL)
        return dvz_font_defaults();
    return gui->font_defaults;
}



/**
 * Destroy an ImGui overlay.
 *
 * @param gui the GUI overlay
 */
void _dvz_gui_destroy(DvzGui* gui)
{
    if (gui == NULL)
        return;

    dvz_window_glfw_set_input_callbacks(gui->window, NULL, NULL);
    _gui_set_current(gui);
    while (gui->viewports != NULL)
        _gui_viewport_destroy(gui->viewports, true);
    if (gui->vulkan_initialized)
        ImGui_ImplVulkan_Shutdown();
    if (gui->glfw_initialized)
        ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(gui->context);
    dvz_free(gui);
}



/**
 * Set the GUI frame callback.
 *
 * @param gui the GUI overlay
 * @param callback callback pointer, or NULL
 * @param user_data opaque callback user data
 */
void _dvz_gui_set_callback(DvzGui* gui, DvzGuiCallback callback, void* user_data)
{
    ANN(gui);
    gui->callback = callback;
    gui->callback_user_data = user_data;
}



/**
 * Begin a new ImGui frame and call user GUI code.
 *
 * @param gui the GUI overlay
 * @param win view passed to user callbacks
 * @param frame current canvas frame
 */
void _dvz_gui_begin_frame(DvzGui* gui, DvzView* win, const DvzStreamFrame* frame)
{
    ANN(gui);
    ANN(frame);
    bool was_initialized = gui->vulkan_initialized;
    if (!_gui_ensure_vulkan(gui, frame))
        return;
    if (!was_initialized)
        _gui_request_frame(gui);
    _gui_set_current(gui);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (DvzGuiViewport* viewport = gui->viewports; viewport != NULL; viewport = viewport->next)
        _gui_viewport_reset_frame_request(viewport);
    if (
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    {
        if (gui->keyboard_viewport != NULL)
            gui->keyboard_viewport->keyboard_focused = false;
        gui->keyboard_viewport = NULL;
    }
    _gui_submit_dockspace(gui);
    if (gui->callback != NULL)
        gui->callback(gui, win, gui->callback_user_data);
}



/**
 * Resolve all visible GUI viewport presentation requests for the current ImGui frame.
 *
 * @param gui GUI overlay
 * @param callback callback synchronizing one offscreen source view when it has pending work
 * @param user_data opaque callback user data
 * @return whether all visible viewport requests were resolved exactly
 */
bool _dvz_gui_resolve_viewports(
    DvzGui* gui, DvzGuiViewportResolveCallback callback, void* user_data)
{
    ANN(gui);
    _gui_set_current(gui);
    bool ok = true;
    for (DvzGuiViewport* viewport = gui->viewports; viewport != NULL; viewport = viewport->next)
    {
        if (!viewport->frame_visible)
            continue;
        if (viewport->source == NULL)
        {
            ok = false;
            continue;
        }

        _gui_viewport_resize_source(
            viewport, viewport->frame_request_width, viewport->frame_request_height);
        if (callback == NULL || callback(viewport->source, user_data) < 0)
        {
            ok = false;
            continue;
        }
        if (!_gui_viewport_frame_matches_request(viewport))
        {
            log_error(
                "Datoviz GUI viewport unresolved: requested %ux%u framebuffer, got %ux%u",
                viewport->frame_request_framebuffer_width,
                viewport->frame_request_framebuffer_height,
                viewport->extent.width, viewport->extent.height);
            ok = false;
            continue;
        }
        _gui_viewport_draw_resolved(viewport);
    }
    return ok;
}



/**
 * Submit the built-in FPS overlay for the current ImGui frame.
 *
 * @param gui the GUI overlay
 * @param fps smoothed frames per second
 * @param frame_ms smoothed frame duration in milliseconds
 * @param frames frames in the latest coarse measurement window
 * @param elapsed_s latest coarse measurement-window duration in seconds
 */
void _dvz_gui_fps_overlay(
    DvzGui* gui, double fps, double frame_ms, uint32_t frames, double elapsed_s)
{
    ANN(gui);
    if (gui->failed || !gui->vulkan_initialized)
        return;

    _gui_set_current(gui);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 pos = viewport != NULL ? viewport->WorkPos : ImVec2(0.0f, 0.0f);
    pos.x += 8.0f;
    pos.y += 8.0f;

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.55f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("Datoviz FPS overlay", NULL, flags))
    {
        if (gui->font_mono != NULL)
            ImGui::PushFont(gui->font_mono);
        ImGui::Text("FPS %6.1f", fps);
        ImGui::Text("%6.2f ms", frame_ms);
        if (frames > 0 && elapsed_s > 0)
            ImGui::Text("%u frames / %.2fs", frames, elapsed_s);
        if (gui->font_mono != NULL)
            ImGui::PopFont();
    }
    ImGui::End();
}



/**
 * Render the current ImGui frame into the borrowed canvas command buffer.
 *
 * @param gui the GUI overlay
 * @param frame current canvas frame
 */
void _dvz_gui_render_frame(DvzGui* gui, const DvzStreamFrame* frame)
{
    ANN(gui);
    ANN(frame);
    if (gui->failed || !gui->vulkan_initialized)
        return;
    if (
        frame->command_buffer == VK_NULL_HANDLE || frame->image == VK_NULL_HANDLE ||
        frame->image_view == VK_NULL_HANDLE)
        return;

    _gui_set_current(gui);
    if (_gui_update_followup_frame_state(gui))
        _gui_request_frame(gui);
    ImGui::Render();

    VkImageMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = frame->image_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = frame->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependency = {};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(frame->command_buffer, &dependency);

    VkRenderingAttachmentInfo color = {};
    color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color.imageView = frame->image_view;
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo rendering = {};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea.offset = {0, 0};
    rendering.renderArea.extent = frame->extent;
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &color;

    vkCmdBeginRendering(frame->command_buffer, &rendering);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame->command_buffer, VK_NULL_HANDLE);
    vkCmdEndRendering(frame->command_buffer);
}



/**
 * Return internal GUI viewport resize/display state for focused regression tests.
 *
 * @param viewport GUI viewport
 * @param out output debug state
 * @return whether state was copied
 */
bool _dvz_gui_viewport_debug_state(
    const DvzGuiViewport* viewport, DvzGuiViewportDebugState* out)
{
    if (viewport == NULL || out == NULL)
        return false;
    *out = {};
    out->requested_width = viewport->requested_width;
    out->requested_height = viewport->requested_height;
    out->requested_framebuffer_width = viewport->requested_framebuffer_width;
    out->requested_framebuffer_height = viewport->requested_framebuffer_height;
    out->pending_width = viewport->pending_width;
    out->pending_height = viewport->pending_height;
    out->pending_stable_frames = viewport->pending_stable_frames;
    out->displayed_framebuffer_width = viewport->extent.width;
    out->displayed_framebuffer_height = viewport->extent.height;
    out->displayed_resource_generation = viewport->resource_generation;
    out->stale_frame_count = viewport->stale_frame_count;
    out->has_frame = viewport->has_frame;
    out->image_valid = viewport->image_valid;
    if (viewport->frame_visible)
        out->display_ready = _gui_viewport_frame_matches_request(viewport);
    else
        out->display_ready = _gui_viewport_display_ready(
            viewport, viewport->requested_width, viewport->requested_height);
    out->display_drawable = _gui_viewport_display_drawable(viewport);
    return true;
}



/*************************************************************************************************/
/*  Public functions                                                                             */
/*************************************************************************************************/

/**
 * Return the default GUI overlay configuration.
 *
 * @return default GUI configuration
 */
DvzGuiConfig dvz_gui_config(void)
{
    DvzGuiConfig config = {};
    config.struct_size = DVZ_STRUCT_SIZE(DvzGuiConfig);
    config.flags = 0;
    config.gui_flags = DVZ_GUI_FLAGS_DOCKING | DVZ_GUI_FLAGS_DOCKSPACE;
    config.default_window_width = DVZ_GUI_DEFAULT_WINDOW_WIDTH;
    return config;
}



/**
 * Return the default dockable Datoviz GUI viewport configuration.
 *
 * @return default GUI viewport configuration
 */
DvzGuiViewportConfig dvz_gui_viewport_config(void)
{
    DvzGuiViewportConfig config = {};
    config.struct_size = DVZ_STRUCT_SIZE(DvzGuiViewportConfig);
    config.flags = 0;
    config.viewport_flags = DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT;
    config.initial_width = DVZ_GUI_VIEWPORT_DEFAULT_INITIAL_WIDTH;
    config.initial_height = DVZ_GUI_VIEWPORT_DEFAULT_INITIAL_HEIGHT;
    config.min_width = DVZ_GUI_VIEWPORT_DEFAULT_MIN_WIDTH;
    config.min_height = DVZ_GUI_VIEWPORT_DEFAULT_MIN_HEIGHT;
    config.resize_step = DVZ_GUI_VIEWPORT_DEFAULT_RESIZE_STEP;
    config.resize_delay_frames = DVZ_GUI_VIEWPORT_DEFAULT_RESIZE_DELAY_FRAMES;
    return config;
}



/**
 * Start an ImGui window.
 *
 * @param gui the GUI overlay
 * @param title the window title
 * @param open optional open flag, or NULL
 * @param flags Dear ImGui window flags
 * @return whether the window body is visible
 */
bool dvz_gui_begin(DvzGui* gui, const char* title, bool* open, int flags)
{
    ANN(gui);
    ANN(title);
    _gui_set_current(gui);
    if (gui->config.default_window_width > 0)
    {
        ImGui::SetNextWindowSize(
            ImVec2((float)gui->config.default_window_width, 0.0f), ImGuiCond_FirstUseEver);
    }
    return ImGui::Begin(title, open, flags);
}



DvzResult dvz_gui_dock_window_once(
    DvzGui* gui, const char* title, DvzGuiDockSlot slot, float size_px)
{
    if (gui == NULL || title == NULL || title[0] == '\0')
        return DVZ_ERROR;
    if ((gui->config.gui_flags & DVZ_GUI_FLAGS_DOCKSPACE) == 0)
        return DVZ_ERROR;

    _gui_set_current(gui);
    const ImGuiID hash = ImHashStr(title);
    if (_gui_dock_window_hash_seen(gui, hash))
        return DVZ_OK;

    const ImGuiID dock_id = _gui_dock_node_for_slot(gui, slot, size_px);
    if (dock_id == 0)
        return DVZ_ERROR;

    ImGui::DockBuilderDockWindow(title, dock_id);
    ImGui::SetNextWindowDockID(dock_id, ImGuiCond_FirstUseEver);
    if (!_gui_dock_window_hash_add(gui, hash))
        return DVZ_ERROR;
    return DVZ_OK;
}



bool dvz_gui_current_window_docked(DvzGui* gui)
{
    if (gui == NULL)
        return false;
    _gui_set_current(gui);
    return ImGui::IsWindowDocked();
}



bool dvz_gui_current_window_rect(DvzGui* gui, DvzRect* out)
{
    if (gui == NULL || out == NULL)
        return false;
    _gui_set_current(gui);
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    out->x = pos.x;
    out->y = pos.y;
    out->width = size.x;
    out->height = size.y;
    return true;
}



/**
 * End the current ImGui window.
 *
 * @param gui the GUI overlay
 */
void dvz_gui_end(DvzGui* gui)
{
    ANN(gui);
    _gui_set_current(gui);
    ImGui::End();
}



/**
 * Show an unformatted text item.
 *
 * @param gui the GUI overlay
 * @param text null-terminated text
 */
void dvz_gui_text(DvzGui* gui, const char* text)
{
    ANN(gui);
    ANN(text);
    _gui_set_current(gui);
    ImGui::TextUnformatted(text);
}



/**
 * Push the default monospace ImGui font.
 *
 * @param gui the GUI overlay
 * @return whether the monospace font was available and pushed
 */
bool dvz_gui_push_mono(DvzGui* gui)
{
    ANN(gui);
    _gui_set_current(gui);
    if (gui->font_mono == NULL)
        return false;
    ImGui::PushFont(gui->font_mono);
    return true;
}



/**
 * Pop the current ImGui font.
 *
 * @param gui the GUI overlay
 */
void dvz_gui_pop_font(DvzGui* gui)
{
    ANN(gui);
    _gui_set_current(gui);
    ImGui::PopFont();
}



/**
 * Show a button.
 *
 * @param gui the GUI overlay
 * @param label button label
 * @return whether the button was pressed
 */
bool dvz_gui_button(DvzGui* gui, const char* label)
{
    ANN(gui);
    ANN(label);
    _gui_set_current(gui);
    return ImGui::Button(label);
}



/**
 * Show a checkbox.
 *
 * @param gui the GUI overlay
 * @param label checkbox label
 * @param value value edited in place
 * @return whether the value changed
 */
bool dvz_gui_checkbox(DvzGui* gui, const char* label, bool* value)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    _gui_set_current(gui);
    return ImGui::Checkbox(label, value);
}


/**
 * Show a dropdown combo.
 *
 * @param gui the GUI overlay
 * @param label combo label
 * @param current_item selected item index edited in place
 * @param items item labels
 * @param item_count number of item labels
 * @return whether the selection changed
 */
bool dvz_gui_combo(
    DvzGui* gui, const char* label, int* current_item, const char* const* items, int item_count)
{
    ANN(gui);
    ANN(label);
    ANN(current_item);
    ANN(items);
    _gui_set_current(gui);
    return ImGui::Combo(label, current_item, items, item_count);
}



/**
 * Show a float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value value edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
bool dvz_gui_slider_float(DvzGui* gui, const char* label, float* value, float min, float max)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    _gui_set_current(gui);
    return ImGui::SliderFloat(label, value, min, max);
}



/**
 * Show an integer slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value value edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
bool dvz_gui_slider_int(DvzGui* gui, const char* label, int* value, int min, int max)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    _gui_set_current(gui);
    return ImGui::SliderInt(label, value, min, max);
}



/**
 * Show a two-component float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value two values edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
bool dvz_gui_slider_float2(DvzGui* gui, const char* label, float value[2], float min, float max)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    _gui_set_current(gui);
    return ImGui::SliderFloat2(label, value, min, max);
}



/**
 * Show a three-component float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value three values edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
bool dvz_gui_slider_float3(DvzGui* gui, const char* label, float value[3], float min, float max)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    _gui_set_current(gui);
    return ImGui::SliderFloat3(label, value, min, max);
}



/**
 * Show a four-component float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value four values edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
bool dvz_gui_slider_float4(DvzGui* gui, const char* label, float value[4], float min, float max)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    _gui_set_current(gui);
    return ImGui::SliderFloat4(label, value, min, max);
}



/**
 * Show a float slider with an explicit display format.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value value edited in place
 * @param min minimum value
 * @param max maximum value
 * @param format printf-style value format
 * @return whether the value changed
 */
bool dvz_gui_slider_float_format(
    DvzGui* gui, const char* label, float* value, float min, float max, const char* format)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    ANN(format);
    _gui_set_current(gui);
    return ImGui::SliderFloat(label, value, min, max, format);
}



/**
 * Show a float range slider with two handles.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param current_min minimum value edited in place
 * @param current_max maximum value edited in place
 * @param min lower clamp value
 * @param max upper clamp value
 * @param format optional printf-style value format
 * @return whether either value changed
 */
bool dvz_gui_slider_range_float(
    DvzGui* gui, const char* label, float* current_min, float* current_max, float min,
    float max, const char* format)
{
    ANN(gui);
    ANN(label);
    ANN(current_min);
    ANN(current_max);
    _gui_set_current(gui);
    return ImGui::SliderFloatRange2(
        label, current_min, current_max, min, max, format != NULL ? format : "%.3f");
}



/**
 * Show a float min/max range editor.
 *
 * @param gui the GUI overlay
 * @param label range label
 * @param current_min minimum value edited in place
 * @param current_max maximum value edited in place
 * @param speed drag speed
 * @param min lower clamp value
 * @param max upper clamp value
 * @param format printf-style value format
 * @return whether either value changed
 */
bool dvz_gui_range_float(
    DvzGui* gui, const char* label, float* current_min, float* current_max, float speed,
    float min, float max, const char* format)
{
    ANN(gui);
    ANN(label);
    ANN(current_min);
    ANN(current_max);
    _gui_set_current(gui);
    return ImGui::DragFloatRange2(
        label, current_min, current_max, speed, min, max, format != NULL ? format : "%.3f");
}



/**
 * Show an RGBA color editor using float channels in [0, 1].
 *
 * @param gui the GUI overlay
 * @param label color label
 * @param rgba RGBA channels edited in place
 * @param flags Dear ImGui color edit flags
 * @return whether the value changed
 */
bool dvz_gui_color_edit4(DvzGui* gui, const char* label, float rgba[4], int flags)
{
    ANN(gui);
    ANN(label);
    ANN(rgba);
    _gui_set_current(gui);
    return ImGui::ColorEdit4(label, rgba, flags);
}



/**
 * Show an RGBA color editor using a DvzColor value.
 *
 * @param gui the GUI overlay
 * @param label color label
 * @param color color edited in place
 * @param flags Dear ImGui color edit flags
 * @return whether the value changed
 */
bool dvz_gui_color_edit_dvz(DvzGui* gui, const char* label, DvzColor* color, int flags)
{
    ANN(gui);
    ANN(label);
    ANN(color);
    float rgba[4] = {};
    _gui_color_to_float(*color, rgba);
    bool changed = dvz_gui_color_edit4(gui, label, rgba, flags);
    if (changed)
        _gui_color_from_float(rgba, color);
    return changed;
}



/**
 * Show an RGBA color picker using float channels in [0, 1].
 *
 * @param gui the GUI overlay
 * @param label color label
 * @param rgba RGBA channels edited in place
 * @param flags Dear ImGui color edit flags
 * @return whether the value changed
 */
bool dvz_gui_color_picker4(DvzGui* gui, const char* label, float rgba[4], int flags)
{
    ANN(gui);
    ANN(label);
    ANN(rgba);
    _gui_set_current(gui);
    return ImGui::ColorPicker4(label, rgba, flags);
}



/**
 * Show a labeled separator.
 *
 * @param gui the GUI overlay
 * @param label separator label
 */
void dvz_gui_separator_text(DvzGui* gui, const char* label)
{
    ANN(gui);
    ANN(label);
    _gui_set_current(gui);
    ImGui::SeparatorText(label);
}



/**
 * Show a collapsible section header.
 *
 * @param gui the GUI overlay
 * @param label section label
 * @param flags Dear ImGui tree node flags
 * @return whether the section is open
 */
bool dvz_gui_collapsing_header(DvzGui* gui, const char* label, int flags)
{
    ANN(gui);
    ANN(label);
    _gui_set_current(gui);
    return ImGui::CollapsingHeader(label, flags);
}



/**
 * Place the next item on the same line.
 *
 * @param gui the GUI overlay
 * @param offset_from_start_x x offset from start, or 0
 * @param spacing spacing between items, or -1 for default
 */
void dvz_gui_same_line(DvzGui* gui, float offset_from_start_x, float spacing)
{
    ANN(gui);
    _gui_set_current(gui);
    ImGui::SameLine(offset_from_start_x, spacing);
}



/**
 * Show Dear ImGui's demo window.
 *
 * @param gui the GUI overlay
 * @param open optional open flag, or NULL
 */
void dvz_gui_demo(DvzGui* gui, bool* open)
{
    ANN(gui);
    _gui_set_current(gui);
    ImGui::ShowDemoWindow(open);
}



/**
 * Create a dockable ImGui viewport from an offscreen source view.
 *
 * @param gui the GUI overlay
 * @param source view providing the rendered image
 * @param config optional viewport configuration
 * @param owns_source whether the viewport owns the source view lifecycle policy
 * @return the GUI viewport, or NULL on failure
 */
static DvzGuiViewport* _gui_viewport_from_window(
    DvzGui* gui, DvzView* source, const DvzGuiViewportConfig* config, bool owns_source)
{
    ANN(gui);
    ANN(source);
    DvzCanvas* canvas = dvz_view_canvas(source);
    if (canvas == NULL)
        return NULL;
    if (dvz_canvas_render_mode(canvas) != DVZ_CANVAS_RENDER_MODE_OFFSCREEN)
    {
        log_error("Datoviz GUI viewports require an offscreen source view");
        return NULL;
    }

    DvzGuiViewport* viewport = (DvzGuiViewport*)dvz_calloc(1, sizeof(DvzGuiViewport));
    if (viewport == NULL)
        return NULL;
    viewport->gui = gui;
    viewport->source = source;
    viewport->canvas = canvas;
    viewport->config = _gui_viewport_config_normalize(config);
    viewport->owns_source = owns_source;
    viewport->texture_dirty = true;
    dvz_view_logical_size(source, &viewport->requested_width, &viewport->requested_height);
    dvz_view_framebuffer_size(
        source, &viewport->requested_framebuffer_width, &viewport->requested_framebuffer_height);
    if (!_gui_viewport_create_sampler(viewport))
    {
        dvz_free(viewport);
        return NULL;
    }

    DvzCanvasLiveImageSinkConfig cfg = dvz_canvas_live_image_sink_config();
    cfg.callback = _gui_viewport_live_image_callback;
    cfg.user_data = viewport;
    if (dvz_canvas_configure_live_image_sink(canvas, true, &cfg) != 0)
    {
        _gui_viewport_destroy(viewport, false);
        return NULL;
    }

    _gui_viewport_attach(gui, viewport);
    return viewport;
}



/**
 * Create a dockable ImGui viewport that renders a figure into an owned offscreen window.
 *
 * @param gui the GUI overlay
 * @param figure figure to render inside the GUI viewport
 * @param config optional viewport configuration
 * @return the GUI viewport, or NULL on failure
 */
DvzGuiViewport*
dvz_gui_viewport(DvzGui* gui, DvzFigure* figure, const DvzGuiViewportConfig* config)
{
    ANN(gui);
    ANN(figure);
    if (gui->app == NULL)
        return NULL;
    if (!_gui_viewport_config_validate(config))
        return NULL;

    DvzGuiViewportConfig cfg = _gui_viewport_config_normalize(config);
    float scale = gui->view != NULL ? dvz_view_device_scale(gui->view) : 1.0f;
    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
    desc.size_policy = DVZ_VIEW_SIZE_FRAMEBUFFER_PX;
    desc.size_width = _gui_viewport_scale_dimension(cfg.initial_width, scale);
    desc.size_height = _gui_viewport_scale_dimension(cfg.initial_height, scale);
    desc.size_requested_device_scale = scale;
    desc.device_scale = scale;
    DvzView* source = dvz_view(gui->app, figure, &desc);
    if (source == NULL)
        return NULL;

    DvzGuiViewport* viewport = _gui_viewport_from_window(gui, source, &cfg, true);
    if (viewport == NULL)
    {
        dvz_view_set_render_enabled(source, false);
        return NULL;
    }
    viewport->figure = figure;
    return viewport;
}



/**
 * Create a dockable ImGui viewport from an existing offscreen view.
 *
 * @param gui the GUI overlay
 * @param source view providing the rendered image
 * @param config optional viewport configuration
 * @return the GUI viewport, or NULL on failure
 */
DvzGuiViewport* dvz_gui_viewport_from_window(
    DvzGui* gui, DvzView* source, const DvzGuiViewportConfig* config)
{
    if (!_gui_viewport_config_validate(config))
        return NULL;
    return _gui_viewport_from_window(gui, source, config, false);
}



/**
 * Return the input router used by a GUI viewport's offscreen view.
 *
 * @param viewport the GUI viewport
 * @return the input router, or NULL
 */
DvzInputRouter* dvz_gui_viewport_input(DvzGuiViewport* viewport)
{
    ANN(viewport);
    if (viewport->source == NULL)
        return NULL;
    return dvz_view_input(viewport->source);
}


/**
 * Return the last mouse position over a dockable GUI viewport image.
 *
 * @param viewport the GUI viewport
 * @param out_pos optional output mouse x/y coordinates
 * @param out_size optional output displayed source width/height
 * @param out_hovered optional output hover state
 * @return whether viewport mouse state was available
 */
bool dvz_gui_viewport_mouse(
    DvzGuiViewport* viewport, float out_pos[2], float out_size[2], bool* out_hovered)
{
    ANN(viewport);
    if (out_pos != NULL)
    {
        out_pos[0] = viewport->mouse_pos[0];
        out_pos[1] = viewport->mouse_pos[1];
    }
    if (out_size != NULL)
    {
        out_size[0] = viewport->mouse_size[0];
        out_size[1] = viewport->mouse_size[1];
    }
    if (out_hovered != NULL)
        *out_hovered = viewport->mouse_hovered;
    return viewport->mouse_valid;
}



/**
 * Destroy a dockable ImGui viewport.
 *
 * @param viewport the GUI viewport
 */
void dvz_gui_viewport_destroy(DvzGuiViewport* viewport)
{
    _gui_viewport_destroy(viewport, true);
}



/**
 * Show a dockable ImGui window containing a Datoviz-rendered viewport image.
 *
 * @param viewport the GUI viewport
 * @param title the ImGui window title
 * @param open optional open flag, or NULL
 * @param flags Dear ImGui window flags
 * @return whether the Datoviz image was visible this frame
 */
bool dvz_gui_viewport_window(DvzGuiViewport* viewport, const char* title, bool* open, int flags)
{
    ANN(viewport);
    ANN(title);
    DvzGui* gui = viewport->gui;
    ANN(gui);
    _gui_set_current(gui);

    bool shown = false;
    bool window_visible = false;
    if (ImGui::Begin(title, open, flags))
    {
        window_visible = true;
        ImVec2 avail = ImGui::GetContentRegionAvail();
        /* Repair stale saved layouts that collapsed the hosted source below its minimum size. */
        if (
            ImGui::IsWindowAppearing() &&
            (avail.x < (float)viewport->config.min_width ||
             avail.y < (float)viewport->config.min_height))
        {
            ImGui::SetWindowSize(
                ImVec2(
                    (float)viewport->config.initial_width,
                    (float)viewport->config.initial_height),
                ImGuiCond_Always);
            avail = ImGui::GetContentRegionAvail();
        }
        if (avail.x < 1.0f)
            avail.x = 1.0f;
        if (avail.y < 1.0f)
            avail.y = 1.0f;

        uint32_t width = _gui_viewport_dimension(
            avail.x, viewport->config.min_width, viewport->config.resize_step);
        uint32_t height = _gui_viewport_dimension(
            avail.y, viewport->config.min_height, viewport->config.resize_step);
        ImVec2 image_min = ImGui::GetCursorScreenPos();
        ImVec2 image_max = ImVec2(image_min.x + avail.x, image_min.y + avail.y);
        ImGui::PushID(viewport);
        ImGui::InvisibleButton(
            "image", avail,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                ImGuiButtonFlags_MouseButtonMiddle);
        ImGui::PopID();
        _gui_viewport_update_mouse(viewport, image_min, avail);
        if ((viewport->config.viewport_flags & DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT) != 0)
            _gui_viewport_forward_input(viewport, image_min, avail);

        float scale = _gui_viewport_device_scale(viewport);
        viewport->frame_visible = width > 0 && height > 0;
        viewport->frame_request_width = width;
        viewport->frame_request_height = height;
        viewport->frame_request_framebuffer_width = _gui_viewport_scale_dimension(width, scale);
        viewport->frame_request_framebuffer_height = _gui_viewport_scale_dimension(height, scale);
        viewport->frame_draw_list = ImGui::GetWindowDrawList();
        viewport->frame_image_min = image_min;
        viewport->frame_image_max = image_max;
        if (viewport->frame_visible)
        {
            _gui_request_frame(gui);
            shown = true;
        }
    }
    ImGui::End();
    _gui_viewport_set_visible(viewport, window_visible);
    return shown;
}
