/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas swapchain sink                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if OS_UNIX
#include <unistd.h>
#endif

#include "../vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "datoviz/vklite/sync.h"

static int canvas_export_timeline_fd(DvzCanvas* canvas);

#include "swizzle_frag_spirv.h"
#include "swizzle_vert_spirv.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAX_SURFACE_FORMATS 32



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCanvasSwapchainSlot DvzCanvasSwapchainSlot;
typedef struct DvzCanvasSwapchainState DvzCanvasSwapchainState;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCanvasSwapchainSlot
{
    VkImage offscreen_image;
    VkImageView offscreen_view;
    VkImage swapchain_image;
    VkImageView swapchain_view;
    DvzAllocation offscreen_alloc;
    DvzSemaphore image_available;
    DvzSemaphore render_finished;
    DvzFence in_flight;
    VkCommandBuffer command_buffer;
    VkImageLayout offscreen_layout;
    VkImageLayout swapchain_layout;
    uint32_t image_index;
    int memory_fd;
    bool ready;
    bool commands_recording;
    bool handles_dirty;
    DvzDescriptors swizzle_descriptors;
    bool swizzle_descriptor_ready;
};



struct DvzCanvasSwapchain
{
    DvzCanvas* canvas;
    VkSwapchainKHR handle;
    VkFormat format;
    VkColorSpaceKHR color_space;
    VkExtent2D extent;
    VkPresentModeKHR present_mode;
    uint32_t image_count;
    DvzCanvasSwapchainSlot* slots;
    VkImage* swapchain_images;
    VkImageLayout* swapchain_layouts;
    uint32_t frame_index;
    bool dirty;
    VkQueue queue;
    DvzQueue* queue_ref;
    DvzCanvasSwapchainSlot* active_slot;
    VkCommandPool command_pool;
    uint32_t queue_family;
    uint64_t export_serial;
    DvzGraphics swizzle_graphics;
    DvzSlots swizzle_slots;
    DvzDescriptors swizzle_descriptors;
    DvzSampler swizzle_sampler;
    DvzShader swizzle_vs;
    DvzShader swizzle_fs;
    bool swizzle_ready;
};



struct DvzCanvasSwapchainState
{
    DvzCanvas* canvas;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static VkDevice canvas_device_handle(const DvzCanvas* canvas)
{
    ANN(canvas);
    return dvz_device_handle(canvas->device);
}



static DvzGpu* canvas_gpu(const DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->device);
    return canvas->device->gpu;
}



static VkSurfaceKHR canvas_surface_handle(DvzCanvas* canvas)
{
    ANN(canvas);
    dvz_canvas_window_surface_refresh(canvas);
    return canvas->surface ? canvas->surface->surface : VK_NULL_HANDLE;
}



static VkExtent2D
canvas_surface_extent(const DvzCanvas* canvas, const VkSurfaceCapabilitiesKHR* caps)
{
    ANN(canvas);
    VkExtent2D extent = canvas->surface ? canvas->surface->extent : (VkExtent2D){0, 0};
    if (caps)
    {
        if (extent.width < caps->minImageExtent.width)
            extent.width = caps->minImageExtent.width;
        if (extent.height < caps->minImageExtent.height)
            extent.height = caps->minImageExtent.height;
        if (extent.width > caps->maxImageExtent.width)
            extent.width = caps->maxImageExtent.width;
        if (extent.height > caps->maxImageExtent.height)
            extent.height = caps->maxImageExtent.height;
    }
    return extent;
}



static VkFormat canvas_surface_format(const DvzCanvas* canvas)
{
    if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        return canvas->cfg.color_format;
    }
    if (canvas->surface && canvas->surface->format != VK_FORMAT_UNDEFINED)
    {
        return canvas->surface->format;
    }
    return VK_FORMAT_B8G8R8A8_UNORM;
}



static VkPresentModeKHR canvas_select_present_mode(DvzCanvas* canvas)
{
    ANN(canvas);

    DvzGpu* gpu = canvas_gpu(canvas);
    VkSurfaceKHR surface = canvas_surface_handle(canvas);
    if (!gpu || surface == VK_NULL_HANDLE)
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    uint32_t count = 0;
    VK_CHECK_RESULT(
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->pdevice, surface, &count, NULL));
    if (count == 0)
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkPresentModeKHR* modes = (VkPresentModeKHR*)dvz_calloc(count, sizeof(VkPresentModeKHR));
    ANN(modes);
    VK_CHECK_RESULT(
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->pdevice, surface, &count, modes));

    VkPresentModeKHR resolved = VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR requested = canvas->cfg.present_mode;
    bool supported = false;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (modes[i] == requested)
        {
            supported = true;
            break;
        }
    }

    if (supported)
    {
        resolved = requested;
    }
    else if (requested != VK_PRESENT_MODE_FIFO_KHR)
    {
        log_warn("canvas present mode %d unsupported, falling back to FIFO", requested);
    }

    dvz_free(modes);
    return resolved;
}



static VkPipelineStageFlags2 canvas_stage_for_layout(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_2_NONE;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    default:
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    }
}



static VkAccessFlags2 canvas_access_for_layout(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_2_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_2_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return 0;
    default:
        return 0;
    }
}



static void canvas_cmd_transition(
    VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    if (old_layout == new_layout || cmd == VK_NULL_HANDLE || image == VK_NULL_HANDLE)
        return;

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = canvas_stage_for_layout(old_layout),
        .srcAccessMask = canvas_access_for_layout(old_layout),
        .dstStageMask = canvas_stage_for_layout(new_layout),
        .dstAccessMask = canvas_access_for_layout(new_layout),
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkDependencyInfo dep = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &dep);
}



static void canvas_cmd_transition_swapchain(
    VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    if (old_layout == new_layout || cmd == VK_NULL_HANDLE || image == VK_NULL_HANDLE)
        return;

    VkPipelineStageFlags2 release_stage =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = release_stage, // same as pWaitDstStageMask
        .srcAccessMask = canvas_access_for_layout(old_layout),
        .dstStageMask = canvas_stage_for_layout(new_layout),
        .dstAccessMask = canvas_access_for_layout(new_layout),
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkDependencyInfo dep = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &dep);
}



static VkResult
canvas_slot_create_swapchain_view(DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot)
{
    if (!swapchain || !slot || slot->swapchain_image == VK_NULL_HANDLE)
    {
        return VK_SUCCESS;
    }
    if (slot->swapchain_view != VK_NULL_HANDLE)
    {
        return VK_SUCCESS;
    }
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = slot->swapchain_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain->format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    VkDevice device = canvas_device_handle(swapchain->canvas);
    VkResult res = vkCreateImageView(device, &info, NULL, &slot->swapchain_view);
    if (res != VK_SUCCESS)
    {
        log_error("failed to create swapchain image view (%d)", res);
        slot->swapchain_view = VK_NULL_HANDLE;
        return res;
    }
    if (!slot->swizzle_descriptor_ready && swapchain->swizzle_ready)
    {
        dvz_descriptors(&swapchain->swizzle_slots, &slot->swizzle_descriptors);
        dvz_descriptors_image(
            &slot->swizzle_descriptors, 0, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            slot->offscreen_view, swapchain->swizzle_sampler.vk_sampler);
        slot->swizzle_descriptor_ready = true;
    }
    return res;
}



static void canvas_swapchain_swizzle_destroy(DvzCanvasSwapchain* swapchain)
{
    if (!swapchain)
    {
        return;
    }
    log_trace("destroy swapchain swizzle");
    dvz_graphics_destroy(&swapchain->swizzle_graphics);
    dvz_shader_destroy(&swapchain->swizzle_vs);
    dvz_shader_destroy(&swapchain->swizzle_fs);
    dvz_sampler_destroy(&swapchain->swizzle_sampler);
    dvz_slots_destroy(&swapchain->swizzle_slots);
    swapchain->swizzle_ready = false;
}



static int canvas_swapchain_swizzle_create(DvzCanvasSwapchain* swapchain)
{
    if (!swapchain || !swapchain->canvas || !swapchain->canvas->device)
    {
        return -1;
    }
    // if (swapchain->handle != VK_NULL_HANDLE)
    //     canvas_swapchain_swizzle_destroy(swapchain);
    DvzDevice* device = swapchain->canvas->device;
    dvz_slots(device, &swapchain->swizzle_slots);
    dvz_slots_binding(
        &swapchain->swizzle_slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    if (dvz_slots_create(&swapchain->swizzle_slots) != 0)
    {
        return -1;
    }
    dvz_descriptors(&swapchain->swizzle_slots, &swapchain->swizzle_descriptors);

    dvz_sampler(device, &swapchain->swizzle_sampler);
    dvz_sampler_min_filter(&swapchain->swizzle_sampler, VK_FILTER_NEAREST);
    dvz_sampler_mag_filter(&swapchain->swizzle_sampler, VK_FILTER_NEAREST);
    dvz_sampler_address_mode(
        &swapchain->swizzle_sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(
        &swapchain->swizzle_sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(
        &swapchain->swizzle_sampler, DVZ_SAMPLER_AXIS_W, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    if (dvz_sampler_create(&swapchain->swizzle_sampler) != 0)
    {
        log_error("unable to create swizzle sampler");
        canvas_swapchain_swizzle_destroy(swapchain);
        return -1;
    }

    if (dvz_shader(
            device, (DvzSize)dvz_canvas_swizzle_vert_spv_size,
            (const uint32_t*)dvz_canvas_swizzle_vert_spv, &swapchain->swizzle_vs) != 0)
    {
        log_error("unable to create swizzle vertex sampler");
        canvas_swapchain_swizzle_destroy(swapchain);
        return -1;
    }
    if (dvz_shader(
            device, (DvzSize)dvz_canvas_swizzle_frag_spv_size,
            (const uint32_t*)dvz_canvas_swizzle_frag_spv, &swapchain->swizzle_fs) != 0)
    {
        log_error("unable to create swizzle fragment sampler");
        canvas_swapchain_swizzle_destroy(swapchain);
        return -1;
    }

    dvz_graphics(device, &swapchain->swizzle_graphics);
    dvz_graphics_shader(
        &swapchain->swizzle_graphics, VK_SHADER_STAGE_VERTEX_BIT,
        dvz_shader_handle(&swapchain->swizzle_vs));
    dvz_graphics_shader(
        &swapchain->swizzle_graphics, VK_SHADER_STAGE_FRAGMENT_BIT,
        dvz_shader_handle(&swapchain->swizzle_fs));
    dvz_graphics_attachment_color(&swapchain->swizzle_graphics, 0, swapchain->format);
    dvz_graphics_primitive(
        &swapchain->swizzle_graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_viewport(
        &swapchain->swizzle_graphics, 0.0f, 0.0f, (float)swapchain->extent.width,
        (float)swapchain->extent.height, 0.0f, 1.0f, 0);
    dvz_graphics_scissor(
        &swapchain->swizzle_graphics, 0, 0, swapchain->extent.width, swapchain->extent.height, 0);
    swapchain->swizzle_graphics.layout = dvz_slots_handle(&swapchain->swizzle_slots);
    if (dvz_graphics_create(&swapchain->swizzle_graphics) != 0)
    {
        canvas_swapchain_swizzle_destroy(swapchain);
        return -1;
    }

    swapchain->swizzle_ready = true;
    return 0;
}



static void canvas_swapchain_swizzle_run(
    DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot, VkCommandBuffer cmd)
{
    if (!swapchain || !slot || !cmd || !swapchain->swizzle_ready ||
        slot->swapchain_view == VK_NULL_HANDLE || slot->offscreen_view == VK_NULL_HANDLE ||
        swapchain->extent.width == 0 || swapchain->extent.height == 0)
    {
        return;
    }
    DvzCommands cmds = {0};
    cmds.cmds[0] = cmd;
    cmds.count = 1;
    cmds.current = 0;

    DvzRendering rendering = {0};
    dvz_rendering(&rendering);
    dvz_rendering_area(&rendering, 0, 0, swapchain->extent.width, swapchain->extent.height);
    DvzAttachment* catt = dvz_rendering_color(&rendering, 0);
    dvz_attachment_image(catt, slot->swapchain_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(catt, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

    dvz_cmd_rendering_begin(&cmds, &rendering);
    dvz_cmd_bind_graphics(&cmds, &swapchain->swizzle_graphics);
    if (!slot->swizzle_descriptor_ready)
    {
        log_error("swapchain slot missing swizzle descriptor");
        dvz_cmd_rendering_end(&cmds);
        return;
    }
    dvz_cmd_bind_descriptors(
        &cmds, VK_PIPELINE_BIND_POINT_GRAPHICS, &slot->swizzle_descriptors, 0, 1, 0, NULL);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    dvz_cmd_rendering_end(&cmds);
}



static int canvas_slot_begin_recording(DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot)
{
    ANN(swapchain);
    ANN(slot);
    VkCommandBuffer cmd = slot->command_buffer;
    // log_trace("canvas_slot_begin_recording");
    if (cmd == VK_NULL_HANDLE)
    {
        log_error("canvas swapchain slot missing command buffer");
        return -1;
    }
    VK_CHECK_RESULT(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin_info));
    canvas_cmd_transition(
        cmd, slot->offscreen_image, slot->offscreen_layout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    slot->offscreen_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    slot->commands_recording = true;
    return 0;
}



static int
canvas_slot_finish_recording(DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot)
{
    ANN(swapchain);
    ANN(slot);
    // log_trace("canvas_slot_finish_recording");
    if (!slot->commands_recording || slot->command_buffer == VK_NULL_HANDLE)
    {
        return 0;
    }
    if (slot->swapchain_image == VK_NULL_HANDLE)
    {
        log_error("canvas slot missing swapchain image");
        slot->commands_recording = false;
        return -1;
    }
    VkCommandBuffer cmd = slot->command_buffer;

    canvas_cmd_transition(
        cmd, slot->offscreen_image, slot->offscreen_layout,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    slot->offscreen_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    canvas_cmd_transition_swapchain(
        cmd, slot->swapchain_image, slot->swapchain_layout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    slot->swapchain_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    canvas_swapchain_swizzle_run(swapchain, slot, cmd);

    canvas_cmd_transition(
        cmd, slot->offscreen_image, slot->offscreen_layout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    slot->offscreen_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    canvas_cmd_transition_swapchain(
        cmd, slot->swapchain_image, slot->swapchain_layout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    slot->swapchain_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // log_trace("end command buffer");
    VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

    slot->commands_recording = false;
    if (swapchain->swapchain_layouts && slot->image_index < swapchain->image_count)
    {
        swapchain->swapchain_layouts[slot->image_index] = slot->swapchain_layout;
    }
    return 0;
}



static VkResult canvas_create_swapchain(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    DvzGpu* gpu = canvas_gpu(canvas);
    ANN(gpu);

    VkSurfaceKHR surface = canvas_surface_handle(canvas);
    if (surface == VK_NULL_HANDLE)
    {
        log_warn("canvas surface unavailable, postponing swapchain creation");
        swapchain->dirty = true;
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    VkSurfaceCapabilitiesKHR caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->pdevice, surface, &caps);
    VkExtent2D extent = canvas_surface_extent(canvas, &caps);
    if (extent.width == 0 || extent.height == 0)
    {
        log_warn("window surface extent is zero, waiting before creating swapchain");
        swapchain->dirty = true;
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    uint32_t min_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && min_count > caps.maxImageCount)
    {
        min_count = caps.maxImageCount;
    }


    // Enumerate the formats supported by the surface.
    uint32_t fcount = 0;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->pdevice, surface, &fcount, NULL));
    if (fcount == 0)
        return -1;
    if (fcount > MAX_SURFACE_FORMATS)
        fcount = MAX_SURFACE_FORMATS;

    VkSurfaceFormatKHR formats[MAX_SURFACE_FORMATS] = {0};
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->pdevice, surface, &fcount, formats));


    VkPresentModeKHR present_mode = canvas_select_present_mode(canvas);

    VkSwapchainCreateInfoKHR info = {0};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = min_count;
    info.imageFormat = canvas_surface_format(canvas);
    info.imageColorSpace =
        canvas->surface ? canvas->surface->color_space : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = swapchain->handle;

    VkDevice device = canvas_device_handle(canvas);
    VkResult swapchain_res = vkCreateSwapchainKHR(device, &info, NULL, &swapchain->handle);
    if (swapchain_res != VK_SUCCESS)
    {
        log_error("failed to create swapchain (%d)", swapchain_res);
        return swapchain_res;
    }

    if (info.oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, info.oldSwapchain, NULL);
    }

    swapchain->format = info.imageFormat;
    swapchain->color_space = info.imageColorSpace;
    swapchain->extent = extent;
    swapchain->present_mode = present_mode;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(device, swapchain->handle, &count, NULL);
    VkImage* images = (VkImage*)dvz_calloc(count, sizeof(VkImage));
    ANN(images);
    vkGetSwapchainImagesKHR(device, swapchain->handle, &count, images);
    if (swapchain->swapchain_images)
    {
        dvz_free(swapchain->swapchain_images);
    }
    swapchain->swapchain_images = images;
    if (swapchain->swapchain_layouts)
    {
        dvz_free(swapchain->swapchain_layouts);
    }
    swapchain->swapchain_layouts = (VkImageLayout*)dvz_calloc(count, sizeof(VkImageLayout));
    ANN(swapchain->swapchain_layouts);
    for (uint32_t i = 0; i < count; ++i)
    {
        swapchain->swapchain_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    dvz_free(swapchain->slots);
    swapchain->slots = (DvzCanvasSwapchainSlot*)dvz_calloc(count, sizeof(DvzCanvasSwapchainSlot));
    ANN(swapchain->slots);
    swapchain->image_count = count;
    swapchain->active_slot = NULL;
    swapchain->export_serial++;

    dvz_canvas_frame_pool_init(&canvas->frame_pool, swapchain->image_count);


    for (uint32_t i = 0; i < count; ++i)
    {
        DvzCanvasSwapchainSlot* slot = &swapchain->slots[i];
        dvz_memset(slot, sizeof(*slot), 0, sizeof(*slot));
        slot->offscreen_view = VK_NULL_HANDLE;
        slot->swapchain_image = VK_NULL_HANDLE;
        slot->swapchain_view = VK_NULL_HANDLE;
        slot->image_index = UINT32_MAX;
        slot->offscreen_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        slot->swapchain_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        slot->handles_dirty = false;
        slot->commands_recording = false;
        slot->memory_fd = -1;
        slot->swizzle_descriptor_ready = false;

        if (swapchain->command_pool == VK_NULL_HANDLE)
        {
            log_error("canvas swapchain missing command pool");
            continue;
        }
        VkCommandBufferAllocateInfo cb_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = swapchain->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cb_info, &slot->command_buffer));

        VkExternalMemoryImageCreateInfoKHR external_info = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR};
        VkImageCreateInfo img_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = DVZ_DEFAULT_COLOR_FORMAT,
            .extent =
                {
                    .width = extent.width,
                    .height = extent.height,
                    .depth = 1,
                },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .pNext = NULL,
        };
        bool use_external = canvas->allocator.external != 0;
        if (use_external)
        {
            external_info.handleTypes = canvas->allocator.external;
            img_info.pNext = &external_info;
        }

        if (dvz_allocator_image(
                &canvas->allocator, &img_info, 0, &slot->offscreen_alloc,
                &slot->offscreen_image) != 0)
        {
            log_error("failed to allocate offscreen canvas image");
            continue;
        }

        VkImageViewCreateInfo offscreen_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = slot->offscreen_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = DVZ_DEFAULT_COLOR_FORMAT,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
        VkResult view_res =
            vkCreateImageView(device, &offscreen_view_info, NULL, &slot->offscreen_view);
        if (view_res != VK_SUCCESS)
        {
            log_error("failed to create offscreen image view (%d)", view_res);
            dvz_allocator_destroy_image(
                &canvas->allocator, &slot->offscreen_alloc, slot->offscreen_image);
            slot->offscreen_image = VK_NULL_HANDLE;
            continue;
        }

        slot->memory_fd = -1;
        if (use_external && dvz_allocator_export(
                                &canvas->allocator, &slot->offscreen_alloc, &slot->memory_fd) != 0)
        {
            log_warn("failed to export canvas render target");
            slot->memory_fd = -1;
        }

        dvz_semaphore(canvas->device, &slot->image_available);
        dvz_semaphore(canvas->device, &slot->render_finished);
        dvz_fence(canvas->device, true, &slot->in_flight);
        slot->ready = true;
    }

    if (canvas_swapchain_swizzle_create(swapchain) != 0)
    {
        log_error("failed to initialize canvas swizzle pass");
        canvas_swapchain_swizzle_destroy(swapchain);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    swapchain->dirty = false;
    return VK_SUCCESS;
}



static void canvas_destroy_slot(
    VkDevice device, VkCommandPool command_pool, DvzCanvasSwapchainSlot* slot, DvzVma* allocator)
{
    if (!slot)
    {
        return;
    }
    if (slot->offscreen_view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, slot->offscreen_view, NULL);
        slot->offscreen_view = VK_NULL_HANDLE;
    }
    if (slot->swapchain_view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, slot->swapchain_view, NULL);
        slot->swapchain_view = VK_NULL_HANDLE;
    }
    if (slot->offscreen_image != VK_NULL_HANDLE)
    {
        dvz_allocator_destroy_image(allocator, &slot->offscreen_alloc, slot->offscreen_image);
        slot->offscreen_image = VK_NULL_HANDLE;
    }
    dvz_semaphore_destroy(&slot->image_available);
    dvz_semaphore_destroy(&slot->render_finished);
    dvz_fence_destroy(&slot->in_flight);
#if OS_UNIX
    if (slot->memory_fd >= 0)
    {
        close(slot->memory_fd);
        slot->memory_fd = -1;
    }
#endif
    slot->ready = false;
    slot->swapchain_image = VK_NULL_HANDLE;
    slot->commands_recording = false;
    slot->handles_dirty = false;
    if (slot->command_buffer != VK_NULL_HANDLE && command_pool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device, command_pool, 1, &slot->command_buffer);
        slot->command_buffer = VK_NULL_HANDLE;
    }
}



static void canvas_swapchain_cleanup(DvzCanvasSwapchain* swapchain)
{
    if (!swapchain)
    {
        return;
    }
    canvas_swapchain_swizzle_destroy(swapchain);
    VkDevice device = canvas_device_handle(swapchain->canvas);
    vkDeviceWaitIdle(device);
    if (swapchain->slots)
    {
        for (uint32_t i = 0; i < swapchain->image_count; ++i)
        {
            canvas_destroy_slot(
                device, swapchain->command_pool, &swapchain->slots[i],
                &swapchain->canvas->allocator);
        }
        dvz_free(swapchain->slots);
        swapchain->slots = NULL;
    }
    if (swapchain->handle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapchain->handle, NULL);
        swapchain->handle = VK_NULL_HANDLE;
    }
    if (swapchain->swapchain_images)
    {
        dvz_free(swapchain->swapchain_images);
        swapchain->swapchain_images = NULL;
    }
    if (swapchain->swapchain_layouts)
    {
        dvz_free(swapchain->swapchain_layouts);
        swapchain->swapchain_layouts = NULL;
    }
    swapchain->image_count = 0;
    swapchain->active_slot = NULL;
    swapchain->frame_index = 0;
    swapchain->dirty = true;
}



static DvzCanvasSwapchain* canvas_state(DvzCanvas* canvas)
{
    ANN(canvas);
    return canvas->swapchain;
}



static DvzCanvasSwapchainState* canvas_swapchain_sink_state(DvzStreamSink* sink)
{
    ANN(sink);
    return (DvzCanvasSwapchainState*)sink->backend_data;
}



static int canvas_swapchain_ensure(DvzCanvas* canvas)
{
    ANN(canvas);
    DvzCanvasSwapchain* state = canvas_state(canvas);
    if (!state)
    {
        return -1;
    }
    if (!state->dirty && state->handle != VK_NULL_HANDLE)
    {
        return 0;
    }

    canvas_swapchain_cleanup(state);
    VkResult res = canvas_create_swapchain(state);
    if (res == VK_SUCCESS)
    {
        return 0;
    }
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_ERROR_SURFACE_LOST_KHR)
    {
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    return -1;
}



/*************************************************************************************************/
/*  Swapchain API                                                                                */
/*************************************************************************************************/

/**
 * Initialize the swapchain state backing a canvas.
 *
 * @param canvas canvas owning the swapchain
 * @returns 0 on success or -1 on failure
 */
int dvz_canvas_swapchain_init(DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->swapchain)
    {
        return 0;
    }
    canvas->swapchain = (DvzCanvasSwapchain*)dvz_calloc(1, sizeof(DvzCanvasSwapchain));
    ANN(canvas->swapchain);
    canvas->swapchain->canvas = canvas;
    canvas->swapchain->handle = VK_NULL_HANDLE;
    canvas->swapchain->dirty = true;
    canvas->swapchain->frame_index = 0;
    canvas->swapchain->queue_ref = dvz_device_queue(canvas->device, DVZ_QUEUE_MAIN);
    ANN(canvas->swapchain->queue_ref);
    canvas->swapchain->queue = dvz_queue_handle(canvas->swapchain->queue_ref);
    ANNVK(canvas->swapchain->queue);
    canvas->swapchain->queue_family = dvz_queue_family(canvas->swapchain->queue_ref);
    VkDevice device = canvas_device_handle(canvas);
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = canvas->swapchain->queue_family,
    };
    VK_CHECK_RESULT(
        vkCreateCommandPool(device, &pool_info, NULL, &canvas->swapchain->command_pool));
    return 0;
}



/**
 * Destroy the swapchain resources owned by a canvas.
 *
 * @param canvas canvas whose swapchain must be destroyed
 */
void dvz_canvas_swapchain_destroy(DvzCanvas* canvas)
{
    if (!canvas || !canvas->swapchain)
    {
        return;
    }
    VkDevice device = canvas_device_handle(canvas);
    canvas_swapchain_cleanup(canvas->swapchain);
    if (canvas->swapchain->command_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, canvas->swapchain->command_pool, NULL);
        canvas->swapchain->command_pool = VK_NULL_HANDLE;
    }
    dvz_free(canvas->swapchain);
    canvas->swapchain = NULL;
}



/**
 * Mark the swapchain so it gets recreated before the next frame.
 *
 * @param canvas canvas whose swapchain became invalid
 */
void dvz_canvas_swapchain_mark_out_of_date(DvzCanvas* canvas)
{
    if (!canvas || !canvas->swapchain)
    {
        return;
    }
    canvas->swapchain->dirty = true;
}



bool dvz_canvas_swapchain_handles_dirty(const DvzCanvas* canvas)
{
    if (!canvas || !canvas->swapchain || !canvas->swapchain->active_slot)
    {
        return false;
    }
    return canvas->swapchain->active_slot->handles_dirty;
}



void dvz_canvas_swapchain_handles_refreshed(DvzCanvas* canvas)
{
    if (!canvas || !canvas->swapchain || !canvas->swapchain->active_slot)
    {
        return;
    }
    canvas->swapchain->active_slot->handles_dirty = false;
}



/**
 * Acquire the next swapchain image and populate the stream frame metadata.
 *
 * @param canvas canvas owning the swapchain
 * @param frame stream frame structure to populate with exported handles
 * @returns 0 when acquisition succeeds, negative on failure
 */
int dvz_canvas_swapchain_acquire(DvzCanvas* canvas, DvzStreamFrame* frame)
{
    ANN(canvas);
    ANN(frame);
    DvzCanvasSwapchain* state = canvas_state(canvas);
    if (!state)
    {
        return -1;
    }
    if (canvas->surface && (canvas->surface->extent.width != state->extent.width ||
                            canvas->surface->extent.height != state->extent.height ||
                            canvas_surface_format(canvas) != state->format))
    {
        state->dirty = true;
    }

    int ensure_rc = canvas_swapchain_ensure(canvas);
    if (ensure_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
    {
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (ensure_rc != 0)
    {
        return -1;
    }

    if (state->image_count == 0)
    {
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }

    VkDevice device = canvas_device_handle(canvas);
    uint32_t slot_idx = state->frame_index % state->image_count;
    DvzCanvasSwapchainSlot* slot = &state->slots[slot_idx];

    dvz_fence_wait(&slot->in_flight);
    dvz_fence_reset(&slot->in_flight);

    uint32_t image_index = 0;
    VkResult res = vkAcquireNextImageKHR(
        device, state->handle, UINT64_MAX, slot->image_available.vk_semaphore, VK_NULL_HANDLE,
        &image_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
    {
        dvz_canvas_swapchain_mark_out_of_date(canvas);
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (res != VK_SUCCESS)
    {
        log_error("failed to acquire swapchain image (%d)", res);
        return -1;
    }

    state->frame_index = (state->frame_index + 1) % state->image_count;
    if (!slot->ready)
    {
        return -1;
    }
    slot->image_index = image_index;
    if (state->swapchain_images && image_index < state->image_count)
    {
        slot->swapchain_image = state->swapchain_images[image_index];
    }
    else
    {
        slot->swapchain_image = VK_NULL_HANDLE;
    }
    if (canvas_slot_create_swapchain_view(state, slot) != VK_SUCCESS)
    {
        log_error("failed to create swapchain image view");
        return -1;
    }
    if (state->swapchain_layouts && image_index < state->image_count)
    {
        slot->swapchain_layout = state->swapchain_layouts[image_index];
    }
    else
    {
        slot->swapchain_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    if (canvas_slot_begin_recording(state, slot) != 0)
    {
        return -1;
    }

    state->active_slot = slot;
    frame->image = slot->offscreen_image;
    frame->memory = slot->offscreen_alloc.info.deviceMemory;
    frame->memory_size = slot->offscreen_alloc.info.size;
    frame->command_buffer = slot->command_buffer;
    frame->image_view = slot->offscreen_view;
    frame->extent = state->extent;
    frame->handles_dirty = slot->handles_dirty;
    frame->memory_fd = slot->memory_fd;
    frame->wait_semaphore_fd = -1;
    return 0;
}



/**
 * Present the previously-acquired image and signal the timeline semaphore.
 *
 * @param canvas canvas owning the swapchain
 * @param wait_value timeline semaphore value signaled for this frame
 * @returns 0 on success or -1 on failure
 */
int dvz_canvas_swapchain_present(DvzCanvas* canvas, uint64_t wait_value)
{
    ANN(canvas);
    // log_trace("dvz_canvas_swapchain_present");
    DvzCanvasSwapchain* state = canvas_state(canvas);
    if (!state || !state->active_slot)
    {
        return -1;
    }

    if (canvas_slot_finish_recording(state, state->active_slot) != 0)
    {
        state->active_slot = NULL;
        return -1;
    }
    VkCommandBuffer cmd = state->active_slot->command_buffer;

    VkPipelineStageFlags2 wait_stage =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    VkPipelineStageFlags2 signal_stage = wait_stage | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

    DvzSubmit submit = {0};
    dvz_submit(&submit);
    dvz_submit_wait(&submit, state->active_slot->image_available.vk_semaphore, 0, wait_stage);
    if (cmd != VK_NULL_HANDLE)
    {
        dvz_submit_command(&submit, cmd);
    }
    dvz_submit_signal(&submit, state->active_slot->render_finished.vk_semaphore, 0, signal_stage);
    dvz_submit_signal(
        &submit, canvas->timeline_semaphore.vk_semaphore, wait_value,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

    VkQueue queue = state->queue;
    // log_trace("submit");
    dvz_submit_send(&submit, queue, state->active_slot->in_flight.vk_fence);

    uint32_t index = state->active_slot->image_index;
    VkSemaphore present_wait_semaphores[] = {
        state->active_slot->render_finished.vk_semaphore,
    };
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = present_wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &state->handle,
        .pImageIndices = &index,
    };

    // log_trace("present");
    VkResult present_res = vkQueuePresentKHR(queue, &present_info);
    if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR)
    {
        dvz_canvas_swapchain_mark_out_of_date(canvas);
    }
    else
    {
        VK_CHECK_RESULT(present_res);
    }

    state->active_slot = NULL;
    if (canvas->video_sink_enabled)
    {
        int fd = canvas_export_timeline_fd(canvas);
        if (fd >= 0)
        {
            DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
            if (frame)
            {
#if OS_UNIX
                if (frame->wait_semaphore_fd >= 0)
                {
                    close(frame->wait_semaphore_fd);
                }
#endif
                frame->wait_semaphore_fd = fd;
            }
        }
        else
        {
            log_warn("canvas timeline semaphore export failed for video sink");
        }
    }
    return 0;
}



static int canvas_export_timeline_fd(DvzCanvas* canvas)
{
    ANN(canvas);
    if (!canvas->timeline_ready || !canvas->supports_external_semaphore)
    {
        return -1;
    }
#if OS_UNIX
    VkDevice vk_device = dvz_device_handle(canvas->device);
    VkExternalSemaphoreHandleTypeFlags handle_type = dvz_canvas_timeline_handle_type();
    if (handle_type == 0)
    {
        return -1;
    }
    VkSemaphoreGetFdInfoKHR fd_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
        .semaphore = canvas->timeline_semaphore.vk_semaphore,
        .handleType = handle_type,
    };
    int fd = -1;
    VkResult res = vkGetSemaphoreFdKHR(vk_device, &fd_info, &fd);
    if (res != VK_SUCCESS)
    {
        log_warn("vkGetSemaphoreFdKHR failed for timeline semaphore (%d)", res);
        return -1;
    }
    return fd;
#else
    return -1;
#endif
}



/*************************************************************************************************/
/*  Backend callbacks                                                                            */
/*************************************************************************************************/

static bool canvas_swapchain_probe(const void* config)
{
    (void)config;
    return true;
}



static int canvas_swapchain_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    uintptr_t canvas_ptr = (uintptr_t)config;
    DvzCanvas* canvas = (DvzCanvas*)canvas_ptr;
    if (!canvas)
    {
        log_error("swapchain sink requires a valid canvas handle");
        return -1;
    }
    DvzCanvasSwapchainState* state =
        (DvzCanvasSwapchainState*)dvz_calloc(1, sizeof(DvzCanvasSwapchainState));
    ANN(state);
    state->canvas = canvas;
    sink->backend_data = state;
    return 0;
}



static int canvas_swapchain_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    (void)sink;
    (void)frame;
    return 0;
}



static int canvas_swapchain_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    DvzCanvasSwapchainState* state = canvas_swapchain_sink_state(sink);
    if (!state)
    {
        return -1;
    }
    DvzCanvas* canvas = state->canvas;
    if (!canvas)
    {
        return -1;
    }
    return dvz_canvas_swapchain_present(canvas, wait_value);
}



static int canvas_swapchain_stop(DvzStreamSink* sink)
{
    (void)sink;
    return 0;
}



static int canvas_swapchain_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    (void)sink;
    (void)frame;
    return 0;
}



static void canvas_swapchain_destroy(DvzStreamSink* sink)
{
    if (!sink || !sink->backend_data)
    {
        return;
    }
    dvz_free(sink->backend_data);
    sink->backend_data = NULL;
}



/*************************************************************************************************/
/*  Backend descriptor                                                                           */
/*************************************************************************************************/

static const DvzStreamSinkBackend CANVAS_SWAPCHAIN_SINK = {
    .name = "canvas_swapchain",
    .probe = canvas_swapchain_probe,
    .create = canvas_swapchain_create,
    .start = canvas_swapchain_start,
    .submit = canvas_swapchain_submit,
    .stop = canvas_swapchain_stop,
    .update = canvas_swapchain_update,
    .destroy = canvas_swapchain_destroy,
};



/**
 * Expose the swapchain backend descriptor so it can be registered with the stream registry.
 *
 * @returns backend descriptor
 */
const DvzStreamSinkBackend* dvz_canvas_swapchain_sink_backend(void)
{
    return &CANVAS_SWAPCHAIN_SINK;
}
