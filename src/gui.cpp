#include <inttypes.h>

#include "../include/datoviz/canvas.h"
#include "../include/datoviz/common.h"
#include "../include/datoviz/controls.h"
#include "../include/datoviz/gui.h"
#include "canvas_utils.h"

BEGIN_INCL_NO_WARN
#include "../external/IconsFontAwesome.h"
#include "../external/imgui/backends/imgui_impl_glfw.h"
#include "../external/imgui/backends/imgui_impl_vulkan.h"
#include "../external/imgui/imgui.h"
END_INCL_NO_WARN



#define CHECK_IMGUI                                                                               \
    if (!_check_imgui_context())                                                                  \
        return;



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



static inline bool _check_imgui_context()
{
    if (ImGui::GetCurrentContext() == NULL)
    {
        log_error("no more ImGui context, skipping Dear ImGui callback");
        return false;
    }
    return true;
}



static void _presend(DvzCanvas* canvas, DvzEvent ev)
{
    bool has_imgui_context = _check_imgui_context();

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
    if (has_imgui_context)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // Call the IMGUI private callbacks to render the GUI.
    {
        DvzEvent ev_imgui = {};
        ev_imgui.type = DVZ_EVENT_IMGUI;
        ev_imgui.u.f.idx = canvas->frame_idx;
        ev_imgui.u.f.interval = canvas->clock.interval;
        ev_imgui.u.f.time = canvas->clock.elapsed;
        _event_produce(canvas, ev_imgui);
    }

    // End frame.
    if (has_imgui_context)
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
/*  Gui style                                                                                    */
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
    const ImGuiIO& io = ImGui::GetIO();

    int flags = ImGuiWindowFlags_NoTitleBar |       //
                ImGuiWindowFlags_NoScrollbar |      //
                ImGuiWindowFlags_NoResize |         //
                ImGuiWindowFlags_NoCollapse |       //
                ImGuiWindowFlags_NoNav |            //
                ImGuiWindowFlags_NoMove |           //
                ImGuiWindowFlags_NoNavInputs |      //
                ImGuiWindowFlags_NoDecoration |     //
                ImGuiWindowFlags_AlwaysAutoResize | //
                ImGuiWindowFlags_NoSavedSettings |  //
                ImGuiWindowFlags_NoFocusOnAppearing;
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
/*  Dear ImGui canvas activation                                                                 */
/*************************************************************************************************/

static void _imgui_init()
{
    if (ImGui::GetCurrentContext() != NULL)
        return;
    log_debug("initializing Dear ImGui");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable docking, requires the docking branch of imgui
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // io.ConfigDockingWithShift = false;  // DEPRECATED

    // TODO: make this customizable
    io.IniFilename = NULL;
}



static void _imgui_context(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);

    ASSERT(canvas->gpu->context != NULL);

    // The GUI context must be created only once per canvas.
    if (canvas->gui_context != NULL)
    {
        log_debug("skip creation of the already-existing GUI context for the canvas");
        return;
    }

    // Retrieve the pointer to the color texture.
    log_debug("creating the Dear ImGui context, with the colormap texture");
    DvzTexture* texture = canvas->gpu->context->color_texture.texture;

    ASSERT(texture != NULL);
    ASSERT(texture->sampler != NULL);
    ASSERT(texture->image != NULL);

    VkSampler sampler = texture->sampler->sampler;
    VkImageView image_view = texture->image->image_views[0];

    ASSERT(sampler != VK_NULL_HANDLE);
    ASSERT(image_view != VK_NULL_HANDLE);

    // GUI context for the GPU.
    canvas->gui_context = (DvzGuiContext*)calloc(1, sizeof(DvzGuiContext));
    canvas->gui_context->colormap_texture =
        ImGui_ImplVulkan_AddTexture(sampler, image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}



static void _imgui_fonts_upload(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(ImGui::GetCurrentContext() != NULL);
    ImGuiIO& io = ImGui::GetIO();

    // Load Fonts.
    // HACK: support Retina display on macOS
    float font_size = (OS_MACOS ? 11 : 14) * canvas->dpi_scaling;
    ASSERT(font_size > 0);

    // Load first fond.
    ImFontConfig config = {};
    config.FontDataOwnedByAtlas = false; // Important!
    {
        unsigned long file_size = 0;
        unsigned char* buffer = dvz_resource_font("Roboto_Medium", &file_size);
        ASSERT(file_size > 0);
        ASSERT(buffer != NULL);
        io.Fonts->AddFontFromMemoryTTF(buffer, file_size, font_size, &config);
    }

    // Load font awesome font for icons.
    // NOTE: this doesn't seem to work on macOS:
    // Assertion failed: (font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found."),
    // function ImFontAtlasBuildWithStbTruetype, file ../external/imgui/imgui_draw.cpp, line 2372.
    if (!OS_MACOS)
    {
        config.MergeMode = true;
        // config.GlyphMinAdvanceX = font_size; // Use if you want to make the icon monospaced
        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        // char path[1024];
        {
            unsigned long file_size = 0;
            unsigned char* buffer = dvz_resource_font("fontawesome_webfont", &file_size);
            ASSERT(file_size > 0);
            ASSERT(buffer != NULL);
            io.Fonts->AddFontFromMemoryTTF(buffer, file_size, font_size, &config, icon_ranges);

            // snprintf(path, sizeof(path), "%s/fonts/fontawesome-webfont.ttf", DATA_DIR);
            // io.Fonts->AddFontFromFileTTF(path, font_size, &config, icon_ranges);
        }
    }

    // Upload Fonts
    DvzCommands cmd = dvz_commands(canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_cmd_begin(&cmd, 0);
    ImGui_ImplVulkan_CreateFontsTexture(cmd.cmds[0]);
    dvz_cmd_end(&cmd, 0);
    dvz_cmd_submit_sync(&cmd, 0);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
    dvz_commands_destroy(&cmd);
}



// Enable ImGUI in a given canvas.
static void _imgui_canvas_enable(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->overlay);

    // Do we need to initialize Dear ImGui?
    bool need_init = ImGui::GetCurrentContext() == NULL;
    if (need_init)
        _imgui_init();

    // To run on every canvas:
    if (canvas->app->backend == DVZ_BACKEND_GLFW)
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)canvas->window->backend_window, true);

    if (need_init)
    {
        // To run on the first canvas only:
        log_debug("initialize Dear ImGui for the first canvas only");

        // HACK: the following must be called only once, for the first canvas.
        DvzGpu* gpu = canvas->gpu;
        ASSERT(gpu != NULL);
        DvzApp* app = gpu->app;
        ASSERT(app != NULL);

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
        ImGui_ImplVulkan_Init(&init_info, canvas->renderpass_overlay.renderpass);

        // Only need to run once, for the first canvas, as Dear ImGui uses a global context.
        _imgui_fonts_upload(canvas);

        // DPI scaling, using the first canvas only again.
        dvz_imgui_dpi_scaling(canvas, canvas->dpi_scaling);
    }

    // Create the ImGui context for every canvas, after initialization of Dear ImGui.
    _imgui_context(canvas);
}



/*************************************************************************************************/
/*  Dear ImGui functions                                                                         */
/*************************************************************************************************/

void dvz_imgui_destroy()
{
    if (ImGui::GetCurrentContext() == NULL)
        return;
    log_debug("destroying the Dear ImGui global context");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ASSERT(ImGui::GetCurrentContext() == NULL);
}



void dvz_imgui_enable(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->overlay);

    if (canvas->app->backend == DVZ_BACKEND_OFFSCREEN)
    {
        log_error("cannot enable imgui in offscreen mode at the moment");
        return;
    }

    // Enable Dear ImGui for the canvas, making sure the Dear ImGui global context and objects are
    // only initialized once.
    _imgui_canvas_enable(canvas);

    // PRE_SEND callback that will call the IMGUI callbacks.
    DvzCommands* cmds =
        dvz_canvas_commands(canvas, DVZ_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);
    dvz_event_callback(canvas, DVZ_EVENT_PRE_SEND, 0, DVZ_EVENT_MODE_SYNC, _presend, cmds);
}



void dvz_imgui_dpi_scaling(DvzCanvas* canvas, float scaling)
{
    if (ImGui::GetCurrentContext() != NULL)
    {
        ImGuiStyle style = ImGui::GetStyle();
        style.ScaleAllSizes(scaling);
    }
}



void dvz_imgui_demo(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_IMGUI, 0, DVZ_EVENT_MODE_SYNC, dvz_gui_callback_demo, NULL);
}



/*************************************************************************************************/
/*  GUI functions                                                                                */
/*************************************************************************************************/

void dvz_gui_begin(const char* title, int flags)
{
    CHECK_IMGUI

    ASSERT(title != NULL);
    ASSERT(strlen(title) > 0);

    int imgui_flags = _gui_style(flags);
    ImGui::Begin(title, NULL, imgui_flags);
}



void dvz_gui_end()
{
    CHECK_IMGUI

    ImGui::End();
}



void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent ev)
{
    CHECK_IMGUI

    ASSERT(canvas != NULL);
    dvz_gui_begin("FPS", DVZ_GUI_FLAGS_FIXED | DVZ_GUI_FLAGS_CORNER_UR);
    ImGui::Text("  FPS: %04.0f", canvas->fps);
    ImGui::Text("eFPS: %04.0f", canvas->efps);
    dvz_gui_end();
}



void dvz_gui_callback_demo(DvzCanvas* canvas, DvzEvent ev)
{
    CHECK_IMGUI

    ImGui::ShowDemoWindow(); //
}



void dvz_gui_callback_player(DvzCanvas* canvas, DvzEvent ev)
{
    CHECK_IMGUI

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



/*************************************************************************************************/
/*  GUI controls implementation                                                                  */
/*************************************************************************************************/

static void _emit_gui_event(DvzGui* gui, DvzGuiControl* control)
{
    ASSERT(gui != NULL);
    ASSERT(control != NULL);

    DvzCanvas* canvas = gui->canvas;
    ASSERT(canvas != NULL);

    DvzEvent ev = {};
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
    if (control->u.sf2.force_increasing)
    {
        float m = .5 * (vp[0] + vp[1]);
        if (vp[0] > vp[1])
            vp[0] = vp[1] = m;
        vp[1] = CLIP(vp[1], vp[0], vmax);
    }
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

    // Get the control position and size.
    {
        ImVec2 pos = ImGui::GetItemRectMin();
        ImVec2 size = ImGui::GetItemRectSize();
        control->pos[0] = pos.x;
        control->pos[1] = pos.y;
        control->size[0] = size.x;
        control->size[1] = size.y;
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
    CHECK_IMGUI

    ASSERT(canvas != NULL);

    // When Dear ImGUI captures the mouse and keyboard, Datoviz should not process user events.
    const ImGuiIO& io = ImGui::GetIO();
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



/*
static inline int _imgui_button(DvzMouseButton button)
{
    int b = 0;
    switch (button)
    {
    case DVZ_MOUSE_BUTTON_LEFT:
        b = 0;
        break;
    case DVZ_MOUSE_BUTTON_RIGHT:
        b = 1;
        break;
    case DVZ_MOUSE_BUTTON_MIDDLE:
        b = 2;
        break;
    default:
        break;
    }
    return b;
}

void _imgui_set_mouse_pos(vec2 pos)
{
    CHECK_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos[0] = pos[0];
    io.MousePos[1] = pos[1];
}

void _imgui_set_mouse_down(DvzMouseButton button, bool is_down)
{
    CHECK_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[_imgui_button(button)] = is_down;
}

void _imgui_set_mouse_click(vec2 pos, DvzMouseButton button)
{
    CHECK_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    int b = _imgui_button(button);
    io.MouseDownDuration[b] = 0;
    ASSERT(ImGui::IsMouseClicked(b, false));
}
*/
