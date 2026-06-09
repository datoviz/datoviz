/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic WASM scene ABI                                                                       */
/*************************************************************************************************/



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

#include "datoviz/drp2.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "datoviz/scene/frame_packets.h"
#include "datoviz/vk/enums.h"
#include "_assertions.h"
#include "_compat.h"
#include "core/_scene.h"
#include "core/frame_artifact_internal.h"
#include "frame_plan/emit.h"
#include "interaction/animation_internal.h"
#include "query/internal.h"
#include "runner/scenario_runner.h"
#include "visuals/_visual_pipeline.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WASM_API_MAX_WRAPPERS 256
#define DVZ_WASM_VISUAL_POINT 1
#define DVZ_WASM_VISUAL_PIXEL 2
#define DVZ_WASM_VISUAL_MARKER 3
#define DVZ_WASM_VISUAL_SEGMENT 4
#define DVZ_WASM_VISUAL_PATH 5
#define DVZ_WASM_VISUAL_IMAGE 6
#define DVZ_WASM_VISUAL_MESH 7
#define DVZ_WASM_VISUAL_GLYPH 8
#define DVZ_WASM_VISUAL_PRIMITIVE 9
#define DVZ_WASM_VISUAL_SPHERE 10
#define DVZ_WASM_VISUAL_TEXT 11
#define DVZ_WASM_VISUAL_LABELS 12
#define DVZ_WASM_API_SCENARIO_COUNT 13
#define DVZ_WASM_QUERY_RESOURCE_ID_BASE 20000
#define DVZ_WASM_QUERY_OBJECT_ID_BASE 40000
#define DVZ_WASM_QUERY_TRANSIENT_ID_BASE 60000

#define DVZ_WASM_BROWSER_SUPPORTED_REQUIREMENTS                                                       \
    (DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_PIXEL_VISUAL |                                 \
     DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_MESH_VISUAL |                                 \
     DVZ_SCENARIO_REQ_IMAGE_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |                                  \
     DVZ_SCENARIO_REQ_SCENE_BUFFERS | DVZ_SCENARIO_REQ_QUERY_READBACK |                              \
     DVZ_SCENARIO_REQ_FRAME_CALLBACKS |                                                             \
     DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_ARCBALL)



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags);

DvzScenarioSpec dvz_example_animation_tracks_scenario(void);
DvzScenarioSpec dvz_example_basic_scene_scenario(void);
DvzScenarioSpec dvz_example_builtin_shapes_2d_scenario(void);
DvzScenarioSpec dvz_example_builtin_shapes_3d_scenario(void);
DvzScenarioSpec dvz_example_timer_animation_scenario(void);
DvzScenarioSpec dvz_example_triangulation_polygon_scenario(void);
DvzScenarioSpec dvz_example_picking_scenario(void);
DvzScenarioSpec dvz_example_image_probe_scenario(void);
DvzScenarioSpec dvz_example_isolines_scenario(void);
DvzScenarioSpec dvz_example_obj_loading_scenario(void);
DvzScenarioSpec dvz_example_selection_mesh_instances_scenario(void);
DvzScenarioSpec dvz_example_selection_pixel_scenario(void);
DvzScenarioSpec dvz_example_selection_sphere_scenario(void);



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

static DvzCapabilitySnapshot _wasm_capability_snapshot(void)
{
    DvzCapabilitySnapshot caps = {0};
    caps.struct_size = DVZ_STRUCT_SIZE(DvzCapabilitySnapshot);
    caps.flags = 0;
    caps.max_buffer_size = 256 * 1024 * 1024;
    caps.max_texture_dimension_2d = 4096;
    caps.max_bind_groups = 4;
    caps.max_vertex_buffers = 8;
    caps.max_color_attachments = 1;
    caps.max_color_sample_count = 16;
    caps.max_depth_sample_count = 16;
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.supports_readback = true;
    caps.min_texture_copy_bytes_per_row_alignment = 4;
    caps.max_readback_size = caps.max_buffer_size;
    caps.texture_format_r32uint = true;
    caps.texture_format_rg32uint = true;
    caps.render_target_format_r32uint = true;
    caps.render_target_format_rg32uint = true;
    caps.query_profile_u32_r32 = true;
    caps.query_profile_u64_rg32 = true;
    caps.query_profile_u64_2xr32 = true;
    return caps;
}

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
    uint32_t color_format;
    bool scenario_active;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzWasmApiScene* _scene(uint32_t handle) { return (DvzWasmApiScene*)(uintptr_t)handle; }



static DvzWasmApiFigure* _figure(uint32_t handle) { return (DvzWasmApiFigure*)(uintptr_t)handle; }



static DvzWasmApiPanel* _panel(uint32_t handle) { return (DvzWasmApiPanel*)(uintptr_t)handle; }



static DvzWasmApiVisual* _visual(uint32_t handle) { return (DvzWasmApiVisual*)(uintptr_t)handle; }



static DvzWasmApiBuffer* _buffer(uint32_t handle) { return (DvzWasmApiBuffer*)(uintptr_t)handle; }



static DvzWasmApiController* _controller(uint32_t handle)
{
    return (DvzWasmApiController*)(uintptr_t)handle;
}



static DvzWasmApiAxis* _axis(uint32_t handle) { return (DvzWasmApiAxis*)(uintptr_t)handle; }



static uint32_t _handle(void* ptr) { return (uint32_t)(uintptr_t)ptr; }



static int _fail(DvzWasmApiScene* scene, const char* diagnostic);



static uint32_t _fail_handle(DvzWasmApiScene* scene, const char* diagnostic);



int dvz_scenario_bind_controller(
    DvzScenarioContext* ctx, DvzPanel* panel, DvzController* controller, DvzDimMask dims)
{
    if (ctx == NULL || panel == NULL || controller == NULL)
        return -1;

    for (uint32_t i = 0; i < ctx->controller_binding_count; i++)
    {
        DvzScenarioControllerBinding* binding = &ctx->controller_bindings[i];
        if (binding->panel == panel && binding->controller == controller && binding->dims == dims)
            return dvz_panel_bind_controller(panel, controller, dims);
    }

    if (ctx->controller_binding_count >= DVZ_SCENARIO_MAX_CONTROLLER_BINDINGS)
        return -1;
    if (dvz_panel_bind_controller(panel, controller, dims) != 0)
        return -1;

    DvzScenarioControllerBinding* binding =
        &ctx->controller_bindings[ctx->controller_binding_count++];
    binding->panel = panel;
    binding->controller = controller;
    binding->dims = dims;
    return 0;
}



DvzPanzoom* dvz_scenario_panzoom(
    DvzScenarioContext* ctx, DvzPanel* panel, const DvzPanzoomDesc* desc, DvzDimMask dims)
{
    if (ctx == NULL || ctx->scene == NULL || panel == NULL)
        return NULL;

    DvzController* controller = dvz_panzoom(ctx->scene, desc);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    if (panzoom == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, dims) != 0)
        return NULL;
    return panzoom;
}



bool dvz_scenario_panel_pointer_position(
    const DvzPanel* panel, const DvzScenarioPointerEvent* event, double* out_x, double* out_y)
{
    if (panel == NULL || event == NULL || out_x == NULL || out_y == NULL)
        return false;

    DvzRect rect = {0};
    if (!dvz_panel_inner_rect_px(panel, &rect) || rect.width <= 0.0f || rect.height <= 0.0f)
        return false;

    float x = event->x - rect.x;
    float y = event->y - rect.y;
    if (x < 0.0f || x >= rect.width || y < 0.0f || y >= rect.height)
        return false;

    *out_x = (double)x;
    *out_y = (double)y;
    return true;
}



int dvz_scenario_panel_query(
    DvzPanel* panel, double x, double y, const DvzQueryRequest* request)
{
    if (panel == NULL || request == NULL)
        return -1;
    return dvz_panel_query(panel, x, y, request);
}



static void _clear_query(DvzWasmApiScene* scene)
{
    if (scene == NULL)
        return;
    if (scene->query_active)
        _scene_query_scratch_destroy(&scene->query_plan.scratch);
    if (scene->frame_artifact != NULL)
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
    scene->frame_artifact = NULL;
    scene->packet_status = 0;
    scene->query_pending = (DvzPendingQueryRequest){0};
    scene->query_build = (DvzSceneQueryBuildContext){0};
    scene->query_plan = (DvzSceneQueryPlan){0};
    scene->query_result = (DvzQueryResult){0};
    scene->query_ops = NULL;
    scene->query_visual_type = DVZ_VISUAL_TYPE_NONE;
    scene->query_family = DVZ_SCENE_VISUAL_FAMILY_NONE;
    scene->query_panel = NULL;
    scene->query_active = false;
}



static DvzScenarioSpec _scenario_spec(uint32_t index)
{
    switch (index)
    {
    case 0:
        return dvz_example_basic_scene_scenario();
    case 1:
        return dvz_example_timer_animation_scenario();
    case 2:
        return dvz_example_triangulation_polygon_scenario();
    case 3:
        return dvz_example_builtin_shapes_2d_scenario();
    case 4:
        return dvz_example_builtin_shapes_3d_scenario();
    case 5:
        return dvz_example_isolines_scenario();
    case 6:
        return dvz_example_animation_tracks_scenario();
    case 7:
        return dvz_example_obj_loading_scenario();
    case 8:
        return dvz_example_picking_scenario();
    case 9:
        return dvz_example_selection_pixel_scenario();
    case 10:
        return dvz_example_selection_sphere_scenario();
    case 11:
        return dvz_example_selection_mesh_instances_scenario();
    case 12:
        return dvz_example_image_probe_scenario();
    default:
        return (DvzScenarioSpec){0};
    }
}



static const char* _requirement_name(uint64_t bit)
{
    switch (bit)
    {
    case DVZ_SCENARIO_REQ_POINT_VISUAL:
        return "point";
    case DVZ_SCENARIO_REQ_PIXEL_VISUAL:
        return "pixel";
    case DVZ_SCENARIO_REQ_MARKER_VISUAL:
        return "marker";
    case DVZ_SCENARIO_REQ_MESH_VISUAL:
        return "mesh";
    case DVZ_SCENARIO_REQ_IMAGE_VISUAL:
        return "image";
    case DVZ_SCENARIO_REQ_TEXT_VISUAL:
        return "text";
    case DVZ_SCENARIO_REQ_SCENE_BUFFERS:
        return "scene-buffers";
    case DVZ_SCENARIO_REQ_STORAGE_BUFFERS:
        return "storage-buffers";
    case DVZ_SCENARIO_REQ_SCENE_COMPUTE:
        return "scene-compute";
    case DVZ_SCENARIO_REQ_QUERY_READBACK:
        return "query-readback";
    case DVZ_SCENARIO_REQ_FRAME_CALLBACKS:
        return "frame-callbacks";
    case DVZ_SCENARIO_REQ_NATIVE_CAPTURE:
        return "native-capture";
    case DVZ_SCENARIO_REQ_NATIVE_VIEW:
        return "native-view";
    case DVZ_SCENARIO_REQ_CONTROLLER:
        return "controller";
    case DVZ_SCENARIO_REQ_PANZOOM:
        return "panzoom";
    case DVZ_SCENARIO_REQ_ARCBALL:
        return "arcball";
    default:
        return "unknown";
    }
}



static uint64_t _scenario_effective_requirements(const DvzScenarioSpec* spec)
{
    uint64_t requirements = spec != NULL ? spec->requirements : 0;
    if (spec != NULL && spec->frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->post_frame != NULL)
        requirements |= DVZ_SCENARIO_REQ_FRAME_CALLBACKS;
    if (spec != NULL && spec->native_view != NULL)
        requirements |= DVZ_SCENARIO_REQ_NATIVE_VIEW;
    return requirements;
}



static int _fail_unsupported_requirements(
    DvzWasmApiScene* scene, const DvzScenarioSpec* spec, uint64_t unsupported)
{
    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
    const char* scenario_id = spec != NULL && spec->id != NULL ? spec->id : "<unknown>";
    int written = snprintf(
        diagnostic, sizeof(diagnostic), "WASM scenario '%s' has unsupported requirements: ",
        scenario_id);
    if (written < 0 || (size_t)written >= sizeof(diagnostic))
        return _fail(scene, "WASM scenario has unsupported requirements");

    size_t offset = (size_t)written;
    bool first = true;
    for (uint32_t bit_index = 0; bit_index < 64; bit_index++)
    {
        const uint64_t bit = 1ull << bit_index;
        if ((unsupported & bit) == 0)
            continue;
        const char* name = _requirement_name(bit);
        written = snprintf(
            diagnostic + offset, sizeof(diagnostic) - offset, "%s%s", first ? "" : ",", name);
        if (written < 0 || (size_t)written >= sizeof(diagnostic) - offset)
            return _fail(scene, "WASM scenario has unsupported requirements");
        offset += (size_t)written;
        first = false;
    }
    return _fail(scene, diagnostic);
}



static void _clear_payload(DvzWasmApiScene* scene)
{
    if (scene == NULL)
        return;
    if (scene->json != NULL)
    {
        dvz_drp2_stream_json_destroy(scene->json);
        scene->json = NULL;
    }
    if (scene->frame_artifact != NULL)
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
    scene->frame_artifact = NULL;
    scene->packet_status = 0;
    dvz_diagnostic_report_init(&scene->report);
}



static void _query_result_init(
    const DvzFigure* figure, const DvzPendingQueryRequest* pending, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(pending);
    ANN(out_result);
    *out_result = (DvzQueryResult){0};
    out_result->request_id = pending->request.request_id;
    out_result->freshness_serial = pending->freshness_serial;
    out_result->status = DVZ_QUERY_STATUS_UNKNOWN;
    out_result->panel_id = _scene_panel_public_id(figure, pending->panel);
    out_result->panel_position[0] = pending->x;
    out_result->panel_position[1] = pending->y;
    (void)_dvz_scene_query_framebuffer_position(
        figure, pending->panel, pending->x, pending->y, out_result->framebuffer_position);
    out_result->raw_target = pending->request.target;
    out_result->resolved_target = pending->request.target;
    out_result->value_kind = DVZ_QUERY_VALUE_NONE;
}



static void _remove_pending_query_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_query_count);
    for (uint32_t i = index + 1; i < scene->pending_query_count; i++)
        scene->pending_queries[i - 1] = scene->pending_queries[i];
    scene->pending_query_count--;
    dvz_memset(
        &scene->pending_queries[scene->pending_query_count], sizeof(DvzPendingQueryRequest), 0,
        sizeof(DvzPendingQueryRequest));
}



static bool _push_immediate_query_result(
    DvzWasmApiScene* scene, DvzPanel* panel, uint64_t freshness_serial,
    const DvzQueryResult* result)
{
    ANN(scene);
    ANN(scene->scene);
    ANN(result);
    return _dvz_scene_query_push_result(scene->scene, panel, freshness_serial, result);
}



static const char* _query_family_name(const DvzSceneQueryFamilyOps* ops)
{
    return ops != NULL && ops->name != NULL ? ops->name : "<none>";
}



static void _query_setup_diagnostic(
    DvzWasmApiScene* scene, const char* reason, const DvzSceneQueryFamilyOps* ops,
    DvzVisualType visual_type, uint64_t visual_id, DvzSceneTargetKind target,
    DvzQueryProfile profile)
{
    if (scene == NULL || reason == NULL)
        return;

    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
    int ret = snprintf(
        diagnostic, sizeof(diagnostic),
        "WASM query setup failed: family=%s visual_type=%u visual_id=%llu target=%u "
        "profile=%u reason=%s",
        _query_family_name(ops), (uint32_t)visual_type, (unsigned long long)visual_id,
        (uint32_t)target, (uint32_t)profile, reason);
    if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
        (void)dvz_diagnostic_report_add(&scene->report, "WASM query setup failed");
    else
        (void)dvz_diagnostic_report_add(&scene->report, diagnostic);
}



static int _emit_current_query_packets(DvzWasmApiScene* scene, DvzDrp2CommandStream* stream)
{
    ANN(scene);
    ANN(stream);
    scene->frame_index++;
    scene->resource_version++;
    scene->frame_artifact = _scene_frame_artifact(stream, scene->resource_version, scene->frame_index);
    if (scene->frame_artifact == NULL)
    {
        (void)_fail(scene, "WASM query frame artifact creation failed");
        scene->packet_status = -2;
        return -1;
    }
    if (dvz_scene_frame_artifact_status(scene->frame_artifact) !=
        DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK)
    {
        (void)_fail(scene, "WASM query DRP2 packet encoding failed");
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
        scene->packet_status = -2;
        return -1;
    }
    scene->packet_status = 0;
    return 0;
}



static bool _ensure_query_emitter(DvzScene* owner)
{
    ANN(owner);
    DvzSceneRequestExecutor* executor = &owner->query_executor;
    if (executor->emitter != NULL)
        return true;

    executor->emitter = dvz_frame_plan_emitter();
    if (executor->emitter == NULL)
        return false;
    executor->emitter->resources.next_id = DVZ_WASM_QUERY_RESOURCE_ID_BASE;
    executor->emitter->objects.next_id = DVZ_WASM_QUERY_OBJECT_ID_BASE;
    executor->emitter->next_transient_id = DVZ_WASM_QUERY_TRANSIENT_ID_BASE;
    executor->emitter_create_count++;
    return true;
}



static void _mark_query_static_upload(DvzWasmApiScene* scene)
{
    ANN(scene);
    DvzSceneRequestExecutor* executor = &scene->scene->query_executor;
    const DvzSceneQueryPlan* plan = &scene->query_plan;
    if (
        !plan->mark_static_cache_uploaded || plan->static_cache_visual == NULL ||
        plan->static_cache_key_count > DVZ_SCENE_QUERY_STATIC_CACHE_KEY_COUNT)
    {
        return;
    }
    executor->query_static_cache_family = plan->static_cache_family;
    executor->query_static_cache_visual = plan->static_cache_visual;
    executor->query_static_cache_key_count = plan->static_cache_key_count;
    for (uint32_t i = 0; i < plan->static_cache_key_count; i++)
        executor->query_static_cache_keys[i] = plan->static_cache_keys[i];
    executor->query_static_cache_upload_count++;
}


static bool _query_emit_result(
    DvzWasmApiScene* scene, DvzFigure* figure, const DvzPendingQueryRequest* pending,
    DvzQueryResult* out_result)
{
    ANN(scene);
    ANN(figure);
    ANN(pending);
    ANN(out_result);
    _query_result_init(figure, pending, out_result);

    vec2 request_ndc = {0};
    if (!_scene_query_request_ndc(figure, pending->panel, pending->x, pending->y, request_ndc))
    {
        out_result->status = DVZ_QUERY_STATUS_OUTSIDE_PANEL;
        return false;
    }

    uint32_t capability = _dvz_scene_query_target_capability(pending->request.target);
    if (capability == 0)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
        return false;
    }

    out_result->profile = _dvz_scene_query_select_profile(&pending->request, &scene->caps);
    if (out_result->profile == DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        out_result->status = scene->caps.supports_readback
                                 ? DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE
                                 : DVZ_QUERY_STATUS_READBACK_FAILED;
        return false;
    }

    bool attempted = false;
    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(pending->panel, order);
    for (int32_t oi = (int32_t)pending->panel->visual_count - 1; oi >= 0; oi--)
    {
        const DvzPanelAttach* attach = &pending->panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        if ((visual->query_capabilities & capability) == 0)
            continue;

        attempted = true;
        uint64_t visual_id = _scene_visual_public_id(figure->scene, visual);
        const DvzSceneQueryFamilyOps* ops =
            _dvz_scene_query_family_ops_for_visual(pending->panel, visual, &pending->request);
        if (ops == NULL || ops->build == NULL || ops->decode == NULL)
        {
            out_result->visual_id = visual_id;
            out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
            _query_setup_diagnostic(
                scene, "missing visual-family query operations", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }
        DvzSceneVisualFamily family = ops->family;

        DvzSceneQueryBuildContext build = {
            .figure = figure,
            .panel = pending->panel,
            .visual = visual,
            .executor = &scene->scene->query_executor,
            .pending = pending,
            .caps = &scene->caps,
            .profile = out_result->profile,
        };
        build.request_ndc[0] = request_ndc[0];
        build.request_ndc[1] = request_ndc[1];
        bool supports_profile = ops->supports_profile != NULL
                                    ? ops->supports_profile(&build, out_result->profile)
                                    : out_result->profile == DVZ_QUERY_PROFILE_U32_R32;
        if (!supports_profile)
        {
            out_result->visual_id = visual_id;
            out_result->visual_family = family;
            out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE;
            _query_setup_diagnostic(
                scene, "unsupported query profile for visual family", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }

        DvzSceneQueryPlan plan = {0};
        bool built = ops->build(&build, &plan);
        if (!built)
        {
            _scene_query_scratch_destroy(&plan.scratch);
            out_result->visual_id = visual_id;
            out_result->visual_family = family;
            out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
            _query_setup_diagnostic(
                scene, "visual-family query plan build failed", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }
        if (!dvz_frame_plan_render_metadata_complete(plan.scratch.plan))
        {
            out_result->visual_id = visual_id;
            out_result->visual_family = family;
            out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
            _scene_query_scratch_destroy(&plan.scratch);
            _query_setup_diagnostic(
                scene, "query frame plan render metadata incomplete", ops, visual->type,
                visual_id, pending->request.target, out_result->profile);
            return false;
        }

        out_result->visual_id = visual_id;
        out_result->visual_family = family;
        scene->query_pending = *pending;
        scene->query_build = build;
        scene->query_build.pending = &scene->query_pending;
        scene->query_plan = plan;
        scene->query_result = *out_result;
        scene->query_ops = ops;
        scene->query_visual_type = visual->type;
        scene->query_family = family;
        scene->query_panel = pending->panel;
        scene->query_active = true;
        return true;
    }

    DvzVisual* visual = _dvz_scene_query_candidate_visual(pending->panel, capability);
    if (visual == NULL)
    {
        out_result->status = DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL;
        return false;
    }

    out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
    if (attempted)
        out_result->status = DVZ_QUERY_STATUS_MISS;
    else
    {
        const DvzSceneQueryFamilyOps* fallback_ops =
            _dvz_scene_query_registry_find_visual_type(visual->type);
        DvzQueryStatus unsupported_status = DVZ_QUERY_STATUS_UNKNOWN;
        if (
            fallback_ops != NULL && fallback_ops->reject_unsupported != NULL &&
            fallback_ops->reject_unsupported(visual, &pending->request, &unsupported_status))
        {
            out_result->status = unsupported_status;
        }
        else
        {
            out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
        }
    }
    return false;
}



static int _fail(DvzWasmApiScene* scene, const char* diagnostic)
{
    if (scene != NULL)
    {
        _clear_payload(scene);
        if (diagnostic != NULL)
            (void)dvz_diagnostic_report_add(&scene->report, diagnostic);
    }
    return -1;
}



static uint32_t _fail_handle(DvzWasmApiScene* scene, const char* diagnostic)
{
    (void)_fail(scene, diagnostic);
    return 0;
}



static int _fail_upload(
    DvzWasmApiScene* scene, const char* kind, const char* attr, uint32_t item_count)
{
    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
    int ret = snprintf(
        diagnostic, sizeof(diagnostic), "WASM %s visual upload failed: attr=%s item_count=%u",
        kind != NULL ? kind : "data", attr != NULL ? attr : "<null>", item_count);
    if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
        return _fail(scene, "WASM visual upload failed");
    return _fail(scene, diagnostic);
}



static bool _remember(DvzWasmApiScene* scene, void* wrapper)
{
    if (scene == NULL || wrapper == NULL || scene->wrapper_count >= DVZ_WASM_API_MAX_WRAPPERS)
        return false;
    scene->wrappers[scene->wrapper_count++] = wrapper;
    return true;
}



static DvzScenarioPointerType _scenario_pointer_type_from_wasm(DvzPointerEventType type)
{
    switch (type)
    {
    case DVZ_POINTER_EVENT_RELEASE:
        return DVZ_SCENARIO_POINTER_RELEASE;
    case DVZ_POINTER_EVENT_PRESS:
        return DVZ_SCENARIO_POINTER_PRESS;
    case DVZ_POINTER_EVENT_MOVE:
        return DVZ_SCENARIO_POINTER_MOVE;
    case DVZ_POINTER_EVENT_CLICK:
        return DVZ_SCENARIO_POINTER_CLICK;
    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        return DVZ_SCENARIO_POINTER_DOUBLE_CLICK;
    case DVZ_POINTER_EVENT_DRAG_START:
        return DVZ_SCENARIO_POINTER_DRAG_START;
    case DVZ_POINTER_EVENT_DRAG:
        return DVZ_SCENARIO_POINTER_DRAG;
    case DVZ_POINTER_EVENT_DRAG_STOP:
        return DVZ_SCENARIO_POINTER_DRAG_STOP;
    case DVZ_POINTER_EVENT_WHEEL:
        return DVZ_SCENARIO_POINTER_WHEEL;
    default:
        return DVZ_SCENARIO_POINTER_NONE;
    }
}



static int _connect_scenario_controller_bindings(DvzWasmApiScene* scene)
{
    if (scene == NULL || scene->router == NULL)
        return _fail(scene, "invalid WASM scenario input router");
    if (scene->scenario_ctx.controller_binding_count == 0)
        return 0;

    for (uint32_t i = 0; i < scene->scenario_ctx.controller_binding_count; i++)
    {
        DvzPanel* panel = scene->scenario_ctx.controller_bindings[i].panel;
        if (panel == NULL)
            return _fail(scene, "invalid WASM scenario controller binding");
        if (dvz_panel_connect_input(panel, scene->router) != 0)
            return _fail(scene, "WASM scenario panel input connection failed");
    }

    if (scene->gestures == NULL)
    {
        scene->gestures = dvz_pointer_gesture_handler(scene->router);
        if (scene->gestures == NULL)
            return _fail(scene, "WASM scenario pointer gesture setup failed");
    }
    return 0;
}



static void _emit_resize(
    DvzWasmApiScene* scene, uint32_t width, uint32_t height, float device_scale)
{
    if (scene == NULL || scene->router == NULL)
        return;

    DvzInputResizeEvent resize = {
        .framebuffer_width = width,
        .framebuffer_height = height,
        .window_width = device_scale > 0.0f ? (uint32_t)((float)width / device_scale) : width,
        .window_height = device_scale > 0.0f ? (uint32_t)((float)height / device_scale) : height,
        .content_scale_x = device_scale > 0.0f ? device_scale : 1.0f,
        .content_scale_y = device_scale > 0.0f ? device_scale : 1.0f,
    };
    dvz_input_emit_resize(scene->router, &resize);
}



/*************************************************************************************************/
/*  Scene lifecycle                                                                              */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scene(uint32_t width, uint32_t height)
{
    if (width == 0)
        width = 640;
    if (height == 0)
        height = 640;

    DvzWasmApiScene* scene = (DvzWasmApiScene*)calloc(1, sizeof(DvzWasmApiScene));
    if (scene == NULL)
        return 0;
    dvz_diagnostic_report_init(&scene->report);
    scene->caps = _wasm_capability_snapshot();
    scene->width = width;
    scene->height = height;
    scene->color_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    scene->scene = dvz_scene();
    scene->router = dvz_input_router();
    if (scene->scene == NULL || scene->router == NULL || !_ensure_query_emitter(scene->scene))
    {
        if (scene->router != NULL)
            dvz_input_router_destroy(scene->router);
        if (scene->scene != NULL)
            dvz_scene_destroy(scene->scene);
        free(scene);
        return 0;
    }
    return _handle(scene);
}



EMSCRIPTEN_KEEPALIVE
void dvz_wasm_api_scene_destroy(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return;
    _clear_query(scene);
    _clear_payload(scene);
    if (scene->scenario_active && scene->scenario_spec.destroy != NULL)
    {
        scene->scenario_spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        scene->scenario_active = false;
    }
    if (scene->gestures != NULL)
    {
        dvz_pointer_gesture_handler_destroy(scene->gestures);
        scene->gestures = NULL;
    }
    if (scene->router != NULL)
    {
        dvz_input_router_destroy(scene->router);
        scene->router = NULL;
    }
    if (scene->scene != NULL)
    {
        dvz_scene_destroy(scene->scene);
        scene->scene = NULL;
    }
    for (uint32_t i = 0; i < scene->wrapper_count; i++)
        free(scene->wrappers[i]);
    free(scene);
}



/*************************************************************************************************/
/*  Portable scenarios                                                                           */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_count(void) { return DVZ_WASM_API_SCENARIO_COUNT; }



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_id(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return _handle((void*)spec.id);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_title(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return _handle((void*)spec.title);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_width(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return spec.width;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_height(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return spec.height;
}



EMSCRIPTEN_KEEPALIVE
double dvz_wasm_api_scenario_fps(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    return spec.fps;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_requirements(uint32_t index)
{
    DvzScenarioSpec spec = _scenario_spec(index);
    const uint64_t requirements = _scenario_effective_requirements(&spec);
    return requirements <= UINT32_MAX ? (uint32_t)requirements : 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_create(uint32_t scene_handle, uint32_t index)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return _fail(scene, "invalid WASM scenario scene handle");
    if (scene->scenario_active || scene->wrapper_count != 0)
        return _fail(scene, "WASM scenario must be the first object created in a scene");

    DvzScenarioSpec spec = _scenario_spec(index);
    if (spec.id == NULL || spec.init == NULL)
        return _fail(scene, "invalid WASM scenario index");

    const uint64_t requirements = _scenario_effective_requirements(&spec);
    const uint64_t unsupported = requirements & ~DVZ_WASM_BROWSER_SUPPORTED_REQUIREMENTS;
    if (unsupported != 0)
        return _fail_unsupported_requirements(scene, &spec, unsupported);

    _clear_payload(scene);
    scene->scenario_spec = spec;
    if (scene->width == 0)
        scene->width = spec.width != 0 ? spec.width : 640;
    if (scene->height == 0)
        scene->height = spec.height != 0 ? spec.height : 640;
    if (spec.fps > 0)
    {
        dvz_scene_set_clock_mode(scene->scene, DVZ_CLOCK_OFFLINE);
        dvz_scene_set_fps(scene->scene, spec.fps);
    }
    scene->scenario_ctx = (DvzScenarioContext){
        .scene = scene->scene,
        .width = scene->width,
        .height = scene->height,
    };

    if (!spec.init(&scene->scenario_ctx, &scene->scenario_user) ||
        scene->scenario_ctx.figure == NULL)
    {
        scene->scenario_user = NULL;
        return _fail(scene, "WASM scenario init failed");
    }
    if (_connect_scenario_controller_bindings(scene) != 0)
    {
        if (spec.destroy != NULL)
            spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        return -1;
    }

    DvzWasmApiFigure* figure = (DvzWasmApiFigure*)calloc(1, sizeof(DvzWasmApiFigure));
    if (figure == NULL)
    {
        if (spec.destroy != NULL)
            spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        return _fail(scene, "WASM scenario figure wrapper allocation failed");
    }
    figure->owner = scene;
    figure->figure = scene->scenario_ctx.figure;
    if (!_remember(scene, figure))
    {
        free(figure);
        if (spec.destroy != NULL)
            spec.destroy(&scene->scenario_ctx, scene->scenario_user);
        scene->scenario_user = NULL;
        return _fail(scene, "WASM scenario figure wrapper registration failed");
    }
    scene->scenario_figure = figure;
    scene->scenario_active = true;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_scenario_figure(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active || scene->scenario_figure == NULL)
        return _fail_handle(scene, "WASM scenario has no active figure");
    return _handle(scene->scenario_figure);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_frame(uint32_t scene_handle, double t, double dt)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario frame requested without an active scenario");
    _clear_payload(scene);
    scene->scenario_ctx.time = t;
    scene->scenario_ctx.dt = dt;
    const uint64_t wall_time_ns = t > 0 ? (uint64_t)(t * 1000000000.0) : 0;
    _dvz_scene_animations_step(scene->scene, wall_time_ns);
    if (scene->scenario_spec.frame != NULL)
        scene->scenario_spec.frame(&scene->scenario_ctx, scene->scenario_user);
    scene->scenario_ctx.frame_index++;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_post_frame(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario post-frame requested without an active scenario");
    if (scene->scenario_spec.post_frame == NULL)
        return 0;
    scene->scenario_spec.post_frame(&scene->scenario_ctx, scene->scenario_user);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_pointer(
    uint32_t scene_handle, int type, float x, float y, int button, int mods, float content_scale,
    double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario pointer requested without an active scenario");
    if (scene->scenario_spec.event == NULL)
        return 0;

    _clear_payload(scene);
    DvzScenarioEvent event = {0};
    event.kind = DVZ_SCENARIO_EVENT_POINTER;
    event.content.pointer.type = _scenario_pointer_type_from_wasm((DvzPointerEventType)type);
    event.content.pointer.x = x;
    event.content.pointer.y = y;
    event.content.pointer.content_scale = content_scale > 0.0f ? content_scale : 1.0f;
    event.content.pointer.button = button >= 0 ? (uint32_t)button : 0;
    event.content.pointer.modifiers = mods >= 0 ? (uint32_t)mods : 0;
    event.content.pointer.timestamp_ns =
        timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    scene->scenario_spec.event(&scene->scenario_ctx, &event, scene->scenario_user);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_scenario_wheel(
    uint32_t scene_handle, float x, float y, float dir_x, float dir_y, int mods,
    float content_scale, double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !scene->scenario_active)
        return _fail(scene, "WASM scenario wheel requested without an active scenario");
    if (scene->scenario_spec.event == NULL)
        return 0;

    _clear_payload(scene);
    DvzScenarioEvent event = {0};
    event.kind = DVZ_SCENARIO_EVENT_POINTER;
    event.content.pointer.type = DVZ_SCENARIO_POINTER_WHEEL;
    event.content.pointer.x = x;
    event.content.pointer.y = y;
    event.content.pointer.dx = dir_x;
    event.content.pointer.dy = dir_y;
    event.content.pointer.content_scale = content_scale > 0.0f ? content_scale : 1.0f;
    event.content.pointer.modifiers = mods >= 0 ? (uint32_t)mods : 0;
    event.content.pointer.timestamp_ns =
        timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    scene->scenario_spec.event(&scene->scenario_ctx, &event, scene->scenario_user);
    return 0;
}



/*************************************************************************************************/
/*  Objects                                                                                      */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_figure(uint32_t scene_handle, uint32_t width, uint32_t height)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    if (width == 0)
        width = scene->width;
    if (height == 0)
        height = scene->height;
    _clear_payload(scene);
    DvzWasmApiFigure* figure = (DvzWasmApiFigure*)calloc(1, sizeof(DvzWasmApiFigure));
    if (figure == NULL)
        return _fail_handle(scene, "WASM figure wrapper allocation failed");
    figure->owner = scene;
    figure->figure = dvz_figure(scene->scene, width, height, 0);
    if (figure->figure == NULL || !_remember(scene, figure))
    {
        free(figure);
        return _fail_handle(scene, "WASM figure creation failed");
    }
    scene->width = width;
    scene->height = height;
    return _handle(figure);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_panel_full(uint32_t figure_handle)
{
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (figure == NULL || figure->owner == NULL || figure->figure == NULL)
        return 0;
    _clear_payload(figure->owner);
    DvzWasmApiPanel* panel = (DvzWasmApiPanel*)calloc(1, sizeof(DvzWasmApiPanel));
    if (panel == NULL)
        return _fail_handle(figure->owner, "WASM panel wrapper allocation failed");
    panel->owner = figure->owner;
    panel->panel = dvz_panel_full(figure->figure);
    if (panel->panel == NULL || !_remember(figure->owner, panel))
    {
        free(panel);
        return _fail_handle(figure->owner, "WASM panel creation failed");
    }
    return _handle(panel);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_visual(uint32_t scene_handle, uint32_t visual_type, uint32_t flags)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    _clear_payload(scene);
    DvzWasmApiVisual* visual = (DvzWasmApiVisual*)calloc(1, sizeof(DvzWasmApiVisual));
    if (visual == NULL)
        return _fail_handle(scene, "WASM visual wrapper allocation failed");
    visual->owner = scene;
    switch (visual_type)
    {
    case DVZ_WASM_VISUAL_POINT:
        visual->visual = dvz_point(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_PIXEL:
        visual->visual = dvz_pixel(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_MARKER:
        visual->visual = dvz_marker(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_SEGMENT:
        visual->visual = dvz_segment(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_PATH:
        visual->visual = dvz_path(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_PRIMITIVE:
        visual->visual = dvz_primitive(scene->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
        break;
    case DVZ_WASM_VISUAL_IMAGE:
        visual->visual = dvz_image(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_MESH:
        visual->visual = dvz_mesh(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_GLYPH:
        visual->visual = dvz_glyph(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_SPHERE:
        visual->visual = dvz_sphere(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_TEXT:
        visual->visual = _scene_text_visual(scene->scene, flags);
        break;
    case DVZ_WASM_VISUAL_LABELS:
        visual->visual = dvz_labels(scene->scene, flags);
        break;
    default:
        free(visual);
        return _fail_handle(scene, "unsupported WASM visual type");
        break;
    }
    if (visual->visual == NULL || !_remember(scene, visual))
    {
        free(visual);
        return _fail_handle(scene, "WASM visual creation failed");
    }
    return _handle(visual);
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_buffer(
    uint32_t scene_handle, uint32_t usage, uint32_t stride, uint32_t byte_size)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    if (usage == 0 || stride == 0)
        return _fail_handle(scene, "invalid WASM scene buffer descriptor");
    _clear_payload(scene);

    DvzWasmApiBuffer* buffer = (DvzWasmApiBuffer*)calloc(1, sizeof(DvzWasmApiBuffer));
    if (buffer == NULL)
        return _fail_handle(scene, "WASM scene buffer wrapper allocation failed");
    buffer->owner = scene;
    DvzSceneBufferDesc desc = dvz_scene_buffer_desc();
    desc.usage = usage;
    desc.stride = stride;
    desc.byte_size = byte_size;
    buffer->buffer = dvz_scene_buffer(scene->scene, &desc);
    if (buffer->buffer == NULL || !_remember(scene, buffer))
    {
        free(buffer);
        return _fail_handle(scene, "WASM scene buffer creation failed");
    }
    return _handle(buffer);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_buffer_set_data(
    uint32_t buffer_handle, const void* data, uint32_t byte_size)
{
    DvzWasmApiBuffer* buffer = _buffer(buffer_handle);
    if (
        buffer == NULL || buffer->owner == NULL || buffer->buffer == NULL || data == NULL ||
        byte_size == 0)
    {
        return _fail(buffer != NULL ? buffer->owner : NULL, "invalid WASM scene buffer upload");
    }
    _clear_payload(buffer->owner);
    if (!dvz_scene_buffer_set_data(buffer->buffer, data, byte_size))
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic), "WASM scene buffer upload failed: byte_size=%u",
            byte_size);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(buffer->owner, "WASM scene buffer upload failed");
        return _fail(buffer->owner, diagnostic);
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_controller(uint32_t scene_handle, uint32_t controller_type)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return 0;
    _clear_payload(scene);
    DvzWasmApiController* controller =
        (DvzWasmApiController*)calloc(1, sizeof(DvzWasmApiController));
    if (controller == NULL)
        return _fail_handle(scene, "WASM controller wrapper allocation failed");
    controller->owner = scene;
    switch ((DvzControllerType)controller_type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        controller->controller = dvz_panzoom(scene->scene, NULL);
        break;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        controller->controller = dvz_arcball(scene->scene, NULL);
        break;
    default:
        free(controller);
        return _fail_handle(scene, "unsupported WASM controller type");
        break;
    }
    if (controller->controller == NULL || !_remember(scene, controller))
    {
        free(controller);
        return _fail_handle(scene, "WASM controller creation failed");
    }
    return _handle(controller);
}



/*************************************************************************************************/
/*  Mutators                                                                                     */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_add_visual(uint32_t panel_handle, uint32_t visual_handle)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        panel == NULL || visual == NULL || panel->owner == NULL || visual->owner != panel->owner ||
        panel->panel == NULL || visual->visual == NULL)
    {
        DvzWasmApiScene* owner = panel != NULL ? panel->owner : visual != NULL ? visual->owner : NULL;
        return _fail(owner, "invalid WASM panel/visual handle");
    }
    _clear_payload(panel->owner);
    if (dvz_panel_add_visual(panel->panel, visual->visual, NULL) != 0)
        return _fail(panel->owner, "WASM panel add visual failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_bind_controller(
    uint32_t panel_handle, uint32_t controller_handle, uint32_t dims)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    DvzWasmApiController* controller = _controller(controller_handle);
    if (
        panel == NULL || controller == NULL || panel->owner == NULL ||
        controller->owner != panel->owner || panel->panel == NULL || controller->controller == NULL)
    {
        DvzWasmApiScene* owner =
            panel != NULL ? panel->owner : controller != NULL ? controller->owner : NULL;
        return _fail(owner, "invalid WASM panel/controller handle");
    }
    _clear_payload(panel->owner);
    if (dvz_panel_bind_controller(panel->panel, controller->controller, (DvzDimMask)dims) != 0)
        return _fail(panel->owner, "WASM panel bind controller failed");
    if (dvz_panel_connect_input(panel->panel, panel->owner->router) != 0)
        return _fail(panel->owner, "WASM panel input connection failed");
    if (panel->owner->gestures == NULL)
    {
        panel->owner->gestures = dvz_pointer_gesture_handler(panel->owner->router);
        if (panel->owner->gestures == NULL)
            return _fail(panel->owner, "WASM pointer gesture setup failed");
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_set_camera(
    uint32_t panel_handle, float eye_x, float eye_y, float eye_z, float target_x, float target_y,
    float target_z, float fov_y, float near, float far)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    if (panel == NULL || panel->owner == NULL || panel->panel == NULL)
        return _fail(panel != NULL ? panel->owner : NULL, "invalid WASM panel handle");
    _clear_payload(panel->owner);
    DvzCameraDesc desc = dvz_camera_desc();
    desc.eye[0] = eye_x;
    desc.eye[1] = eye_y;
    desc.eye[2] = eye_z;
    desc.target[0] = target_x;
    desc.target[1] = target_y;
    desc.target[2] = target_z;
    desc.fov_y = fov_y;
    desc.near = near;
    desc.far = far;
    if (dvz_panel_set_camera(panel->panel, &desc) == NULL)
        return _fail(panel->owner, "WASM panel camera setup failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_panel_set_domain(
    uint32_t panel_handle, uint32_t dim, double min, double max)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    if (panel == NULL || panel->owner == NULL || panel->panel == NULL)
        return _fail(panel != NULL ? panel->owner : NULL, "invalid WASM panel handle");
    if (dim != DVZ_DIM_X && dim != DVZ_DIM_Y)
        return _fail(panel->owner, "unsupported WASM panel domain dimension");
    _clear_payload(panel->owner);
    if (dvz_panel_set_domain(panel->panel, (DvzDim)dim, min, max) != 0)
        return _fail(panel->owner, "WASM panel domain setup failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_panel_axis(uint32_t panel_handle, uint32_t dim)
{
    DvzWasmApiPanel* panel = _panel(panel_handle);
    if (panel == NULL || panel->owner == NULL || panel->panel == NULL)
        return _fail_handle(panel != NULL ? panel->owner : NULL, "invalid WASM panel handle");
    if (dim != DVZ_DIM_X && dim != DVZ_DIM_Y)
        return _fail_handle(panel->owner, "unsupported WASM panel axis dimension");
    _clear_payload(panel->owner);
    DvzWasmApiAxis* axis = (DvzWasmApiAxis*)calloc(1, sizeof(DvzWasmApiAxis));
    if (axis == NULL)
        return _fail_handle(panel->owner, "WASM axis wrapper allocation failed");
    axis->owner = panel->owner;
    axis->axis = dvz_panel_axis(panel->panel, (DvzDim)dim);
    if (axis->axis != NULL)
    {
        DvzAxisStyle style = dvz_axis_style();
        style.text_renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
        (void)dvz_axis_set_style(axis->axis, &style);
    }
    if (axis->axis == NULL || !_remember(panel->owner, axis))
    {
        free(axis);
        return _fail_handle(panel->owner, "WASM panel axis creation failed");
    }
    return _handle(axis);
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_visible(uint32_t axis_handle, uint32_t visible)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis handle");
    _clear_payload(axis->owner);
    if (!dvz_axis_set_visible(axis->axis, visible != 0))
        return _fail(axis->owner, "WASM axis visibility update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_grid(uint32_t axis_handle, uint32_t visible)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis handle");
    _clear_payload(axis->owner);
    if (!dvz_axis_set_grid(axis->axis, visible != 0))
        return _fail(axis->owner, "WASM axis grid update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_label(uint32_t axis_handle, const char* label)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL || label == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis label");
    _clear_payload(axis->owner);
    if (!dvz_axis_set_label(axis->axis, label))
        return _fail(axis->owner, "WASM axis label update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_axis_set_plot_margins(
    uint32_t axis_handle, float left, float right, float bottom, float top)
{
    DvzWasmApiAxis* axis = _axis(axis_handle);
    if (axis == NULL || axis->owner == NULL || axis->axis == NULL)
        return _fail(axis != NULL ? axis->owner : NULL, "invalid WASM axis handle");
    _clear_payload(axis->owner);
    if (!dvz_axis_set_plot_margins(axis->axis, left, right, bottom, top))
        return _fail(axis->owner, "WASM axis plot margins update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_arcball_initial(
    uint32_t controller_handle, float angle_x, float angle_y, float angle_z)
{
    DvzWasmApiController* controller = _controller(controller_handle);
    if (controller == NULL || controller->owner == NULL || controller->controller == NULL)
        return _fail(
            controller != NULL ? controller->owner : NULL, "invalid WASM controller handle");
    DvzArcball* arcball = dvz_controller_arcball(controller->controller);
    if (arcball == NULL)
        return _fail(controller->owner, "WASM controller is not an arcball");
    _clear_payload(controller->owner);
    dvz_arcball_initial(arcball, (vec3){angle_x, angle_y, angle_z});
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_f32(
    uint32_t visual_handle, const char* attr, const float* data, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        data == NULL || item_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM f32 visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_data(visual->visual, attr, data, item_count) != 0)
        return _fail_upload(visual->owner, "f32", attr, item_count);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_rgba8(
    uint32_t visual_handle, const char* attr, const uint8_t* data, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        data == NULL || item_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM rgba8 visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_data(visual->visual, attr, data, item_count) != 0)
        return _fail_upload(visual->owner, "rgba8", attr, item_count);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_u32(
    uint32_t visual_handle, const char* attr, const uint32_t* data, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        data == NULL || item_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM u32 visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_data(visual->visual, attr, data, item_count) != 0)
        return _fail_upload(visual->owner, "u32", attr, item_count);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_strings(
    uint32_t visual_handle, const char* attr, const char* const* strings, uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        strings == NULL || item_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM string visual upload");
    }
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (strings[i] == NULL)
            return _fail(visual->owner, "invalid WASM string visual upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_strings(visual->visual, attr, strings, item_count) != 0)
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic),
            "WASM string visual upload failed: attr=%s item_count=%u",
            attr != NULL ? attr : "<null>", item_count);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(visual->owner, "WASM string visual upload failed");
        return _fail(visual->owner, diagnostic);
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_attr_buffer(
    uint32_t visual_handle, const char* attr, uint32_t buffer_handle, uint32_t byte_offset,
    uint32_t item_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    DvzWasmApiBuffer* buffer = _buffer(buffer_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || attr == NULL ||
        buffer == NULL || buffer->owner == NULL || buffer->buffer == NULL ||
        buffer->owner != visual->owner || item_count == 0)
    {
        return _fail(
            visual != NULL ? visual->owner : NULL, "invalid WASM visual attribute buffer bind");
    }
    _clear_payload(visual->owner);
    if (!dvz_visual_set_attr_buffer(
            visual->visual, attr, buffer->buffer, byte_offset, item_count))
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic),
            "WASM visual attribute buffer bind failed: attr=%s item_count=%u",
            attr != NULL ? attr : "<null>", item_count);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(visual->owner, "WASM visual attribute buffer bind failed");
        return _fail(visual->owner, diagnostic);
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_texture_rgba8(
    uint32_t visual_handle, const uint8_t* rgba, uint32_t width, uint32_t height)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->visual == NULL || rgba == NULL ||
        width == 0 || height == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM RGBA8 texture upload");
    }
    _clear_payload(visual->owner);
    if (dvz_visual_set_texture(visual->visual, rgba, width, height) != 0)
    {
        char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = snprintf(
            diagnostic, sizeof(diagnostic), "WASM RGBA8 texture upload failed: %ux%u", width,
            height);
        if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
            return _fail(visual->owner, "WASM RGBA8 texture upload failed");
        return _fail(visual->owner, diagnostic);
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_labels_s32(
    uint32_t visual_handle, const int32_t* values, uint32_t width, uint32_t height,
    const int32_t* category_ids, const uint8_t* colors_rgba, uint32_t category_count)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (
        visual == NULL || visual->owner == NULL || visual->owner->scene == NULL ||
        visual->visual == NULL || values == NULL || category_ids == NULL || colors_rgba == NULL ||
        width == 0 || height == 0 || category_count == 0)
    {
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM S32 labels upload");
    }
    if (dvz_labels_state(visual->visual) == NULL)
        return _fail(visual->owner, "WASM S32 labels upload requires a labels visual");
    _clear_payload(visual->owner);

    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.dim = DVZ_FIELD_DIM_2D;
    field_desc.format = DVZ_FIELD_FORMAT_R32_SINT;
    field_desc.semantic = DVZ_FIELD_SEMANTIC_LABEL;
    field_desc.width = width;
    field_desc.height = height;
    field_desc.depth = 1;

    DvzSampledField* field = dvz_sampled_field(visual->owner->scene, &field_desc);
    if (field == NULL)
        return _fail(visual->owner, "WASM S32 labels field creation failed");

    DvzFieldDataView view = {DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView)};
    view.data = values;
    view.bytes_per_row = (uint64_t)width * sizeof(int32_t);
    view.rows_per_image = height;
    if (!dvz_sampled_field_set_data(field, &view))
        return _fail(visual->owner, "WASM S32 labels field upload failed");
    if (!dvz_visual_set_field(visual->visual, "field", field))
        return _fail(visual->owner, "WASM S32 labels field bind failed");

    DvzScale* scale = dvz_scale(
        visual->owner->scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    if (scale == NULL)
        return _fail(visual->owner, "WASM S32 labels scale creation failed");

    DvzScaleCategory* categories =
        (DvzScaleCategory*)calloc(category_count, sizeof(DvzScaleCategory));
    if (categories == NULL)
        return _fail(visual->owner, "WASM S32 labels category allocation failed");
    for (uint32_t i = 0; i < category_count; i++)
    {
        categories[i].category_id = category_ids[i];
        categories[i].order = i;
        categories[i].color = dvz_color_rgba(
            colors_rgba[4 * i + 0], colors_rgba[4 * i + 1], colors_rgba[4 * i + 2],
            colors_rgba[4 * i + 3]);
    }
    bool ok = dvz_scale_set_categories(scale, categories, category_count);
    free(categories);
    if (!ok)
        return _fail(visual->owner, "WASM S32 labels categories failed");
    if (dvz_visual_set_scale(visual->visual, "labels", scale) != 0)
        return _fail(visual->owner, "WASM S32 labels scale bind failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_material(
    uint32_t visual_handle, uint32_t model, float opacity, float base_r, float base_g,
    float base_b, float base_a, float light_x, float light_y, float light_z, float ambient,
    float diffuse, float specular, float shininess, float roughness, float standard_specular,
    float metallic, float emissive_r, float emissive_g, float emissive_b, float rim_strength)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM material visual handle");

    _clear_payload(visual->owner);
    DvzMaterialDesc material = model == DVZ_MATERIAL_MODEL_STANDARD ? dvz_standard_material_desc()
                               : model == DVZ_MATERIAL_MODEL_PHONG ? dvz_phong_material_desc()
                                                                   : dvz_material_desc();
    material.model = (DvzMaterialModel)model;
    material.alpha_mode = DVZ_ALPHA_OPAQUE;
    material.opacity = opacity;
    material.base_color_factor[0] = base_r;
    material.base_color_factor[1] = base_g;
    material.base_color_factor[2] = base_b;
    material.base_color_factor[3] = base_a;
    material.light_direction[0] = light_x;
    material.light_direction[1] = light_y;
    material.light_direction[2] = light_z;
    material.phong.ambient = ambient;
    material.phong.diffuse = diffuse;
    material.phong.specular = specular;
    material.phong.shininess = shininess;
    material.standard.roughness = roughness;
    material.standard.specular = standard_specular;
    material.standard.metallic = metallic;
    material.standard.emissive[0] = emissive_r;
    material.standard.emissive[1] = emissive_g;
    material.standard.emissive[2] = emissive_b;
    material.standard.rim_strength = rim_strength;
    if (dvz_visual_set_material(visual->visual, &material) != 0)
        return _fail(visual->owner, "WASM visual material update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_segment_caps(
    uint32_t visual_handle, uint32_t start_cap, uint32_t end_cap)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(
            visual != NULL ? visual->owner : NULL, "invalid WASM segment cap visual handle");

    _clear_payload(visual->owner);
    if (dvz_segment_set_caps(
            visual->visual, (DvzSegmentCap)start_cap, (DvzSegmentCap)end_cap) != 0)
    {
        return _fail(visual->owner, "WASM segment cap update failed");
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_path_caps(
    uint32_t visual_handle, uint32_t start_cap, uint32_t end_cap)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM path cap visual handle");

    _clear_payload(visual->owner);
    if (dvz_path_set_caps(visual->visual, (DvzSegmentCap)start_cap, (DvzSegmentCap)end_cap) != 0)
        return _fail(visual->owner, "WASM path cap update failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_visual_set_path_join(
    uint32_t visual_handle, uint32_t join, float miter_limit)
{
    DvzWasmApiVisual* visual = _visual(visual_handle);
    if (visual == NULL || visual->owner == NULL || visual->visual == NULL)
        return _fail(visual != NULL ? visual->owner : NULL, "invalid WASM path join visual handle");

    _clear_payload(visual->owner);
    if (dvz_path_set_join(visual->visual, (DvzPathJoin)join, miter_limit) != 0)
        return _fail(visual->owner, "WASM path join update failed");
    return 0;
}



/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_resize(
    uint32_t scene_handle, uint32_t figure_handle, uint32_t width, uint32_t height,
    float device_scale)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (
        scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL ||
        width == 0 || height == 0)
    {
        return _fail(scene, "invalid WASM resize request");
    }
    _clear_payload(scene);
    scene->width = width;
    scene->height = height;
    dvz_figure_resize(figure->figure, width, height);
    _emit_resize(scene, width, height, device_scale);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_pointer(
    uint32_t scene_handle, int type, float x, float y, int button, int mods, float content_scale,
    double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->router == NULL)
        return _fail(scene, "invalid WASM pointer request");
    _clear_payload(scene);
    uint64_t timestamp_ns = timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    dvz_pointer_emit_position(
        scene->router, (DvzPointerEventType)type, x, y, x, y, (DvzPointerButton)button, mods,
        content_scale, timestamp_ns, NULL);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_wheel(
    uint32_t scene_handle, float x, float y, float dir_x, float dir_y, int mods,
    float content_scale, double timestamp_ms)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->router == NULL)
        return _fail(scene, "invalid WASM wheel request");
    _clear_payload(scene);
    uint64_t timestamp_ns = timestamp_ms > 0.0 ? (uint64_t)(timestamp_ms * 1000000.0) : 0;
    dvz_pointer_emit_wheel(
        scene->router, x, y, x, y, dir_x, dir_y, mods, content_scale, timestamp_ns, NULL);
    return 0;
}



/*************************************************************************************************/
/*  Emission and diagnostics                                                                     */
/*************************************************************************************************/

EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_set_canvas_format(uint32_t scene_handle, uint32_t color_format)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (color_format != DVZ_FORMAT_R8G8B8A8_UNORM && color_format != DVZ_FORMAT_B8G8R8A8_UNORM)
        return _fail(scene, "unsupported WASM canvas format");
    _clear_payload(scene);
    scene->color_format = color_format;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_set_capabilities(
    uint32_t scene_handle, uint32_t max_texture_dimension_2d, uint32_t max_bind_groups,
    uint32_t max_vertex_buffers, uint32_t max_buffer_size,
    uint32_t min_texture_copy_bytes_per_row_alignment, uint32_t max_sample_count,
    uint32_t supports_color_blending)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (
        max_texture_dimension_2d == 0 || max_bind_groups == 0 || max_vertex_buffers == 0 ||
        max_buffer_size == 0 || min_texture_copy_bytes_per_row_alignment == 0 ||
        max_sample_count == 0)
    {
        return _fail(scene, "invalid WASM capability snapshot");
    }
    _clear_payload(scene);
    scene->caps.max_texture_dimension_2d = max_texture_dimension_2d;
    scene->caps.max_bind_groups = max_bind_groups;
    scene->caps.max_vertex_buffers = max_vertex_buffers;
    scene->caps.max_buffer_size = max_buffer_size;
    scene->caps.max_color_sample_count = max_sample_count;
    scene->caps.max_depth_sample_count = max_sample_count;
    scene->caps.min_texture_copy_bytes_per_row_alignment =
        min_texture_copy_bytes_per_row_alignment;
    scene->caps.supports_color_blending = supports_color_blending != 0;
    return 0;
}



static DvzFramePlanEmitConfig _wasm_emit_config(const DvzWasmApiScene* scene)
{
    ANN(scene);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 0;
    emit_cfg.color_target_format = scene->color_format;
    emit_cfg.target_width = scene->width;
    emit_cfg.target_height = scene->height;
    return emit_cfg;
}



static int _emit_frame_artifact(
    uint32_t scene_handle, uint32_t figure_handle, const char* failure_message)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
    {
        int ret = _fail(scene, failure_message);
        if (scene != NULL)
            scene->packet_status = -1;
        return ret;
    }
    _clear_payload(scene);

    DvzFramePlanEmitConfig emit_cfg = _wasm_emit_config(scene);
    uint64_t next_frame_index = scene->frame_index + 1;
    uint64_t next_resource_version = scene->resource_version + 1;
    scene->frame_artifact = _scene_emit_frame_artifact(
        figure->figure, &scene->caps, &scene->report, &emit_cfg, next_resource_version,
        next_frame_index);
    if (scene->frame_artifact == NULL)
    {
        if (dvz_diagnostic_report_count(&scene->report) == 0)
            (void)dvz_diagnostic_report_add(&scene->report, "WASM scene frame emission failed");
        scene->packet_status = -1;
        return -1;
    }
    if (dvz_diagnostic_report_count(&scene->report) > 0)
    {
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
        scene->packet_status = -1;
        return -1;
    }

    scene->frame_index = next_frame_index;
    scene->resource_version = next_resource_version;
    if (dvz_scene_frame_artifact_status(scene->frame_artifact) != DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK)
    {
        (void)_fail(scene, "WASM DRP2 packet encoding failed");
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
        scene->packet_status = -2;
        return -1;
    }
    scene->packet_status = 0;
    return 0;
}



static int _emit(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (_emit_frame_artifact(scene_handle, figure_handle, "invalid WASM emit request") != 0)
        return -1;
    scene = _scene(scene_handle);
    ANN(scene);
    scene->json = dvz_scene_frame_artifact_json(scene->frame_artifact, "wasm_api_scene");
    if (scene->json == NULL)
    {
        scene->packet_status = -3;
        return _fail(scene, "WASM frame artifact JSON serialization failed");
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit(scene_handle, figure_handle);
}



static int _emit_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit_frame_artifact(scene_handle, figure_handle, "invalid WASM packet emit request");
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit_packets(scene_handle, figure_handle);
}


EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_release_packets(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (scene->frame_artifact != NULL)
    {
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
    }
    scene->packet_status = 0;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_query_pending_count(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->scene != NULL ? scene->scene->pending_query_count : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_query_active(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->query_active ? 1 : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_query_readback_size(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->query_active ? scene->query_plan.byte_size : 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit_query_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
    {
        int ret = _fail(scene, "invalid WASM query packet emit request");
        if (scene != NULL)
            scene->packet_status = -1;
        return ret;
    }

    _clear_query(scene);
    _clear_payload(scene);
    DvzScene* owner = scene->scene;
    DvzFigure* target = figure->figure;
    if (owner == NULL)
        return _fail(scene, "invalid WASM query scene");
    if (owner->pending_query_count == 0)
        return 0;
    if (!_scene_figure_resolve_layouts(target))
        return _fail(scene, "WASM query layout resolution failed");

    uint32_t pending_index = UINT32_MAX;
    DvzPendingQueryRequest pending = {0};
    for (uint32_t i = 0; i < owner->pending_query_count; i++)
    {
        if (owner->pending_queries[i].panel != NULL &&
            owner->pending_queries[i].panel->figure == target)
        {
            pending_index = i;
            pending = owner->pending_queries[i];
            break;
        }
    }
    if (pending_index == UINT32_MAX)
        return 0;

    DvzQueryResult immediate = {0};
    bool needs_gpu = _query_emit_result(scene, target, &pending, &immediate);
    if (!needs_gpu)
    {
        _remove_pending_query_at(owner, pending_index);
        if (!_push_immediate_query_result(scene, pending.panel, pending.freshness_serial, &immediate))
            return _fail(scene, "WASM query result queue push failed");
        return 0;
    }

    dvz_diagnostic_report_init(&scene->report);
    DvzCapabilitySnapshot query_caps = _wasm_capability_snapshot();
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.target_width = scene->query_plan.target_width > 0 ? scene->query_plan.target_width : 1;
    emit_cfg.target_height =
        scene->query_plan.target_height > 0 ? scene->query_plan.target_height : 1;
    emit_cfg.color_target_format = scene->query_plan.format;

    DvzSceneRequestExecutor* executor = &owner->query_executor;
    if (executor->emitter == NULL)
    {
        if (!_ensure_query_emitter(owner))
            return _fail(scene, "WASM query emitter creation failed");
    }

    DvzDrp2CommandStream* query_stream = dvz_frame_plan_emitter_emit_drp2(
        executor->emitter, scene->query_plan.scratch.plan, &query_caps, &scene->report, &emit_cfg);
    if (query_stream == NULL)
    {
        DvzQueryResult result = scene->query_result;
        result.status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        if (dvz_diagnostic_report_count(&scene->report) == 0)
        {
            _query_setup_diagnostic(
                scene, "DRP2 query stream snapshot emission failed", scene->query_ops,
                scene->query_visual_type, result.visual_id, pending.request.target, result.profile);
        }
        _remove_pending_query_at(owner, pending_index);
        _clear_query(scene);
        if (!_push_immediate_query_result(scene, pending.panel, pending.freshness_serial, &result))
            return _fail(scene, "WASM query emission failure push failed");
        return 0;
    }

    if (_emit_current_query_packets(scene, query_stream) != 0)
        return -1;
    _remove_pending_query_at(owner, pending_index);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_query_resolve(uint32_t scene_handle, const uint8_t* bytes, uint32_t byte_size)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL || !scene->query_active)
        return _fail(scene, "WASM query resolve requested without an active query");
    if (bytes == NULL || byte_size < scene->query_plan.byte_size)
        return _fail(scene, "WASM query readback payload is too small");

    DvzQueryResult result = scene->query_result;
    DvzSceneQueryDecodeContext decode = {
        .build = &scene->query_build,
        .plan = &scene->query_plan,
        .bytes = bytes,
        .byte_size = scene->query_plan.byte_size,
    };
    bool decoded = false;
    if (scene->query_visual_type == DVZ_VISUAL_TYPE_POINT)
        decoded = _point_query_decode(&decode, &result);
    else if (scene->query_ops != NULL && scene->query_ops->decode != NULL)
        decoded = scene->query_ops->decode(&decode, &result);
    if (!decoded)
        result.status = DVZ_QUERY_STATUS_MISS;

    if (scene->query_ops != NULL && scene->query_ops->readout != NULL)
    {
        DvzSceneQueryReadoutContext readout = {
            .build = &scene->query_build,
            .plan = &scene->query_plan,
        };
        if (!scene->query_ops->readout(&readout, &result))
            result.status = DVZ_QUERY_STATUS_DECODE_FAILED;
    }

    _mark_query_static_upload(scene);
    DvzPanel* panel = scene->query_panel;
    const uint64_t freshness_serial = scene->query_pending.freshness_serial;
    _clear_query(scene);
    if (!_push_immediate_query_result(scene, panel, freshness_serial, &result))
        return _fail(scene, "WASM query result queue push failed");
    return 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_payload_ptr(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->json != NULL ? (uint32_t)(uintptr_t)scene->json : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_payload_size(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->json != NULL ? (uint32_t)strlen(scene->json) : 0;
}



static bool _valid_packet_kind(uint32_t kind)
{
    return kind >= DVZ_DRP2_PACKET_SETUP && kind <= DVZ_DRP2_PACKET_FRAME;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_packet_status(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    return scene->packet_status;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_ptr(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !_valid_packet_kind(kind))
        return 0;
    const void* ptr = NULL;
    (void)dvz_scene_frame_artifact_get_packet(
        scene->frame_artifact, (DvzDrp2PacketKind)kind, &ptr, NULL, NULL, NULL);
    return ptr != NULL ? (uint32_t)(uintptr_t)ptr : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_size(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    uint64_t size = 0;
    if (scene != NULL && _valid_packet_kind(kind))
    {
        (void)dvz_scene_frame_artifact_get_packet(
            scene->frame_artifact, (DvzDrp2PacketKind)kind, NULL, &size, NULL, NULL);
    }
    return size <= UINT32_MAX ? (uint32_t)size : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_arena_ptr(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !_valid_packet_kind(kind))
        return 0;
    const void* ptr = NULL;
    (void)dvz_scene_frame_artifact_get_packet(
        scene->frame_artifact, (DvzDrp2PacketKind)kind, NULL, NULL, &ptr, NULL);
    return ptr != NULL ? (uint32_t)(uintptr_t)ptr : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_arena_size(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    uint64_t size = 0;
    if (scene != NULL && _valid_packet_kind(kind))
    {
        (void)dvz_scene_frame_artifact_get_packet(
            scene->frame_artifact, (DvzDrp2PacketKind)kind, NULL, NULL, NULL, &size);
    }
    return size <= UINT32_MAX ? (uint32_t)size : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_resource_version(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene != NULL && scene->frame_artifact != NULL)
    {
        uint64_t resource_version = dvz_scene_frame_artifact_resource_version(scene->frame_artifact);
        return resource_version <= UINT32_MAX ? (uint32_t)resource_version : 0;
    }
    return scene != NULL && scene->resource_version <= UINT32_MAX ? (uint32_t)scene->resource_version
                                                                  : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_frame_index(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene != NULL && scene->frame_artifact != NULL)
    {
        uint64_t frame_index = dvz_scene_frame_artifact_frame_index(scene->frame_artifact);
        return frame_index <= UINT32_MAX ? (uint32_t)frame_index : 0;
    }
    return scene != NULL && scene->frame_index <= UINT32_MAX ? (uint32_t)scene->frame_index : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_diagnostic_count(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL ? dvz_diagnostic_report_count(&scene->report) : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_diagnostic(uint32_t scene_handle, uint32_t index)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    const char* diagnostic = scene != NULL ? dvz_diagnostic_report_get(&scene->report, index) : NULL;
    return diagnostic != NULL ? (uint32_t)(uintptr_t)diagnostic : 0;
}
