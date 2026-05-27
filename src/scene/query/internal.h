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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSceneQueryPayload DvzSceneQueryPayload;
typedef struct DvzSceneQueryFamilyOps DvzSceneQueryFamilyOps;
typedef struct DvzSceneQueryBuildContext DvzSceneQueryBuildContext;
typedef struct DvzSceneQueryDecodeContext DvzSceneQueryDecodeContext;
typedef struct DvzSceneQueryReadoutContext DvzSceneQueryReadoutContext;
typedef struct DvzSceneQueryPlan DvzSceneQueryPlan;

typedef bool (*DvzSceneQueryEligible)(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request);
typedef bool (*DvzSceneQueryBuild)(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan);
typedef bool (*DvzSceneQueryDecode)(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result);
typedef bool (*DvzSceneQueryReadout)(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSceneQueryPayload
{
    uint32_t words[DVZ_SCENE_QUERY_PAYLOAD_WORDS];
};


struct DvzSceneQueryBuildContext
{
    DvzFigure* figure;
    DvzPanel* panel;
    DvzVisual* visual;
    const DvzPendingQueryRequest* pending;
    const DvzCapabilitySnapshot* caps;
    DvzQueryProfile profile;
    vec2 request_ndc;
};


struct DvzSceneQueryPlan
{
    DvzSceneProbePlan scratch;
    uint32_t target_width;
    uint32_t target_height;
    uint32_t format;
    uint32_t byte_size;
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
    uint32_t pick_capabilities;
    uint32_t query_flags;
    DvzSceneQueryEligible eligible;
    DvzSceneQueryBuild build;
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

bool _dvz_scene_query_execute_readback(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint32_t color_format,
    uint8_t* bytes, uint32_t byte_size, bool* out_executed);

void _dvz_scene_query_from_pick(const DvzPickResult* pick, DvzQueryResult* out_result);

void _dvz_scene_query_from_probe(const DvzProbeResult* probe, DvzQueryResult* out_result);

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
