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

#include "../_scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_QUERY_PAYLOAD_WORDS 4
#define DVZ_SCENE_QUERY_FLAG_COMPAT_PROBE 0x80000000u



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSceneQueryFamilyOps DvzSceneQueryFamilyOps;
typedef struct DvzSceneQueryBuildContext DvzSceneQueryBuildContext;
typedef struct DvzSceneQueryDecodeContext DvzSceneQueryDecodeContext;
typedef struct DvzSceneQueryReadoutContext DvzSceneQueryReadoutContext;
typedef struct DvzSceneQueryPlan DvzSceneQueryPlan;

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
/*  Structs                                                                                      */
/*************************************************************************************************/

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
    DvzSceneProbePlan scratch;
    DvzSampledField* field;
    uint32_t target_width;
    uint32_t target_height;
    uint32_t format;
    uint32_t byte_size;
    uint32_t texel_x;
    uint32_t texel_y;
    double uvw[3];
    bool mark_image_probe_static_uploaded;
    DvzVisual* image_probe_visual;
    uint64_t image_probe_position_version;
    uint64_t image_probe_texcoord_version;
    uint64_t image_probe_texture_version;
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

const DvzSceneQueryFamilyOps* _dvz_scene_query_segment_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_path_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_primitive_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_mesh_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_image_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_labels_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_volume_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_text_ops(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_glyph_ops(void);
