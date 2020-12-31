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

/*
static void _presend(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    if (!canvas->overlay)
        return;
    VklCommands* cmds = ev.user_data;
    ASSERT(cmds != NULL);
    uint32_t idx = canvas->swapchain.img_idx;
    // log_debug("canvas frame %d, swapchain idx %d", canvas->frame_idx, idx);

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(
        cmds, idx, &canvas->renderpass_overlay, &canvas->framebuffers_overlay);
    vkl_imgui_frame(canvas, cmds, idx);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);

    ASSERT(canvas != NULL);
    vkl_submit_commands(&canvas->submit, ev.user_data);
}


    // ImGUI.
    VklCommands* cmds = vkl_canvas_commands(canvas, VKL_DEFAULT_QUEUE_RENDER, 0, 0);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_PRE_SEND, 0, _presend, cmds);
*/


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
}

void vkl_imgui_frame(VklCanvas* canvas, VklCommands* cmds, uint32_t cmd_idx)
{
    if (!canvas->overlay)
        return;
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Enable docking in main window.
    // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
    // ImGuiDockNodeFlags_PassthruCentralNode);

    // ImGuiIO& io = ImGui::GetIO();
    int flags = 0;

    ImGui::Begin("Hello", NULL, flags);
    // TODO: GUI code
    ImGui::End();
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmds->cmds[cmd_idx]);
}

void vkl_imgui_destroy()
{
    if (ImGui::GetCurrentContext() == NULL)
        return;
    _imgui_destroy();
}
