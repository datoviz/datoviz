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

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_vk_utils.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
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
    DvzImages* offscreen_images;
    DvzImageViews* offscreen_views;
    DvzAllocation* offscreen_alloc;
    DvzSemaphore* image_available;
    DvzFence* in_flight;
    VkCommandBuffer command_buffer;
    VkImageLayout offscreen_layout;
    VkImageLayout swapchain_layout;
    VkExtent2D offscreen_extent;
    uint32_t image_index;
    uint64_t resource_generation;
    int memory_fd;
    bool ready;
    bool commands_recording;
    bool handles_dirty;
};



struct DvzCanvasSwapchain
{
    DvzCanvas* canvas;
    DvzSurface* surface_wrapper;
    DvzSwapchain* swapchain_wrapper;
    bool wrappers_ready;
    uint32_t image_count;
    DvzCanvasSwapchainSlot* slots;
    VkImageLayout* swapchain_layouts;
    // Present-wait semaphores, owned per swapchain image rather than per frame slot: a binary
    // present semaphore may only be re-signaled once the present waiting on it has consumed the
    // signal, which is only guaranteed when the same image is re-acquired
    // (VUID-vkQueueSubmit2-semaphore-03868).
    DvzSemaphore** render_finished;
    uint32_t frame_index;
    uint32_t last_presented_slot_index;
    bool dirty;
    VkQueue queue;
    DvzCanvasSwapchainSlot* active_slot;
    uint32_t queue_family;
    uint64_t export_serial;
    uint64_t resource_generation;
    VkFormat frame_format;
    DvzCanvasPresentRuntimeState runtime_state;
    int32_t test_fail_slot_index;
    int32_t test_force_recreate_status;
    int32_t test_force_acquire_status;
    int32_t test_force_present_status;
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
    log_trace(
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
    return VK_FORMAT_UNDEFINED;
}



static VkFormat canvas_frame_format(const DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->stream)
    {
        const DvzStreamConfig* stream_cfg = dvz_stream_get_config(canvas->stream);
        if (stream_cfg && stream_cfg->color_format != VK_FORMAT_UNDEFINED)
        {
            return stream_cfg->color_format;
        }
    }
    if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        return canvas->cfg.color_format;
    }
    if (canvas->swapchain != NULL && canvas->swapchain->swapchain_wrapper != NULL)
    {
        VkFormat format = dvz_swapchain_image_format(canvas->swapchain->swapchain_wrapper);
        if (format != VK_FORMAT_UNDEFINED)
            return format;
    }
    return dvz_format_srgb_counterpart(DVZ_DEFAULT_COLOR_FORMAT);
}



static VkPresentModeKHR canvas_select_present_mode(DvzCanvas* canvas)
{
    ANN(canvas);
    return canvas->cfg.present_mode;
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

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, cmd, cmds);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, image);
    dvz_barrier_image_stage(bimg, src_stage, canvas_stage_for_layout(new_layout));
    dvz_barrier_image_access(
        bimg, canvas_access_for_layout(old_layout), canvas_access_for_layout(new_layout));
    dvz_barrier_image_layout(bimg, old_layout, new_layout);

    dvz_cmd_barriers(cmds, &barriers);
    dvz_commands_free(cmds);
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
    ASSERT(cmd != VK_NULL_HANDLE);

    VkExtent2D src_extent = slot->offscreen_extent;
    VkExtent2D dst_extent = dvz_swapchain_extent(swapchain->swapchain_wrapper);
    if (
        src_extent.width == 0 || src_extent.height == 0 || dst_extent.width == 0 ||
        dst_extent.height == 0)
    {
        return;
    }

    VkImage src = slot->offscreen_image;
    VkImage dst = slot->swapchain_image;
    if (src == VK_NULL_HANDLE || dst == VK_NULL_HANDLE)
    {
        return;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(swapchain->canvas->device, cmd, cmds);

    if (
        swapchain->frame_format == dvz_swapchain_image_format(swapchain->swapchain_wrapper) &&
        src_extent.width == dst_extent.width && src_extent.height == dst_extent.height)
    {
        DvzImageCopy* copy = dvz_image_copy_create();
        ANN(copy);
        dvz_cmd_copy_source(
            copy, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 0, src_extent.width,
            src_extent.height, 1);
        dvz_cmd_copy_destination(copy, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0);
        dvz_cmd_copy_image(cmds, copy);
        dvz_image_copy_free(copy);
    }
    else
    {
        DvzImageBlit* blit = dvz_image_blit_create();
        ANN(blit);
        dvz_cmd_blit_source(
            blit, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 0,
            (int32_t)src_extent.width, (int32_t)src_extent.height, 1);
        dvz_cmd_blit_destination(
            blit, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0,
            (int32_t)dst_extent.width, (int32_t)dst_extent.height, 1);
        dvz_cmd_blit_filter(blit, VK_FILTER_NEAREST);
        dvz_cmd_blit_image(cmds, blit);
        dvz_image_blit_free(blit);
    }
    dvz_commands_free(cmds);
}



static VkResult
canvas_slot_create_swapchain_view(DvzCanvasSwapchain* swapchain, DvzCanvasSwapchainSlot* slot)
{
    if (!swapchain || !slot || slot->image_index >= dvz_swapchain_image_count(swapchain->swapchain_wrapper))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (!dvz_swapchain_image_view(swapchain->swapchain_wrapper, slot->image_index, &slot->swapchain_view))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
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
    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(swapchain->canvas->device, cmd, cmds);
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);
    canvas_cmd_transition(
        swapchain->canvas, cmd, slot->offscreen_image, slot->offscreen_layout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    slot->offscreen_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    slot->commands_recording = true;
    dvz_commands_free(cmds);
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
    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(swapchain->canvas->device, cmd, cmds);
    dvz_cmd_end(cmds);
    dvz_commands_free(cmds);

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
    slot->offscreen_extent = extent;
    slot->handles_dirty = handles_changed;
    slot->resource_generation = ++swapchain->resource_generation;
    if (slot->resource_generation == 0)
        slot->resource_generation = ++swapchain->resource_generation;
    slot->commands_recording = false;
    slot->memory_fd = -1;

    if (
        swapchain->test_fail_slot_index >= 0 &&
        (int32_t)slot_index == swapchain->test_fail_slot_index)
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
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .pNext = NULL,
    };
    bool use_external = canvas->allocator != NULL && dvz_allocator_external(canvas->allocator) != 0;
    if (use_external)
    {
        external_info.handleTypes = dvz_allocator_external(canvas->allocator);
        img_info.pNext = &external_info;
    }

    slot->offscreen_alloc = dvz_allocation_create();
    ANN(slot->offscreen_alloc);
    if (dvz_allocator_image(
            canvas->allocator, &img_info, 0, slot->offscreen_alloc, &slot->offscreen_image) !=
        0)
    {
        dvz_allocation_free(slot->offscreen_alloc);
        slot->offscreen_alloc = NULL;
        log_error("failed to allocate offscreen canvas image for slot %u", slot_index);
        return false;
    }

    if (slot->offscreen_images == NULL)
    {
        slot->offscreen_images = dvz_images_create_wrapper();
        ANN(slot->offscreen_images);
    }
    if (slot->offscreen_views == NULL)
    {
        slot->offscreen_views = dvz_image_views_create_wrapper();
        ANN(slot->offscreen_views);
    }
    dvz_images_wrap(
        canvas->device, canvas->allocator, VK_IMAGE_TYPE_2D, slot->offscreen_image,
        slot->offscreen_images);
    dvz_images_format(slot->offscreen_images, frame_format);
    dvz_image_views(slot->offscreen_images, slot->offscreen_views);
    dvz_image_views_create(slot->offscreen_views);
    slot->offscreen_view = dvz_image_views_handle(slot->offscreen_views, 0);
    if (slot->offscreen_view == VK_NULL_HANDLE)
    {
        log_error("failed to create offscreen image view for slot %u", slot_index);
        dvz_allocator_destroy_image(
            canvas->allocator, slot->offscreen_alloc, slot->offscreen_image);
        dvz_allocation_free(slot->offscreen_alloc);
        slot->offscreen_alloc = NULL;
        slot->offscreen_image = VK_NULL_HANDLE;
        dvz_image_views_free(slot->offscreen_views);
        slot->offscreen_views = NULL;
        dvz_images_free(slot->offscreen_images);
        slot->offscreen_images = NULL;
        return false;
    }

    if (use_external && dvz_allocator_export(
                            canvas->allocator, slot->offscreen_alloc, &slot->memory_fd) != 0)
    {
        log_warn("failed to export canvas render target");
        slot->memory_fd = -1;
    }

    slot->image_available = dvz_semaphore_create_wrapper();
    slot->in_flight = dvz_fence_create_wrapper();
    ANN(slot->image_available);
    ANN(slot->in_flight);
    dvz_semaphore(canvas->device, slot->image_available);
    dvz_fence(canvas->device, true, slot->in_flight);
    slot->ready = true;
    return true;
}



/**
 * Resolve and validate the native surface wrapper state before swapchain creation.
 *
 * @param swapchain canvas swapchain state to initialize
 * @param[out] extent resolved non-zero surface extent
 * @return VK_SUCCESS when the surface is ready, Vulkan error code otherwise
 */
static VkResult canvas_swapchain_prepare_surface(DvzCanvasSwapchain* swapchain, VkExtent2D* extent)
{
    ANN(swapchain);
    ANN(extent);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    VkSurfaceKHR surface = canvas_surface_handle(canvas);
    if (surface == VK_NULL_HANDLE)
    {
        log_debug("canvas surface unavailable, postponing swapchain creation");
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
    VkExtent2D extent_hint = canvas_surface_extent(canvas);
    if (!dvz_surface_wrap_native(swapchain->surface_wrapper, surface, &extent_hint))
    {
        log_warn("canvas surface wrapper refresh failed, postponing swapchain creation");
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "surface wrap failed");
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    *extent = canvas_surface_extent(canvas);
    if (extent->width == 0 || extent->height == 0)
    {
        log_warn("window surface extent is zero, waiting before creating swapchain");
        swapchain->dirty = true;
        canvas_runtime_transition(
            swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "surface extent zero");
        return VK_ERROR_SURFACE_LOST_KHR;
    }
    return VK_SUCCESS;
}



/**
 * Configure the vklite swapchain wrapper from current canvas surface settings.
 *
 * @param swapchain canvas swapchain state being recreated
 */
static void canvas_swapchain_apply_config(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    DvzSwapchainConfig config = {0};
    config.image_format = canvas_surface_format(canvas);
    config.color_space =
        canvas->surface ? canvas->surface->color_space : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    config.present_mode = canvas_select_present_mode(canvas);
    config.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    config.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    config.min_image_count = 0;
    config.clipped = true;
    dvz_swapchain_config(swapchain->swapchain_wrapper, config);
}



/**
 * Translate wrapper recreate status into canvas swapchain creation result.
 *
 * @param swapchain canvas swapchain state receiving status transitions
 * @param status status returned by dvz_swapchain_recreate()
 * @return VK_SUCCESS on ready, Vulkan error code otherwise
 */
static VkResult
canvas_swapchain_handle_recreate_status(DvzCanvasSwapchain* swapchain, DvzPresentStatus status)
{
    ANN(swapchain);
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
    return VK_SUCCESS;
}



/**
 * Destroy and free the per-image present-wait semaphores owned by the canvas swapchain.
 *
 * @param swapchain canvas swapchain state owning the semaphores
 */
static void canvas_swapchain_release_render_finished(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    if (!swapchain->render_finished)
    {
        return;
    }
    for (uint32_t i = 0; i < swapchain->image_count; ++i)
    {
        dvz_semaphore_destroy(swapchain->render_finished[i]);
        dvz_semaphore_free(swapchain->render_finished[i]);
        swapchain->render_finished[i] = NULL;
    }
    dvz_free(swapchain->render_finished);
    swapchain->render_finished = NULL;
}



/**
 * Allocate and initialize per-image slot/layout state after a successful wrapper recreate.
 *
 * @param swapchain canvas swapchain state receiving slot resources
 * @param extent render extent used for per-slot offscreen image allocation
 * @param frame_format render target format used by the canvas stream
 * @return true on success or false on initialization failure
 */
static bool canvas_swapchain_init_slot_state(
    DvzCanvasSwapchain* swapchain, VkExtent2D extent, VkFormat frame_format)
{
    ANN(swapchain);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    uint32_t count = dvz_swapchain_image_count(swapchain->swapchain_wrapper);
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

    // Recreate the per-image present-wait semaphores (the release consumes the previous
    // image_count, so it must run before image_count is updated below).
    canvas_swapchain_release_render_finished(swapchain);
    swapchain->render_finished = (DvzSemaphore**)dvz_calloc(count, sizeof(DvzSemaphore*));
    ANN(swapchain->render_finished);
    for (uint32_t i = 0; i < count; ++i)
    {
        swapchain->render_finished[i] = dvz_semaphore_create_wrapper();
        ANN(swapchain->render_finished[i]);
        dvz_semaphore(canvas->device, swapchain->render_finished[i]);
    }

    dvz_free(swapchain->slots);
    swapchain->slots = (DvzCanvasSwapchainSlot*)dvz_calloc(count, sizeof(DvzCanvasSwapchainSlot));
    ANN(swapchain->slots);
    swapchain->image_count = count;
    swapchain->active_slot = NULL;
    swapchain->export_serial++;

    dvz_canvas_frame_pool_init(&canvas->frame_pool, swapchain->image_count);

    bool handles_changed = swapchain->export_serial > 1;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!canvas_slot_init(
                swapchain, &swapchain->slots[i], i, extent, frame_format, handles_changed))
        {
            return false;
        }
    }
    return true;
}



static VkResult canvas_create_swapchain(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);

    VkExtent2D extent = {0};
    VkResult surface_rc = canvas_swapchain_prepare_surface(swapchain, &extent);
    if (surface_rc != VK_SUCCESS)
    {
        return surface_rc;
    }
    canvas_swapchain_apply_config(swapchain);

    uvec2 size = {extent.width, extent.height};
    DvzPresentStatus status = DVZ_PRESENT_STATUS_OK;
    if (!canvas_test_consume_forced_status(&swapchain->test_force_recreate_status, &status))
    {
        status = dvz_swapchain_recreate(swapchain->swapchain_wrapper, size);
    }
    VkResult recreate_rc = canvas_swapchain_handle_recreate_status(swapchain, status);
    if (recreate_rc != VK_SUCCESS)
    {
        return recreate_rc;
    }

    /* Use the actual post-recreate swapchain extent for the offscreen images. The driver
     * may resolve the extent to caps.currentExtent (e.g. during a fullscreen transition),
     * which can differ from the GLFW-reported surface->extent. Allocating the offscreen
     * image at the requested size while the swapchain ends up at the resolved size means
     * the frame copy goes through a non-uniform blit and points/primitives get stretched. */
    VkExtent2D resolved_extent = dvz_swapchain_extent(swapchain->swapchain_wrapper);
    if (resolved_extent.width > 0 && resolved_extent.height > 0)
        extent = resolved_extent;

    VkFormat frame_format = canvas->cfg.color_format != VK_FORMAT_UNDEFINED
                                ? canvas_frame_format(canvas)
                                : dvz_swapchain_image_format(swapchain->swapchain_wrapper);
    if (frame_format == VK_FORMAT_UNDEFINED)
        frame_format = canvas_frame_format(canvas);

    swapchain->frame_format = frame_format;
    if (!canvas_swapchain_init_slot_state(swapchain, extent, frame_format))
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
    if (slot->command_buffer != VK_NULL_HANDLE)
    {
        dvz_command_buffer_free(device, queue_family, slot->command_buffer);
        slot->command_buffer = VK_NULL_HANDLE;
        slot->commands_recording = false;
    }
    if (slot->offscreen_views != NULL)
    {
        dvz_image_views_destroy(slot->offscreen_views);
        dvz_image_views_free(slot->offscreen_views);
        slot->offscreen_views = NULL;
        slot->offscreen_view = VK_NULL_HANDLE;
    }
    if (slot->offscreen_images != NULL)
    {
        dvz_images_free(slot->offscreen_images);
        slot->offscreen_images = NULL;
    }
    slot->swapchain_view = VK_NULL_HANDLE;
    if (slot->offscreen_image != VK_NULL_HANDLE)
    {
        if (slot->offscreen_alloc != NULL)
        {
            dvz_allocator_destroy_image(allocator, slot->offscreen_alloc, slot->offscreen_image);
            dvz_allocation_free(slot->offscreen_alloc);
            slot->offscreen_alloc = NULL;
        }
        slot->offscreen_image = VK_NULL_HANDLE;
    }
    dvz_semaphore_destroy(slot->image_available);
    dvz_semaphore_free(slot->image_available);
    slot->image_available = NULL;
    dvz_fence_destroy(slot->in_flight);
    dvz_fence_free(slot->in_flight);
    slot->in_flight = NULL;
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
    slot->offscreen_extent = (VkExtent2D){0, 0};
}



/**
 * Release per-slot and per-image resources owned by the canvas swapchain.
 *
 * @param swapchain canvas swapchain state to teardown
 */
static void canvas_swapchain_release_slot_state(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    DvzCanvas* canvas = swapchain->canvas;
    ANN(canvas);
    if (swapchain->slots)
    {
        for (uint32_t i = 0; i < swapchain->image_count; ++i)
        {
            canvas_destroy_slot(
                canvas->device, swapchain->queue_family, &swapchain->slots[i], canvas->allocator);
        }
        dvz_free(swapchain->slots);
        swapchain->slots = NULL;
    }
    if (swapchain->swapchain_layouts)
    {
        dvz_free(swapchain->swapchain_layouts);
        swapchain->swapchain_layouts = NULL;
    }
    canvas_swapchain_release_render_finished(swapchain);
}



/**
 * Reset canvas swapchain runtime fields after cleanup.
 *
 * @param swapchain canvas swapchain state to reset
 */
static void canvas_swapchain_reset_runtime(DvzCanvasSwapchain* swapchain)
{
    ANN(swapchain);
    swapchain->image_count = 0;
    swapchain->active_slot = NULL;
    swapchain->frame_index = 0;
    swapchain->last_presented_slot_index = UINT32_MAX;
    swapchain->dirty = true;
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
    canvas_swapchain_release_slot_state(swapchain);
    dvz_swapchain_destroy(swapchain->swapchain_wrapper);
    canvas_swapchain_reset_runtime(swapchain);
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
    if (!state->dirty && dvz_swapchain_handle(state->swapchain_wrapper) != VK_NULL_HANDLE)
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



/**
 * Handle queue-submit status for a canvas frame submission.
 *
 * @param state canvas swapchain state receiving the submission result
 * @param submit_res submission result code returned by the queue submit wrapper
 * @return 0 when submission can proceed to present, -1 otherwise
 */
static int canvas_handle_submit_status(DvzCanvasSwapchain* state, int32_t submit_res)
{
    ANN(state);
    ANN(state->active_slot);

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
    return 0;
}



/**
 * Handle swapchain present status transitions and failure reporting.
 *
 * @param canvas canvas owning the swapchain state
 * @param state canvas swapchain state receiving the present result
 * @param present_status present result status from vklite
 * @param image_index swapchain image index associated with the present call
 * @return 0 when present handling completed, -1 on fatal or hard failure
 */
static int canvas_handle_present_status(
    DvzCanvas* canvas, DvzCanvasSwapchain* state, DvzPresentStatus present_status,
    uint32_t image_index)
{
    ANN(canvas);
    ANN(state);

    if (present_status == DVZ_PRESENT_STATUS_RECREATE)
    {
        dvz_canvas_swapchain_mark_out_of_date(canvas);
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "present recreate");
        return 0;
    }
    if (present_status == DVZ_PRESENT_STATUS_DEVICE_LOST)
    {
        canvas_runtime_device_lost(state, "swapchain present");
        state->active_slot = NULL;
        return -1;
    }
    if (present_status != DVZ_PRESENT_STATUS_OK)
    {
        log_error(
            "failed to present swapchain image (frame=%u image=%u status=%d)", state->frame_index,
            image_index, present_status);
        state->active_slot = NULL;
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "present failed");
        return -1;
    }
    return 0;
}



/**
 * Handle swapchain acquire status transitions and failure reporting.
 *
 * @param canvas canvas owning the swapchain state
 * @param state canvas swapchain state receiving the acquire result
 * @param acquire_status acquire result status from vklite
 * @param slot_idx slot index selected for the acquire call
 * @return 0 when acquire can proceed, wait-surface code on transient recreate/extent states, -1
 * otherwise
 */
static int canvas_handle_acquire_status(
    DvzCanvas* canvas, DvzCanvasSwapchain* state, DvzPresentStatus acquire_status, uint32_t slot_idx)
{
    ANN(canvas);
    ANN(state);

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
    return 0;
}



/*************************************************************************************************/
/*  Acquire/Present helper dispatch                                                              */
/*************************************************************************************************/

/**
 * Ensure swapchain readiness before attempting image acquire.
 *
 * @param canvas canvas owning the swapchain
 * @param state swapchain runtime state
 * @return 0 when acquire can proceed, wait-surface code on transient states, -1 on failure
 */
static int canvas_swapchain_prepare_acquire(DvzCanvas* canvas, DvzCanvasSwapchain* state)
{
    ANN(canvas);
    ANN(state);

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
    return 0;
}



/**
 * Bind acquired swapchain image resources into the selected slot.
 *
 * @param state canvas swapchain state
 * @param slot slot selected for acquisition
 * @param slot_idx slot index used by this frame
 * @param image_index image index returned by swapchain acquire
 * @return 0 on success or -1 on failure
 */
static int canvas_slot_bind_acquired_image(
    DvzCanvasSwapchain* state, DvzCanvasSwapchainSlot* slot, uint32_t slot_idx, uint32_t image_index)
{
    ANN(state);
    ANN(slot);

    if (!slot->ready)
    {
        log_error("acquired swapchain slot %u is not ready", slot_idx);
        return -1;
    }

    slot->image_index = image_index;
    if (dvz_swapchain_image(state->swapchain_wrapper, image_index, &slot->swapchain_image))
    {
        // handled above
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
    return 0;
}



/**
 * Populate stream-frame metadata from the active canvas slot.
 *
 * @param state canvas swapchain state
 * @param slot acquired slot with frame resources
 * @param frame destination stream frame
 */
static void canvas_frame_from_slot(
    DvzCanvasSwapchain* state, DvzCanvasSwapchainSlot* slot, DvzStreamFrame* frame)
{
    ANN(state);
    ANN(slot);
    ANN(frame);

    frame->image = slot->offscreen_image;
    frame->memory =
        slot->offscreen_alloc != NULL ? dvz_allocation_memory(slot->offscreen_alloc) : VK_NULL_HANDLE;
    frame->memory_size =
        slot->offscreen_alloc != NULL ? dvz_allocation_size(slot->offscreen_alloc) : 0;
    frame->command_buffer = slot->command_buffer;
    frame->image_view = slot->offscreen_view;
    frame->extent = slot->offscreen_extent;
    frame->color_format = state->frame_format;
    frame->image_layout = slot->offscreen_layout;
    frame->usage = DVZ_STREAM_FRAME_USAGE_RENDER_TARGET | DVZ_STREAM_FRAME_USAGE_COPY_SRC |
                   DVZ_STREAM_FRAME_USAGE_COPY_DST;
    frame->command_buffer_recording = slot->commands_recording;
    frame->image_borrowed = true;
    frame->image_view_borrowed = true;
    frame->command_buffer_borrowed = true;
    frame->handles_dirty = slot->handles_dirty;
    frame->resource_generation = slot->resource_generation;
    frame->image_valid = frame->image != VK_NULL_HANDLE && frame->image_view != VK_NULL_HANDLE &&
                         frame->extent.width > 0 && frame->extent.height > 0;
    frame->memory_fd = slot->memory_fd;
    frame->wait_semaphore_fd = -1;
}



/**
 * Submit the active slot command buffer and timeline signal to the queue.
 *
 * @param canvas canvas owning the timeline semaphore
 * @param state canvas swapchain state with active slot selected
 * @param wait_value timeline value to signal on submit
 * @param[out] queue_out queue used for submission/presentation
 * @return 0 on success or -1 on failure
 */
static int canvas_submit_active_slot(
    DvzCanvas* canvas, DvzCanvasSwapchain* state, uint64_t wait_value, VkQueue* queue_out)
{
    ANN(canvas);
    ANN(state);
    ANN(state->active_slot);
    ANN(queue_out);

    VkCommandBuffer cmd = state->active_slot->command_buffer;
    VkPipelineStageFlags2 wait_stage =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    VkPipelineStageFlags2 signal_stage = wait_stage | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

    // Ownership contract: one slot owns one fence + the per-frame acquire semaphore, while the
    // present-wait (render_finished) semaphores are owned per swapchain image; the main queue is
    // used for both submit and present, and slot synchronization primitives are reset on next
    // acquire.
    DvzSubmit* submit = dvz_submit_create_wrapper();
    ANN(submit);
    dvz_submit(submit);
    dvz_submit_wait(submit, dvz_semaphore_handle(state->active_slot->image_available), 0, wait_stage);
    if (cmd != VK_NULL_HANDLE)
    {
        dvz_submit_command(submit, cmd);
    }
    // Signal the acquired image's own present-wait semaphore: the slot's in-flight fence only
    // proves the submit finished, not that the present consuming the semaphore has executed, so
    // a per-slot semaphore could be re-signaled while still signaled
    // (VUID-vkQueueSubmit2-semaphore-03868).
    dvz_submit_signal(
        submit, dvz_semaphore_handle(state->render_finished[state->active_slot->image_index]),
        0, signal_stage);
    dvz_submit_signal(
        submit, dvz_semaphore_handle(canvas->timeline_semaphore), wait_value,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

    VkQueue queue = state->queue;
    int32_t submit_res =
        dvz_submit_send(submit, queue, dvz_fence_handle(state->active_slot->in_flight));
    dvz_submit_free(submit);
    if (canvas_handle_submit_status(state, submit_res) != 0)
    {
        return -1;
    }
    *queue_out = queue;
    return 0;
}



/*************************************************************************************************/
/*  Acquire/Present flow helpers                                                                 */
/*************************************************************************************************/

/**
 * Mark the swapchain dirty when the live surface extent/format changed since last recreate.
 *
 * @param canvas canvas owning the surface
 * @param state swapchain runtime state to update
 */
static void canvas_swapchain_sync_surface_changes(DvzCanvas* canvas, DvzCanvasSwapchain* state)
{
    ANN(canvas);
    ANN(state);

    VkSurfaceKHR live_surface = canvas_surface_handle(canvas);
    VkSurfaceKHR wrapped_surface = state->surface_wrapper != NULL
                                       ? dvz_surface_handle(state->surface_wrapper)
                                       : VK_NULL_HANDLE;
    if (dvz_swapchain_ready(state->swapchain_wrapper) && live_surface != wrapped_surface)
    {
        state->dirty = true;
        canvas_runtime_transition(
            state, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "surface handle changed");
        return;
    }

    VkExtent2D current_extent = dvz_swapchain_extent(state->swapchain_wrapper);
    if (
        canvas->surface && dvz_swapchain_ready(state->swapchain_wrapper) &&
        (canvas->surface->extent.width != current_extent.width ||
         canvas->surface->extent.height != current_extent.height ||
         canvas_surface_format(canvas) != dvz_swapchain_image_format(state->swapchain_wrapper)))
    {
        state->dirty = true;
    }
}



/**
 * Select the acquire slot for the current frame and prepare its fence for reuse.
 *
 * @param state canvas swapchain state
 * @param[out] slot_idx_out selected slot index
 * @param[out] slot_out selected slot pointer
 * @return 0 on success or -1 when the selected slot is not reusable
 */
static int canvas_select_acquire_slot(
    DvzCanvasSwapchain* state, uint32_t* slot_idx_out, DvzCanvasSwapchainSlot** slot_out)
{
    ANN(state);
    ANN(slot_idx_out);
    ANN(slot_out);

    uint32_t slot_idx = state->frame_index % state->image_count;
    DvzCanvasSwapchainSlot* slot = &state->slots[slot_idx];
    if (!dvz_fence_wait(slot->in_flight))
    {
        log_error("failed to wait for canvas swapchain slot %u", slot_idx);
        return -1;
    }
    dvz_fence_reset(slot->in_flight);

    *slot_idx_out = slot_idx;
    *slot_out = slot;
    return 0;
}



/**
 * Acquire next swapchain image for the selected slot and map status transitions.
 *
 * @param canvas canvas owning the swapchain
 * @param state canvas swapchain state
 * @param slot slot selected for acquire
 * @param slot_idx slot index selected for acquire
 * @param[out] image_index_out acquired image index on success
 * @return 0 on success, wait-surface code on transient status, -1 on failure
 */
static int canvas_acquire_image_for_slot(
    DvzCanvas* canvas, DvzCanvasSwapchain* state, DvzCanvasSwapchainSlot* slot, uint32_t slot_idx,
    uint32_t* image_index_out)
{
    ANN(canvas);
    ANN(state);
    ANN(slot);
    ANN(image_index_out);

    uint32_t image_index = 0;
    DvzPresentStatus acquire_status = DVZ_PRESENT_STATUS_OK;
    if (!canvas_test_consume_forced_status(&state->test_force_acquire_status, &acquire_status))
    {
        acquire_status = dvz_swapchain_acquire(
            state->swapchain_wrapper, dvz_semaphore_handle(slot->image_available), UINT64_MAX,
            &image_index);
    }
    int acquire_status_rc = canvas_handle_acquire_status(canvas, state, acquire_status, slot_idx);
    if (acquire_status_rc != 0)
    {
        return acquire_status_rc;
    }
    *image_index_out = image_index;
    return 0;
}



/**
 * Check whether present can proceed with the current active slot.
 *
 * @param state canvas swapchain state
 * @return 0 when present can proceed, -1 otherwise
 */
static int canvas_present_preflight(DvzCanvasSwapchain* state)
{
    ANN(state);
    if (!state->active_slot)
    {
        return -1;
    }
    if (state->runtime_state == DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST)
    {
        log_error("canvas swapchain present aborted after device loss");
        state->active_slot = NULL;
        return -1;
    }
    return 0;
}



/**
 * Dispatch swapchain present for the active slot and process wrapper status.
 *
 * @param canvas canvas owning the swapchain
 * @param state canvas swapchain state
 * @param queue queue used for presentation
 * @return 0 on success or -1 on failure
 */
static int canvas_dispatch_present(DvzCanvas* canvas, DvzCanvasSwapchain* state, VkQueue queue)
{
    ANN(canvas);
    ANN(state);
    ANN(state->active_slot);

    uint32_t index = state->active_slot->image_index;
    DvzPresentStatus present_status = DVZ_PRESENT_STATUS_OK;
    if (!canvas_test_consume_forced_status(&state->test_force_present_status, &present_status))
    {
        // Wait on the presented image's own semaphore (see the ownership note at submit).
        present_status = dvz_swapchain_present(
            state->swapchain_wrapper, queue, index,
            dvz_semaphore_handle(state->render_finished[index]));
    }
    return canvas_handle_present_status(canvas, state, present_status, index);
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
    canvas->swapchain->last_presented_slot_index = UINT32_MAX;
    canvas->swapchain->frame_format = canvas_frame_format(canvas);
    canvas->swapchain->runtime_state = DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED;
    canvas->swapchain->test_fail_slot_index = -1;
    canvas->swapchain->test_force_recreate_status = -1;
    canvas->swapchain->test_force_acquire_status = -1;
    canvas->swapchain->test_force_present_status = -1;
    canvas->swapchain->surface_wrapper = dvz_surface_create_wrapper();
    ANN(canvas->swapchain->surface_wrapper);
    canvas->swapchain->swapchain_wrapper = dvz_swapchain_create_wrapper();
    ANN(canvas->swapchain->swapchain_wrapper);
    DvzQueue* queue_ref = dvz_device_queue(canvas->device, DVZ_QUEUE_MAIN);
    ANN(queue_ref);
    canvas->swapchain->queue = dvz_queue_handle(queue_ref);
    ASSERT(canvas->swapchain->queue != VK_NULL_HANDLE);
    canvas->swapchain->queue_family = dvz_queue_family(queue_ref);
    bool surface_initialized = false;
    bool swapchain_initialized = false;
    if (!dvz_surface_init_from_device(
            canvas->swapchain->surface_wrapper, canvas->device, canvas->swapchain->queue_family))
    {
        log_error("failed to initialize canvas surface wrapper");
        goto swapchain_init_failed;
    }
    surface_initialized = true;
    if (!dvz_swapchain_init_from_device(
            canvas->swapchain->swapchain_wrapper, canvas->device,
            canvas->swapchain->surface_wrapper))
    {
        log_error("failed to initialize canvas swapchain wrapper");
        goto swapchain_init_failed;
    }
    swapchain_initialized = true;
    canvas->swapchain->wrappers_ready = true;
    canvas_runtime_transition(
        canvas->swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "initialized wrappers");
    return 0;

swapchain_init_failed:
    canvas_runtime_transition(
        canvas->swapchain, DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE, "wrapper init failed");
    if (swapchain_initialized)
    {
        dvz_swapchain_destroy(canvas->swapchain->swapchain_wrapper);
    }
    if (surface_initialized)
    {
        dvz_surface_destroy(canvas->swapchain->surface_wrapper);
    }
    dvz_swapchain_free(canvas->swapchain->swapchain_wrapper);
    dvz_surface_free(canvas->swapchain->surface_wrapper);
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
        dvz_surface_destroy(canvas->swapchain->surface_wrapper);
        canvas->swapchain->wrappers_ready = false;
    }
    dvz_swapchain_free(canvas->swapchain->swapchain_wrapper);
    canvas->swapchain->swapchain_wrapper = NULL;
    dvz_surface_free(canvas->swapchain->surface_wrapper);
    canvas->swapchain->surface_wrapper = NULL;
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
 * @param canvas canvas whose swapchain test controls are updated
 * @param slot_index slot index to fail, or -1 to disable forced failure
 */
void dvz_canvas_swapchain_test_fail_slot(DvzCanvas* canvas, int32_t slot_index)
{
    if (!canvas || !canvas->swapchain)
    {
        return;
    }
    canvas->swapchain->test_fail_slot_index = slot_index;
}



/**
 * Force the next swapchain recreate call to return a specific present status.
 *
 * @param canvas canvas whose swapchain test controls are updated
 * @param status Present status to inject once, or -1 to disable
 */
void dvz_canvas_swapchain_test_force_recreate_status(DvzCanvas* canvas, int32_t status)
{
    if (!canvas || !canvas->swapchain)
    {
        return;
    }
    canvas->swapchain->test_force_recreate_status = status;
}



/**
 * Force the next swapchain acquire call to return a specific present status.
 *
 * @param canvas canvas whose swapchain test controls are updated
 * @param status Present status to inject once, or -1 to disable
 */
void dvz_canvas_swapchain_test_force_acquire_status(DvzCanvas* canvas, int32_t status)
{
    if (!canvas || !canvas->swapchain)
    {
        return;
    }
    canvas->swapchain->test_force_acquire_status = status;
}



/**
 * Force the next swapchain present call to return a specific present status.
 *
 * @param canvas canvas whose swapchain test controls are updated
 * @param status Present status to inject once, or -1 to disable
 */
void dvz_canvas_swapchain_test_force_present_status(DvzCanvas* canvas, int32_t status)
{
    if (!canvas || !canvas->swapchain)
    {
        return;
    }
    canvas->swapchain->test_force_present_status = status;
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



/**
 * Return the resolved present mode of the live swapchain.
 *
 * @param canvas canvas owning the swapchain
 * @param out_mode destination for the resolved present mode
 * @returns true when a live swapchain has a resolved present mode
 */
bool dvz_canvas_swapchain_present_mode(const DvzCanvas* canvas, VkPresentModeKHR* out_mode)
{
    if (out_mode == NULL)
    {
        return false;
    }
    if (
        !canvas || !canvas->swapchain || canvas->swapchain->swapchain_wrapper == NULL ||
        !dvz_swapchain_ready(canvas->swapchain->swapchain_wrapper))
    {
        return false;
    }
    *out_mode = dvz_swapchain_present_mode(canvas->swapchain->swapchain_wrapper);
    return true;
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
 * Validate capture preconditions and resolve the source slot.
 *
 * @param canvas canvas owning the swapchain
 * @param width expected frame width
 * @param height expected frame height
 * @param out_size_bytes destination buffer size in bytes
 * @param state_out resolved swapchain state on success
 * @param slot_out resolved source slot on success
 * @param expected_size_out expected destination size in bytes on success
 * @returns 0 when capture preconditions are satisfied, or -1 on failure
 */
static int canvas_capture_validate(
    DvzCanvas* canvas, uint32_t width, uint32_t height, DvzSize out_size_bytes,
    DvzCanvasSwapchain** state_out, DvzCanvasSwapchainSlot** slot_out, size_t* expected_size_out)
{
    ANN(canvas);
    ANN(state_out);
    ANN(slot_out);
    ANN(expected_size_out);

    DvzCanvasSwapchain* state = canvas_state(canvas);
    if (!state || state->image_count == 0)
    {
        log_error("canvas capture requires an initialized swapchain");
        return -1;
    }
    if (state->active_slot != NULL)
    {
        log_error("canvas capture cannot run while a frame is currently acquired");
        return -1;
    }
    if (state->last_presented_slot_index == UINT32_MAX || state->last_presented_slot_index >= state->image_count)
    {
        log_error("canvas capture requires at least one successful present");
        return -1;
    }

    VkExtent2D extent = dvz_swapchain_extent(state->swapchain_wrapper);
    if (extent.width == 0 || extent.height == 0)
    {
        log_error("canvas capture requires a non-zero swapchain extent");
        return -1;
    }

    size_t expected_size = (size_t)extent.width * (size_t)extent.height * 4;
    if (width != extent.width || height != extent.height)
    {
        log_error(
            "canvas capture dimension mismatch, expected %ux%u but got %ux%u", extent.width,
            extent.height, width, height);
        return -1;
    }
    if (out_size_bytes < expected_size)
    {
        log_error(
            "canvas capture destination buffer too small (%zu < %zu)",
            (size_t)out_size_bytes, expected_size);
        return -1;
    }

    DvzCanvasSwapchainSlot* slot = &state->slots[state->last_presented_slot_index];
    if (slot->offscreen_image == VK_NULL_HANDLE)
    {
        log_error("canvas capture source image is unavailable");
        return -1;
    }

    *state_out = state;
    *slot_out = slot;
    *expected_size_out = expected_size;
    return 0;
}



/**
 * Create a host-visible staging buffer for capture readback.
 *
 * @param canvas canvas providing device and allocator
 * @param size staging buffer size in bytes
 * @param staging output staging buffer
 * @returns 0 on success or -1 on allocation failure
 */
static int canvas_capture_create_staging(DvzCanvas* canvas, size_t size, DvzBuffer* staging)
{
    ANN(canvas);
    ANN(staging);

    dvz_buffer(canvas->device, canvas->allocator, staging);
    dvz_buffer_size(staging, size);
    dvz_buffer_flags(staging, DVZ_ALLOC_HOST_ACCESS_RANDOM | DVZ_ALLOC_MAPPED);
    dvz_buffer_usage(staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    if (dvz_buffer_create(staging) != 0)
    {
        log_error("failed to allocate staging buffer for canvas capture");
        return -1;
    }
    return 0;
}



/**
 * Record and submit capture copy commands from offscreen image to staging buffer.
 *
 * @param canvas canvas owning device resources
 * @param state swapchain state providing queue metadata
 * @param slot source slot containing the offscreen image
 * @param width capture width
 * @param height capture height
 * @param staging destination staging buffer
 * @returns 0 on success or -1 on command submission failure
 */
static int canvas_capture_copy_to_staging(
    DvzCanvas* canvas, DvzCanvasSwapchain* state, DvzCanvasSwapchainSlot* slot, uint32_t width,
    uint32_t height, DvzBuffer* staging)
{
    ANN(canvas);
    ANN(state);
    ANN(slot);
    ANN(staging);

    VkCommandBuffer cmd = dvz_command_buffer_alloc(canvas->device, state->queue_family);
    if (cmd == VK_NULL_HANDLE)
    {
        log_error("failed to allocate command buffer for canvas capture");
        return -1;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, cmd, cmds);
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);

    VkImageLayout original_layout = slot->offscreen_layout;
    if (original_layout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        original_layout = VK_IMAGE_LAYOUT_GENERAL;
    }
    canvas_cmd_transition(
        canvas, cmd, slot->offscreen_image, original_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, width, height, 1);
    dvz_cmd_copy_image_to_buffer(
        cmds, slot->offscreen_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(staging), 0);

    canvas_cmd_transition(
        canvas, cmd, slot->offscreen_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, original_layout);
    dvz_cmd_end(cmds);
    dvz_commands_free(cmds);

    DvzFence* fence = dvz_fence_create_wrapper();
    DvzSubmit* submit = dvz_submit_create_wrapper();
    ANN(fence);
    ANN(submit);
    dvz_fence(canvas->device, false, fence);
    dvz_submit(submit);
    dvz_submit_command(submit, cmd);
    int32_t submit_rc = dvz_submit_send(submit, state->queue, dvz_fence_handle(fence));
    if (submit_rc != VK_SUCCESS)
    {
        dvz_fence_destroy(fence);
        dvz_fence_free(fence);
        dvz_submit_free(submit);
        dvz_command_buffer_free(canvas->device, state->queue_family, cmd);
        log_error("failed to submit canvas capture copy commands (%d)", submit_rc);
        return -1;
    }

    dvz_fence_wait(fence);
    dvz_fence_destroy(fence);
    dvz_fence_free(fence);
    dvz_submit_free(submit);
    dvz_command_buffer_free(canvas->device, state->queue_family, cmd);
    return 0;
}



/**
 * Return whether a Vulkan format stores 8-bit color channels as BGRA in memory.
 *
 * @param format image format
 * @return true when capture readback must swap red and blue channels for RGBA output
 */
static bool canvas_capture_format_is_bgra(VkFormat format)
{
    return (
        format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB ||
        format == VK_FORMAT_B8G8R8A8_SNORM || format == VK_FORMAT_B8G8R8A8_UINT ||
        format == VK_FORMAT_B8G8R8A8_SINT);
}



/**
 * Convert tightly packed BGRA8 bytes to RGBA8 in place.
 *
 * @param rgba capture buffer
 * @param pixel_count number of pixels in the buffer
 */
static void canvas_capture_bgra_to_rgba_in_place(uint8_t* rgba, size_t pixel_count)
{
    ANN(rgba);
    for (size_t i = 0; i < pixel_count; i++)
    {
        uint8_t* px = rgba + 4 * i;
        const uint8_t b = px[0];
        px[0] = px[2];
        px[2] = b;
    }
}



/**
 * Capture the latest presented canvas frame into caller-managed RGBA storage.
 *
 * @param canvas canvas whose latest presented slot should be captured
 * @param width expected frame width
 * @param height expected frame height
 * @param out_rgba destination buffer
 * @param out_size_bytes destination buffer size in bytes
 * @returns 0 on success or -1 on failure
 */
int dvz_canvas_swapchain_capture_rgba_into(
    DvzCanvas* canvas, uint32_t width, uint32_t height, uint8_t* out_rgba,
    DvzSize out_size_bytes)
{
    ANN(canvas);
    ANN(out_rgba);
    DvzCanvasSwapchain* state = NULL;
    DvzCanvasSwapchainSlot* slot = NULL;
    size_t expected_size = 0;
    if (canvas_capture_validate(
            canvas, width, height, out_size_bytes, &state, &slot, &expected_size) != 0)
    {
        return -1;
    }

    dvz_device_wait(canvas->device);

    DvzBuffer* staging = dvz_buffer_create_wrapper();
    ANN(staging);
    if (canvas_capture_create_staging(canvas, expected_size, staging) != 0)
    {
        dvz_buffer_free(staging);
        return -1;
    }
    if (canvas_capture_copy_to_staging(canvas, state, slot, width, height, staging) != 0)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        return -1;
    }

    dvz_buffer_download(staging, 0, expected_size, out_rgba);
    if (canvas_capture_format_is_bgra(state->frame_format))
        canvas_capture_bgra_to_rgba_in_place(out_rgba, (size_t)width * (size_t)height);

    dvz_buffer_destroy(staging);
    dvz_buffer_free(staging);
    return 0;
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
    canvas_swapchain_sync_surface_changes(canvas, state);

    int prepare_rc = canvas_swapchain_prepare_acquire(canvas, state);
    if (prepare_rc != 0)
    {
        return prepare_rc;
    }

    uint32_t slot_idx = 0;
    DvzCanvasSwapchainSlot* slot = NULL;
    if (canvas_select_acquire_slot(state, &slot_idx, &slot) != 0)
    {
        return -1;
    }

    uint32_t image_index = 0;
    int acquire_status_rc = canvas_acquire_image_for_slot(canvas, state, slot, slot_idx, &image_index);
    if (acquire_status_rc != 0)
    {
        return acquire_status_rc;
    }

    state->frame_index = (state->frame_index + 1) % state->image_count;
    if (canvas_slot_bind_acquired_image(state, slot, slot_idx, image_index) != 0)
    {
        return -1;
    }

    if (canvas_slot_begin_recording(state, slot) != 0)
    {
        return -1;
    }

    state->active_slot = slot;
    canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_ACQUIRED, "acquire success");
    canvas_frame_from_slot(state, slot, frame);
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
    if (!state)
    {
        return -1;
    }
    if (canvas_present_preflight(state) != 0)
    {
        return -1;
    }
    canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_PRESENT_PENDING, "submit+present");

    if (canvas_slot_finish_recording(state, state->active_slot) != 0)
    {
        state->active_slot = NULL;
        canvas_runtime_transition(state, DVZ_CANVAS_PRESENT_STATE_READY, "recording failed");
        return -1;
    }
    VkQueue queue = VK_NULL_HANDLE;
    // log_trace("submit");
    if (canvas_submit_active_slot(canvas, state, wait_value, &queue) != 0)
    {
        return -1;
    }

    // log_trace("present");
    if (canvas_dispatch_present(canvas, state, queue) != 0)
    {
        return -1;
    }

    state->last_presented_slot_index =
        (state->frame_index + state->image_count - 1) % state->image_count;
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
