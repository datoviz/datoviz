/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime internals                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/runtime.h"

#if DVZ_DRP2_HAS_VKLITE
#include <volk.h>
#include "datoviz/stream/frame_stream.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/compute.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_RUNTIME_INITIAL_OBJECT_CAPACITY 64
#define DVZ_DRP2_RGBA8_BYTES_PER_TEXEL 4



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DRP2_OBJECT_NONE,
    DRP2_OBJECT_BUFFER,
    DRP2_OBJECT_TEXTURE,
    DRP2_OBJECT_SHADER_VERTEX,
    DRP2_OBJECT_SHADER_FRAGMENT,
    DRP2_OBJECT_SHADER_COMPUTE,
    DRP2_OBJECT_RENDER_PIPELINE,
    DRP2_OBJECT_COMPUTE_PIPELINE,
    DRP2_OBJECT_SAMPLER,
    DRP2_OBJECT_BIND_GROUP_LAYOUT,
    DRP2_OBJECT_BIND_GROUP,
    DRP2_OBJECT_ENCODER,
    DRP2_OBJECT_RENDER_PASS,
    DRP2_OBJECT_COMPUTE_PASS,
    DRP2_OBJECT_COMMAND_BUFFER,
} Drp2ObjectKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct Drp2Object Drp2Object;
typedef struct Drp2RuntimeState Drp2RuntimeState;
#if DVZ_DRP2_HAS_VKLITE
typedef struct Drp2VkliteObject Drp2VkliteObject;
typedef struct Drp2VkliteState Drp2VkliteState;
#endif

struct DvzDrp2Runtime
{
    DvzDevice* device;
    DvzVma* allocator;
    bool semantic_only;
    Drp2RuntimeState* semantic_state;
#if DVZ_DRP2_HAS_VKLITE
    Drp2VkliteState* vklite_state;
#endif
};


struct Drp2Object
{
    uint64_t id;
    Drp2ObjectKind kind;
    uint64_t size;
    uint32_t usage;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t vertex_buffer_slots;
    uint64_t vertex_shader_module_id;
    uint64_t fragment_shader_module_id;
    uint64_t compute_shader_module_id;
    uint64_t bind_group_layout_id;
    uint64_t bind_group_layout_id2;
    uint64_t texture_id;
    uint64_t sampler_id;
    uint64_t buffer0_id;
    uint64_t buffer1_id;
    uint64_t buffer_size;
    bool destroyed;
    bool referenced_by_work;
    bool open;
    bool submitted;
    uint64_t encoder_id;
    uint64_t pipeline_id;
    uint32_t bound_vertex_mask;
    bool index_buffer_bound;
    uint32_t bound_bind_group_mask;
    uint64_t render_pipeline_id;
    uint32_t render_bound_vertex_mask;
    bool render_index_buffer_bound;
    uint32_t render_bound_bind_group_mask;
    bool storage_buffers;
    bool uniform_buffer;
    bool has_depth_attachment;
    bool depth_write_enabled;
    uint32_t depth_compare_op;
    float viewport_x;
    float viewport_y;
    float viewport_width;
    float viewport_height;
    float scissor_x;
    float scissor_y;
    float scissor_width;
    float scissor_height;
};


struct Drp2RuntimeState
{
    bool hello_seen;
    bool reply_seen;
    bool failed;
    uint32_t capacity;
    uint32_t count;
    Drp2Object* objects;
};

#if DVZ_DRP2_HAS_VKLITE
struct Drp2VkliteObject
{
    uint64_t id;
    Drp2ObjectKind kind;
    DvzBuffer* buffer;
    DvzImages* images;
    DvzImageViews* views;
    DvzShader* shader;
    DvzGraphics* graphics;
    DvzCompute* compute;
    DvzSlots* slots;
    DvzDescriptors* descriptors;
    DvzSampler* sampler;
    DvzCommands* commands;
    DvzRendering* rendering;
    DvzImages* depth_images;
    DvzImageViews* depth_views;
    VkCommandBuffer command_buffer;
    VkImageView image_view;
    VkImageLayout image_layout;
    uint64_t texture_id;
    uint64_t sampler_id;
    uint32_t width;
    uint32_t height;
    float viewport_x;
    float viewport_y;
    float viewport_width;
    float viewport_height;
    float scissor_x;
    float scissor_y;
    float scissor_width;
    float scissor_height;
    uint64_t current_pipeline_id;
    VkPipelineLayout combined_pipeline_layout; /* owned combined layout for two-set pipelines */
    VkDevice         combined_layout_device;   /* VkDevice needed to destroy combined_pipeline_layout */
    bool borrowed_slots;
    bool borrowed_commands;
    bool borrowed_frame_target;
    bool destroyed;
};


typedef struct Drp2DeferredDestroy Drp2DeferredDestroy;

struct Drp2DeferredDestroy
{
    VkCommandBuffer command_buffer;
    Drp2VkliteObject object;
};


struct Drp2VkliteState
{
    DvzDrp2Runtime* runtime;
    uint32_t capacity;
    uint32_t count;
    Drp2VkliteObject* objects;
    uint32_t deferred_capacity;
    uint32_t deferred_count;
    Drp2DeferredDestroy* deferred;
    VkCommandBuffer active_borrowed_command_buffer;
};



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

bool _vklite_ensure_capacity(Drp2VkliteState* state);
Drp2VkliteObject* _vklite_find(Drp2VkliteState* state, uint64_t id);
Drp2VkliteObject* _vklite_add(
    Drp2VkliteState* state, uint64_t id, Drp2ObjectKind kind);
void _vklite_destroy_object(Drp2VkliteObject* object);
VkImageView _vklite_object_image_view(const Drp2VkliteObject* object);
void _vklite_state_cleanup(Drp2VkliteState* state);
bool _vklite_defer_destroy_object(
    Drp2VkliteState* state, Drp2VkliteObject* object, VkCommandBuffer command_buffer);
void _vklite_flush_deferred_for_command_buffer(
    Drp2VkliteState* state, VkCommandBuffer command_buffer);
void _vklite_owned_commands_destroy(DvzCommands* cmds);
void _vklite_borrowed_frame_commands_free(DvzCommands* cmds);
#endif
