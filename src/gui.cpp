#include "../include/visky/gui.h"
#include <inttypes.h>

BEGIN_INCL_NO_WARN
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Internal ImGui helpers                                                                       */
/*************************************************************************************************/

static bool imgui_supported;



static void _imgui_check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    log_error("VkResult %d\n", err);
    if (err < 0)
        abort();
}



void vky_imgui_render(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    ImGui::Render();
    vky_begin_live_render_pass(cmd_buf, canvas);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buf);
    vky_end_render_pass(cmd_buf, canvas);
}



void vky_imgui_newframe()
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}



void vky_imgui_init(VkyCanvas* canvas)
{
    if (canvas->gui_count > 0)
        return;

    if (canvas->app->backend != VKY_BACKEND_GLFW)
    {
        log_warn("only the GLFW backend is supported currently");
        imgui_supported = false;
        return;
    }
    else
    {
        imgui_supported = true;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGuiStyle style = ImGui::GetStyle();
    style.ScaleAllSizes(canvas->dpi_factor);

    // Setup Platform/Renderer bindings
    VkyGpu* gpu = canvas->gpu;
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)canvas->window, true);
    ImGui_ImplVulkan_InitInfo init_info = {0};
    init_info.Instance = gpu->instance;
    init_info.PhysicalDevice = gpu->physical_device;
    init_info.Device = gpu->device;
    init_info.QueueFamily = gpu->queue_indices.graphics_family;
    init_info.Queue = gpu->graphics_queue;
    init_info.PipelineCache = gpu->pipeline_cache;
    init_info.DescriptorPool = gpu->descriptor_pool;
    init_info.Allocator = gpu->allocator;
    init_info.MinImageCount = canvas->image_count;
    init_info.ImageCount = canvas->image_count;
    init_info.CheckVkResultFn = _imgui_check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, canvas->live_render_pass);

    // Load Fonts
    io.Fonts->AddFontFromFileTTF(
        "data/fonts/Roboto-Medium.ttf", VKY_IMGUI_FONT_SIZE * canvas->dpi_factor);
    // Upload Fonts
    VkCommandBuffer cmd_buf = begin_single_time_commands(gpu->device, gpu->command_pool);
    ImGui_ImplVulkan_CreateFontsTexture(cmd_buf);
    end_single_time_commands(gpu->device, gpu->command_pool, &cmd_buf, gpu->graphics_queue);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}



VkyImGuiTexture vky_imgui_image_from_texture(VkyTexture texture)
{
    VkyImGuiTexture out = {
        texture,
        ImGui_ImplVulkan_AddTexture(
            texture.sampler, texture.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)};
    return out;
}



VkyImGuiTexture vky_imgui_image_from_canvas(VkyCanvas* canvas)
{
    VkyTextureParams params = vky_default_texture_params(
        canvas->size.framebuffer_width, canvas->size.framebuffer_height, 1);

    VkyTexture texture = {0};
    texture.gpu = canvas->gpu;
    texture.params = params;
    texture.image = canvas->images[0];
    texture.image_view = canvas->image_views[0];
    texture.sampler = create_texture_sampler(
        canvas->gpu->device, params.filter, params.filter, params.address_mode);

    return vky_imgui_image_from_texture(texture);
}



VkyCanvas* vky_imgui_canvas_create(VkyCanvas* canvas_, uint32_t width, uint32_t height)
{
    // Create the render pass.
    log_trace("create offscreen render pass");

    VkyGpu* gpu = canvas_->gpu;

    VkyCanvas canvas_s = {0};
    canvas_s.gpu = gpu;
    canvas_s.is_offscreen = true;
    // canvas_s.window_size.lw = width;
    // canvas_s.window_size.w = width;
    // canvas_s.window_size.lh = height;
    // canvas_s.window_size.h = height;
    canvas_s.size.framebuffer_width = canvas_s.size.window_width = width;
    canvas_s.size.framebuffer_height = canvas_s.size.window_height = height;
    canvas_s.dpi_factor = VKY_DPI_SCALING_FACTOR;
    canvas_s.image_count = 1;
    canvas_s.depth_format = VK_FORMAT_D32_SFLOAT;
    canvas_s.image_format = gpu->image_format;
    canvas_s.command_buffers = (VkCommandBuffer*)calloc(1, sizeof(VkCommandBuffer));

    VkRenderPass render_pass = {0};
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    create_offscreen_render_pass(
        gpu->device, format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &render_pass);
    canvas_s.render_pass = render_pass;

    VkyCanvas* canvas = (VkyCanvas*)calloc(1, sizeof(VkyCanvas));
    *canvas = canvas_s;

    VkImage image = {0};
    VkDeviceMemory image_memory = {0};

    create_image(
        gpu->device, width, height, 1, canvas->image_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &image, &image_memory,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, canvas->gpu->memory_properties);

    VkImageView image_view = create_image_view(
        gpu->device, image, VK_IMAGE_VIEW_TYPE_2D, canvas->image_format,
        VK_IMAGE_ASPECT_COLOR_BIT);

    // Create the depth objects.
    VkImage depth_image = {0};
    VkImageView depth_image_view = {0};
    VkDeviceMemory depth_image_memory = {0};

    create_image(
        gpu->device, width, height, 1, canvas->depth_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &depth_image, &depth_image_memory,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, canvas->gpu->memory_properties);

    // Create depth image view.
    depth_image_view = create_image_view(
        gpu->device, depth_image, VK_IMAGE_VIEW_TYPE_2D, canvas->depth_format,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create the frame buffer.
    VkFramebuffer framebuffer = {0};
    // Create FrameBuffer
    VkImageView attachments[] = {image_view, depth_image_view};

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = canvas->render_pass;
    framebuffer_info.attachmentCount = 2;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    framebuffer_info.layers = 1;

    if (vkCreateFramebuffer(gpu->device, &framebuffer_info, NULL, &framebuffer) != VK_SUCCESS)
    {
        exit(1);
    }

    // Fill the VkyCanvas struct.
    canvas->size.framebuffer_width = width;
    canvas->size.framebuffer_height = height;
    canvas->depth_image = depth_image;
    canvas->depth_image_view = depth_image_view;
    canvas->depth_image_memory = depth_image_memory;

    // One per swap image.
    canvas->images = (VkImage*)malloc(sizeof(VkImage));
    canvas->images[0] = image;
    canvas->image_views = (VkImageView*)malloc(sizeof(VkImageView));
    canvas->image_views[0] = image_view;
    canvas->image_memory = image_memory;
    canvas->framebuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer));
    canvas->framebuffers[0] = framebuffer;

    // Create the command buffers.
    vky_create_command_buffers(canvas->gpu, 2 * canvas->image_count, canvas->command_buffers);
    canvas->live_command_buffers = &canvas->command_buffers[canvas->image_count];

    vky_create_event_controller(canvas);

    return canvas;
}



void vky_imgui_canvas_init(VkyCanvas* canvas)
{
    vky_fill_command_buffers(canvas);
    // vky_submit_command_buffer_offscreen(canvas);
    vky_offscreen_frame(canvas, VKY_TIME);
}



void vky_imgui_canvas_next_frame(VkyCanvas* canvas)
{
    // vky_next_frame(canvas);
    // vky_submit_command_buffer_offscreen(canvas);
    vky_offscreen_frame(canvas, VKY_TIME);
    canvas->frame_count++;
}



void vky_imgui_image(VkyImGuiTexture* imtexture, float width, float height)
{
    ImGui::Image((void*)imtexture->id, ImVec2(width, height));
}



void vky_imgui_destroy()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}



/*************************************************************************************************/
/*  Public GUI API                                                                               */
/*************************************************************************************************/

static void _set_style(ImGuiIO& io, VkyGui* gui, int* flags)
{
    switch (gui->style)
    {

    case VKY_GUI_STANDARD:
        *flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
        break;

    case VKY_GUI_PROMPT:
    {
        *flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        ImGui::SetNextWindowBgAlpha(0.5f);

        ImVec2 window_pos = ImVec2(0, io.DisplaySize.y);
        ImVec2 window_pos_pivot = ImVec2(0, 1);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

        ImVec2 size = ImVec2(io.DisplaySize.x, 30);
        ImGui::SetNextWindowSize(size);

        break;
    }

    case VKY_GUI_FIXED_TL:
    case VKY_GUI_FIXED_TR:
    case VKY_GUI_FIXED_LL:
    case VKY_GUI_FIXED_LR:
    {
        *flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        ImGui::SetNextWindowBgAlpha(0.5f);

        float distance = 0;
        int corner = (int32_t)gui->style - 1; // 0 = TL, 1 = TR, 2 = LL, 3 = LR
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
}



static void _add_control(VkyGui* gui, VkyGuiControl* control)
{
    switch (control->control_type)
    {

    case VKY_GUI_CHECKBOX:
        ImGui::Checkbox(control->name, (bool*)control->value);
        break;

    case VKY_GUI_TEXTBOX_PROMPT:
        ASSERT(control->value != NULL);
        ImGui::PushItemWidth(400);
        if (ImGui::InputText(
                "", (char*)gui->canvas->prompt, VKY_MAX_PROMPT_SIZE,
                ImGuiInputTextFlags_EnterReturnsTrue))
        {
            gui->canvas->prompt->state = VKY_PROMPT_ACTIVE;
            gui->canvas->prompt->gui->is_visible = false;
        }
        ImGui::PushItemWidth(-400);

        break;

    case VKY_GUI_FPS:
        ImGui::Text("FPS: %4" PRIu64 "", gui->canvas->fps);
        break;

    case VKY_GUI_LISTBOX:
    {
        const VkyGuiListParams* params = (const VkyGuiListParams*)control->params;
        ImGui::ListBox(
            control->name, (int*)control->value, params->item_names, (int)params->item_count, -1);
    }
    break;

    case VKY_GUI_COMBO:
    {
        const VkyGuiListParams* params = (const VkyGuiListParams*)control->params;
        ImGui::Combo(
            control->name, (int*)control->value, params->item_names, (int)params->item_count);
    }
    break;

    case VKY_GUI_COLOR:
        ImGui::ColorPicker4(
            control->name, (float*)control->value,
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs |
                ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions |
                ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoTooltip |
                ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoSidePreview);
        break;

    default:
        break;
    }
}


static void fill_live_command_buffer(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    if (canvas->gui_count == 0)
        return;
    if (!imgui_supported)
        return;

    // NOTE: skip the live render pass while we're still resizing the window.
    if (canvas->size.resized)
    {
        log_trace("skip imgui render as we're resizing the window");
        return;
    }

    vky_imgui_newframe();
    VkyGui* gui = NULL;
    ImGuiIO& io = ImGui::GetIO();
    int flags = 0;

    // Stop processing Visky events if the user is interacting with the GUI.
    canvas->event_controller->do_process_input = !io.WantCaptureMouse && !io.WantCaptureKeyboard;

    for (uint32_t k = 0; k < canvas->gui_count; k++)
    {
        gui = canvas->guis[k];
        // Skip non-existing or hidden GUIs.
        if (gui == NULL)
            continue;
        if (!gui->is_visible)
        {
            gui->frame_count = 0;
            continue;
        }

        // Fixed GUI.
        _set_style(io, gui, &flags);


        // Start the GUI creation.
        ImGui::Begin(gui->title, NULL, flags);

        // HACK: set the keyboard focus on the prompt, when the GUI is created.
        if (gui->style == VKY_GUI_PROMPT && gui->frame_count == 0)
        {
            ImGui::SetKeyboardFocusHere(0);
        }

        // bool x = true;
        // ImGui::ShowDemoWindow(&x);
        // Add all GUI controls.
        VkyGuiControl* control = NULL;
        for (uint32_t i = 0; i < gui->control_count; i++)
        {
            control = &gui->controls[i];
            ASSERT(control != NULL);
            _add_control(gui, control);
        }

        ImGui::End();
        gui->frame_count++;
    }

    vky_imgui_render(canvas, cmd_buf);
}



VkyGui* vky_create_gui(VkyCanvas* canvas, VkyGuiParams params)
{
    vky_imgui_init(canvas);

    VkyGui* gui = (VkyGui*)calloc(1, sizeof(VkyGui));
    gui->canvas = canvas;
    gui->title = params.title != NULL ? params.title : "GUI";
    gui->style = params.style;
    gui->is_visible = true;
    canvas->cb_fill_live_command_buffer = fill_live_command_buffer;
    if (canvas->guis == NULL)
        canvas->guis = (VkyGui**)calloc(VKY_MAX_GUI_COUNT, sizeof(VkyGui*));
    canvas->guis[canvas->gui_count] = gui;
    canvas->gui_count++;
    return gui;
}



void vky_gui_control(
    VkyGui* gui, VkyGuiControlType control_type, const char* name, const void* params, void* value)
{
    gui->controls[gui->control_count].control_type = control_type;
    gui->controls[gui->control_count].name = name;
    gui->controls[gui->control_count].params = params;
    gui->controls[gui->control_count].value = value;
    gui->control_count++;
}



void vky_gui_fps(VkyGui* gui) { vky_gui_control(gui, VKY_GUI_FPS, NULL, NULL, NULL); }



void vky_destroy_guis(VkyCanvas* canvas)
{
    if (!imgui_supported)
        return;
    if (canvas->gui_count == 0)
        return;
    for (uint32_t i = 0; i < canvas->gui_count; i++)
    {
        FREE(canvas->guis[i]);
    }
    vky_imgui_destroy();
}



/*************************************************************************************************/
/*  Prompt GUI                                                                                   */
/*************************************************************************************************/

void vky_prompt(VkyCanvas* canvas)
{
    if (canvas->prompt == NULL)
        canvas->prompt = (VkyPrompt*)calloc(1, sizeof(VkyPrompt));
    ASSERT(canvas->prompt != NULL);
    if (canvas->prompt->state == VKY_PROMPT_SHOWN)
        return;

    // Create the prompt GUI only once.
    if (canvas->prompt->gui == NULL)
    {
        VkyGuiParams params = {};
        params.style = VKY_GUI_PROMPT;
        canvas->prompt->gui = vky_create_gui(canvas, params);
        vky_gui_control(
            canvas->prompt->gui, VKY_GUI_TEXTBOX_PROMPT, NULL, NULL, canvas->prompt->text);
    }

    canvas->prompt->text[0] = 0;
    canvas->prompt->gui->is_visible = true;
    canvas->prompt->state = VKY_PROMPT_SHOWN;
}



char* vky_prompt_get(VkyCanvas* canvas)
{
    if (canvas->prompt == NULL)
    {
        // log_error("you need to call vky_prompt() first");
        return NULL;
    }
    if (canvas->prompt->state != VKY_PROMPT_ACTIVE)
        return NULL;
    ASSERT(canvas->prompt->gui != NULL);
    canvas->prompt->state = VKY_PROMPT_HIDDEN;
    return canvas->prompt->text;
}
