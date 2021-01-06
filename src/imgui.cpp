#include <inttypes.h>

#include "../include/visky/canvas.h"
#include "imgui.h"

BEGIN_INCL_NO_WARN
#include "../external/imgui/backends/imgui_impl_glfw.h"
#include "../external/imgui/backends/imgui_impl_vulkan.h"
#include "../external/imgui/imgui.h"
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _imgui_check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    log_error("VkResult %d\n", err);
    if (err < 0)
        abort();
}

static void _imgui_dpi(float dpi_factor)
{
    ImGuiStyle style = ImGui::GetStyle();
    style.ScaleAllSizes(dpi_factor);
}

static void _imgui_init_context()
{
    log_debug("initializing Dear ImGui");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable docking, requires the docking branch of imgui
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    ImGui::StyleColorsDark();
}

static void _imgui_enable(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklApp* app = gpu->app;
    ASSERT(app != NULL);

    if (canvas->app->backend == VKL_BACKEND_GLFW)
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)canvas->window->backend_window, true);

    ImGui_ImplVulkan_InitInfo init_info = {0};
    init_info.Instance = app->instance;
    init_info.PhysicalDevice = gpu->physical_device;
    init_info.Device = gpu->device;
    init_info.QueueFamily = gpu->queues.queue_families[VKL_DEFAULT_QUEUE_RENDER];
    init_info.Queue = gpu->queues.queues[VKL_DEFAULT_QUEUE_RENDER];
    init_info.DescriptorPool = gpu->dset_pool;
    // init_info.PipelineCache = gpu->pipeline_cache;
    // init_info.Allocator = gpu->allocator;
    init_info.MinImageCount = canvas->swapchain.img_count;
    init_info.ImageCount = canvas->swapchain.img_count;
    init_info.CheckVkResultFn = _imgui_check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, canvas->renderpass.renderpass);
}

static void _imgui_destroy()
{
    log_debug("shutting down Dear ImGui");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

static void _presend(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    if (!canvas->overlay)
        return;
    VklCommands* cmds = (VklCommands*)ev.user_data;
    ASSERT(cmds != NULL);
    uint32_t idx = canvas->swapchain.img_idx;

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(
        cmds, idx, &canvas->renderpass_overlay, &canvas->framebuffers_overlay);

    // Begin new frame.
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // Call the IMGUI private callbacks to render the GUI.
    {
        VklEvent ev_imgui;
        ev_imgui.type = VKL_EVENT_IMGUI;
        ev_imgui.u.f.idx = canvas->frame_idx;
        ev_imgui.u.f.interval = canvas->clock.interval;
        ev_imgui.u.f.time = canvas->clock.elapsed;
        _event_produce(canvas, ev_imgui);
    }

    // End frame.
    {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmds->cmds[idx]);
    }

    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);

    ASSERT(canvas != NULL);
    vkl_submit_commands(&canvas->submit, cmds);
}



/*************************************************************************************************/
/*  Dear ImGui functions                                                                         */
/*************************************************************************************************/

void vkl_imgui_init(VklCanvas* canvas)
{
    if (ImGui::GetCurrentContext() == NULL)
        _imgui_init_context();
    ASSERT(canvas->overlay);

    _imgui_enable(canvas);
    ImGuiIO& io = ImGui::GetIO();

    // TODO: dpi scaling factor
    _imgui_dpi(1);

    // Load Fonts
    float font_size = 16;
    char path[1024];
    snprintf(path, sizeof(path), "%s/fonts/Roboto-Medium.ttf", DATA_DIR);
    io.Fonts->AddFontFromFileTTF(path, font_size);

    // Upload Fonts
    VklCommands cmd = vkl_commands(canvas->gpu, VKL_DEFAULT_QUEUE_RENDER, 1);
    vkl_cmd_begin(&cmd, 0);
    ImGui_ImplVulkan_CreateFontsTexture(cmd.cmds[0]);
    vkl_cmd_end(&cmd, 0);
    vkl_cmd_submit_sync(&cmd, 0);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
    vkl_commands_destroy(&cmd);

    // PRE_SEND callback that will call the IMGUI callbacks.
    VklCommands* cmds =
        vkl_canvas_commands(canvas, VKL_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);
    vkl_event_callback(canvas, VKL_EVENT_PRE_SEND, 0, VKL_EVENT_MODE_SYNC, _presend, cmds);
}

void vkl_imgui_begin(const char* title, VklGuiStyle style)
{
    ASSERT(title != NULL);
    ASSERT(strlen(title) > 0);

    ImGuiIO& io = ImGui::GetIO();
    int flags = 0;

    switch (style)
    {

    case VKL_GUI_STANDARD:
        flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
        break;

    case VKL_GUI_PROMPT:
    {
        flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        ImGui::SetNextWindowBgAlpha(0.25f);

        ImVec2 window_pos = ImVec2(0, io.DisplaySize.y);
        ImVec2 window_pos_pivot = ImVec2(0, 1);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

        // ImVec2 size = ImVec2(io.DisplaySize.x, 30);
        // ImGui::SetNextWindowSize(size);

        break;
    }

    case VKL_GUI_FIXED_TL:
    case VKL_GUI_FIXED_TR:
    case VKL_GUI_FIXED_LL:
    case VKL_GUI_FIXED_LR:
    {
        flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        ImGui::SetNextWindowBgAlpha(0.5f);

        float distance = 0;
        int corner = (int32_t)style - 10; // 0 = TL, 1 = TR, 2 = LL, 3 = LR
        ASSERT(corner >= 0);
        ImVec2 window_pos = ImVec2(
            (corner & 1) ? io.DisplaySize.x - distance : distance,
            (corner & 2) ? io.DisplaySize.y - distance : distance);
        ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        break;
    }
    default:
        log_error("unknown GUI style");
        break;
    }

    ImGui::Begin(title, NULL, flags);
}

void vkl_imgui_end() { ImGui::End(); }

void vkl_imgui_callback_fps(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    vkl_imgui_begin("FPS", VKL_GUI_FIXED_TR);
    ImGui::Text("FPS: %.1f", canvas->fps);
    vkl_imgui_end();
}

void vkl_imgui_destroy()
{
    if (ImGui::GetCurrentContext() == NULL)
        return;
    _imgui_destroy();
}
