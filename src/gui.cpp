/*************************************************************************************************/
/*  Dear ImGUI wrapper                                                                           */
/*************************************************************************************************/

#include <stdarg.h>

#include "canvas.h"
#include "datoviz.h"
#include "fileio.h"
#include "glfw_utils.h"
#include "gui.h"
#include "host.h"
#include "resources.h"
#include "vklite.h"
#include "window.h"

// ImGUI includes
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"



/*************************************************************************************************/
/*  Utils functions                                                                              */
/*************************************************************************************************/

static inline void _imgui_check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    log_error("VkResult %d\n", err);
    if (err < 0)
        abort();
}



static inline bool _imgui_has_context() { return ImGui::GetCurrentContext() != NULL; }



static inline bool _imgui_has_glfw()
{
    return ImGui::GetCurrentContext() && ImGui::GetIO().BackendPlatformUserData != NULL;
}



static void _imgui_init(DvzGpu* gpu, uint32_t queue_idx, DvzRenderpass* renderpass)
{
    ASSERT(!_imgui_has_context());
    log_debug("initialize the Dear ImGui context");

    ANN(gpu);

    ImGui::DebugCheckVersionAndDataLayout(
        IMGUI_VERSION, sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2), sizeof(ImVec4),
        sizeof(ImDrawVert), sizeof(ImDrawIdx));
    ImGui::CreateContext(NULL);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;

    // Load a font.
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(ttf_bytes, (int)ttf_size, 16, &font_cfg);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(.2, .5, .8, 1));

    INIT(ImGui_ImplVulkan_InitInfo, init_info)
    init_info.Instance = gpu->host->instance;
    init_info.PhysicalDevice = gpu->physical_device;
    init_info.Device = gpu->device;
    // should be the render queue idx
    init_info.QueueFamily = gpu->queues.queue_families[queue_idx];
    init_info.Queue = gpu->queues.queues[queue_idx];
    init_info.DescriptorPool = gpu->dset_pool;

    ASSERT(renderpass->renderpass != VK_NULL_HANDLE);
    init_info.RenderPass = renderpass->renderpass;
    // init_info.PipelineCache = gpu->pipeline_cache;
    // init_info.Allocator = gpu->allocator;

    // TODO: better selection of image count (from Vulkan instead of hard-coded)
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;

    init_info.CheckVkResultFn = _imgui_check_vk_result;
    ImGui_ImplVulkan_Init(&init_info); //, renderpass->renderpass);
}



static void _imgui_set_window(DvzWindow* window)
{
    ANN(window);

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
        if (window->backend_window != NULL)
        {
            ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->backend_window, true);
        }
        break;
    default:
        break;
    }
}



static DvzRenderpass _imgui_renderpass(DvzGpu* gpu, bool offscreen)
{
    ANN(gpu);

    log_trace("create Dear ImGui renderpass");

    DvzRenderpass renderpass = dvz_renderpass(gpu);

    // Color attachment.
    dvz_renderpass_attachment(
        &renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, (VkFormat)DVZ_DEFAULT_FORMAT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        offscreen ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    dvz_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);

    dvz_renderpass_create(&renderpass);

    return renderpass;
}



static void _imgui_framebuffers(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzImages* images, DvzFramebuffers* framebuffers)
{
    ANN(gpu);
    ANN(renderpass);
    ANN(images);

    log_trace("creating Dear ImGui framebuffers");

    *framebuffers = dvz_framebuffers(gpu);
    dvz_framebuffers_attachment(framebuffers, 0, images);
    dvz_framebuffers_create(framebuffers, renderpass);
}



static void _imgui_destroy_window(DvzWindow* window)
{
    log_trace("calling ImGui_ImplVulkan_Shutdown()");
    ImGui_ImplVulkan_Shutdown();

    if (window != NULL)
    {
        // NOTE: gui_window->window may be NULL if offscreen.
        backend_poll_events(DVZ_BACKEND_GLFW);

        backend_window_clear_callbacks(DVZ_BACKEND_GLFW, window->backend_window);

        log_trace("calling ImGui_ImplGlfw_Shutdown()");
        ImGui_ImplGlfw_Shutdown();
    }
}



static void _imgui_destroy(DvzGui* gui)
{
    ANN(gui);

    // IMPORTANT NOTE: to avoid segfaults while destroying ImGui or the windows, the following
    // destruction order must be followed:
    // 1) ImGui_ImplVulkan_Shutdown()
    // 2) ImGui_ImplGlfw_Shutdown()
    // 3) ImGui::DestroyContext()
    // 4) glfwDestroyWindow()

    ANN(ImGui::GetCurrentContext());

    // HACK: fix segfault when destroying the application while no window was created,
    // because ImGui destruction requires Vulkan to shutdown before, but _imgui_destroy_window()
    // is responsible for this, and it is never called if there was no window.
    if (gui->gui_windows.count == 0)
        ImGui_ImplVulkan_Shutdown();

    log_trace("calling ImGui::DestroyContext()");
    ImGui::DestroyContext(ImGui::GetCurrentContext());
    ASSERT(ImGui::GetCurrentContext() == NULL);
}



/*************************************************************************************************/
/*  GUI functions                                                                                */
/*************************************************************************************************/

DvzGui* dvz_gui(DvzGpu* gpu, uint32_t queue_idx, int flags)
{
    ANN(gpu);
    ANN(gpu->host);

    if (_imgui_has_context())
    {
        log_warn("GUI context already created, skipping");
        return NULL;
    }
    log_debug("initialize the Dear ImGui context");

    DvzGui* gui = (DvzGui*)calloc(1, sizeof(DvzGui));
    gui->gpu = gpu;

    gui->gui_windows = dvz_container(
        DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGuiWindow), DVZ_OBJECT_TYPE_GUI_WINDOW);

    bool offscreen =
        (flags & DVZ_GUI_FLAGS_OFFSCREEN) != 0 || gpu->host->backend == DVZ_BACKEND_OFFSCREEN;
    gui->renderpass = _imgui_renderpass(gpu, offscreen);
    ASSERT(dvz_obj_is_created(&gui->renderpass.obj));

    _imgui_init(gpu, queue_idx, &gui->renderpass);
    return gui;
}



void dvz_gui_destroy(DvzGui* gui)
{
    log_debug("destroy the GUI");
    ANN(gui);

    _imgui_destroy(gui);

    // Destroy the GUI windows.
    CONTAINER_DESTROY_ITEMS(DvzGuiWindow, gui->gui_windows, dvz_gui_window_destroy)
    dvz_container_destroy(&gui->gui_windows);

    dvz_renderpass_destroy(&gui->renderpass);

    FREE(gui);
}



/*************************************************************************************************/
/*  GUI window                                                                                   */
/*************************************************************************************************/

DvzGuiWindow* dvz_gui_window(DvzGui* gui, DvzWindow* window, DvzImages* images, uint32_t queue_idx)
{
    // NOTE: window is optional (offscreen tests)
    // NOTE: glfw is the only supported backend for now

    ANN(gui);
    ANN(window);
    ANN(images);

    ASSERT(!window || window->gui_window == NULL); // Only set it once.
    ASSERT(images->count > 0);

    DvzGpu* gpu = gui->gpu;
    ANN(gpu);

    DvzGuiWindow* gui_window = (DvzGuiWindow*)dvz_container_alloc(&gui->gui_windows);
    gui_window->gui = gui;
    gui_window->window = window;

    // GUI window width and height relate to the framebuffer, not the window size.
    gui_window->width = images->shape[0];
    gui_window->height = images->shape[1];

    gui_window->is_offscreen = false;

    // Create the command buffers.
    gui_window->cmds = dvz_commands(gpu, queue_idx, images->count);

    // Create the framebuffers.
    _imgui_framebuffers(gpu, &gui->renderpass, images, &gui_window->framebuffers);

    if (window->gui_window == NULL)
        _imgui_set_window(window);
    window->gui_window = gui_window;

    dvz_obj_created(&gui_window->obj);
    return gui_window;
}



DvzGuiWindow* dvz_gui_offscreen(DvzGui* gui, DvzImages* images, uint32_t queue_idx)
{
    ANN(gui);
    ANN(images);

    DvzGpu* gpu = gui->gpu;
    ANN(gpu);

    DvzGuiWindow* gui_window = (DvzGuiWindow*)dvz_container_alloc(&gui->gui_windows);
    gui_window->gui = gui;

    // GUI window width and height relate to the framebuffer, not the window size.
    gui_window->width = images->shape[0];
    gui_window->height = images->shape[1];

    gui_window->is_offscreen = true;

    // Create the command buffers.
    gui_window->cmds = dvz_commands(gpu, queue_idx, 1);

    // Create the framebuffers.
    _imgui_framebuffers(gpu, &gui->renderpass, images, &gui_window->framebuffers);

    dvz_obj_created(&gui_window->obj);
    return gui_window;
}



void dvz_gui_window_begin(DvzGuiWindow* gui_window, uint32_t cmd_idx)
{
    ANN(gui_window);

    DvzCommands* cmds = &gui_window->cmds;
    ANN(cmds);

    // When Dear ImGUI captures the mouse and keyboard, Datoviz should not process user events.
    ImGuiIO& io = ImGui::GetIO();
    if (gui_window->window)
        gui_window->window->is_captured = io.WantCaptureMouse || io.WantCaptureKeyboard;

    DvzGui* gui = gui_window->gui;
    ANN(gui);

    io.DisplaySize.x = gui_window->width;
    io.DisplaySize.y = gui_window->height;

    ImGui_ImplVulkan_NewFrame();

    if (!gui_window->is_offscreen)
        ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();

    dvz_cmd_begin(cmds, cmd_idx);
    dvz_cmd_begin_renderpass(cmds, cmd_idx, &gui->renderpass, &gui_window->framebuffers);
}



void dvz_gui_window_end(DvzGuiWindow* gui_window, uint32_t cmd_idx)
{
    ANN(gui_window);

    DvzCommands* cmds = &gui_window->cmds;
    ANN(cmds);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmds->cmds[cmd_idx], VK_NULL_HANDLE);

    dvz_cmd_end_renderpass(cmds, cmd_idx);
    dvz_cmd_end(cmds, cmd_idx);
}



void dvz_gui_window_resize(DvzGuiWindow* gui_window, uint32_t width, uint32_t height)
{
    ANN(gui_window);
    gui_window->width = width;
    gui_window->height = height;

    DvzGui* gui = gui_window->gui;
    ANN(gui);

    // Recreate the framebuffers.
    dvz_framebuffers_destroy(&gui_window->framebuffers);
    dvz_framebuffers_create(&gui_window->framebuffers, &gui->renderpass);
}



void dvz_gui_window_destroy(DvzGuiWindow* gui_window)
{
    log_trace("destroy gui window");
    ANN(gui_window);

    // NOTE: gui_window->window may be NULL if offscreen.
    _imgui_destroy_window(gui_window->window);

    dvz_framebuffers_destroy(&gui_window->framebuffers);
    dvz_obj_destroyed(&gui_window->obj);
}



/*************************************************************************************************/
/*  DearImGui Wrappers                                                                           */
/*************************************************************************************************/

void dvz_gui_pos(vec2 pos, vec2 pivot)
{
    ImGui::SetNextWindowPos(ImVec2(pos[0], pos[1]), ImGuiCond_Once, ImVec2(pivot[0], pivot[1]));
}



void dvz_gui_corner(DvzCorner corner, vec2 pad)
{
    // 0 = TL, 1 = TR, 2 = LL, 3 = LR
    // NOTE: by default, always top right
    const ImGuiIO& io = ImGui::GetIO();
    ImVec2 pos = ImVec2(
        (corner & 1) ? io.DisplaySize.x - pad[0] : pad[0],
        (corner & 2) ? io.DisplaySize.y - pad[1] : pad[1]);
    ImVec2 pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);

    vec2 pos_ = {pos.x, pos.y};
    vec2 pivot_ = {pivot.x, pivot.y};
    dvz_gui_pos(pos_, pivot_);
}



void dvz_gui_size(vec2 size)
{
    ImGui::SetNextWindowSize(ImVec2(size[0], size[1]), ImGuiCond_FirstUseEver);
}



int dvz_gui_flags(int flags)
{
    int imgui_flags = ImGuiWindowFlags_NoSavedSettings;

    // Overlay.
    if ((flags & DVZ_DIALOG_FLAGS_OVERLAY) != 0)
    {
        imgui_flags |= ImGuiWindowFlags_NoTitleBar |        //
                       ImGuiWindowFlags_NoScrollbar |       //
                       ImGuiWindowFlags_NoResize |          //
                       ImGuiWindowFlags_NoCollapse |        //
                       ImGuiWindowFlags_NoNav |             //
                       ImGuiWindowFlags_NoNavInputs |       //
                       ImGuiWindowFlags_NoDecoration |      //
                       ImGuiWindowFlags_NoMove |            //
                       ImGuiWindowFlags_NoFocusOnAppearing; //
        dvz_gui_alpha(0.5f);
    }

    return imgui_flags;
}



void dvz_gui_alpha(float alpha)
{
    ImGui::SetNextWindowBgAlpha(alpha); //
}



void dvz_gui_begin(const char* title, int gui_flags)
{
    // WARNING: the title should be unique for each different dialog!
    ANN(title);
    bool open = true;
    ImGui::Begin(title, &open, gui_flags);
}



void dvz_gui_text(const char* fmt, ...)
{
    ANN(fmt);
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}



bool dvz_gui_slider(const char* name, float vmin, float vmax, float* value)
{
    ANN(name);
    ANN(value);
    return ImGui::SliderFloat(name, value, vmin, vmax, "%.5f", 0);
}



bool dvz_gui_button(const char* name, float width, float height)
{
    ANN(name);
    return ImGui::Button(name, ImVec2(width, height));
}



bool dvz_gui_checkbox(const char* name, bool* checked)
{
    ANN(name);
    ANN(checked);
    return ImGui::Checkbox(name, checked);
}



void dvz_gui_progress(float fraction, float width, float height, const char* fmt, ...)
{
    ANN(fmt);
    va_list args;
    va_start(args, fmt);
    char overlay[1024] = {0};
    vsnprintf(overlay, 1024, fmt, args);
    ImGui::ProgressBar(fraction, ImVec2(width, height), overlay);
    va_end(args);
}



void dvz_gui_image(DvzTex* tex, float width, float height)
{
    ANN(tex);

    ASSERT(tex->dims == DVZ_TEX_2D);

    // HACK: create a Vulkan descriptor set for ImGui.
    if (tex->_imgui_texid == VK_NULL_HANDLE)
    {
        DvzSampler* sampler = dvz_resources_sampler(
            tex->res, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

        tex->_imgui_texid = ImGui_ImplVulkan_AddTexture(
            sampler->sampler, tex->img->image_views[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    ASSERT(tex->_imgui_texid != VK_NULL_HANDLE);

    ImVec2 uv_min = ImVec2(0.0f, 0.0f);               // Top left
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);               // Bottom right
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
    // ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

    ImGui::Image((ImTextureID)tex->_imgui_texid, ImVec2(width, height), uv_min, uv_max, tint_col);
}



bool dvz_gui_colorpicker(const char* name, vec3 color, int flags)
{
    ANN(name);
    return ImGui::ColorPicker3(name, color, 0);
}



bool dvz_gui_node(const char* name)
{
    // ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf |
    // ImGuiTreeNodeFlags_NoTreePushOnOpen;
    // bool res = ImGui::TreeNodeEx(name, node_flags);//

    bool res = ImGui::TreeNode(name);

    // && !ImGui::IsItemToggledOpen())
    // if (ImGui::IsItemClicked())
    // {
    //     log_info("clicked %s", name);
    // }
    return res;
}



void dvz_gui_pop() { ImGui::TreePop(); }



bool dvz_gui_clicked() { return ImGui::IsItemClicked(); }



bool dvz_gui_selectable(const char* name) { return ImGui::Selectable(name); }



bool dvz_gui_table(
    const char* name, uint32_t row_count, uint32_t column_count, //
    const char** labels, bool* selected, int flags)
{
    // length of selected is row_count
    ANN(name);
    ASSERT(column_count > 0);
    uint32_t label_count = row_count * column_count;
    if (row_count > 0)
    {
        ANN(labels);
    }

    bool sel = false, out = false;
    uint32_t idx = column_count;

    int imgui_flags =
        (ImGuiTableFlags_RowBg |   //
         ImGuiTableFlags_Borders | //
         ImGuiTableFlags_Resizable // | //

         //  ImGuiTableFlags_Sortable    //
        );

    if (ImGui::BeginTable(name, (int)column_count, imgui_flags))
    {
        // Header row.
        for (uint32_t i = 0; i < column_count; i++)
        {
            ImGui::TableSetupColumn(labels[i]);
        }
        // ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        // Rows.
        for (uint32_t row_idx = 0; row_idx < row_count; row_idx++)
        {
            sel = selected ? selected[row_idx] : false;
            ImGui::TableNextRow();

            // First column.
            ImGui::TableSetColumnIndex(0);
            // NOTE: the first item in each row should be a Selectable that spans all columns,
            // such that the entire row is selectable. All other items are just text labels.
            if (ImGui::Selectable(
                    labels[idx++], sel, //
                    ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0)) &&
                selected)
            {
                // Only the clicked row should be selected if Control is pressed.
                if (!ImGui::GetIO().KeyCtrl)
                {
                    memset(selected, 0, row_count * sizeof(bool));
                    selected[row_idx] = !sel;
                }
                else
                {
                    // Toggle the clicked row.
                    selected[row_idx] = !selected[row_idx];
                }
            }

            // Other columns.
            for (uint32_t column_idx = 1; column_idx < column_count; column_idx++)
            {
                ImGui::TableSetColumnIndex((int)column_idx);
                ImGui::TextUnformatted(labels[idx++]);
            }

            // Return true if there is at least one selection change.
            if (selected && selected[row_idx] != sel)
            {
                out = true;
            }
        }

        ImGui::EndTable();
    }
    return out;
}



void dvz_gui_demo()
{
    bool open = true;
    ImGui::ShowDemoWindow(&open);
}



void dvz_gui_end()
{
    ImGui::End(); //
}
