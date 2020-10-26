#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/visky/visky.h"
#include "video.h"
#include "vkutils.h"
// #include "vnc.h"



/*************************************************************************************************/
/*  Offscreen rendering                                                                          */
/*************************************************************************************************/

void create_offscreen_render_pass(
    VkDevice device, VkFormat format, VkImageLayout layout, VkRenderPass* render_pass)
{
    log_trace("create offscreen render pass");
    VkAttachmentDescription attachmentDescriptions[2];

    VkAttachmentDescription colorAttachment = {0};
    VkAttachmentDescription depthAttachment = {0};

    // Color attachment
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = layout;
    // Depth attachment
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachmentDescriptions[0] = colorAttachment;
    attachmentDescriptions[1] = depthAttachment;

    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {0};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for layout transitions
    VkSubpassDependency dependencies[2];

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachmentDescriptions;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;
    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, NULL, render_pass));
}

VkyCanvas* vky_create_offscreen_canvas(VkyGpu* gpu, uint32_t width, uint32_t height)
{
    // Queues.
    uint32_t queue_count = 1;
    ASSERT(queue_count <= 100);
    VkDeviceQueueCreateInfo queue_create_infos[100];
    float queue_priority = 1.0f;

    queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[0].pNext = NULL;
    queue_create_infos[0].flags = 0;
    queue_create_infos[0].queueFamilyIndex = 0;
    queue_create_infos[0].queueCount = 1;
    queue_create_infos[0].pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = queue_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;

    // Device features.
    VkPhysicalDeviceFeatures enabled_features = {0};
    enabled_features.samplerAnisotropy = VK_TRUE;
    enabled_features.geometryShader = VK_FALSE;
    device_create_info.pEnabledFeatures = &enabled_features;

    // Device extensions.
    device_create_info.enabledExtensionCount = 0;
    device_create_info.ppEnabledExtensionNames = NULL;

    // Validation layers.
    const char* layers[] = {"VK_LAYER_KHRONOS_validation"};

    if (gpu->has_validation)
    {
        device_create_info.enabledLayerCount = 1;
        device_create_info.ppEnabledLayerNames = layers;
    }
    else
    {
        device_create_info.enabledLayerCount = 0;
    }

    // Create the device.
    VkDevice device = {0};
    VK_CHECK_RESULT(vkCreateDevice(gpu->physical_device, &device_create_info, NULL, &device));

    // Create the render pass.
    log_trace("create offscreen render pass");
    VkRenderPass render_pass = {0};
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    create_offscreen_render_pass(
        device, format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &render_pass);

    // Create the queues.
    VkQueue graphics_queue = {0};
    vkGetDeviceQueue(device, 0, 0, &graphics_queue);

    gpu->device = device;
    gpu->graphics_queue = graphics_queue;

    vky_create_command_pool(gpu);
    vky_create_descriptor_pool(gpu);

    // Prepare compute on offscreen canvas.

    // TODO: find properly the compute family index
    vkGetDeviceQueue(device, 0, 0, &gpu->compute_queue);

    // Create the compute command buffer.
    log_trace("allocate compute command buffer");
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = gpu->compute_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(gpu->device, &alloc_info, &gpu->compute_command_buffer));
    ASSERT(gpu->compute_command_buffer != 0);

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
    canvas_s.image_format = format;
    canvas_s.command_buffers = calloc(1, sizeof(VkCommandBuffer));
    canvas_s.render_pass = render_pass;
    VkyCanvas* canvas = &canvas_s;

    VkImage image = {0};
    VkDeviceMemory image_memory = {0};

    create_image(
        device, width, height, 1, canvas->image_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &image,
        &image_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, canvas->gpu->memory_properties);

    VkImageView image_view = create_image_view(
        device, image, VK_IMAGE_VIEW_TYPE_2D, canvas->image_format, VK_IMAGE_ASPECT_COLOR_BIT);

    // Create the depth objects.
    VkImage depth_image = {0};
    VkImageView depth_image_view = {0};
    VkDeviceMemory depth_image_memory = {0};

    create_image(
        device, width, height, 1, canvas->depth_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &depth_image, &depth_image_memory,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, canvas->gpu->memory_properties);

    // Create depth image view.
    depth_image_view = create_image_view(
        device, depth_image, VK_IMAGE_VIEW_TYPE_2D, canvas->depth_format,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create the frame buffer.
    VkFramebuffer framebuffer = {0};
    // Create FrameBuffer
    VkImageView attachments[] = {image_view, depth_image_view};

    VkFramebufferCreateInfo framebuffer_info = {0};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = canvas->render_pass;
    framebuffer_info.attachmentCount = 2;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    framebuffer_info.layers = 1;

    VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebuffer_info, NULL, &framebuffer));

    // Fill the VkyCanvas struct.
    canvas->size.framebuffer_width = width;
    canvas->size.framebuffer_height = height;
    canvas->depth_image = depth_image;
    canvas->depth_image_view = depth_image_view;
    canvas->depth_image_memory = depth_image_memory;

    // One per swap image.
    canvas->images = malloc(sizeof(VkImage));
    canvas->images[0] = image;
    canvas->image_views = malloc(sizeof(VkImageView));
    canvas->image_views[0] = image_view;
    canvas->image_memory = image_memory;
    canvas->framebuffers = malloc(sizeof(VkFramebuffer));
    canvas->framebuffers[0] = framebuffer;

    // Create the command buffers.
    vky_create_command_buffers(canvas->gpu, 2 * canvas->image_count, canvas->command_buffers);
    canvas->live_command_buffers = &canvas->command_buffers[canvas->image_count];

    vky_create_shared_objects(canvas->gpu);

    VkyCanvas* canvas_ptr = calloc(1, sizeof(VkyCanvas));
    *canvas_ptr = canvas_s;
    return canvas_ptr;
}

void vky_offscreen_frame(VkyCanvas* canvas, double time)
{

    // Command buffers to submit.
    uint32_t cmd_buf_count = canvas->cb_fill_live_command_buffer == NULL ? 1 : 2;
    ASSERT(cmd_buf_count > 0);
    ASSERT(cmd_buf_count <= 100);
    VkCommandBuffer submit_cmd_bufs[100];

    // The main command buffer.
    submit_cmd_bufs[0] = canvas->command_buffers[canvas->image_index];

    // Also, the live command buffer if there is one.
    if (canvas->cb_fill_live_command_buffer != NULL)
    {
        submit_cmd_bufs[1] = canvas->live_command_buffers[canvas->image_index];
    }

    // Update the local time.
    canvas->dt = time - canvas->local_time; // time since last frame
    canvas->local_time = time;

    // Update the mouse/keyboard states, and call the user callbacks.
    vky_next_frame(canvas);

    // Fill the live command buffer (currently used by Dear ImGui).
    vky_fill_live_command_buffers(canvas);

    // Submit the graphics command buffers.
    vky_submit_command_buffers(canvas, cmd_buf_count, submit_cmd_bufs);

    // Submit the compute command buffer.
    vky_compute_submit(canvas->gpu);

    vkQueueWaitIdle(canvas->gpu->graphics_queue);
}



/*************************************************************************************************/
/*  Screenshot                                                                                   */
/*************************************************************************************************/

VkyScreenshot* vky_create_screenshot(VkyCanvas* canvas)
{
    // log_trace("create screenshot");

    VkyGpu* gpu = canvas->gpu;
    VkDevice device = gpu->device;
    VkCommandPool command_pool = gpu->command_pool;

    uint32_t width = canvas->size.framebuffer_width;
    uint32_t height = canvas->size.framebuffer_height;

    VkImage dstImage;
    VkDeviceMemory dstImageMemory;

    create_image(
        device, width, height, 1, canvas->image_format, VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT, &dstImage, &dstImageMemory,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        gpu->memory_properties);

    // Create a command buffer.
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buf = {0};
    vkAllocateCommandBuffers(device, &alloc_info, &cmd_buf);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd_buf, &begin_info);

    // Transition screenshot image.
    add_image_transition(
        cmd_buf, dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
        VK_ACCESS_TRANSFER_WRITE_BIT);

    // Transition swapchain image.
    if (!canvas->is_offscreen)
    {
        add_image_transition(
            cmd_buf, canvas->images[canvas->image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT,
            VK_ACCESS_TRANSFER_READ_BIT);
    }

    // Copy swapchain => screenshot image.
    VkImageCopy imageCopyRegion = {0};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = width;
    imageCopyRegion.extent.height = height;
    imageCopyRegion.extent.depth = 1;
    vkCmdCopyImage(
        cmd_buf, canvas->images[canvas->image_index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    // Transition back swapchain image.
    if (!canvas->is_offscreen)
    {
        add_image_transition(
            cmd_buf, canvas->images[canvas->image_index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_MEMORY_READ_BIT);
    }

    // Transition back screenshot image.
    add_image_transition(
        cmd_buf, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);

    // Prepare the submission but do not submit yet.
    vkEndCommandBuffer(cmd_buf);

    // Create the screenshot instance.
    VkyScreenshot screenshot = {0};
    screenshot.canvas = canvas;
    screenshot.dstImage = dstImage;
    screenshot.dstImageMemory = dstImageMemory;
    screenshot.width = width;
    screenshot.height = height;
    screenshot.cmd_buf = cmd_buf;

    VkImageSubresource subResource = {0};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout = {0};
    vkGetImageSubresourceLayout(device, screenshot.dstImage, &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    vkMapMemory(device, screenshot.dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&screenshot.image);
    screenshot.image += subResourceLayout.offset;
    screenshot.row_pitch = subResourceLayout.rowPitch;

    VkyScreenshot* screenshot_ptr = malloc(sizeof(VkyScreenshot));
    memcpy(screenshot_ptr, &screenshot, sizeof(VkyScreenshot));
    return screenshot_ptr;
}

void vky_begin_screenshot(VkyScreenshot* screenshot)
{
    // log_trace("begin screenshot");
    VkyGpu* gpu = screenshot->canvas->gpu;

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &screenshot->cmd_buf;

    vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(gpu->graphics_queue);
}

void vky_end_screenshot(VkyScreenshot* screenshot)
{
    // log_trace("end screenshot");
}

void vky_destroy_screenshot(VkyScreenshot* screenshot)
{
    // log_trace("destroy screenshot");
    VkDevice device = screenshot->canvas->gpu->device;

    // Clean up resources
    vkUnmapMemory(device, screenshot->dstImageMemory);
    vkFreeMemory(device, screenshot->dstImageMemory, NULL);
    vkDestroyImage(device, screenshot->dstImage, NULL);
    vkFreeCommandBuffers(device, screenshot->canvas->gpu->command_pool, 1, &screenshot->cmd_buf);

    FREE(screenshot);
}

uint8_t* vky_screenshot_to_rgb(VkyScreenshot* screenshot, bool swizzle)
{
    // NOTE: the caller must free the output.
    uint32_t w = screenshot->width;
    uint32_t h = screenshot->height;
    // First, memcopy from the GPU to the CPU.
    uint8_t* image = calloc(w * h, 4);
    uint8_t* image_orig = image;
    memcpy(image, screenshot->image, w * h * 4);

    uint8_t* rgb = calloc(w * h, 3);

    // Then, swizzle.
    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;
    for (uint32_t y = 0; y < h; y++)
    {
        src_offset = 0;
        for (uint32_t x = 0; x < w; x++)
        {
            if (swizzle)
            {
                rgb[dst_offset + 0] = image[src_offset + 2];
                rgb[dst_offset + 1] = image[src_offset + 1];
                rgb[dst_offset + 2] = image[src_offset + 0];
            }
            else
            {
                rgb[dst_offset + 0] = image[src_offset + 0];
                rgb[dst_offset + 1] = image[src_offset + 1];
                rgb[dst_offset + 2] = image[src_offset + 2];
            }
            src_offset += 4;
            dst_offset += 3;
        }
        image += screenshot->row_pitch;
    }
    ASSERT(dst_offset == w * h * 3);
    FREE(image_orig);
    return rgb;
}

void vky_save_screenshot(VkyCanvas* canvas, char* filename)
{
    // Make a screenshot and save to a PPM (fast, uncompressed) or PNG (slow, compressed).
    log_trace("saving screenshot to %s.", filename);
    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    vky_begin_screenshot(screenshot);
    uint8_t* image = vky_screenshot_to_rgb(screenshot, true);
    // Write PNG or PPM image.
    if (strstr(filename, ".png") != NULL)
    {
        if (write_png(filename, screenshot->width, screenshot->height, image) != 0)
        {
            log_warn("could not save screenshot %s", filename);
        }
        else
        {
            log_info("saved screenshot %s", filename);
        }
    }
    else if (strstr(filename, ".ppm") != NULL)
    {
        write_ppm(filename, screenshot->width, screenshot->height, image);
        log_info("saved screenshot %s", filename);
    }
    else
    {
        log_error("Unknown image format, skip saving of %s.", filename);
    }
    FREE(image);
    vky_end_screenshot(screenshot);
    vky_destroy_screenshot(screenshot);
}



/*************************************************************************************************/
/*  Video rendering                                                                              */
/*************************************************************************************************/

static VkyVideo* _create_video(VkyCanvas* canvas, const char* filename, int fps, int bitrate)
{
    ASSERT(canvas->app != NULL);
    if (!canvas->is_offscreen)
    {
        log_error("Video recording is only supported on offscreen canvases at the moment.");
        return NULL;
    }
    log_trace(
        "create video %s %dx%d, fps=%d, bitrate=%d", filename, canvas->size.framebuffer_width,
        canvas->size.framebuffer_height, fps, bitrate);
    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    Video* video = create_video(
        filename, (int)canvas->size.framebuffer_width, (int)canvas->size.framebuffer_height, fps,
        bitrate);

    VkyVideo* vky_video = calloc(1, sizeof(VkyVideo));
    vky_video->canvas = canvas;
    vky_video->screenshot = screenshot;
    vky_video->video = video;

    // Buffer
    vky_video->image_size = canvas->size.framebuffer_width * canvas->size.framebuffer_height * 4;
    vky_video->buffer_frame_count = 1; // only a single image in the buffer
    vky_video->buffer = malloc(vky_video->image_size * vky_video->buffer_frame_count);
    vky_video->current_frame_index = 0;

    return vky_video;
}

static void _add_frame(VkyVideo* vky_video)
{
    // log_trace("add video frame");
    VkyScreenshot* screenshot = vky_video->screenshot;
    vky_begin_screenshot(screenshot);

    // add_frame(vky_video->video, screenshot->image);
    // Copy the frame to the buffer.
    memcpy(
        &vky_video->buffer[vky_video->current_frame_index * vky_video->image_size],
        screenshot->image, vky_video->image_size);

    vky_end_screenshot(screenshot);

    vky_video->current_frame_index++;
    // Flush the buffer and write to the video file.
    if (vky_video->current_frame_index == vky_video->buffer_frame_count)
    {
        for (uint32_t i = 0; i < vky_video->current_frame_index; i++)
        {
            add_frame(vky_video->video, &vky_video->buffer[i * vky_video->image_size]);
        }
        vky_video->current_frame_index = 0;
    }
}

static void _end_video(VkyVideo* vky_video)
{
    log_trace("end video");
    // Finish flushing the video.
    for (uint32_t i = 0; i < vky_video->current_frame_index; i++)
    {
        add_frame(vky_video->video, &vky_video->buffer[i * vky_video->image_size]);
    }

    end_video(vky_video->video);
    vky_destroy_screenshot(vky_video->screenshot);
    FREE(vky_video->buffer);
    FREE(vky_video);
}

void vky_create_video(
    VkyCanvas* canvas, const char* filename, double duration, int fps, int bitrate)
{
    uint32_t frame_count = round(fps * duration);
    // Create the video.
    VkyVideo* video = _create_video(canvas, filename, fps, bitrate);
    // Fill the command buffer.
    vky_fill_command_buffers(canvas);
    for (uint32_t i = 0; i < frame_count; i++)
    {
        if (video->video == NULL)
        {
            log_error("could not create video %s", filename);
            break;
        }
        printf("\rCreating video: %.1f%%", 100 * (float)i / frame_count);
        fflush(stdout);
        vky_offscreen_frame(canvas, (double)i / fps);
        _add_frame(video);
        canvas->frame_count++;
    }
    _end_video(video);
}



/*************************************************************************************************/
/*  Offscreen backend                                                                            */
/*************************************************************************************************/

void vky_run_offscreen_app(VkyApp* app)
{
    // NOTE: only 1 canvas is supported here.
    VkyCanvas* canvas = app->canvases[0];
    vky_fill_command_buffers(canvas);
    // vky_submit_command_buffer_offscreen(canvas);
    vky_offscreen_frame(canvas, VKY_TIME);

    // Event loop.
    while (!canvas->to_close)
    {
        // vky_next_frame(canvas);
        // vky_submit_command_buffer_offscreen(canvas);
        vky_offscreen_frame(canvas, VKY_TIME);
        canvas->frame_count++;
    }
}



/*************************************************************************************************/
/*  Screenshot backend                                                                           */
/*************************************************************************************************/

void vky_run_screenshot_app(VkyApp* app)
{
    // NOTE: only 1 canvas is supported here.
    VkyCanvas* canvas = app->canvases[0];
    VkyBackendScreenshotParams* params = (VkyBackendScreenshotParams*)app->backend_params;
    ASSERT(params != NULL);

    vky_fill_command_buffers(canvas);
    vky_offscreen_frame(canvas, VKY_TIME);

    // Event loop.
    while (!canvas->to_close)
    {
        vky_offscreen_frame(canvas, VKY_TIME);
        // Save screenshot for the requested frame index.
        if (canvas->frame_count == params->frame_index)
        {
            vky_save_screenshot(canvas, params->filename);
            canvas->to_close = true;
        }
        canvas->frame_count++;
    }
}
