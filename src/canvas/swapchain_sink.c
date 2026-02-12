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
#include "datoviz/vk/device.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/surface.h"
#include "datoviz/vklite/swapchain.h"
#include "datoviz/vklite/sync.h"

static void canvas_swapchain_cleanup(DvzCanvasSwapchain* swapchain);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

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
    DvzImages offscreen_images;
    DvzImageViews offscreen_views;
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
};



struct DvzCanvasSwapchain
{
    DvzCanvas* canvas;
    DvzSurface surface_wrapper;
    DvzSwapchain swapchain_wrapper;
    bool wrappers_ready;
    uint32_t image_count;
    DvzCanvasSwapchainSlot* slots;
    VkImageLayout* swapchain_layouts;
    uint32_t frame_index;
    bool dirty;
    VkQueue queue;
    DvzCanvasSwapchainSlot* active_slot;
    uint32_t queue_family;
    uint64_t export_serial;
    VkFormat frame_format;
    DvzCanvasPresentRuntimeState runtime_state;
};



struct DvzCanvasSwapchainState
{
    DvzCanvas* canvas;
};



static int32_t canvas_test_fail_slot_index = -1;
static int32_t canvas_test_force_recreate_status = -1;
static int32_t canvas_test_force_acquire_status = -1;
static int32_t canvas_test_force_present_status = -1;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static VkDevice canvas_device_handle(const DvzCanvas* canvas)
{
    ANN(canvas);
    return dvz_device_handle(canvas->device);
}



static const char* canvas_runtime_state_name(DvzCanvasPresentRuntimeState state)
{
    switch (state)
    {
    case DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED:
        return "UNINITIALIZED";
    case DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE:
        return "WAIT_SURFACE";
    case DVZ_CANVAS_PRESENT_STATE_READY:
        return "READY";
    case DVZ_CANVAS_PRESENT_STATE_ACQUIRED:
        return "ACQUIRED";
    case DVZ_CANVAS_PRESENT_STATE_PRESENT_PENDING:
        return "PRESENT_PENDING";
    case DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST:
        return "FATAL_DEVICE_LOST";
    default:
        return "UNKNOWN";
    }
}



static void canvas_runtime_transition(
    DvzCanvasSwapchain* swapchain, DvzCanvasPresentRuntimeState state, const char* reason)
{
    ANN(swapchain);
    if (swapchain->runtime_state == state)
    {
        return;
    }
    log_debug(
        "canvas present state %s -> %s (%s)",
        canvas_runtime_state_name(swapchain->runtime_state), canvas_runtime_state_name(state),
        reason ? reason : "no reason");
    swapchain->runtime_state = state;
}



static void canvas_runtime_device_lost(DvzCanvasSwapchain* swapchain, const char* reason)
{
    ANN(swapchain);
    canvas_runtime_transition(swapchain, DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST, reason);
}



static bool canvas_test_consume_forced_status(int32_t* forced_status, DvzPresentStatus* status)
{
    ANN(forced_status);
    ANN(status);
    if (*forced_status < 0)
    {
        return false;
    }
    *status = (DvzPresentStatus)(*forced_status);
    *forced_status = -1;
    return true;
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



static VkExtent2D canvas_surface_extent(const DvzCanvas* canvas)
{
    ANN(canvas);
    VkExtent2D extent = canvas->surface ? canvas->surface->extent : (VkExtent2D){0, 0};
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



static VkFormat canvas_frame_format(const DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->stream)
    {
        const DvzStreamConfig* stream_cfg = dvz_stream_config(canvas->stream);
        if (stream_cfg && stream_cfg->color_format != VK_FORMAT_UNDEFINED)
        {
            return stream_cfg->color_format;
        }
    }
    if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        return canvas->cfg.color_format;
    }
    return DVZ_DEFAULT_COLOR_FORMAT;
}



static VkPresentModeKHR canvas_select_present_mode(DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->cfg.present_mode != 0)
    {
        return canvas->cfg.present_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
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
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
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
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    default:
        return 0;
    }
}

static void canvas_cmd_pipeline_barrier(
    DvzCanvas* canvas, VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout,
    VkImageLayout new_layout, VkPipelineStageFlags2 src_stage)
{
    if (!canvas || cmd == VK_NULL_HANDLE || image == VK_NULL_HANDLE || old_layout == new_layout)
    {
        return;
    }

    DvzCommands cmds = {0};
    dvz_commands_wrap(canvas->device, cmd, &cmds);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, image);
    dvz_barrier_image_stage(bimg, src_stage, canvas_stage_for_layout(new_layout));
    dvz_barrier_image_access(
        bimg, canvas_access_for_layout(old_layout), canvas_access_for_layout(new_layout));
    dvz_barrier_image_layout(bimg, old_layout, new_layout);

    dvz_cmd_barriers(&cmds, &barriers);
}


static void canvas_cmd_transition(
    DvzCanvas* canvas, VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    canvas_cmd_pipeline_barrier(
        canvas, cmd, image, old_layout, new_layout, canvas_stage_for_layout(old_layout));
}


static void canvas_cmd_transition_swapchain(
    DvzCanvas* canvas, VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    VkPipelineStageFlags2 release_stage =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    canvas_cmd_pipeline_barrier(canvas, cmd, image, old_layout, new_layout, release_stage);
}



static void canvas_cmd_copy_frame(
    DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot, VkCommandBuffer cmd)
{
    ANN(swapchain);
    ANN(swapchain->canvas);
    ANN(slot);
    ANNVK(cmd);

    VkExtent2D extent = swapchain->swapchain_wrapper.extent;
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    VkImage src = slot->offscreen_image;
    VkImage dst = slot->swapchain_image;
    if (src == VK_NULL_HANDLE || dst == VK_NULL_HANDLE)
    {
        return;
    }

    DvzCommands cmds = {0};
    dvz_commands_wrap(swapchain->canvas->device, cmd, &cmds);

    if (swapchain->frame_format == swapchain->swapchain_wrapper.image_format)
    {
        DvzImageCopy copy = {0};
        dvz_cmd_copy_source(
            &copy, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 0, extent.width, extent.height,
            1);
        dvz_cmd_copy_destination(&copy, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0);
        dvz_cmd_copy_image(&cmds, &copy);
    }
    else
    {
        DvzImageBlit blit = {0};
        dvz_cmd_blit_source(
            &blit, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 0,
            (int32_t)extent.width, (int32_t)extent.height, 1);
        dvz_cmd_blit_destination(
            &blit, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0,
            (int32_t)extent.width, (int32_t)extent.height, 1);
        dvz_cmd_blit_filter(&blit, VK_FILTER_NEAREST);
        dvz_cmd_blit_image(&cmds, &blit);
    }
}



static VkResult
canvas_slot_create_swapchain_view(DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot)
{
    if (!swapchain || !slot || slot->image_index >= swapchain->swapchain_wrapper.image_count)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (swapchain->swapchain_wrapper.image_views == NULL)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    slot->swapchain_view = swapchain->swapchain_wrapper.image_views[slot->image_index];
    if (slot->swapchain_view == VK_NULL_HANDLE)
    {
        log_error("swapchain image view %u is unavailable", slot->image_index);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return VK_SUCCESS;
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
    DvzCommands cmds = {0};
    dvz_commands_wrap(swapchain->canvas->device, cmd, &cmds);
    dvz_cmd_reset(&cmds);
    dvz_cmd_begin(&cmds);
    canvas_cmd_transition(
        swapchain->canvas, cmd, slot->offscreen_image, slot->offscreen_layout,
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
        swapchain->canvas, cmd, slot->offscreen_image, slot->offscreen_layout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    slot->offscreen_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    canvas_cmd_transition_swapchain(
        swapchain->canvas, cmd, slot->swapchain_image, slot->swapchain_layout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    slot->swapchain_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    canvas_cmd_copy_frame(swapchain, slot, cmd);

    canvas_cmd_transition_swapchain(
        swapchain->canvas, cmd, slot->swapchain_image, slot->swapchain_layout,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    slot->swapchain_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    canvas_cmd_transition(
        swapchain->canvas, cmd, slot->offscreen_image, slot->offscreen_layout,
        VK_IMAGE_LAYOUT_GENERAL);
    slot->offscreen_layout = VK_IMAGE_LAYOUT_GENERAL;

    // log_trace("end command buffer");
    DvzCommands cmds = {0};
    dvz_commands_wrap(swapchain->canvas->device, cmd, &cmds);
    dvz_cmd_end(&cmds);

    slot->commands_recording = false;
    if (swapchain->swapchain_layouts && slot->image_index < swapchain->image_count)
    {
        swapchain->swapchain_layouts[slot->image_index] = slot->swapchain_layout;
    }
    return 0;
}



/**
 * Initialize a single swapchain slot and its per-frame resources.
 *
 * @param swapchain swapchain owning the slot
 * @param slot slot structure to initialize
 * @param slot_index index of the slot in the swapchain slot array
 * @param extent render extent used to allocate offscreen images
 * @param frame_format render target format used by the canvas stream
 * @param handles_changed true when exported handles changed after a recreate
 * @return true on success or false on failure
 */
static bool canvas_slot_init(
    DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot, uint32_t slot_index,
    VkExtent2D extent, VkFormat frame_format, bool handles_changed)
{
    ANN(swapchain);
    ANN(slot);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    dvz_memset(slot, sizeof(*slot), 0, sizeof(*slot));
    slot->offscreen_view = VK_NULL_HANDLE;
    slot->swapchain_image = VK_NULL_HANDLE;
    slot->swapchain_view = VK_NULL_HANDLE;
    slot->image_index = UINT32_MAX;
    slot->offscreen_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    slot->swapchain_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    slot->handles_dirty = handles_changed;
    slot->commands_recording = false;
    slot->memory_fd = -1;

    if (canvas_test_fail_slot_index >= 0 && (int32_t)slot_index == canvas_test_fail_slot_index)
    {
        log_warn(
            "forcing canvas slot initialization failure at slot %u for testing", slot_index);
        return false;
    }

    slot->command_buffer = dvz_command_buffer_alloc(canvas->device, swapchain->queue_family);
    if (slot->command_buffer == VK_NULL_HANDLE)
    {
        log_error("failed to allocate canvas command buffer for slot %u", slot_index);
        return false;
    }

    VkExternalMemoryImageCreateInfoKHR external_info = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR};
    VkImageCreateInfo img_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = frame_format,
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
            &canvas->allocator, &img_info, 0, &slot->offscreen_alloc, &slot->offscreen_image) !=
        0)
    {
        log_error("failed to allocate offscreen canvas image for slot %u", slot_index);
        return false;
    }

    slot->offscreen_images = (DvzImages){0};
    slot->offscreen_views = (DvzImageViews){0};
    dvz_images_wrap(
        canvas->device, &canvas->allocator, VK_IMAGE_TYPE_2D, slot->offscreen_image,
        &slot->offscreen_images);
    dvz_images_format(&slot->offscreen_images, frame_format);
    dvz_image_views(&slot->offscreen_images, &slot->offscreen_views);
    dvz_image_views_create(&slot->offscreen_views);
    slot->offscreen_view = dvz_image_views_handle(&slot->offscreen_views, 0);
    if (slot->offscreen_view == VK_NULL_HANDLE)
    {
        log_error("failed to create offscreen image view for slot %u", slot_index);
        dvz_allocator_destroy_image(
            &canvas->allocator, &slot->offscreen_alloc, slot->offscreen_image);
        slot->offscreen_image = VK_NULL_HANDLE;
        slot->offscreen_images = (DvzImages){0};
        slot->offscreen_views = (DvzImageViews){0};
        return false;
    }

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
    return true;
}



static VkResult canvas_create_swapchain(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    VkSurfaceKHR surface = canvas_surface_handle(canvas);
    if (surface == VK_NULL_HANDLE)
    {
        log_warn("canvas surface unavailable, postponing swapchain creation");
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "surface unavailable");
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    if (!swapchain->wrappers_ready)
    {
        log_error("canvas swapchain wrappers are not initialized");
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "wrappers unavailable");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (!dvz_surface_wrap_native(&swapchain->surface_wrapper, surface, canvas->window))
    {
        log_warn("canvas surface wrapper refresh failed, postponing swapchain creation");
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "surface wrap failed");
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    VkExtent2D extent = canvas_surface_extent(canvas);
    if (extent.width == 0 || extent.height == 0)
    {
        log_warn("window surface extent is zero, waiting before creating swapchain");
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "surface extent zero");
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    DvzSwapchainConfig config = {0};
    config.image_format = canvas_surface_format(canvas);
    config.color_space =
        canvas->surface ? canvas->surface->color_space : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    config.present_mode = canvas_select_present_mode(canvas);
    config.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    config.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    config.min_image_count = 0;
    config.clipped = true;
    dvz_swapchain_config(&swapchain->swapchain_wrapper, config);

    VkFormat frame_format = canvas_frame_format(canvas);
    uvec2 size = {extent.width, extent.height};
    DvzPresentStatus status = DVZ_PRESENT_STATUS_OK;
    if (!canvas_test_consume_forced_status(&canvas_test_force_recreate_status, &status))
    {
        status = dvz_swapchain_recreate(&swapchain->swapchain_wrapper, size);
    }
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("window surface extent is zero, waiting before creating swapchain");
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "recreate: zero extent");
        return VK_ERROR_SURFACE_LOST_KHR;
    }
    if (status == DVZ_PRESENT_STATUS_RECREATE)
    {
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "recreate requested");
        return VK_ERROR_OUT_OF_DATE_KHR;
    }
    if (status == DVZ_PRESENT_STATUS_DEVICE_LOST)
    {
        canvas_runtime_device_lost(swapchain, "swapchain recreate");
        return VK_ERROR_DEVICE_LOST;
    }
    if (status != DVZ_PRESENT_STATUS_OK)
    {
        canvas_runtime_transition(swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "recreate failed");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    swapchain->frame_format = frame_format;
    uint32_t count = swapchain->swapchain_wrapper.image_count;
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


    bool handles_changed = swapchain->export_serial > 1;
    bool slot_init_failed = false;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!canvas_slot_init(
                swapchain, &swapchain->slots[i], i, extent, frame_format, handles_changed))
        {
            slot_init_failed = true;
            break;
        }
    }
    if (slot_init_failed)
    {
        canvas_swapchain_cleanup(swapchain);
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "slot initialization failed");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    swapchain->dirty = false;
    canvas_runtime_transition(swapchain, DVZ_CANVAS_PRESENT_STATE_READY, "swapchain created");
    return VK_SUCCESS;
}



static void canvas_destroy_slot(
    DvzDevice* device, uint32_t queue_family, DvzCanvasSwapchainSlot* slot, DvzVma* allocator)
{
    if (!slot)
    {
        return;
    }
    if (slot->offscreen_views.img)
    {
        dvz_image_views_destroy(&slot->offscreen_views);
        slot->offscreen_view = VK_NULL_HANDLE;
        slot->offscreen_images = (DvzImages){0};
        slot->offscreen_views = (DvzImageViews){0};
    }
    slot->swapchain_view = VK_NULL_HANDLE;
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
    if (slot->command_buffer != VK_NULL_HANDLE)
    {
        dvz_command_buffer_free(device, queue_family, slot->command_buffer);
        slot->command_buffer = VK_NULL_HANDLE;
    }
}



static void canvas_swapchain_cleanup(DvzCanvasSwapchain* swapchain)
{
    if (!swapchain)
    {
        return;
    }
    DvzCanvas* canvas = swapchain->canvas;
    if (canvas == NULL || canvas->device == NULL)
    {
        return;
    }
    dvz_device_wait(canvas->device);
    if (swapchain->slots)
    {
        for (uint32_t i = 0; i < swapchain->image_count; ++i)
        {
            canvas_destroy_slot(
                canvas->device, swapchain->queue_family, &swapchain->slots[i], &canvas->allocator);
        }
        dvz_free(swapchain->slots);
        swapchain->slots = NULL;
    }
    dvz_swapchain_destroy(&swapchain->swapchain_wrapper);
    if (swapchain->swapchain_layouts)
    {
        dvz_free(swapchain->swapchain_layouts);
        swapchain->swapchain_layouts = NULL;
    }
    swapchain->image_count = 0;
    swapchain->active_slot = NULL;
    swapchain->frame_index = 0;
    swapchain->dirty = true;
    if (swapchain->runtime_state != DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST)
    {
        canvas_runtime_transition(swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "cleanup");
    }
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
    if (state->runtime_state == DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST)
    {
        return -1;
    }
    if (!state->dirty && state->swapchain_wrapper.handle != VK_NULL_HANDLE)
    {
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "ensure existing");
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
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "ensure recreate");
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (res == VK_ERROR_DEVICE_LOST)
    {
        canvas_runtime_device_lost(state, "ensure recreate");
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
    canvas->swapchain->dirty = true;
    canvas->swapchain->frame_index = 0;
    canvas->swapchain->frame_format = canvas_frame_format(canvas);
    canvas->swapchain->runtime_state = DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED;
    DvzQueue* queue_ref = dvz_device_queue(canvas->device, DVZ_QUEUE_MAIN);
    ANN(queue_ref);
    canvas->swapchain->queue = dvz_queue_handle(queue_ref);
    ANNVK(canvas->swapchain->queue);
    canvas->swapchain->queue_family = dvz_queue_family(queue_ref);
    DvzGpu* gpu = canvas_gpu(canvas);
    ANN(gpu);
    bool surface_initialized = false;
    bool swapchain_initialized = false;
    if (!dvz_surface_init(&canvas->swapchain->surface_wrapper, gpu, canvas->swapchain->queue_family))
    {
        log_error("failed to initialize canvas surface wrapper");
        goto swapchain_init_failed;
    }
    surface_initialized = true;
    if (!dvz_swapchain_init(
            &canvas->swapchain->swapchain_wrapper, gpu, &canvas->swapchain->surface_wrapper))
    {
        log_error("failed to initialize canvas swapchain wrapper");
        goto swapchain_init_failed;
    }
    swapchain_initialized = true;
    if (!dvz_swapchain_device(
            &canvas->swapchain->swapchain_wrapper, dvz_device_handle(canvas->device)))
    {
        log_error("failed to bind device to canvas swapchain wrapper");
        goto swapchain_init_failed;
    }
    canvas->swapchain->wrappers_ready = true;
    canvas_runtime_transition(
        canvas->swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "initialized wrappers");
    return 0;

swapchain_init_failed:
    canvas_runtime_transition(
        canvas->swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "wrapper init failed");
    if (swapchain_initialized)
    {
        dvz_swapchain_destroy(&canvas->swapchain->swapchain_wrapper);
    }
    if (surface_initialized)
    {
        dvz_surface_destroy(&canvas->swapchain->surface_wrapper);
    }
    dvz_free(canvas->swapchain);
    canvas->swapchain = NULL;
    return -1;
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
    canvas_swapchain_cleanup(canvas->swapchain);
    if (canvas->swapchain->wrappers_ready)
    {
        dvz_surface_destroy(&canvas->swapchain->surface_wrapper);
        canvas->swapchain->wrappers_ready = false;
    }
    canvas_runtime_transition(
        canvas->swapchain, DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED, "destroy swapchain");
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
    if (canvas->swapchain->runtime_state == DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST)
    {
        return;
    }
    canvas->swapchain->dirty = true;
    canvas_runtime_transition(
        canvas->swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "marked out of date");
}



/**
 * Configure a test-only slot index that forces swapchain slot initialization failure.
 *
 * @param slot_index slot index to fail, or -1 to disable forced failure
 */
void dvz_canvas_swapchain_test_fail_slot(int32_t slot_index)
{
    canvas_test_fail_slot_index = slot_index;
}



/**
 * Force the next swapchain recreate call to return a specific present status.
 *
 * @param status Present status to inject once, or -1 to disable
 */
void dvz_canvas_swapchain_test_force_recreate_status(int32_t status)
{
    canvas_test_force_recreate_status = status;
}



/**
 * Force the next swapchain acquire call to return a specific present status.
 *
 * @param status Present status to inject once, or -1 to disable
 */
void dvz_canvas_swapchain_test_force_acquire_status(int32_t status)
{
    canvas_test_force_acquire_status = status;
}



/**
 * Force the next swapchain present call to return a specific present status.
 *
 * @param status Present status to inject once, or -1 to disable
 */
void dvz_canvas_swapchain_test_force_present_status(int32_t status)
{
    canvas_test_force_present_status = status;
}



/**
 * Return the current canvas presentation runtime state.
 *
 * @param canvas canvas owning the swapchain
 * @returns the current runtime state or UNINITIALIZED if unavailable
 */
DvzCanvasPresentRuntimeState dvz_canvas_swapchain_runtime_state(const DvzCanvas* canvas)
{
    if (!canvas || !canvas->swapchain)
    {
        return DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED;
    }
    return canvas->swapchain->runtime_state;
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
    if (state->runtime_state == DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST)
    {
        log_error("canvas swapchain acquire aborted after device loss");
        return -1;
    }
    VkExtent2D current_extent = state->swapchain_wrapper.extent;
    if (
        canvas->surface && state->swapchain_wrapper.ready &&
        (canvas->surface->extent.width != current_extent.width ||
         canvas->surface->extent.height != current_extent.height ||
         canvas_surface_format(canvas) != state->swapchain_wrapper.image_format))
    {
        state->dirty = true;
    }

    int ensure_rc = canvas_swapchain_ensure(canvas);
    if (ensure_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
    {
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "acquire wait surface");
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (ensure_rc != 0)
    {
        return -1;
    }

    if (state->image_count == 0)
    {
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "acquire no images");
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }

    uint32_t slot_idx = state->frame_index % state->image_count;
    DvzCanvasSwapchainSlot* slot = &state->slots[slot_idx];

    dvz_fence_wait(&slot->in_flight);
    dvz_fence_reset(&slot->in_flight);

    uint32_t image_index = 0;
    DvzPresentStatus acquire_status = DVZ_PRESENT_STATUS_OK;
    if (!canvas_test_consume_forced_status(&canvas_test_force_acquire_status, &acquire_status))
    {
        acquire_status = dvz_swapchain_acquire(
            &state->swapchain_wrapper, slot->image_available.vk_semaphore, UINT64_MAX, &image_index);
    }
    if (acquire_status == DVZ_PRESENT_STATUS_RECREATE)
    {
        dvz_canvas_swapchain_mark_out_of_date(canvas);
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "acquire recreate");
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (acquire_status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        dvz_canvas_swapchain_mark_out_of_date(canvas);
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "acquire zero extent");
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (acquire_status == DVZ_PRESENT_STATUS_DEVICE_LOST)
    {
        canvas_runtime_device_lost(state, "swapchain acquire");
        return -1;
    }
    if (acquire_status != DVZ_PRESENT_STATUS_OK)
    {
        log_error(
            "failed to acquire swapchain image (frame=%u slot=%u status=%d)", state->frame_index,
            slot_idx, acquire_status);
        return -1;
    }

    state->frame_index = (state->frame_index + 1) % state->image_count;
    if (!slot->ready)
    {
        log_error("acquired swapchain slot %u is not ready", slot_idx);
        return -1;
    }
    slot->image_index = image_index;
    if (state->swapchain_wrapper.images && image_index < state->swapchain_wrapper.image_count)
    {
        slot->swapchain_image = state->swapchain_wrapper.images[image_index];
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
    canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_ACQUIRED, "acquire success");
    frame->image = slot->offscreen_image;
    frame->memory = slot->offscreen_alloc.info.deviceMemory;
    frame->memory_size = slot->offscreen_alloc.info.size;
    frame->command_buffer = slot->command_buffer;
    frame->image_view = slot->offscreen_view;
    frame->extent = state->swapchain_wrapper.extent;
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
    if (state->runtime_state == DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST)
    {
        log_error("canvas swapchain present aborted after device loss");
        state->active_slot = NULL;
        return -1;
    }
    canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_PRESENT_PENDING, "submit+present");

    if (canvas_slot_finish_recording(state, state->active_slot) != 0)
    {
        state->active_slot = NULL;
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "recording failed");
        return -1;
    }
    VkCommandBuffer cmd = state->active_slot->command_buffer;

    VkPipelineStageFlags2 wait_stage =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    VkPipelineStageFlags2 signal_stage = wait_stage | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

    // Ownership contract: one slot owns one fence + per-frame semaphores; the main queue is used
    // for both submit and present, and slot synchronization primitives are reset on next acquire.
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
    int32_t submit_res = dvz_submit_send(&submit, queue, state->active_slot->in_flight.vk_fence);
    if (submit_res == VK_ERROR_DEVICE_LOST)
    {
        canvas_runtime_device_lost(state, "queue submit");
        state->active_slot = NULL;
        return -1;
    }
    if (submit_res != VK_SUCCESS)
    {
        log_error(
            "failed to submit canvas frame (frame=%u image=%u vk=%d)", state->frame_index,
            state->active_slot->image_index, submit_res);
        state->active_slot = NULL;
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "submit failed");
        return -1;
    }

    uint32_t index = state->active_slot->image_index;

    // log_trace("present");
    DvzPresentStatus present_status = DVZ_PRESENT_STATUS_OK;
    if (!canvas_test_consume_forced_status(&canvas_test_force_present_status, &present_status))
    {
        present_status = dvz_swapchain_present(
            &state->swapchain_wrapper, queue, index, state->active_slot->render_finished.vk_semaphore);
    }
    if (present_status == DVZ_PRESENT_STATUS_RECREATE)
    {
        dvz_canvas_swapchain_mark_out_of_date(canvas);
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "present recreate");
    }
    else if (present_status == DVZ_PRESENT_STATUS_DEVICE_LOST)
    {
        canvas_runtime_device_lost(state, "swapchain present");
        state->active_slot = NULL;
        return -1;
    }
    else if (present_status != DVZ_PRESENT_STATUS_OK)
    {
        log_error(
            "failed to present swapchain image (frame=%u image=%u status=%d)", state->frame_index,
            index, present_status);
        state->active_slot = NULL;
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "present failed");
        return -1;
    }

    state->active_slot = NULL;
    if (state->runtime_state != DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE)
    {
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "present success");
    }
    return 0;
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
