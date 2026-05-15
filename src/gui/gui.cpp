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

#include <stdbool.h>
#include <stdint.h>

#include <volk.h>

#include "_assertions.h"
#include "_log.h"
#include "datoviz/canvas.h"
#include "datoviz/input/pointer.h"
#include "datoviz/window/backend.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "_alloc.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "gui_fonts.inc"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GUI_DEFAULT_FONT_SIZE 16.0f
#define DVZ_GUI_DEFAULT_MONO_FONT_SIZE 16.0f



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
    DvzGpuCtx* gpu_ctx;
    DvzDevice* device;
    DvzQueue* queue;
    DvzWindow* window;
    GLFWwindow* glfw_window;
    ImGuiContext* context;
    DvzGuiConfig config;
    DvzGuiCallback callback;
    void* callback_user_data;
    DvzGuiPanel* panels;
    ImFont* font_regular;
    ImFont* font_mono;
    VkFormat color_format;
    bool glfw_initialized;
    bool vulkan_initialized;
    bool failed;
};


struct DvzGuiPanel
{
    DvzGui* gui;
    DvzAppWindow* source;
    DvzCanvas* canvas;
    VkSampler sampler;
    VkDescriptorSet texture;
    VkImage image;
    VkImageView image_view;
    VkExtent2D extent;
    uint32_t requested_width;
    uint32_t requested_height;
    bool has_frame;
    bool texture_dirty;
    DvzGuiPanel* next;
};



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



/**
 * Load Datoviz's embedded ImGui fonts into the current font atlas.
 *
 * @param gui the GUI overlay
 */
static void _gui_load_fonts(DvzGui* gui)
{
    ANN(gui);
    _gui_set_current(gui);

    ImGuiIO& io = ImGui::GetIO();
    const float font_size = _gui_font_size(gui->config.font_size, DVZ_GUI_DEFAULT_FONT_SIZE);
    const float mono_font_size =
        _gui_font_size(gui->config.mono_font_size, DVZ_GUI_DEFAULT_MONO_FONT_SIZE);

    ImFontConfig regular_config = {};
    regular_config.OversampleH = 2;
    regular_config.OversampleV = 1;
    gui->font_regular = io.Fonts->AddFontFromMemoryCompressedTTF(
        dvz_gui_font_karla_regular_compressed_data,
        (int)dvz_gui_font_karla_regular_compressed_size, font_size, &regular_config,
        io.Fonts->GetGlyphRangesDefault());
    if (gui->font_regular == NULL)
    {
        log_error("Dear ImGui failed to load the embedded Karla font");
        gui->font_regular = io.Fonts->AddFontDefault();
    }

    ImFontConfig mono_config = {};
    mono_config.OversampleH = 2;
    mono_config.OversampleV = 1;
    gui->font_mono = io.Fonts->AddFontFromMemoryCompressedTTF(
        dvz_gui_font_cousine_regular_compressed_data,
        (int)dvz_gui_font_cousine_regular_compressed_size, mono_font_size, &mono_config,
        io.Fonts->GetGlyphRangesDefault());
    if (gui->font_mono == NULL)
        log_error("Dear ImGui failed to load the embedded Cousine font");

    if (gui->font_regular != NULL)
        io.FontDefault = gui->font_regular;
}



/**
 * Attach a GUI panel to the overlay-owned list.
 *
 * @param gui the GUI overlay
 * @param panel the panel to attach
 */
static void _gui_panel_attach(DvzGui* gui, DvzGuiPanel* panel)
{
    ANN(gui);
    ANN(panel);
    panel->next = gui->panels;
    gui->panels = panel;
}



/**
 * Detach a GUI panel from the overlay-owned list.
 *
 * @param gui the GUI overlay
 * @param panel the panel to detach
 */
static void _gui_panel_detach(DvzGui* gui, DvzGuiPanel* panel)
{
    ANN(gui);
    ANN(panel);
    DvzGuiPanel* prev = NULL;
    DvzGuiPanel* cur = gui->panels;
    while (cur != NULL)
    {
        if (cur == panel)
        {
            if (prev != NULL)
                prev->next = cur->next;
            else
                gui->panels = cur->next;
            panel->next = NULL;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}



/**
 * Create the sampler used when displaying Datoviz images through ImGui.
 *
 * @param panel GUI panel receiving the sampler
 * @return whether the sampler was created
 */
static bool _gui_panel_create_sampler(DvzGuiPanel* panel)
{
    ANN(panel);
    ANN(panel->gui);
    VkDevice device = dvz_device_handle(panel->gui->device);
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
    VkResult res = vkCreateSampler(device, &info, NULL, &panel->sampler);
    if (res != VK_SUCCESS)
    {
        log_error("Dear ImGui Datoviz panel sampler creation failed: %d", (int)res);
        panel->sampler = VK_NULL_HANDLE;
        return false;
    }
    return true;
}



/**
 * Remove the ImGui descriptor for a panel image.
 *
 * @param panel GUI panel
 */
static void _gui_panel_remove_texture(DvzGuiPanel* panel)
{
    ANN(panel);
    if (panel->texture == VK_NULL_HANDLE)
        return;
    if (panel->gui != NULL && panel->gui->vulkan_initialized)
    {
        _gui_set_current(panel->gui);
        ImGui_ImplVulkan_RemoveTexture(panel->texture);
    }
    panel->texture = VK_NULL_HANDLE;
}



/**
 * Ensure the ImGui descriptor points at the current Datoviz source image view.
 *
 * @param panel GUI panel
 * @return whether a texture descriptor is ready
 */
static bool _gui_panel_ensure_texture(DvzGuiPanel* panel)
{
    ANN(panel);
    DvzGui* gui = panel->gui;
    ANN(gui);
    if (!gui->vulkan_initialized || panel->image_view == VK_NULL_HANDLE ||
        panel->sampler == VK_NULL_HANDLE)
    {
        return false;
    }
    if (!panel->texture_dirty && panel->texture != VK_NULL_HANDLE)
        return true;

    _gui_panel_remove_texture(panel);
    _gui_set_current(gui);
    panel->texture = ImGui_ImplVulkan_AddTexture(
        panel->sampler, panel->image_view, VK_IMAGE_LAYOUT_GENERAL);
    panel->texture_dirty = panel->texture == VK_NULL_HANDLE;
    return panel->texture != VK_NULL_HANDLE;
}



/**
 * Receive a live source-canvas image after submission.
 *
 * @param frame live image metadata
 * @param user_data GUI panel
 * @return 0 on success
 */
static int _gui_panel_live_image_callback(
    const DvzCanvasLiveImageFrame* frame, void* user_data)
{
    ANN(frame);
    DvzGuiPanel* panel = (DvzGuiPanel*)user_data;
    ANN(panel);
    if (frame->image_view == VK_NULL_HANDLE || frame->extent.width == 0 ||
        frame->extent.height == 0)
    {
        return 0;
    }
    if (panel->image_view != frame->image_view)
    {
        panel->texture_dirty = true;
        panel->image_view = frame->image_view;
        panel->image = frame->image;
    }
    panel->extent = frame->extent;
    panel->has_frame = true;
    return 0;
}



/**
 * Rebuild the source live-image stream after a panel resize.
 *
 * @param panel GUI panel
 * @param width new source width
 * @param height new source height
 */
static void _gui_panel_resize_source(DvzGuiPanel* panel, uint32_t width, uint32_t height)
{
    ANN(panel);
    if (panel->requested_width == width && panel->requested_height == height)
        return;
    if (dvz_app_window_resize(panel->source, width, height) != 0)
        return;
    panel->requested_width = width;
    panel->requested_height = height;
    if (panel->canvas != NULL)
    {
        DvzCanvasLiveImageSinkConfig cfg = {};
        cfg.callback = _gui_panel_live_image_callback;
        cfg.user_data = panel;
        (void)dvz_canvas_configure_live_image_sink(panel->canvas, false, NULL);
        if (dvz_canvas_configure_live_image_sink(panel->canvas, true, &cfg) != 0)
            log_error("failed to rebuild Datoviz GUI panel live-image sink after resize");
    }
}



/**
 * Forward ImGui item input to the source app-window router.
 *
 * @param panel GUI panel
 * @param image_min top-left image position in ImGui coordinates
 * @param size displayed image size
 */
static void _gui_panel_forward_input(
    DvzGuiPanel* panel, ImVec2 image_min, ImVec2 size)
{
    ANN(panel);
    DvzInputRouter* router = dvz_app_window_input(panel->source);
    if (router == NULL || size.x <= 0 || size.y <= 0)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    if (!hovered && !active)
        return;

    float x = io.MousePos.x - image_min.x;
    float y = io.MousePos.y - image_min.y;
    const uint64_t now = dvz_input_timestamp_ns();
    const float window_x = size.x;
    const float window_y = size.y;
    const int mods = 0;

    if (hovered)
    {
        dvz_pointer_emit_position(
            router, DVZ_POINTER_EVENT_MOVE, x, y, window_x, window_y,
            DVZ_POINTER_BUTTON_NONE, mods, 1.0f, now, NULL);
        if (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f)
        {
            dvz_pointer_emit_wheel(
                router, x, y, window_x, window_y, io.MouseWheelH, io.MouseWheel, mods, 1.0f,
                now, NULL);
        }
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
            dvz_pointer_emit_position(
                router, DVZ_POINTER_EVENT_PRESS, x, y, window_x, window_y, buttons[i], mods,
                1.0f, now, NULL);
        }
        if (ImGui::IsMouseReleased(i))
        {
            dvz_pointer_emit_position(
                router, DVZ_POINTER_EVENT_RELEASE, x, y, window_x, window_y, buttons[i], mods,
                1.0f, now, NULL);
        }
    }
}



/**
 * Destroy a GUI panel, optionally unlinking it from the owning overlay.
 *
 * @param panel GUI panel
 * @param detach whether to detach from the overlay-owned list
 */
static void _gui_panel_destroy(DvzGuiPanel* panel, bool detach)
{
    if (panel == NULL)
        return;
    DvzGui* gui = panel->gui;
    if (gui != NULL)
    {
        _gui_set_current(gui);
        if (detach)
            _gui_panel_detach(gui, panel);
    }
    if (panel->canvas != NULL)
        (void)dvz_canvas_configure_live_image_sink(panel->canvas, false, NULL);
    _gui_panel_remove_texture(panel);
    if (gui != NULL && panel->sampler != VK_NULL_HANDLE)
    {
        VkDevice device = dvz_device_handle(gui->device);
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);
            vkDestroySampler(device, panel->sampler, NULL);
        }
    }
    dvz_free(panel);
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
    return ImGui::GetIO().WantCaptureKeyboard;
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
    _gui_set_current(gui);
    ImGui_ImplGlfw_KeyCallback(gui->glfw_window, key, scancode, action, mods);
    return _gui_want_capture_keyboard(gui);
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
    return true;
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
    if ((gui->config.flags & DVZ_GUI_FLAGS_DOCKSPACE) == 0)
        return;
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), flags);
}



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

/**
 * Create an ImGui overlay.
 *
 * @param gpu_ctx GPU context borrowed from the app
 * @param window GLFW window borrowed from the app window
 * @param config optional GUI configuration
 * @return created GUI overlay, or NULL
 */
DvzGui* _dvz_gui_create(DvzGpuCtx* gpu_ctx, DvzWindow* window, const DvzGuiConfig* config)
{
    ANN(gpu_ctx);
    ANN(window);

    GLFWwindow* glfw_window = (GLFWwindow*)dvz_window_backend_handle(window);
    if (glfw_window == NULL)
    {
        log_error("Dear ImGui overlay requires a GLFW window");
        return NULL;
    }

    DvzGui* gui = (DvzGui*)dvz_calloc(1, sizeof(DvzGui));
    if (gui == NULL)
        return NULL;

    gui->gpu_ctx = gpu_ctx;
    gui->device = dvz_gpu_ctx_device(gpu_ctx);
    gui->queue = dvz_gpu_ctx_queue(gpu_ctx, DVZ_QUEUE_MAIN);
    gui->window = window;
    gui->glfw_window = glfw_window;
    gui->config = config != NULL ? *config : dvz_gui_config();
    gui->context = ImGui::CreateContext(NULL);
    if (gui->context == NULL)
    {
        dvz_free(gui);
        return NULL;
    }

    _gui_set_current(gui);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (gui->config.ini_path != NULL)
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
    while (gui->panels != NULL)
        _gui_panel_destroy(gui->panels, true);
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
 * @param win app window passed to user callbacks
 * @param frame current canvas frame
 */
void _dvz_gui_begin_frame(DvzGui* gui, DvzAppWindow* win, const DvzStreamFrame* frame)
{
    ANN(gui);
    ANN(frame);
    if (!_gui_ensure_vulkan(gui, frame))
        return;
    _gui_set_current(gui);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    _gui_submit_dockspace(gui);
    if (gui->callback != NULL)
        gui->callback(gui, win, gui->callback_user_data);
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
    if (frame->command_buffer == VK_NULL_HANDLE || frame->image_view == VK_NULL_HANDLE)
        return;

    _gui_set_current(gui);
    ImGui::Render();

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
    config.flags = DVZ_GUI_FLAGS_DOCKING | DVZ_GUI_FLAGS_DOCKSPACE;
    config.font_size = DVZ_GUI_DEFAULT_FONT_SIZE;
    config.mono_font_size = DVZ_GUI_DEFAULT_MONO_FONT_SIZE;
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
    return ImGui::Begin(title, open, flags);
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
 * Create a dockable ImGui panel that displays an app window's latest rendered image.
 *
 * @param gui the GUI overlay
 * @param source app window providing the rendered image
 * @return the GUI panel, or NULL on failure
 */
DvzGuiPanel* dvz_gui_panel(DvzGui* gui, DvzAppWindow* source)
{
    ANN(gui);
    ANN(source);
    DvzCanvas* canvas = dvz_app_window_canvas(source);
    if (canvas == NULL)
        return NULL;
    if (dvz_canvas_render_mode(canvas) != DVZ_CANVAS_RENDER_MODE_OFFSCREEN)
    {
        log_error("Datoviz GUI panels require an offscreen source app-window");
        return NULL;
    }

    DvzGuiPanel* panel = (DvzGuiPanel*)dvz_calloc(1, sizeof(DvzGuiPanel));
    if (panel == NULL)
        return NULL;
    panel->gui = gui;
    panel->source = source;
    panel->canvas = canvas;
    panel->texture_dirty = true;
    if (!_gui_panel_create_sampler(panel))
    {
        dvz_free(panel);
        return NULL;
    }

    DvzCanvasLiveImageSinkConfig cfg = {};
    cfg.callback = _gui_panel_live_image_callback;
    cfg.user_data = panel;
    if (dvz_canvas_configure_live_image_sink(canvas, true, &cfg) != 0)
    {
        _gui_panel_destroy(panel, false);
        return NULL;
    }

    _gui_panel_attach(gui, panel);
    return panel;
}



/**
 * Destroy a dockable ImGui panel.
 *
 * @param panel the GUI panel
 */
void dvz_gui_panel_destroy(DvzGuiPanel* panel)
{
    _gui_panel_destroy(panel, true);
}



/**
 * Show a dockable ImGui window containing a Datoviz-rendered panel image.
 *
 * @param panel the GUI panel
 * @param title the ImGui window title
 * @param open optional open flag, or NULL
 * @param flags Dear ImGui window flags
 * @return whether the Datoviz image was visible this frame
 */
bool dvz_gui_panel_window(DvzGuiPanel* panel, const char* title, bool* open, int flags)
{
    ANN(panel);
    ANN(title);
    DvzGui* gui = panel->gui;
    ANN(gui);
    _gui_set_current(gui);

    ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
    bool shown = false;
    if (ImGui::Begin(title, open, flags))
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x < 1.0f)
            avail.x = 1.0f;
        if (avail.y < 1.0f)
            avail.y = 1.0f;

        uint32_t width = (uint32_t)(avail.x + 0.5f);
        uint32_t height = (uint32_t)(avail.y + 0.5f);
        if (width > 0 && height > 0)
            _gui_panel_resize_source(panel, width, height);

        if (_gui_panel_ensure_texture(panel))
        {
            ImVec2 image_min = ImGui::GetCursorScreenPos();
            ImVec2 image_max = ImVec2(image_min.x + avail.x, image_min.y + avail.y);
            ImGui::PushID(panel);
            ImGui::InvisibleButton(
                "image", avail,
                ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                    ImGuiButtonFlags_MouseButtonMiddle);
            ImGui::PopID();
            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)panel->texture, image_min, image_max, ImVec2(0, 0), ImVec2(1, 1));
            _gui_panel_forward_input(panel, image_min, avail);
            shown = true;
        }
        else
        {
            ImGui::Dummy(avail);
        }
    }
    ImGui::End();
    return shown;
}
