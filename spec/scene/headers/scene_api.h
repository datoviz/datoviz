/*
 * Draft header sketch derived from spec/scene/.
 * This file is informative only and is not part of the installed public API.
 */

/*************************************************************************************************/
/*  Scene API sketch                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/common/macros.h"

#include "diagnostics.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzCapabilitySnapshot DvzCapabilitySnapshot;

typedef struct DvzScene DvzScene;
typedef struct DvzScenePanel DvzScenePanel;
typedef struct DvzSceneVisual DvzSceneVisual;
typedef struct DvzSceneResource DvzSceneResource;
typedef struct DvzSceneAxis DvzSceneAxis;
typedef struct DvzSceneAnnotation DvzSceneAnnotation;
typedef struct DvzSceneLegend DvzSceneLegend;
typedef struct DvzSceneColorbar DvzSceneColorbar;
typedef struct DvzSceneController DvzSceneController;
typedef struct DvzSceneScale DvzSceneScale;
typedef struct DvzSceneFramePlan DvzSceneFramePlan;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_PANEL_KIND_2D = 0,
    DVZ_SCENE_PANEL_KIND_3D = 1,
    DVZ_SCENE_PANEL_KIND_OFFSCREEN = 2,
} DvzScenePanelKind;


typedef enum
{
    DVZ_SCENE_VISUAL_FAMILY_BASIC = 0,
    DVZ_SCENE_VISUAL_FAMILY_PIXEL = 1,
    DVZ_SCENE_VISUAL_FAMILY_POINT = 2,
    DVZ_SCENE_VISUAL_FAMILY_MARKER = 3,
    DVZ_SCENE_VISUAL_FAMILY_SEGMENT = 4,
    DVZ_SCENE_VISUAL_FAMILY_PATH = 5,
    DVZ_SCENE_VISUAL_FAMILY_GLYPH = 6,
    DVZ_SCENE_VISUAL_FAMILY_IMAGE = 7,
    DVZ_SCENE_VISUAL_FAMILY_MESH = 8,
    DVZ_SCENE_VISUAL_FAMILY_SPHERE = 9,
    DVZ_SCENE_VISUAL_FAMILY_VOLUME = 10,
} DvzSceneVisualFamily;


typedef enum
{
    DVZ_SCENE_RESOURCE_KIND_ITEM_TABLE = 0,
    DVZ_SCENE_RESOURCE_KIND_GROUPED_ITEM_TABLE = 1,
    DVZ_SCENE_RESOURCE_KIND_INDEXED_GEOMETRY = 2,
    DVZ_SCENE_RESOURCE_KIND_SAMPLED_FIELD = 3,
    DVZ_SCENE_RESOURCE_KIND_STYLE_BLOCK = 4,
    DVZ_SCENE_RESOURCE_KIND_DERIVED_FIELD = 5,
    DVZ_SCENE_RESOURCE_KIND_READBACK_TARGET = 6,
} DvzSceneResourceKind;


typedef enum
{
    DVZ_SCENE_ROLE_ITEMS = 0,
    DVZ_SCENE_ROLE_GROUPED_ITEMS = 1,
    DVZ_SCENE_ROLE_STYLE = 2,
    DVZ_SCENE_ROLE_FIELD = 3,
    DVZ_SCENE_ROLE_GEOMETRY = 4,
    DVZ_SCENE_ROLE_INDICES = 5,
    DVZ_SCENE_ROLE_READBACK = 6,
    DVZ_SCENE_ROLE_DERIVED = 7,
} DvzSceneResourceRole;


typedef enum
{
    DVZ_SCENE_MAPPING_COLOR = 0,
    DVZ_SCENE_MAPPING_SIZE = 1,
    DVZ_SCENE_MAPPING_SYMBOL = 2,
} DvzSceneMappingKind;


typedef enum
{
    DVZ_SCENE_PICK_HOVER = 0,
    DVZ_SCENE_PICK_CLICK = 1,
    DVZ_SCENE_PICK_QUERY = 2,
} DvzScenePickKind;


typedef enum
{
    DVZ_SCENE_REDRAW_SCENE = 0,
    DVZ_SCENE_REDRAW_PANEL = 1,
} DvzSceneRedrawScope;


typedef enum
{
    DVZ_SCENE_BUILD_FLAGS_NONE = 0x00,
    DVZ_SCENE_BUILD_FLAGS_OFFSCREEN_ONLY = 0x01,
    DVZ_SCENE_BUILD_FLAGS_INCLUDE_PICKING = 0x02,
} DvzSceneBuildFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    uint32_t initial_panel_capacity;
    uint32_t initial_visual_capacity;
    uint32_t initial_resource_capacity;
} DvzSceneCreateDesc;


typedef struct
{
    DvzScenePanelKind kind;
    float x;
    float y;
    float width;
    float height;
    bool offscreen;
} DvzScenePanelDesc;


typedef struct
{
    DvzSceneVisualFamily family;
    uint32_t variant_flags;
    bool enable_picking;
    bool visible;
} DvzSceneVisualDesc;


typedef struct
{
    DvzSceneResourceKind kind;
    uint64_t schema_id;
    size_t item_stride;
    uint32_t usage_flags;
    bool persistent;
} DvzSceneResourceDesc;


typedef struct
{
    DvzSceneMappingKind kind;
    uint64_t semantic_id;
    double domain_min;
    double domain_max;
} DvzSceneScaleDesc;


typedef struct
{
    DvzScenePickKind kind;
    uint64_t request_id;
    uint64_t panel_generation;
    double x;
    double y;
} DvzScenePickRequest;


typedef struct
{
    uint64_t request_id;
    uint64_t panel_id;
    uint64_t visual_id;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id;
    bool valid;
} DvzScenePickResult;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a new scene object from the supplied descriptor.
 *
 * @param desc scene creation descriptor or NULL for implementation defaults
 * @returns scene handle or NULL on failure
 */
DVZ_EXPORT DvzScene* dvz_scene_create(const DvzSceneCreateDesc* desc);



/**
 * Destroy a scene and every scene-owned object attached to it.
 *
 * @param scene scene handle
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



/**
 * Create and attach one panel to the scene.
 *
 * @param scene owning scene
 * @param desc panel descriptor
 * @returns created panel handle or NULL on failure
 */
DVZ_EXPORT DvzScenePanel* dvz_scene_panel(DvzScene* scene, const DvzScenePanelDesc* desc);



/**
 * Create and attach one visual to the scene.
 *
 * @param scene owning scene
 * @param desc visual descriptor
 * @returns created visual handle or NULL on failure
 */
DVZ_EXPORT DvzSceneVisual* dvz_scene_visual(DvzScene* scene, const DvzSceneVisualDesc* desc);



/**
 * Create one logical scene resource.
 *
 * @param scene owning scene
 * @param desc resource descriptor
 * @returns created resource handle or NULL on failure
 */
DVZ_EXPORT DvzSceneResource* dvz_scene_resource(DvzScene* scene, const DvzSceneResourceDesc* desc);



/**
 * Create one explicit scale or mapping object.
 *
 * @param scene owning scene
 * @param desc scale descriptor
 * @returns created scale handle or NULL on failure
 */
DVZ_EXPORT DvzSceneScale* dvz_scene_scale(DvzScene* scene, const DvzSceneScaleDesc* desc);



/**
 * Bind one resource to one visual by semantic role.
 *
 * @param visual target visual
 * @param role semantic resource role
 * @param resource scene resource handle
 * @returns 0 on success, <0 on failure
 */
DVZ_EXPORT int
dvz_scene_visual_set_resource(DvzSceneVisual* visual, DvzSceneResourceRole role, DvzSceneResource* resource);



/**
 * Bind one explicit mapping object to one visual.
 *
 * @param visual target visual
 * @param kind mapping kind
 * @param scale mapping object
 * @returns 0 on success, <0 on failure
 */
DVZ_EXPORT int
dvz_scene_visual_set_mapping(DvzSceneVisual* visual, DvzSceneMappingKind kind, DvzSceneScale* scale);



/**
 * Attach one visual to one panel.
 *
 * @param panel target panel
 * @param visual visual handle
 * @returns 0 on success, <0 on failure
 */
DVZ_EXPORT int dvz_scene_panel_add_visual(DvzScenePanel* panel, DvzSceneVisual* visual);



/**
 * Update the active runtime capability snapshot used by validation and adaptation.
 *
 * @param scene target scene
 * @param capabilities capability snapshot
 * @returns 0 on success, <0 on failure
 */
DVZ_EXPORT int
dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* capabilities);



/**
 * Run scene validation and write a diagnostic report.
 *
 * @param scene target scene
 * @param out_report diagnostic report written by the implementation
 * @returns 0 when validation succeeds, <0 when validation fails fatally
 */
DVZ_EXPORT int dvz_scene_validate(DvzScene* scene, DvzDiagnosticReport* out_report);



/**
 * Apply capability adaptation to the currently validated scene state.
 *
 * @param scene target scene
 * @param out_report diagnostic report written by the implementation
 * @returns 0 when adaptation succeeds, <0 when no valid adapted outcome exists
 */
DVZ_EXPORT int dvz_scene_adapt(DvzScene* scene, DvzDiagnosticReport* out_report);



/**
 * Build one scene-level frame plan from the current scene state.
 *
 * @param scene target scene
 * @param flags frame-build flags
 * @param out_report diagnostic report written by the implementation
 * @returns scene-level frame plan handle or NULL on failure
 */
DVZ_EXPORT DvzSceneFramePlan*
dvz_scene_build_frame(DvzScene* scene, DvzSceneBuildFlags flags, DvzDiagnosticReport* out_report);



/**
 * Destroy one scene-level frame plan.
 *
 * @param frame_plan frame plan handle
 */
DVZ_EXPORT void dvz_scene_frame_plan_destroy(DvzSceneFramePlan* frame_plan);



/**
 * Record a redraw request on the scene or one panel.
 *
 * @param scene target scene
 * @param scope redraw scope
 * @param panel optional panel handle when the scope is panel-local
 * @returns 0 on success, <0 on failure
 */
DVZ_EXPORT int
dvz_scene_request_redraw(DvzScene* scene, DvzSceneRedrawScope scope, DvzScenePanel* panel);



/**
 * Enqueue one picking request in scene coordinates for later execution.
 *
 * @param scene target scene
 * @param panel target panel
 * @param request pick request descriptor
 * @returns 0 on success, <0 on failure
 */
DVZ_EXPORT int
dvz_scene_request_pick(DvzScene* scene, DvzScenePanel* panel, const DvzScenePickRequest* request);



/**
 * Poll one interpreted picking result from the scene layer.
 *
 * @param scene target scene
 * @param out_result pick result written by the implementation
 * @returns true when one result was written, false when no result is available
 */
DVZ_EXPORT bool dvz_scene_poll_pick_result(DvzScene* scene, DvzScenePickResult* out_result);



EXTERN_C_OFF
