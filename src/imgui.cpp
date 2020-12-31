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

static void _imgui_init(VklCanvas* canvas)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable docking, requires the docking branch of imgui
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    ImGui::StyleColorsDark();

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
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}



void vkl_imgui_init(VklCanvas* canvas)
{
    _imgui_init(canvas);
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

void vkl_imgui_destroy() { _imgui_destroy(); }
