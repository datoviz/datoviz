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



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_QUERY_PAYLOAD_WORDS 4



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
typedef bool (*DvzSceneQueryBuild)(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan);
typedef bool (*DvzSceneQueryExecute)(
    const DvzSceneQueryBuildContext* ctx, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzSceneQueryPlan* plan, uint8_t* bytes,
    uint32_t byte_size, bool* out_executed);
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
    bool mark_image_query_static_uploaded;
    DvzVisual* image_query_visual;
    uint64_t image_query_position_version;
    uint64_t image_query_texcoord_version;
    uint64_t image_query_texture_version;
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
    DvzSceneQueryBuild build;
    DvzSceneQueryExecute execute;
    DvzSceneQueryDecode decode;
    DvzSceneQueryReadout readout;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

uint32_t _dvz_scene_query_registry_count(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_get(uint32_t index);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_find(DvzSceneVisualFamily family);

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
