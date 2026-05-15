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
#include "datoviz/window/backend.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "_alloc.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"



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
    VkFormat color_format;
    bool glfw_initialized;
    bool vulkan_initialized;
    bool failed;
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
