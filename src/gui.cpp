/*************************************************************************************************/
/*  Dear ImGUI wrapper                                                                           */
/*************************************************************************************************/

#include "gui.h"
#include "host.h"
#include "vklite.h"
#include "window.h"

// ImGUI includes
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils functions                                                                              */
/*************************************************************************************************/

static void _imgui_check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    log_error("VkResult %d\n", err);
    if (err < 0)
        abort();
}

static inline bool _check_imgui_context()
{
    if (ImGui::GetCurrentContext() == NULL)
    {
        log_error("no more ImGui context, skipping Dear ImGui callback");
        return false;
    }
    return true;
}



static void _imgui_init(
    DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t queue_idx, uint32_t width, uint32_t height)
{
    ImGui::DebugCheckVersionAndDataLayout(
        IMGUI_VERSION, sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2), sizeof(ImVec4),
        sizeof(ImDrawVert), sizeof(ImDrawIdx));
    ImGui::CreateContext(NULL);
    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename = NULL;

    INIT(ImGui_ImplVulkan_InitInfo, init_info)
    init_info.Instance = gpu->host->instance;
    init_info.PhysicalDevice = gpu->physical_device;
    init_info.Device = gpu->device;
    // should be the render queue idx
    init_info.QueueFamily = gpu->queues.queue_families[queue_idx];
    init_info.Queue = gpu->queues.queues[queue_idx];
    init_info.DescriptorPool = gpu->dset_pool;
    // init_info.PipelineCache = gpu->pipeline_cache;
    // init_info.Allocator = gpu->allocator;

    // TODO
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;

    init_info.CheckVkResultFn = _imgui_check_vk_result;

    ImGui_ImplVulkan_Init(&init_info, renderpass->renderpass);

    io.DisplaySize.x = width;
    io.DisplaySize.y = height;
}



static void _imgui_setup()
{
    // ImGuiIO* io = ImGui::GetIO();
    // int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
    //             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav
    //             | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
    //             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
    //             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    // ImGui::SetNextWindowBgAlpha(0.5f);

    // float distance = 0;
    // int corner = 0;
    // ASSERT(corner >= 0);
    // ImVec2 window_pos = (ImVec2){
    //     (corner & 1) ? io.DisplaySize.x - distance : distance,
    //     (corner & 2) ? io.DisplaySize.y - distance : distance};
    // ImVec2 window_pos_pivot = (ImVec2){(corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f};
    // ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
}



static void _imgui_fonts_upload(DvzGpu* gpu)
{
    ASSERT(ImGui::GetCurrentContext() != NULL);

    // Load Fonts.
    // Load first font.
    {
        // float font_size = 14.0f;
        // ASSERT(font_size > 0);
        // ImFontConfig config = {0};
        // config.FontDataOwnedByAtlas = false; // Important!
        // ImGuiIO* io = ImGui::GetIO();
        // unsigned long file_size = 0;
        // unsigned char* buffer = dvz_resource_font("Roboto_Medium", &file_size);
        // ASSERT(file_size > 0);
        // ASSERT(buffer != NULL);
        // ImFontAtlas_AddFontDefault(io.Fonts, NULL);
        // font = ImFontAtlas_AddFontFromMemoryTTF(
        //     io.Fonts, buffer, file_size, font_size, &config, NULL);
        // ASSERT(font != NULL);
        // ASSERT(ImFont_IsLoaded(font));
    }


    DvzCommands cmd = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmd, 0);
    ImGui_ImplVulkan_CreateFontsTexture(cmd.cmds[0]);
    dvz_cmd_end(&cmd, 0);
    dvz_cmd_submit_sync(&cmd, 0);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
    dvz_commands_destroy(&cmd);
}



static void _imgui_set_window(DvzWindow* window)
{
    ASSERT(window != NULL);

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
        if (window->backend_window)
            ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->backend_window, true);
        break;
    default:
        break;
    }
}



static void _imgui_destroy(bool use_glfw)
{
    ImGui_ImplVulkan_Shutdown();
    if (use_glfw)
        ImGui_ImplGlfw_Shutdown();
    ASSERT(ImGui::GetCurrentContext() != NULL);
    ImGui::DestroyContext(ImGui::GetCurrentContext());
    ASSERT(ImGui::GetCurrentContext() == NULL);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzGui dvz_gui(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzWindow* window, //
    uint32_t queue_idx, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(renderpass != NULL);
    _imgui_init(gpu, renderpass, queue_idx, width, height);
    _imgui_setup();
    _imgui_fonts_upload(gpu);
    if (window)
        _imgui_set_window(window);

    INIT(DvzGui, gui)
    gui.gpu = gpu;
    // gui.io = ImGui::GetIO();
    gui.use_glfw = window != NULL && window->backend_window != NULL;
    return gui;
}



void dvz_gui_frame_begin(DvzGui* gui)
{
    ASSERT(gui != NULL);

    ImGui_ImplVulkan_NewFrame();
    if (gui->use_glfw)
        ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}



void dvz_gui_dialog_begin(DvzGui* gui, vec2 pos, vec2 size)
{
    ASSERT(gui != NULL);

    // const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(pos[0], pos[1]), ImGuiCond_FirstUseEver, ImVec2(0, 0));
    // (ImVec2){main_viewport->WorkPos.x, main_viewport->WorkPos.y},
    ImGui::SetNextWindowSize(ImVec2(size[0], size[1]), ImGuiCond_FirstUseEver);

    bool open = true;
    // ImGui::PushFont(font);
    ImGui::Begin("Hello", &open, ImGuiWindowFlags_NoSavedSettings);
}


void dvz_gui_text(DvzGui* gui, const char* str)
{
    ASSERT(gui != NULL);
    ImGui::Text(str);
}



void dvz_gui_dialog_end(DvzGui* gui)
{
    ASSERT(gui != NULL);
    ImGui::End();
}



void dvz_gui_demo(DvzGui* gui)
{
    ASSERT(gui != NULL);
    bool open = true;
    ImGui::ShowDemoWindow(&open);
}



void dvz_gui_frame_end(DvzGui* gui, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(gui != NULL);
    ASSERT(cmds != NULL);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmds->cmds[idx], VK_NULL_HANDLE);
}



void dvz_gui_destroy(DvzGui* gui)
{
    ASSERT(gui != NULL);
    _imgui_destroy(gui->use_glfw);
}
