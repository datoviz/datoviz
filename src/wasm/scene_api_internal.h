/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic WASM scene ABI internals                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "_assertions.h"
#include "_compat.h"
#include "core/_scene.h"
#include "core/frame_artifact_internal.h"
#include "datoviz/drp2.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "datoviz/scene/frame_packets.h"
#include "datoviz/vk/enums.h"
#include "frame_plan/emit.h"
#include "interaction/animation_internal.h"
#include "query/internal.h"
#include "runner/scenario_runner.h"
#include "visuals/_visual_pipeline.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WASM_API_MAX_WRAPPERS        256
#define DVZ_WASM_VISUAL_POINT            1
#define DVZ_WASM_VISUAL_PIXEL            2
#define DVZ_WASM_VISUAL_MARKER           3
#define DVZ_WASM_VISUAL_SEGMENT          4
#define DVZ_WASM_VISUAL_PATH             5
#define DVZ_WASM_VISUAL_IMAGE            6
#define DVZ_WASM_VISUAL_MESH             7
#define DVZ_WASM_VISUAL_GLYPH            8
#define DVZ_WASM_VISUAL_PRIMITIVE        9
#define DVZ_WASM_VISUAL_SPHERE           10
#define DVZ_WASM_VISUAL_TEXT             11
#define DVZ_WASM_VISUAL_LABELS           12
#define DVZ_WASM_API_SCENARIO_COUNT      80
#define DVZ_WASM_QUERY_RESOURCE_ID_BASE  20000
#define DVZ_WASM_QUERY_OBJECT_ID_BASE    40000
#define DVZ_WASM_QUERY_TRANSIENT_ID_BASE 60000

#define DVZ_WASM_BROWSER_SUPPORTED_REQUIREMENTS                                                   \
    (DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_PIXEL_VISUAL |                              \
     DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_MESH_VISUAL |                              \
     DVZ_SCENARIO_REQ_IMAGE_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |                               \
     DVZ_SCENARIO_REQ_SCENE_BUFFERS | DVZ_SCENARIO_REQ_STORAGE_BUFFERS |                          \
     DVZ_SCENARIO_REQ_SCENE_COMPUTE | DVZ_SCENARIO_REQ_QUERY_READBACK |                           \
     DVZ_SCENARIO_REQ_FRAME_CALLBACKS | DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES |                      \
     DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_ARCBALL)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzWasmApiScene DvzWasmApiScene;

typedef struct
{
    DvzWasmApiScene* owner;
    DvzFigure* figure;
} DvzWasmApiFigure;


typedef struct
{
    DvzWasmApiScene* owner;
    DvzPanel* panel;
} DvzWasmApiPanel;


typedef struct
{
    DvzWasmApiScene* owner;
    DvzVisual* visual;
} DvzWasmApiVisual;


typedef struct
{
    DvzWasmApiScene* owner;
    DvzSceneBuffer* buffer;
} DvzWasmApiBuffer;


typedef struct
{
    DvzWasmApiScene* owner;
    DvzController* controller;
} DvzWasmApiController;


typedef struct
{
    DvzWasmApiScene* owner;
    DvzAxis* axis;
} DvzWasmApiAxis;


struct DvzWasmApiScene
{
    DvzScene* scene;
    DvzScenarioContext scenario_ctx;
    DvzScenarioSpec scenario_spec;
    void* scenario_user;
    DvzWasmApiFigure* scenario_figure;
    DvzSceneFrameArtifact* frame_artifact;
    DvzInputRouter* router;
    DvzPointerGestureHandler* gestures;
    DvzCapabilitySnapshot caps;
    char* json;
    DvzDiagnosticReport report;
    int packet_status;
    uint64_t resource_version;
    uint64_t frame_index;
    DvzPendingQueryRequest query_pending;
    DvzSceneQueryBuildContext query_build;
    DvzSceneQueryPlan query_plan;
    DvzQueryResult query_result;
    const DvzSceneQueryFamilyOps* query_ops;
    DvzVisualType query_visual_type;
    DvzSceneVisualFamily query_family;
    DvzPanel* query_panel;
    bool query_active;
    void* wrappers[DVZ_WASM_API_MAX_WRAPPERS];
    uint32_t wrapper_count;
    uint32_t width;
    uint32_t height;
    uint32_t logical_width;
    uint32_t logical_height;
    float device_scale;
    uint32_t color_format;
    bool scenario_active;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static inline DvzWasmApiScene* _scene(uint32_t handle)
{
    return (DvzWasmApiScene*)(uintptr_t)handle;
}


static inline DvzWasmApiFigure* _figure(uint32_t handle)
{
    return (DvzWasmApiFigure*)(uintptr_t)handle;
}


static inline DvzWasmApiPanel* _panel(uint32_t handle)
{
    return (DvzWasmApiPanel*)(uintptr_t)handle;
}


static inline DvzWasmApiVisual* _visual(uint32_t handle)
{
    return (DvzWasmApiVisual*)(uintptr_t)handle;
}


static inline DvzWasmApiBuffer* _buffer(uint32_t handle)
{
    return (DvzWasmApiBuffer*)(uintptr_t)handle;
}


static inline DvzWasmApiController* _controller(uint32_t handle)
{
    return (DvzWasmApiController*)(uintptr_t)handle;
}


static inline DvzWasmApiAxis* _axis(uint32_t handle) { return (DvzWasmApiAxis*)(uintptr_t)handle; }


static inline uint32_t _handle(void* ptr) { return (uint32_t)(uintptr_t)ptr; }


DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags);

DvzCapabilitySnapshot _wasm_capability_snapshot(void);
void _clear_query(DvzWasmApiScene* scene);
void _clear_payload(DvzWasmApiScene* scene);
bool _ensure_query_emitter(DvzScene* owner);
int _fail(DvzWasmApiScene* scene, const char* diagnostic);
uint32_t _fail_handle(DvzWasmApiScene* scene, const char* diagnostic);
int _fail_upload(DvzWasmApiScene* scene, const char* kind, const char* attr, uint32_t item_count);
bool _remember(DvzWasmApiScene* scene, void* wrapper);
void _emit_resize(
    DvzWasmApiScene* scene, uint32_t logical_width, uint32_t logical_height,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float device_scale);
