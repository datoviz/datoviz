#include <inttypes.h>

#include "../include/datoviz/canvas.h"
#include "../include/datoviz/controls.h"
#include "../include/datoviz/gui.h"
#include "canvas_utils.h"

BEGIN_INCL_NO_WARN
#include "../external/IconsFontAwesome.h"
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
/*  Gui creation                                                                                 */
/*************************************************************************************************/

/*
prompt style:
flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav
| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration |
ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
ImGuiWindowFlags_NoFocusOnAppearing; ImGui::SetNextWindowBgAlpha(0.25f);

        ImVec2 window_pos = ImVec2(0, io.DisplaySize.y);
        ImVec2 window_pos_pivot = ImVec2(0, 1);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
*/

// 0 = TL, 1 = TR, 2 = LL, 3 = LR
static int _fixed_style(int corner)
{
    ImGuiIO& io = ImGui::GetIO();

    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    ImGui::SetNextWindowBgAlpha(0.5f);

    float distance = 0;
    ASSERT(corner >= 0);
    ImVec2 window_pos = ImVec2(
        (corner & 1) ? io.DisplaySize.x - distance : distance,
        (corner & 2) ? io.DisplaySize.y - distance : distance);
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    return flags;
}

static int _gui_style(int flags)
{
    bool fixed = ((flags >> 0) & DVZ_GUI_FLAGS_FIXED) != 0;
    int corner = ((flags >> 4) & 7) - 1;

    if (fixed)
    {
        ASSERT(corner >= 0);
        return _fixed_style(corner);
    }
    else
        return 0;
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

    // DPI scaling.
    dvz_imgui_dpi_scaling(canvas, canvas->dpi_scaling);

    // Load Fonts.
    float font_size = 16 * canvas->dpi_scaling;
    char path[1024];
    snprintf(path, sizeof(path), "%s/fonts/Roboto-Medium.ttf", DATA_DIR);
    io.Fonts->AddFontFromFileTTF(path, font_size);

    // Font awesome icons.
    ImFontConfig config;
    config.MergeMode = true;
    // config.GlyphMinAdvanceX = font_size; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    snprintf(path, sizeof(path), "%s/fonts/fontawesome-webfont.ttf", DATA_DIR);
    io.Fonts->AddFontFromFileTTF(path, font_size, &config, icon_ranges);

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

    // Make the colormap texture available.
    DvzTexture* texture = canvas->gpu->context->color_texture.texture;
    VkSampler sampler = texture->sampler->sampler;
    VkImageView image_view = texture->image->image_views[0];

    // GUI context.
    canvas->gui_context = (DvzGuiContext*)calloc(1, sizeof(DvzGuiContext));
    canvas->gui_context->colormap_texture =
        ImGui_ImplVulkan_AddTexture(sampler, image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}



void dvz_gui_begin(const char* title, int flags)
{
    ASSERT(title != NULL);
    ASSERT(strlen(title) > 0);

    int imgui_flags = _gui_style(flags);
    ImGui::Begin(title, NULL, imgui_flags);
}



void dvz_gui_end() { ImGui::End(); }



void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    dvz_gui_begin("FPS", DVZ_GUI_FLAGS_FIXED | DVZ_GUI_FLAGS_CORNER_UR);
    ImGui::Text("  FPS: %.0f", canvas->fps);
    ImGui::Text("eFPS: %.0f", canvas->efps);
    dvz_gui_end();
}



void dvz_gui_callback_demo(DvzCanvas* canvas, DvzEvent ev) { ImGui::ShowDemoWindow(); }



void dvz_gui_callback_player(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    if (canvas->screencast == NULL)
        return;
    ASSERT(canvas->screencast != NULL);
    dvz_gui_begin("Screencast recording", DVZ_GUI_FLAGS_FIXED | DVZ_GUI_FLAGS_CORNER_LR);

    // Play/pause button.
    if (ImGui::Button(canvas->screencast->is_active ? ICON_FA_PAUSE : ICON_FA_PLAY))
    {
        dvz_canvas_pause(canvas, !canvas->screencast->is_active);
    }

    ImGui::SameLine();

    // Stop button.
    if (ImGui::Button(ICON_FA_STOP))
    {
        dvz_canvas_stop(canvas);
    }

    dvz_gui_end();
}



void dvz_imgui_dpi_scaling(DvzCanvas* canvas, float scaling)
{
    if (ImGui::GetCurrentContext() != NULL)
    {
        ImGuiStyle style = ImGui::GetStyle();
        style.ScaleAllSizes(scaling);
    }
}



void dvz_imgui_destroy(DvzCanvas* canvas)
{
    if (ImGui::GetCurrentContext() == NULL)
        return;
    _imgui_destroy();
    FREE(canvas->gui_context);
}



void dvz_imgui_demo(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_IMGUI, 0, DVZ_EVENT_MODE_SYNC, dvz_gui_callback_demo, NULL);
}



/*************************************************************************************************/
/*  Gui controls implementation                10 */
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



static bool _show_checkbox(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_CHECKBOX);
    return ImGui::Checkbox(control->name, (bool*)control->value);
}

static bool _show_slider_float(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_SLIDER_FLOAT);

    float vmin = control->u.sf.vmin;
    float vmax = control->u.sf.vmax;
    ASSERT(vmin < vmax);
    return ImGui::SliderFloat(
        control->name, (float*)control->value, vmin, vmax, "%.5f", control->flags);
}

static bool _show_slider_float2(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_SLIDER_FLOAT2);

    float vmin = control->u.sf2.vmin;
    float vmax = control->u.sf2.vmax;
    float* vp = (float*)control->value;
    ASSERT(vmin < vmax);
    return ImGui::SliderFloat2(control->name, vp, vmin, vmax, "%.5f", control->flags);
}

static bool _show_input_float(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_INPUT_FLOAT);

    float step = control->u.f.step;
    float step_fast = control->u.f.step_fast;
    return ImGui::InputFloat(control->name, (float*)control->value, step, step_fast, "%.5f");
}

static bool _show_slider_int(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_SLIDER_INT);

    int vmin = control->u.si.vmin;
    int vmax = control->u.si.vmax;
    ASSERT(vmin < vmax);
    return ImGui::SliderInt(control->name, (int*)control->value, vmin, vmax, "%d", control->flags);
}

static void _show_label(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_LABEL);
    ImGui::LabelText(control->name, "%s", (const char*)control->value);
}

static bool _show_textbox(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_TEXTBOX);
    return ImGui::InputText(control->name, (char*)control->value, MAX_TEXT_LENGTH);
}

static bool _show_button(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    ASSERT(control->type == DVZ_GUI_CONTROL_BUTTON);
    return ImGui::Button(control->name);
}

static void _show_colormap(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    DvzCanvas* canvas = control->gui->canvas;
    ASSERT(canvas != NULL);
    // ImGuiIO& io = ImGui::GetIO();

    float tex_w = 256;
    float tex_h = 50;

    DvzColormap cmap = *((DvzColormap*)control->value);
    vec4 uvuv;
    dvz_colormap_extent(cmap, uvuv);

    ImVec2 uv_min = ImVec2(uvuv[0], uvuv[1]);           // Top-left
    ImVec2 uv_max = ImVec2(uvuv[2], uvuv[3]);           // Lower-right
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
    ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f); // 50% opaque white

    ASSERT(canvas->gui_context != NULL);
    ASSERT(canvas->gui_context->colormap_texture != NULL);
    ImGui::Image(
        canvas->gui_context->colormap_texture, ImVec2(tex_w, tex_h), uv_min, uv_max, tint_col,
        border_col);
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

    case DVZ_GUI_CONTROL_CHECKBOX:
        changed = _show_checkbox(control);
        break;

    case DVZ_GUI_CONTROL_SLIDER_FLOAT:
        changed = _show_slider_float(control);
        break;

    case DVZ_GUI_CONTROL_SLIDER_FLOAT2:
        changed = _show_slider_float2(control);
        break;

    case DVZ_GUI_CONTROL_INPUT_FLOAT:
        changed = _show_input_float(control);
        break;

    case DVZ_GUI_CONTROL_SLIDER_INT:
        changed = _show_slider_int(control);
        break;

    case DVZ_GUI_CONTROL_TEXTBOX:
        changed = _show_textbox(control);
        break;

    case DVZ_GUI_CONTROL_BUTTON:
        changed = _show_button(control);
        break;

    case DVZ_GUI_CONTROL_LABEL:
        _show_label(control);
        break;

    case DVZ_GUI_CONTROL_COLORMAP:
        _show_colormap(control);
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
    dvz_gui_begin(gui->title, gui->flags);

    for (uint32_t i = 0; i < gui->control_count; i++)
    {
        _show_control(&gui->controls[i]);
    }

    // End the GUI window.
    dvz_gui_end();
}



void dvz_gui_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);

    // When Dear ImGUI captures the mouse and keyboard, Datoviz should not process user events.
    ImGuiIO& io = ImGui::GetIO();
    canvas->captured = io.WantCaptureMouse || io.WantCaptureKeyboard;

    DvzGui* gui = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&canvas->guis);
    while (iter.item != NULL)
    {
        gui = (DvzGui*)iter.item;
        ASSERT(gui != NULL);
        ASSERT(gui->obj.type == DVZ_OBJECT_TYPE_GUI);
        _show_gui(gui);
        dvz_container_iter(&iter);
    }
}
