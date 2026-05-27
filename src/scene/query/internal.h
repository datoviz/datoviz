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

typedef bool (*DvzSceneQueryEligible)(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSceneQueryPayload
{
    uint32_t words[DVZ_SCENE_QUERY_PAYLOAD_WORDS];
};


struct DvzSceneQueryFamilyOps
{
    const char* name;
    DvzSceneVisualFamily family;
    uint32_t pick_capabilities;
    uint32_t query_flags;
    DvzSceneQueryEligible eligible;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

uint32_t _dvz_scene_query_registry_count(void);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_get(uint32_t index);

const DvzSceneQueryFamilyOps* _dvz_scene_query_registry_find(DvzSceneVisualFamily family);

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
