#include <inttypes.h>

#include "../include/datoviz/canvas.h"
#include "../include/datoviz/imgui.h"

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

static void _imgui_enable(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    DvzApp* app = gpu->app;
    ASSERT(app != NULL);

    if (canvas->app->backend == DVZ_BACKEND_GLFW)
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)canvas->window->backend_window, true);

    ImGui_ImplVulkan_InitInfo init_info = {0};
    init_info.Instance = app->instance;
    init_info.PhysicalDevice = gpu->physical_device;
    init_info.Device = gpu->device;
    init_info.QueueFamily = gpu->queues.queue_families[DVZ_DEFAULT_QUEUE_RENDER];
    init_info.Queue = gpu->queues.queues[DVZ_DEFAULT_QUEUE_RENDER];
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

static void _presend(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    if (!canvas->overlay)
        return;
    DvzCommands* cmds = (DvzCommands*)ev.user_data;
    ASSERT(cmds != NULL);
    uint32_t idx = canvas->swapchain.img_idx;

    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(
        cmds, idx, &canvas->renderpass_overlay, &canvas->framebuffers_overlay);

    // Begin new frame.
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // Call the IMGUI private callbacks to render the GUI.
    {
        DvzEvent ev_imgui;
        ev_imgui.type = DVZ_EVENT_IMGUI;
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

    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);

    ASSERT(canvas != NULL);
    dvz_submit_commands(&canvas->submit, cmds);
}



/*************************************************************************************************/
/*  Dear ImGui functions                                                                         */
/*************************************************************************************************/

void dvz_imgui_init(DvzCanvas* canvas)
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
    DvzCommands cmd = dvz_commands(canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_cmd_begin(&cmd, 0);
    ImGui_ImplVulkan_CreateFontsTexture(cmd.cmds[0]);
    dvz_cmd_end(&cmd, 0);
    dvz_cmd_submit_sync(&cmd, 0);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
    dvz_commands_destroy(&cmd);

    // PRE_SEND callback that will call the IMGUI callbacks.
    DvzCommands* cmds =
        dvz_canvas_commands(canvas, DVZ_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);
    dvz_event_callback(canvas, DVZ_EVENT_PRE_SEND, 0, DVZ_EVENT_MODE_SYNC, _presend, cmds);
}



void dvz_gui_begin(const char* title, DvzGuiStyle style)
{
    ASSERT(title != NULL);
    ASSERT(strlen(title) > 0);

    ImGuiIO& io = ImGui::GetIO();
    int flags = 0;

    switch (style)
    {

    case DVZ_GUI_STANDARD:
        // flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
        break;

    case DVZ_GUI_PROMPT:
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

    case DVZ_GUI_FIXED_TL:
    case DVZ_GUI_FIXED_TR:
    case DVZ_GUI_FIXED_LL:
    case DVZ_GUI_FIXED_LR:
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



void dvz_gui_end() { ImGui::End(); }



void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    dvz_gui_begin("FPS", DVZ_GUI_FIXED_TR);
    ImGui::Text("FPS: %.1f", canvas->fps);
    dvz_gui_end();
}



void dvz_imgui_destroy()
{
    if (ImGui::GetCurrentContext() == NULL)
        return;
    _imgui_destroy();
}



/*************************************************************************************************/
/*  Gui controls API                                                                             */
/*************************************************************************************************/

DvzGui* dvz_gui(DvzCanvas* canvas, const char* title, int flags)
{
    ASSERT(canvas != NULL);
    DvzGui* gui = (DvzGui*)dvz_container_alloc(&canvas->guis);
    gui->canvas = canvas;
    gui->title = title;
    gui->flags = flags;
    dvz_obj_init(&gui->obj);
    return gui;
}



void dvz_gui_float_slider(DvzGui* gui, const char* name, double vmin, double vmax)
{
    ASSERT(gui != NULL);
    ASSERT(vmin < vmax);

    DvzGuiControl* control = &gui->controls[gui->control_count++];
    control->gui = gui;
    control->name = name;
    control->type = DVZ_GUI_CONTROL_FLOAT_SLIDER;
    control->value = (float*)calloc(1, sizeof(float));
    control->u.fs.vmin = (float)vmin;
    control->u.fs.vmax = (float)vmax;
    ASSERT(control->u.fs.vmin < control->u.fs.vmax);
    *((float*)control->value) = .5 * (vmax + vmin);
}



/*************************************************************************************************/
/*  Gui controls implementation                                                                  */
/*************************************************************************************************/

static void _emit_gui_event(DvzGui* gui, DvzGuiControl* control)
{
    ASSERT(gui != NULL);
    ASSERT(control != NULL);

    DvzCanvas* canvas = gui->canvas;
    ASSERT(canvas != NULL);

    DvzEvent ev;
    ev.type = DVZ_EVENT_GUI;
    ev.u.g.gui = control->gui;
    ev.u.g.control = control;
    _event_produce(canvas, ev);
}



static bool _show_float_slider(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_FLOAT_SLIDER);

    float vmin = control->u.fs.vmin;
    float vmax = control->u.fs.vmax;
    ASSERT(vmin < vmax);
    return ImGui::SliderFloat(
        control->name, (float*)control->value, vmin, vmax, "%.3f", control->flags);
}



static void _show_control(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->gui != NULL);
    ASSERT(control->gui->canvas != NULL);
    ASSERT(control->name != NULL);
    ASSERT(control->value != NULL);

    bool changed = false;

    switch (control->type)
    {

    case DVZ_GUI_CONTROL_FLOAT_SLIDER:
        changed = _show_float_slider(control);
        break;

    default:
        log_error("unknown GUI control");
        break;
    }

    if (changed)
    {
        _emit_gui_event(control->gui, control);
    }
}



static void _show_gui(DvzGui* gui)
{
    ASSERT(gui != NULL);
    ASSERT(gui->title != NULL);
    ASSERT(strlen(gui->title) > 0);

    // Start the GUI window.
    dvz_gui_begin(gui->title, (DvzGuiStyle)gui->flags);

    for (uint32_t i = 0; i < gui->control_count; i++)
    {
        _show_control(&gui->controls[i]);
    }

    // ImGui::ShowDemoWindow();

    // End the GUI window.
    dvz_gui_end();
}



void dvz_gui_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzGui* gui = (DvzGui*)dvz_container_iter_init(&canvas->guis);
    while (gui != NULL)
    {
        ASSERT(gui != NULL);
        ASSERT(gui->obj.type == DVZ_OBJECT_TYPE_GUI);
        _show_gui(gui);
        gui = (DvzGui*)dvz_container_iter(&canvas->guis);
    }
}



void dvz_gui_destroy(DvzGui* gui)
{
    ASSERT(gui != NULL);
    for (uint32_t i = 0; i < gui->control_count; i++)
    {
        FREE(gui->controls[i].value);
    }
}
