/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan emission internals                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DRP2_ID_COLOR_TARGET 1
#define DRP2_ID_ENCODER 2
#define DRP2_ID_RENDER_PASS 3
#define DRP2_ID_COMMAND_BUFFER 4
#define DRP2_ID_SUBMISSION 5
#define DRP2_ID_COMPUTE_PASS 6
#define DRP2_ID_PIPELINE 10
#define DRP2_ID_COMPUTE_PIPELINE 30
#define DRP2_ID_RESOURCE_BASE 20
#define DRP2_ID_READBACK_BUFFER 12
#define DRP2_ID_BIND_GROUP_LAYOUT 100
#define DRP2_ID_BIND_GROUP 13
#define DRP2_ID_SAMPLER 200
#define DRP2_ID_VERTEX_SHADER 9000
#define DRP2_ID_FRAGMENT_SHADER 9001
#define DRP2_ID_COMPUTE_SHADER 9002
#define DRP2_MAX_FIXTURE_RESOURCES 64
#define DRP2_RUNTIME_TRANSIENT_ID_BASE 10000
#define DRP2_EMITTER_OBJECT_ID_BASE 5000
#define DVZ_SCENE_COMMON_CACHE_CAPACITY (2 * DVZ_SCENE_MAX_PANELS)
#define DVZ_SCENE_VOLUME_CACHE_CAPACITY DVZ_SCENE_MAX_VISUALS
#define DVZ_SCENE_LABELS_CACHE_CAPACITY DVZ_SCENE_MAX_VISUALS
#define DVZ_SCENE_LABELS_HIDDEN_VEC4_COUNT ((DVZ_LABELS_MAX_HIDDEN + 3u) / 4u)
#define DVZ_SCENE_LABELS_FLAG_SELECTED 0x01u
#define DVZ_SCENE_LABELS_FLAG_BOUNDARY 0x02u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResourceId ResourceId;
typedef struct ConverterState ConverterState;
typedef struct SceneRenderStateCache SceneRenderStateCache;
typedef struct DvzSceneLabelsUniform DvzSceneLabelsUniform;
typedef struct DvzSceneVolumeUniform DvzSceneVolumeUniform;

struct DvzSceneLabelsUniform
{
    uint32_t ids[4]; /* background bits, selected bits, reserved, reserved */
    uint32_t params[4]; /* flags, fallback seed, hidden count, reserved */
    float floats[4]; /* opacity, boundary width in texels, reserved, reserved */
    float boundary_color[4];
    uint32_t hidden_ids[DVZ_SCENE_LABELS_HIDDEN_VEC4_COUNT][4];
    uint32_t label_lookup[DVZ_SCENE_LABELS_LOOKUP_CAPACITY][4];
};

struct DvzSceneVolumeUniform
{
    float clip_min[4];
    float clip_max[4];
    float clip_plane[4];
    float clip_plane_params[4];
    float params[4];
    float slice[4];
    float bounds_min[4];
    float bounds_max[4];
    float axis_order[4];
    float axis_flip[4];
    float value_range[4];
    float occlusion[4];
};

struct ResourceId
{
    char key[DVZ_SCENE_LABEL_SIZE];
    uint64_t id;
    char data_tag[DVZ_SCENE_LABEL_SIZE]; /* attribute name, e.g. "position", "color", "size" */
    uint64_t byte_size;                  /* total bytes uploaded to this buffer               */
    uint64_t logical_item_count;          /* logical items covered by the current payload      */
    uint32_t usage;                      /* DRP2 buffer usage flags                           */
    uint32_t item_stride;                /* optional element stride (index buffers)          */
    uint32_t topology;                   /* primitive topology hint (UINT32_MAX = unset)      */
    uint32_t texture_width;              /* allocated texture width, when this is a texture   */
    uint32_t texture_height;             /* allocated texture height, when this is a texture  */
    uint32_t texture_depth;              /* allocated texture depth, when this is a texture   */
    uint32_t texture_format;             /* texture format, when this is a texture            */
    uint32_t texture_sample_count;       /* texture sample count, defaulting to one           */
    DvzFramePlanResourceKind kind;        /* typed resource kind, when supplied by FramePlan   */
    DvzFramePlanResourceRole role;        /* typed resource role, when supplied by FramePlan   */
    DvzColorRole color_role;              /* texture color role, when supplied by FramePlan    */
};



struct ConverterState
{
    uint32_t count;
    uint32_t capacity;
    uint64_t next_id;
    uint64_t first_vertex_buffer_id;
    uint64_t first_texture_id;
    uint64_t first_compute_input_id;
    uint64_t first_compute_output_id;
    uint64_t compute_buffer_size;
    ResourceId* resources;
};



struct SceneRenderStateCache
{
    uint64_t pipeline_id;
    uint64_t bg_set0;
};



struct DvzFramePlanEmitter
{
    ConverterState resources;
    ConverterState objects;
    uint64_t next_transient_id;
    bool handshake_sent;
    uint32_t max_color_sample_count;
    uint32_t max_depth_sample_count;

    /* Common cache: APPLY and FIXED slots are panel-specific once viewport is part of set 0. */
    char mvp_panel_ids[DVZ_SCENE_COMMON_CACHE_CAPACITY][DVZ_SCENE_LABEL_SIZE];
    DvzMVP mvp_cache[DVZ_SCENE_COMMON_CACHE_CAPACITY];
    uint32_t mvp_panel_count;
    char viewport_panel_ids[DVZ_SCENE_COMMON_CACHE_CAPACITY][DVZ_SCENE_LABEL_SIZE];
    DvzSceneViewportUniform viewport_cache[DVZ_SCENE_COMMON_CACHE_CAPACITY];
    uint32_t viewport_panel_count;
    char volume_ids[DVZ_SCENE_VOLUME_CACHE_CAPACITY][DVZ_SCENE_LABEL_SIZE];
    DvzSceneVolumeUniform volume_cache[DVZ_SCENE_VOLUME_CACHE_CAPACITY];
    uint32_t volume_count;
    char labels_ids[DVZ_SCENE_LABELS_CACHE_CAPACITY][DVZ_SCENE_LABEL_SIZE];
    DvzSceneLabelsUniform labels_cache[DVZ_SCENE_LABELS_CACHE_CAPACITY];
    uint32_t labels_count;
};



/*************************************************************************************************/
/*  Runtime emitter state                                                                        */
/*************************************************************************************************/

void _state_init(ConverterState* state);

void _state_destroy(ConverterState* state);

uint64_t _emitter_next_transient_id(DvzFramePlanEmitter* emitter);

DvzMVP* _emitter_mvp_slot(DvzFramePlanEmitter* emitter, const char* key);

DvzSceneViewportUniform*
_emitter_viewport_slot(DvzFramePlanEmitter* emitter, const char* key);

DvzSceneVolumeUniform*
_emitter_volume_slot(DvzFramePlanEmitter* emitter, const char* key);

DvzSceneLabelsUniform*
_emitter_labels_slot(DvzFramePlanEmitter* emitter, const char* key);

uint64_t _resource_id(ConverterState* state, const char* key);

uint64_t _resource_lookup_id(const ConverterState* state, const char* key);

ResourceId* _resource_find(ConverterState* state, const char* key);

ResourceId* _resource_entry(ConverterState* state, const char* key, bool* is_new);

bool _resource_ensure_byte_size(
    ConverterState* state, ResourceId* resource, uint64_t required_size, bool* needs_create);

bool _resource_ensure_texture_2d(
    ConverterState* state, ResourceId* resource, uint32_t width, uint32_t height,
    bool* needs_create);

bool _resource_ensure_texture(
    ConverterState* state, ResourceId* resource, uint32_t width, uint32_t height,
    uint32_t depth, uint32_t format, bool* needs_create);

const char* _resource_data_tag(const ConverterState* state, uint64_t id);

DvzFramePlanResourceRole _resource_role(const ConverterState* state, uint64_t id);

uint64_t _resource_byte_size(const ConverterState* state, uint64_t id);

uint32_t _resource_usage(const ConverterState* state, uint64_t id);

uint64_t _resource_logical_item_count(const ConverterState* state, uint64_t id);

uint32_t _resource_item_stride(const ConverterState* state, uint64_t id);

uint32_t _resource_topology(const ConverterState* state, uint64_t id);

uint64_t _obj_id(DvzFramePlanEmitter* emitter, const char* key, bool* is_new);

uint64_t
_obj_buffer_id(DvzFramePlanEmitter* emitter, const char* key, uint64_t byte_size, bool* is_new);



/*************************************************************************************************/
/*  Shared emit helpers                                                                          */
/*************************************************************************************************/

bool _zero_base64(uint64_t byte_size, char* out, uint64_t out_size);

char* _zero_base64_alloc(uint64_t byte_size);

const DvzFramePlanNode* _first_node_of_type(
    const DvzFramePlan* plan, DvzFramePlanNodeType type);

bool _render_uses_texture(const DvzFramePlanNode* node);

void _diagnostic(DvzDiagnosticReport* report, const char* message);

bool _validate_capabilities(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report);

uint64_t _color_target_id(const DvzFramePlanEmitConfig* cfg);
