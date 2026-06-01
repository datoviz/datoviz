/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query internals                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_QUERY_PAYLOAD_WORDS 4
#define DVZ_SCENE_QUERY_STATIC_CACHE_KEY_COUNT 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSceneQueryFamilyOps DvzSceneQueryFamilyOps;
typedef struct DvzSceneQueryBuildContext DvzSceneQueryBuildContext;
typedef struct DvzSceneQueryDecodeContext DvzSceneQueryDecodeContext;
typedef struct DvzSceneQueryReadoutContext DvzSceneQueryReadoutContext;
typedef struct DvzSceneQueryPlan DvzSceneQueryPlan;
typedef struct DvzSceneQuerySchema DvzSceneQuerySchema;

typedef bool (*DvzSceneQueryEligible)(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request);
typedef bool (*DvzSceneQuerySupportsProfile)(
    const DvzSceneQueryBuildContext* ctx, DvzQueryProfile profile);
typedef bool (*DvzSceneQueryBuild)(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan);
typedef bool (*DvzSceneQueryExecute)(
    const DvzSceneQueryBuildContext* ctx, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzSceneQueryPlan* plan, uint8_t* bytes,
    uint32_t byte_size, bool* out_executed);
typedef bool (*DvzSceneQueryRejectUnsupported)(
    const DvzVisual* visual, const DvzQueryRequest* request, DvzQueryStatus* out_status);
typedef bool (*DvzSceneQueryDecode)(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result);
typedef bool (*DvzSceneQueryReadout)(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_QUERY_SCHEMA_FIELD_NONE         = 0,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_VISUAL_ID    = 1u << 0,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_ITEM_ID      = 1u << 1,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_SAMPLE_VALUE = 1u << 2,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_LABEL_ID     = 1u << 3,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_UVW          = 1u << 4,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_VOXEL_COORD  = 1u << 5,
    DVZ_SCENE_QUERY_SCHEMA_FIELD_DISPLAY_RGBA = 1u << 6,
} DvzSceneQuerySchemaField;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSceneQuerySchema
{
    uint32_t fields;
    DvzQueryValueKind value_kind;
    DvzQueryProfile profile;
    uint32_t format;
    uint32_t byte_size;
};

struct DvzSceneQueryBuildContext
{
    DvzFigure* figure;
    DvzPanel* panel;
    DvzVisual* visual;
    DvzSceneRequestExecutor* executor;
    const DvzPendingQueryRequest* pending;
    const DvzCapabilitySnapshot* caps;
    DvzQueryProfile profile;
    vec2 request_ndc;
};


struct DvzSceneQueryScratch
{
    DvzFramePlan* plan;
    vec3* query_positions;
    vec2* query_texcoords;
    DvzColor* query_colors;
    uint32_t* query_ids;
    float* query_position_start;
    float* query_position_curr;
    float* query_position_end;
    float* query_line_width;
    uint32_t* query_path_flags;
    float* query_path_distance;
    uint32_t* query_indices;
};


struct DvzSceneQueryPlan
{
    DvzSceneQueryScratch scratch;
    DvzSampledField* field;
    uint32_t target_width;
    uint32_t target_height;
    uint32_t format;
    uint32_t byte_size;
    DvzSceneQuerySchema schema;
    double uvw[3];
    bool mark_static_cache_uploaded;
    DvzSceneVisualFamily static_cache_family;
    DvzVisual* static_cache_visual;
    uint64_t static_cache_keys[DVZ_SCENE_QUERY_STATIC_CACHE_KEY_COUNT];
    uint32_t static_cache_key_count;
};


struct DvzSceneQueryDecodeContext
{
    const DvzSceneQueryBuildContext* build;
    const DvzSceneQueryPlan* plan;
    const uint8_t* bytes;
    uint32_t byte_size;
};


struct DvzSceneQueryReadoutContext
{
    const DvzSceneQueryBuildContext* build;
    const DvzSceneQueryPlan* plan;
};


struct DvzSceneQueryFamilyOps
{
    const char* name;
    DvzSceneVisualFamily family;
    DvzSceneQueryEligible eligible;
    DvzSceneQuerySupportsProfile supports_profile;
    DvzSceneQueryBuild build;
    DvzSceneQueryExecute execute;
    DvzSceneQueryRejectUnsupported reject_unsupported;
    DvzSceneQueryDecode decode;
    DvzSceneQueryReadout readout;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

uint64_t _scene_next_request_serial(DvzScene* scene);

bool _scene_query_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc);

uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel);

void _scene_request_apply_mvp(const DvzPanel* panel, const vec2 request_ndc, DvzMVP* out);

void _scene_request_visual_mvp(
    const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc, DvzMVP* out);

void _scene_request_executor_init(DvzSceneRequestExecutor* executor);

void _scene_request_executor_destroy(DvzSceneRequestExecutor* executor);

bool _scene_request_executor_prepare(
    DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime);

uint32_t _dvz_scene_query_registry_count(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_get(uint32_t index);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_find(DvzSceneVisualFamily family);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_find_visual_type(DvzVisualType type);

uint32_t _dvz_scene_query_target_capability(DvzSceneTargetKind target);

DvzQueryProfile _dvz_scene_query_select_profile(
    const DvzQueryRequest* request, const DvzCapabilitySnapshot* caps);

bool _dvz_scene_query_framebuffer_position(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y,
    uint32_t out_position[2]);

DvzVisual* _dvz_scene_query_candidate_visual(const DvzPanel* panel, uint32_t capability);

const DvzSceneQueryFamilyOps* _dvz_scene_query_family_ops_for_visual(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request);

bool _dvz_scene_query_item_target_eligible(
    const DvzVisual* visual, const DvzQueryRequest* request, DvzVisualType type);

bool _dvz_scene_query_execute_family(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, DvzQueryProfile profile, DvzVisual* visual,
    const DvzSceneQueryFamilyOps* ops, DvzQueryResult* out_result);

bool _dvz_scene_query_process_pending(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps,
    const DvzPendingQueryRequest* pending, DvzQueryResult* out_result);

bool _dvz_scene_query_execute_readback(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint32_t color_format,
    uint8_t* bytes, uint32_t byte_size, bool* out_executed);

void _dvz_scene_query_drop_superseded_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id);

bool _dvz_scene_query_push_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial,
    const DvzQueryResult* result);

bool _dvz_scene_query_decode_item_id(
    const DvzSceneQueryDecodeContext* ctx, DvzSceneVisualFamily family,
    DvzQueryResult* out_result);

void _scene_query_scratch_destroy(DvzSceneQueryScratch* plan);

bool _dvz_scene_query_alloc(
    const char* label, void** out_ptr, uint64_t count, uint64_t item_size);

bool _dvz_scene_query_dense_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr);

bool _dvz_scene_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height);

void _dvz_scene_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc,
    uint32_t target_width, uint32_t target_height);

const DvzSceneQueryFamilyOps* _dvz_scene_query_point_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_pixel_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_marker_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_sphere_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_vector_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_segment_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_path_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_primitive_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_mesh_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_image_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_labels_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_volume_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_text_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_glyph_ops(void);
