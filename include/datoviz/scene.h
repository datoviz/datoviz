/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "scene/annotation.h"
#include "scene/animation.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/enums.h"
#include "scene/field.h"
#include "scene/fly.h"
#include "scene/frame_plan.h"
#include "scene/interaction.h"
#include "scene/panzoom.h"
#include "scene/scale.h"
#include "scene/text.h"
#include "scene/turntable.h"
#include "scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Scene lifecycle                                                                              */
/*************************************************************************************************/

/**
 * Create a scene.
 *
 * @return the scene
 */
DVZ_EXPORT DvzScene* dvz_scene(void);


/**
 * Set the runtime capability snapshot used for frame planning.
 *
 * @param scene the scene
 * @param caps the capability snapshot
 */
DVZ_EXPORT void dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps);


/**
 * Destroy a scene and all objects it owns.
 *
 * This call is rejected while any emitted scene stream is still live. Destroy
 * all streams returned by dvz_figure_emit() / dvz_figure_emit_ex() first.
 *
 * @param scene the scene
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);


/**
 * Serialize the scene to a JSON string.
 *
 * The JSON document contains the full scene graph: figures, panels, visuals, and attribute data
 * (base64-encoded). The caller must free the returned string with dvz_scene_json_destroy().
 *
 * @param scene the scene
 * @return an owned NUL-terminated JSON string, or NULL on failure
 */
DVZ_EXPORT char* dvz_scene_json(const DvzScene* scene);


/**
 * Free a JSON string returned by dvz_scene_json().
 *
 * @param json the JSON string
 */
DVZ_EXPORT void dvz_scene_json_destroy(char* json);



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

/**
 * Create a figure (output layout container) owned by the scene.
 *
 * @param scene the scene
 * @param width width in logical pixels (0 = inherit from window)
 * @param height height in logical pixels (0 = inherit from window)
 * @param flags creation flags
 * @return the figure
 */
DVZ_EXPORT DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height,
                                  uint32_t flags);


/**
 * Update a figure logical size.
 *
 * @param figure the figure
 * @param width width in logical pixels
 * @param height height in logical pixels
 */
DVZ_EXPORT void dvz_figure_resize(DvzFigure* figure, uint32_t width, uint32_t height);


/**
 * Return a figure logical size.
 *
 * @param figure the figure
 * @param out_width output width in logical pixels, may be NULL
 * @param out_height output height in logical pixels, may be NULL
 */
DVZ_EXPORT void
dvz_figure_size(const DvzFigure* figure, uint32_t* out_width, uint32_t* out_height);


/**
 * Destroy a figure.
 *
 * @param figure the figure
 */
DVZ_EXPORT void dvz_figure_destroy(DvzFigure* figure);


/**
 * Return the type of a scene-owned controller.
 *
 * @param controller the controller
 * @return the controller type, or DVZ_CONTROLLER_TYPE_NONE
 */
DVZ_EXPORT DvzControllerType dvz_controller_type(const DvzController* controller);


/**
 * Return the panzoom payload of a panzoom controller.
 *
 * @param controller the controller
 * @return the borrowed panzoom payload, or NULL for the wrong family
 */
DVZ_EXPORT DvzPanzoom* dvz_controller_panzoom(DvzController* controller);


/**
 * Return the arcball payload of an arcball controller.
 *
 * @param controller the controller
 * @return the borrowed arcball payload, or NULL for the wrong family
 */
DVZ_EXPORT DvzArcball* dvz_controller_arcball(DvzController* controller);


/**
 * Return the fly payload of a fly controller.
 *
 * @param controller the controller
 * @return the borrowed fly payload, or NULL for the wrong family
 */
DVZ_EXPORT DvzFly* dvz_controller_fly(DvzController* controller);


/**
 * Return the turntable payload of a turntable controller.
 *
 * @param controller the controller
 * @return the borrowed turntable payload, or NULL for the wrong family
 */
DVZ_EXPORT DvzTurntable* dvz_controller_turntable(DvzController* controller);


/**
 * Build the ordered frame execution plan for one frame.
 *
 * Lifetime: the returned stream embeds borrowed pointers into the visuals'
 * attribute buffers (see dvz_visual_set_data). The stream remains live until
 * dvz_drp2_stream_destroy() is called. While it is live, calls that mutate or
 * destroy scene-owned visual data are rejected.
 *
 * @param figure the figure
 * @param caps the capability snapshot
 * @param report output diagnostic report
 * @return an owned DRP2 command stream, or NULL on failure
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_figure_emit(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report);


/**
 * Emit a DRP2 command stream from a figure with an explicit emit configuration.
 *
 * Lifetime: same borrowed-pointer contract as dvz_figure_emit. The returned
 * stream remains live until dvz_drp2_stream_destroy() is called.
 *
 * @param figure the figure
 * @param caps the capability snapshot (nullable — defaults applied if NULL)
 * @param report the diagnostic report (nullable)
 * @param cfg the emission configuration (nullable — defaults applied if NULL)
 * @return an owned DRP2 command stream, or NULL on failure
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_figure_emit_ex(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg);


/**
 * Execute queued pick/probe requests for one figure through the DRP2 runtime.
 *
 * This helper is intended for live/offscreen scene runtimes after the figure's main draw has
 * already realized the current scene resources in the runtime. Supported first-slice resolution
 * currently focuses on point picking and basic image probing. Freshness is tracked per
 * panel/request-kind scope: non-zero request ids supersede older work with the same panel-local
 * id, while zero-id requests use one latest-request-wins scope per panel/kind. Late results are
 * discarded once a newer request has claimed the same scope, even if that newer result was already
 * resolved and polled. Pending requests are coalesced before execution so only the newest request
 * in each active scope runs.
 *
 * @param figure the figure
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of requests that were consumed from the scene queues
 */
DVZ_EXPORT uint32_t dvz_figure_process_requests(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps);



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 * Return default panel MSAA options.
 *
 * @return MSAA descriptor with 4x samples and alpha-to-coverage enabled
 */
DVZ_EXPORT DvzMsaaDesc dvz_msaa_desc(void);


/**
 * Create a panel inside a figure.
 *
 * @param figure the figure
 * @param desc panel position and size in normalized [0, 1] figure coordinates
 * @return the panel
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc);


/**
 * Return the default panel layout reservation.
 *
 * The default is zero on every side so plot panels remain edge-to-edge unless callers opt in.
 *
 * @return default panel layout reservation
 */
DVZ_EXPORT DvzPanelLayoutReserve dvz_panel_layout_reserve(void);


/**
 * Reserve visual-space room around one panel's plot area for future adornments.
 *
 * Reservations are in panel visual-space units. Defaults are zero. Nonzero values are intended for
 * tick labels, axis labels, legends, colorbars, or other panel-level adornments.
 *
 * @param panel the panel
 * @param reserve reservation descriptor, or NULL for defaults
 * @return whether the reservation was accepted
 */
DVZ_EXPORT bool dvz_panel_set_layout_reserve(
    DvzPanel* panel, const DvzPanelLayoutReserve* reserve);


/**
 * Return one panel's layout reservation.
 *
 * @param panel the panel
 * @param out output reservation
 * @return whether the reservation was written
 */
DVZ_EXPORT bool dvz_panel_get_layout_reserve(DvzPanel* panel, DvzPanelLayoutReserve* out);


/**
 * Destroy a panel.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);


/**
 * Bind a scene-owned controller to one panel.
 *
 * Fly controllers must be bound to DVZ_DIM_MASK_XYZ in this first slice.
 *
 * @param panel the panel
 * @param controller the scene-owned controller
 * @param dims dimension mask
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int
dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims);


/**
 * Return the controller bound to one panel dimension.
 *
 * @param panel the panel
 * @param dim the dimension
 * @return the borrowed controller handle, or NULL
 */
DVZ_EXPORT DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim);


/**
 * Add a visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 * @return 0 on success, -1 on error
 */
/**
 * Add a visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 * @param desc per-visual attachment options (z_layer, controller_mode); pass NULL for
 *             defaults (z_layer=0, controller_mode=DVZ_CONTROLLER_APPLY)
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_add_visual(
    DvzPanel* panel, DvzVisual* visual, const DvzVisualAttachDesc* desc);


/**
 * Set or update a uniform-color background for a panel.
 *
 * Internally creates a fullscreen-quad visual attached at z_layer=-1 with
 * controller_mode=FIXED so the background fills the panel rect and is unaffected by
 * panzoom/arcball navigation. Repeat calls update the existing background's color
 * instead of stacking new visuals.
 *
 * Components are in [0, 1].
 *
 * @param panel the panel
 * @param r red component
 * @param g green component
 * @param b blue component
 * @param a alpha component
 */
DVZ_EXPORT void dvz_panel_set_background_color(
    DvzPanel* panel, float r, float g, float b, float a);


/**
 * Configure Eye-Dome Lighting for one panel.
 *
 * EDL is a depth-based post-process intended to improve local depth perception for dense point,
 * pixel, and opaque geometry views. Pass NULL to disable EDL on the panel. The descriptor values
 * are clamped to implementation-supported ranges.
 *
 * @param panel the panel
 * @param desc EDL descriptor, or NULL to disable
 * @return whether the panel EDL state was updated
 */
DVZ_EXPORT bool dvz_panel_set_edl(DvzPanel* panel, const DvzEdlDesc* desc);


/**
 * Configure internal multisample antialiasing for one panel.
 *
 * The panel renders opaque scene color/depth into transient multisample attachments and resolves
 * into the figure target. Pass NULL or a descriptor with enabled=false to disable MSAA.
 *
 * @param panel the panel
 * @param desc MSAA descriptor, or NULL to disable
 * @return whether the panel MSAA state was updated
 */
DVZ_EXPORT bool dvz_panel_set_msaa(DvzPanel* panel, const DvzMsaaDesc* desc);


/**
 * Configure screen-space ambient occlusion for one panel.
 *
 * SSAO renders eligible opaque normal-producing visuals through an internal G-buffer, computes an
 * occlusion texture from panel depth and normals, optionally blurs it, and composites the result
 * into the panel output. Pass NULL to disable SSAO on the panel. Descriptor values are clamped to
 * implementation-supported ranges.
 *
 * @param panel the panel
 * @param desc SSAO descriptor, or NULL to disable
 * @return whether the panel SSAO state was updated
 */
DVZ_EXPORT bool dvz_panel_set_ssao(DvzPanel* panel, const DvzSsaoDesc* desc);


/**
 * Configure a panel volume visual as the screen-space occluder for embedded visuals.
 *
 * @param panel the panel
 * @param volume the volume visual attached to the same panel, or NULL to disable
 * @param desc volume occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_set_volume_occluder(
    DvzPanel* panel, DvzVisual* volume, const DvzVolumeOcclusionDesc* desc);


/**
 * Configure generic screen-space scene occlusion for one panel.
 *
 * Scene occlusion is active only when the panel contains at least one visible visual marked as a
 * scene occluder and at least one visible visual marked as scene-occluded. Pass NULL or a disabled
 * descriptor to disable the panel path.
 *
 * @param panel the panel
 * @param desc scene occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int
dvz_panel_set_scene_occlusion(DvzPanel* panel, const DvzSceneOcclusionDesc* desc);


/**
 * Attach a panzoom controller to a panel and connect it to an input router.
 *
 * Pan: left-drag. Zoom: right-drag or scroll wheel. Double-click: reset.
 *
 * @param panel the panel
 * @param router input router to subscribe to (may be NULL to create without connecting)
 * @param flags DvzPanzoomFlags bitmask
 */
DVZ_EXPORT void dvz_panel_set_panzoom(DvzPanel* panel, DvzInputRouter* router, int flags);


/**
 * Return the panzoom controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned panzoom, or NULL
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel);


/**
 * Set a panel data domain for one axis dimension.
 *
 * The first WIP axis slice supports finite linear X/Y domains. Axis geometry is derived from this
 * domain and the panel panzoom extent during frame emission.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param min data minimum
 * @param max data maximum
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_set_domain(DvzPanel* panel, DvzDim dim, double min, double max);


/**
 * Return the current visible data domain for one panel dimension.
 *
 * The panel's domain is combined with the current panzoom extent. When no explicit domain has
 * been configured, the default visual domain [-1, +1] is used.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param out_min output visible data minimum
 * @param out_max output visible data maximum
 * @return whether the visible domain was written
 */
DVZ_EXPORT bool
dvz_panel_visible_domain(DvzPanel* panel, DvzDim dim, double* out_min, double* out_max);


/**
 * Normalize tightly packed 3D data positions through the panel X/Y domains.
 *
 * X and Y are mapped from data coordinates into panel visual coordinates in [-1, +1]. Z is copied
 * unchanged. Unset domains fall back to pass-through visual coordinates for that dimension.
 *
 * @param panel the panel
 * @param data_positions tightly packed input positions, 3 floats per item
 * @param visual_positions tightly packed output positions, 3 floats per item
 * @param count number of positions
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_data_to_visual_positions(
    DvzPanel* panel, const float* data_positions, float* visual_positions, uint32_t count);


/**
 * Return a panel-owned axis, creating its WIP geometry visual on first use.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @return the panel-owned axis, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzAxis* dvz_panel_axis(DvzPanel* panel, DvzDim dim);


/**
 * Show or hide one panel-owned axis.
 *
 * @param axis the axis
 * @param visible whether the axis is visible
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_visible(DvzAxis* axis, bool visible);


/**
 * Enable or disable grid lines for one panel-owned axis.
 *
 * @param axis the axis
 * @param visible whether grid lines are visible
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_grid(DvzAxis* axis, bool visible);


/**
 * Set the label stored on one panel-owned axis.
 *
 * The active 2D axis path renders this label through the scene text visual pipeline.
 *
 * @param axis the axis
 * @param label label string, or NULL to clear
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_label(DvzAxis* axis, const char* label);


/**
 * Set the tick policy for one panel-owned axis.
 *
 * @param axis the axis
 * @param policy tick policy, or NULL for defaults
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_tick_policy(DvzAxis* axis, const DvzAxisTickPolicy* policy);


/**
 * Set the line style for one panel-owned axis.
 *
 * @param axis the axis
 * @param style axis style, or NULL for defaults
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_style(DvzAxis* axis, const DvzAxisStyle* style);


/**
 * Set plot-area margins for one panel-owned axis.
 *
 * Margins are in visual-space units relative to the panel bounds. Defaults are zero to preserve
 * edge-to-edge Datoviz behavior. Nonzero values can reserve space for future tick labels, legends,
 * or other panel adornments.
 *
 * @param axis the axis
 * @param left left margin
 * @param right right margin
 * @param bottom bottom margin
 * @param top top margin
 * @return whether the margins were updated
 */
DVZ_EXPORT bool dvz_axis_set_plot_margins(
    DvzAxis* axis, float left, float right, float bottom, float top);


/**
 * Attach an arcball controller to a panel and connect it to an input router.
 *
 * Rotate: left-drag. Pan rotation center: right-drag or middle-drag. Zoom: scroll wheel.
 * Double-click: reset.
 *
 * @param panel the panel
 * @param router input router to subscribe to (may be NULL to create without connecting)
 * @param flags DvzArcballFlags bitmask
 */
DVZ_EXPORT void dvz_panel_set_arcball(DvzPanel* panel, DvzInputRouter* router, int flags);


/**
 * Return the arcball controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned arcball, or NULL
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzPanel* panel);


/**
 * Attach a fly camera controller to a panel and connect it to an input router.
 *
 * The fly controller updates the panel camera. Keyboard movement supports WASD and arrow keys;
 * left-drag controls look, and right-drag moves on the camera vertical plane unless a pivot is set.
 *
 * @param panel the panel
 * @param router input router to subscribe to (may be NULL to create without connecting)
 * @param desc fly descriptor, or NULL for defaults
 * @return the panel-bound fly controller payload
 */
DVZ_EXPORT DvzFly*
dvz_panel_set_fly(DvzPanel* panel, DvzInputRouter* router, const DvzFlyDesc* desc);


/**
 * Return the fly controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-bound fly controller payload, or NULL
 */
DVZ_EXPORT DvzFly* dvz_panel_fly(DvzPanel* panel);


/**
 * Attach a turntable camera controller to a panel and connect it to an input router.
 *
 * The turntable controller updates the panel camera by orbiting around a stable-up pivot.
 *
 * @param panel the panel
 * @param router input router to subscribe to (may be NULL to create without connecting)
 * @param desc turntable descriptor, or NULL for defaults
 * @return the panel-owned turntable controller
 */
DVZ_EXPORT DvzTurntable* dvz_panel_set_turntable(
    DvzPanel* panel, DvzInputRouter* router, const DvzTurntableDesc* desc);


/**
 * Return the turntable controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned turntable controller, or NULL
 */
DVZ_EXPORT DvzTurntable* dvz_panel_turntable(DvzPanel* panel);



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

/**
 * Destroy a visual.
 *
 * @param visual the visual
 */
DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);


/**
 * Set visual visibility.
 *
 * @param visual the visual
 * @param visible true to show, false to hide
 */
DVZ_EXPORT void dvz_visual_set_visible(DvzVisual* visual, bool visible);


/**
 * Enable or disable depth testing for the visual.
 *
 * Depth-tested opaque visuals write and test against scene depth. Transparent visuals use depth
 * testing to decide whether transparent fragments are occluded by previously rendered opaque
 * geometry. Disabling it is primarily useful for diagnostics and overlays.
 *
 * @param visual the visual
 * @param enabled true to depth-test, false to ignore scene depth
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_depth_test(DvzVisual* visual, bool enabled);


/**
 * Return whether depth testing is enabled for the visual.
 *
 * @param visual the visual
 * @return whether depth testing is enabled
 */
DVZ_EXPORT bool dvz_visual_depth_test(const DvzVisual* visual);


/**
 * Set the visual alpha handling mode.
 *
 * This controls which transparency path the scene planner should use for the visual. Use
 * DVZ_ALPHA_BLENDED for ordinary source-over alpha blending, DVZ_ALPHA_WBOIT for weighted
 * blended order-independent transparency, and DVZ_ALPHA_DEPTH_PEEL for the depth-peeling
 * order-independent transparency path.
 *
 * @param visual the visual
 * @param mode the alpha handling mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_alpha_mode(DvzVisual* visual, DvzAlphaMode mode);


/**
 * Mark a visual as embedded in the panel volume occluder.
 *
 * @param visual the visual
 * @param enabled whether the visual should sample panel volume occlusion
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_visual_set_volume_occluded(DvzVisual* visual, bool enabled);


/**
 * Mark a visual as contributing front depth to panel scene occlusion.
 *
 * @param visual the visual
 * @param enabled whether the visual should act as a scene occluder
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_visual_set_scene_occluder(DvzVisual* visual, bool enabled);


/**
 * Mark a visual as sampling panel scene occlusion.
 *
 * @param visual the visual
 * @param enabled whether the visual should be attenuated by scene occlusion
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_visual_set_scene_occluded(DvzVisual* visual, bool enabled);


/**
 * Return the visual alpha handling mode.
 *
 * @param visual the visual
 * @return the alpha handling mode
 */
DVZ_EXPORT DvzAlphaMode dvz_visual_alpha_mode(const DvzVisual* visual);


/**
 * Declare the semantic source for a visual attribute.
 *
 * This metadata is used by scene planning and future external-buffer lowering. The active dense
 * data path remains `DVZ_VISUAL_ATTR_SOURCE_PER_ITEM`; non-per-item sources may be declared only
 * before dense data is attached to the attribute.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param source the semantic attribute source
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_attr_source(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrSource source);


/**
 * Return the semantic source for a visual attribute.
 *
 * Missing attributes default to `DVZ_VISUAL_ATTR_SOURCE_PER_ITEM`.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @return the semantic attribute source
 */
DVZ_EXPORT DvzVisualAttrSource
dvz_visual_attr_source(const DvzVisual* visual, const char* attr_name);


/**
 * Declare the expected update frequency for a visual attribute.
 *
 * The hint is advisory and does not change ownership. It should be set before attaching data when
 * callers know that an attribute is static or updated every frame.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param mutability the expected update frequency
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_attr_mutability(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrMutability mutability);


/**
 * Return the expected update frequency for a visual attribute.
 *
 * Missing attributes default to `DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC`.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @return the mutability hint
 */
DVZ_EXPORT DvzVisualAttrMutability
dvz_visual_attr_mutability(const DvzVisual* visual, const char* attr_name);


/**
 * Write attribute data to a visual.
 *
 * First-slice visual families currently accept:
 * point: `"position"` (vec3f), `"color"` (RGBA8), `"diameter"` (float pixels),
 *        optional `"selection"` (uint8 mask)
 * pixel: `"position"` (vec3f), `"color"` (RGBA8), `"pixel_size"` (float pixels)
 * marker: `"position"` (vec3f), `"color"` (RGBA8), `"diameter"` (float pixels),
 *         `"angle"` (float radians), `"shape"` (uint32_t DvzMarkerShape),
 *         optional `"selection"` (uint8 mask)
 * sphere: `"position"` (vec3f), `"color"` (RGBA8), `"radius"` (float scene units)
 * segment: `"position_start"` (vec3f), `"position_end"` (vec3f), `"color"` (RGBA8),
 *          `"stroke_width"` (float pixels)
 * primitive: `"position"` (vec3f), `"color"` (RGBA8)
 * path: `"position"` (vec3f), `"color"` (RGBA8), optional `"stroke_width"` (float pixels)
 * mesh: `"position"` (vec3f), optional `"color"` (RGBA8), optional `"normal"` (vec3f),
 *       optional `"instance_transform"` (mat4f, one per instance)
 * primitive only: `"normal"` (vec3f)
 * image: legacy `"position"` (vec3f) + `"texcoords"` (vec2f) corner vertices, or
 *        per-item `"position"` (vec3f) + `"extent"` (vec2f) with optional `"tex_rect"`
 *        (vec4f) and `"anchor"` (vec2f)
 * text: string attribute `"text"` plus per-string `"position"` (vec3f pixels), optional
 *       `"anchor"` (vec2f), `"size"` (float points), `"color"` (RGBA8), `"angle"` (float radians)
 * glyph: `"position"` (vec3f anchor), `"bounds"` (vec4f local pixel bounds),
 *        `"texcoords"` (vec4f atlas UV bounds), `"color"` (RGBA8), `"angle"` (float radians)
 *
 * All configured attributes on one visual must use the same item_count. This
 * call is rejected while any emitted scene stream is still live.
 *
 * @param visual the visual
 * @param attr_name attribute name (family-specific, e.g. "position", "color")
 * @param data packed data array
 * @param item_count number of items
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_data(DvzVisual* visual, const char* attr_name, const void* data,
                                    uint32_t item_count);


/**
 * Write variable-length string data to a visual.
 *
 * Text visuals accept the `"text"` string attribute. The item count must match any configured
 * per-item dense attributes.
 *
 * @param visual the visual
 * @param attr_name string attribute name
 * @param strings string array
 * @param item_count number of strings
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_strings(
    DvzVisual* visual, const char* attr_name, const char* const* strings, uint32_t item_count);


/**
 * Atomically replace several dense visual attribute payloads.
 *
 * This is the preferred API when changing the item count of a visual with several per-item
 * attributes, such as point position/color/size. All updates are validated before any existing
 * attribute payload is replaced. Every update in the batch must use the same item_count, and any
 * existing dense per-item attribute with a different item_count must also be included in the batch.
 * This call is rejected while any emitted scene stream is still live.
 *
 * @param visual the visual
 * @param updates attribute update descriptors
 * @param update_count number of update descriptors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_data_many(
    DvzVisual* visual, const DvzVisualDataUpdate* updates, uint32_t update_count);


/**
 * Write a contiguous sub-range of attribute data to a visual.
 *
 * The attribute must already be fully allocated by a prior
 * dvz_visual_set_data() call. Only the items in
 * [first_item, first_item + item_count) are uploaded on the next emit. This
 * call is rejected while any emitted scene stream is still live.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param data packed array of item_count items to write
 * @param first_item index of the first item to update
 * @param item_count number of items to update
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_data_range(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_item, uint32_t item_count);


/**
 * Create a reusable scene-owned buffer resource.
 *
 * First retained slice: visuals bind these buffers through `dvz_visual_set_buffer()`.
 * The initial supported slot is primitive `"index"` buffers. `stride` is the byte stride
 * of one item in the uploaded payload (for example `sizeof(DvzIndex)` for 32-bit index buffers,
 * or `sizeof(uint16_t)` for 16-bit).
 *
 * @param scene the scene
 * @param desc the buffer descriptor
 * @return the buffer, or NULL on error
 */
DVZ_EXPORT DvzSceneBuffer* dvz_scene_buffer(DvzScene* scene, const DvzSceneBufferDesc* desc);


/**
 * Destroy a scene-owned buffer resource.
 *
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_scene_buffer_destroy(DvzSceneBuffer* buffer);


/**
 * Replace the full payload of a scene-owned buffer resource.
 *
 * @param buffer the buffer
 * @param data the packed byte payload
 * @param byte_size the payload size in bytes
 * @return true on success, false on error
 */
DVZ_EXPORT bool
dvz_scene_buffer_set_data(DvzSceneBuffer* buffer, const void* data, uint64_t byte_size);


/**
 * Return the immutable buffer descriptor.
 *
 * @param buffer the buffer
 * @return the descriptor, or NULL on error
 */
DVZ_EXPORT const DvzSceneBufferDesc* dvz_scene_buffer_desc(const DvzSceneBuffer* buffer);


/**
 * Bind a scene-owned buffer to a named visual slot.
 *
 * First retained slice: primitive and mesh visuals accept the `"index"` slot. The bound scene
 * buffer must advertise `DVZ_SCENE_BUFFER_USAGE_INDEX`.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param buffer the buffer, or NULL to clear the binding
 * @return true on success, false on error
 */
DVZ_EXPORT bool
dvz_visual_set_buffer(DvzVisual* visual, const char* slot_name, DvzSceneBuffer* buffer);


/**
 * Bind a scene-owned buffer to a per-item visual attribute.
 *
 * This is the C-level groundwork for externally produced GPU attributes. The first slice supports
 * planar vertex attributes only: the scene buffer stride must match the attribute item size, and
 * the attribute source must remain `DVZ_VISUAL_ATTR_SOURCE_PER_ITEM`.
 *
 * If the bound scene buffer has CPU data, the scene emits normal buffer upload commands. If it has
 * no CPU data, the scene registers the resource label for a live runtime to satisfy separately and
 * emits no CPU upload for that attribute.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param buffer the scene buffer, or NULL to clear the binding
 * @param byte_offset byte offset into the buffer
 * @param item_count number of attribute items
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_visual_set_attr_buffer(
    DvzVisual* visual, const char* attr_name, DvzSceneBuffer* buffer,
    uint64_t byte_offset, uint32_t item_count);


/**
 * Return default visual material options.
 *
 * The default material is the fast Phong model with opaque alpha, full opacity, a white base-color
 * factor, light direction `(0, 0, 1)`, ambient `0.2`, diffuse `0.8`, specular `0.25`, and
 * shininess `32`.
 *
 * @return default material descriptor
 */
DVZ_EXPORT DvzMaterialDesc dvz_material_desc(void);


/**
 * Set the shared material parameters for a primitive, mesh, or sphere visual.
 *
 * The Phong model maps directly to the current material shader payload. The standard model is
 * retained in the scene material state and lowered to the current shader payload until the standard
 * shader path is broadened. Pass NULL to restore default material parameters.
 *
 * @param visual the visual
 * @param desc the material descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_material(DvzVisual* visual, const DvzMaterialDesc* desc);


/**
 * Override primitive shading parameters.
 *
 * Compatibility wrapper around `dvz_visual_set_material()` with `DVZ_MATERIAL_MODEL_PHONG`.
 * The current primitive/mesh slice uses these parameters only when a visual also has a bound
 * `normal` attribute. Sphere visuals use the same material light parameters.
 *
 * @param visual the visual
 * @param desc the shading descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_visual_set_primitive_shading(DvzVisual* visual, const DvzPrimitiveShadingDesc* desc);


/**
 * Configure depth cueing for a point, pixel, primitive, mesh, or sphere visual.
 *
 * Primitive/mesh/sphere visuals use depth cueing through the material shader path. Point/pixel
 * visuals use the same cue parameters without lighting. The `near_depth` and `far_depth` values
 * are interpreted in the descriptor metric, where lower values are closer. The default metric is
 * normalized clip depth after the visual's scene transform. Pass NULL to disable depth cueing.
 *
 * @param visual the visual
 * @param desc the depth-cue descriptor, or NULL to disable depth cueing
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_depth_cue(DvzVisual* visual, const DvzDepthCueDesc* desc);


/**
 * Return default point styling.
 *
 * The default point style renders filled circular points with no stroke. The `color` visual
 * attribute remains the fill color; `edge_color` and `stroke_width` apply only when `stroke` or
 * `outline` is enabled.
 *
 * @return default point style descriptor
 */
DVZ_EXPORT DvzPointStyleDesc dvz_point_style_desc(void);


/**
 * Configure circular point fill/stroke styling.
 *
 * Pass NULL to restore the default filled/no-stroke style. `outline` renders only the stroke ring;
 * otherwise `filled` controls whether the disc interior is drawn.
 *
 * @param visual the point visual
 * @param desc the point style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_point_set_style(DvzVisual* visual, const DvzPointStyleDesc* desc);


/**
 * Return default marker styling.
 *
 * The default marker style renders filled markers with no stroke. The `color` visual attribute
 * remains the fill color; `edge_color` and `stroke_width` apply when `stroke` or `outline` is
 * enabled.
 *
 * @return default marker style descriptor
 */
DVZ_EXPORT DvzMarkerStyle dvz_marker_style(void);


/**
 * Configure marker fill/stroke styling.
 *
 * Pass NULL to restore the default filled/no-stroke style. `outline` renders only the stroke;
 * otherwise `filled` controls whether the marker interior is drawn.
 *
 * @param visual the marker visual
 * @param style the marker style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_marker_set_style(DvzVisual* visual, const DvzMarkerStyle* style);



/*************************************************************************************************/
/*  Visual family constructors                                                                   */
/*************************************************************************************************/

/**
 * Create a point visual.
 *
 * Renders screen-space antialiased circular sprites with `position` (vec3), `color` (RGBA8),
 * and `diameter` (float, in pixels). An optional `selection` (uint8) mask dims unselected
 * items. `dvz_point_set_style()` controls optional edge styling with `edge_color`,
 * `stroke_width`, and filled/stroke/outline aspects.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzScene* scene, uint32_t flags);


/**
 * Create a pixel visual.
 *
 * Renders screen-space square sprites with `position` (vec3), `color` (RGBA8), and
 * `pixel_size` (float, in pixels). WGSL/WebGPU emission lowers each item to an instanced quad.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_pixel(DvzScene* scene, uint32_t flags);


/**
 * Create a marker visual.
 *
 * Renders screen-space code-SDF marker sprites with dense `position` (vec3), `color` (RGBA8),
 * `diameter` (float in pixels), `angle` (float radians), and `shape` (uint32_t
 * DvzMarkerShape) attributes. An optional `selection` (uint8) mask dims unselected items.
 * First-slice shapes are disc, square, triangle, diamond, cross, and ring.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_marker(DvzScene* scene, uint32_t flags);


/**
 * Create a sphere impostor visual.
 *
 * Sphere visuals render one analytic 3D sphere per item from `position` (vec3 center), `color`
 * (RGBA8), and `radius` (float). The GLSL backend reconstructs the sphere surface in the
 * fragment shader, writes sphere-surface depth, and uses analytic antialiasing at the silhouette.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_sphere(DvzScene* scene, uint32_t flags);


/**
 * Set the sphere impostor rendering mode.
 *
 * `DVZ_SPHERE_MODE_FAST_IMPOSTOR` uses the cheap point-coordinate sphere reconstruction.
 * `DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR` intersects the camera ray with the sphere in the fragment
 * shader for more accurate surface position, normal, and depth.
 *
 * @param visual the sphere visual
 * @param mode the rendering mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_sphere_mode(DvzVisual* visual, DvzSphereMode mode);


/**
 * Create a segment visual.
 *
 * First-slice segment visuals render independent endpoint pairs as analytic screen-space
 * stroked line segments. Dense attributes are `position_start` (vec3), `position_end` (vec3),
 * `color` (RGBA8), and `stroke_width` (float width in pixels). Segment caps default to butt at
 * both ends.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_segment(DvzScene* scene, uint32_t flags);


/**
 * Configure segment endpoint caps.
 *
 * Arrow caps, dashes, scalar color, and per-item cap attributes are deferred. Supported first
 * slice caps are none, round, triangle-in, triangle-out, square, and butt.
 *
 * @param visual the segment visual
 * @param start_cap cap applied to `position_start`
 * @param end_cap cap applied to `position_end`
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_segment_set_caps(DvzVisual* visual, DvzSegmentCap start_cap, DvzSegmentCap end_cap);


/**
 * Create a primitive visual.
 *
 * Renders raw GPU primitives (point lists, line lists/strips, triangle lists/strips) with
 * built-in shaders. Accepts `position` (vec3) and `color` (RGBA8), plus optional `normal`
 * (vec3) and optional `"index"` buffer bindings for indexed draws.
 *
 * @param scene the scene
 * @param topology primitive topology, fixed at construction time
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_primitive(
    DvzScene* scene, DvzPrimitiveTopology topology, uint32_t flags);


/**
 * Create a mesh visual.
 *
 * First retained slice: meshes use a triangle-list topology with `position` (vec3), optional
 * `color` (RGBA8, defaulting to opaque white when omitted), optional `normal` (vec3), optional
 * `instance_transform` (mat4, one per instance), and optional `"index"` buffer bindings for
 * indexed draws.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags);


/**
 * Create a path visual.
 *
 * A path accepts `position` (vec3), `color` (RGBA8), and optional per-point `stroke_width`
 * (float, pixels). Without `stroke_width`, paths use the primitive line-strip pipeline. With
 * `stroke_width`, paths are lowered to screen-space stroked segments built from consecutive points.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_path(DvzScene* scene, uint32_t flags);


/**
 * Set explicit subpath lengths for a path visual.
 *
 * When unset, all points belong to one open subpath. Lengths are consumed in order and must sum to
 * the path position count at emission time.
 *
 * @param visual the path visual
 * @param subpath_count number of subpaths
 * @param lengths point count for each subpath
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_path_set_subpaths(
    DvzVisual* visual, uint32_t subpath_count, const uint32_t* lengths);


/**
 * Create an image visual.
 *
 * First-slice scope: one sampled 2D texture per visual. The legacy path accepts `position`
 * (vec3, 4 corner vertices in triangle-strip order) and `texcoords` (vec2, matching UVs).
 * The retained per-item path accepts one anchor `position` and `extent` per image item, with
 * optional `tex_rect` atlas rectangles and per-item `anchor`. Bind a sampled field via
 * `dvz_visual_set_field()`. The legacy texture convenience wrappers remain available and lower
 * to scene-owned sampled fields internally.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzScene* scene, uint32_t flags);


/**
 * Create a batched text visual.
 *
 * Text visuals render one string per item. Use `dvz_visual_set_strings(text, "text", ...)` and
 * regular visual data attributes for positions, text-box anchors, sizes, colors, and angles.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_text(DvzScene* scene, uint32_t flags);


/**
 * Select the renderer used by a batched text visual.
 *
 * The current retained text visual path supports the built-in bitmap atlas renderers and an
 * SDF-backed atlas for the `DVZ_TEXT_RENDERER_MSDF_ATLAS` selection.
 *
 * @param visual text visual
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_text_set_renderer(DvzVisual* visual, DvzTextRenderer renderer);


/**
 * Create a glyph visual.
 *
 * Renders atlas-backed glyph quads with `position` (vec3 anchor), `bounds` (vec4 local pixel
 * bounds), `texcoords` (vec4 atlas UV bounds), `color` (RGBA8), `angle` (float radians), and a
 * bound 2D sampled field.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_glyph(DvzScene* scene, uint32_t flags);


/**
 * Create a volume visual.
 *
 * Volume visuals retain a 3D sampled field bound through
 * `dvz_visual_set_field(volume, "field", field)`. The native runtime renders a box proxy and
 * supports full-volume composite rendering by default, plus slice and MIP modes.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_volume(DvzScene* scene, uint32_t flags);


/**
 * Set the global opacity multiplier on a volume visual.
 *
 * @param visual the volume visual
 * @param opacity opacity multiplier in [0, 1]
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_opacity(DvzVisual* visual, float opacity);


/**
 * Set the texture sampling mode on a volume visual.
 *
 * @param visual the volume visual
 * @param sampling the sampling mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_volume_set_sampling(DvzVisual* visual, DvzVolumeSamplingMode sampling);


/**
 * Set the volume render mode.
 *
 * @param visual the volume visual
 * @param mode the render mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_render_mode(DvzVisual* visual, DvzVolumeRenderMode mode);


/**
 * Set the volume slice axis.
 *
 * @param visual the volume visual
 * @param axis axis normal for slicing planes (X/Y/Z)
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis);


/**
 * Set the normalized volume slice position.
 *
 * @param visual the volume visual
 * @param position slice position in [0, 1], where 0 is the minimum axis coordinate
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_slice_position(DvzVisual* visual, double position);


/**
 * Set the volume raymarch step count used by MIP and composite rendering.
 *
 * @param visual the volume visual
 * @param step_count number of raymarch samples
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_step_count(DvzVisual* visual, uint32_t step_count);


/**
 * Set the object-space volume proxy bounds.
 *
 * The bounds control the rendered box geometry while preserving normalized UVW texture sampling.
 * They are useful for displaying anisotropic volumes in their physical aspect ratio.
 *
 * @param visual the volume visual
 * @param bounds_min minimum object-space coordinate
 * @param bounds_max maximum object-space coordinate
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_volume_set_bounds(DvzVisual* visual, const double bounds_min[3], const double bounds_max[3]);


/**
 * Set the mapping from normalized volume coordinates to texture UVW coordinates.
 *
 * `axis_order[i]` selects the normalized volume coordinate used for texture axis `i`.
 * `axis_flip[i]` flips texture axis `i` after reordering. Pass NULL for `axis_flip` to disable
 * all flips.
 *
 * @param visual the volume visual
 * @param axis_order texture-axis source order, a permutation of 0, 1, 2
 * @param axis_flip optional per-texture-axis flips
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_axis_mapping(
    DvzVisual* visual, const uint32_t axis_order[3], const bool axis_flip[3]);


/**
 * Set the scalar value range used before transfer texture lookup.
 *
 * @param visual the volume visual
 * @param min minimum scalar value mapped to 0
 * @param max maximum scalar value mapped to 1
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_value_range(DvzVisual* visual, double min, double max);


/**
 * Set piecewise-linear opacity stops for scalar volume transfer.
 *
 * @param visual the volume visual
 * @param stops alpha stops sorted or unsorted by position
 * @param count number of stops, at most 8
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_alpha_stops(
    DvzVisual* visual, const DvzVolumeAlphaStop* stops, uint32_t count);


/**
 * Enable axis-aligned clipping on a volume visual.
 *
 * @param visual the volume visual
 * @param clip_min minimum normalized clip coordinate
 * @param clip_max maximum normalized clip coordinate
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_volume_set_clipping_box(DvzVisual* visual, const double clip_min[3], const double clip_max[3]);


/**
 * Enable arbitrary plane clipping on a volume visual.
 *
 * The plane is defined in normalized volume coordinates. Voxels for which
 * `dot(normal, uvw - point)` has the selected sign are kept.
 *
 * @param visual the volume visual
 * @param point point on the clipping plane, in normalized volume coordinates
 * @param normal non-zero clipping plane normal
 * @param keep_positive whether to keep the positive side of the plane
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_set_clipping_plane(
    DvzVisual* visual, const double point[3], const double normal[3], bool keep_positive);


/**
 * Disable arbitrary plane clipping on a volume visual.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_clear_clipping_plane(DvzVisual* visual);


/**
 * Disable all clipping on a volume visual.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_volume_clear_clipping(DvzVisual* visual);


/**
 * Return the retained volume state for inspection.
 *
 * @param visual the volume visual
 * @return the volume state, or NULL on error
 */
DVZ_EXPORT const DvzVolumeState* dvz_volume_state(const DvzVisual* visual);


/**
 * Attach a 2D RGBA8 texture to an image or glyph visual.
 *
 * Transitional convenience wrapper: this creates or updates a scene-owned sampled field and
 * binds it to the visual's `"field"` slot. Prefer `dvz_sampled_field()` plus
 * `dvz_visual_set_field()` in new code.
 *
 * @param visual the visual (must be of type IMAGE or GLYPH)
 * @param rgba RGBA8 pixel data, tightly packed, row-major (`width * height * 4` bytes)
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_texture(
    DvzVisual* visual, const void* rgba, uint32_t width, uint32_t height);


/**
 * Attach a 2D scalar F32 texture to an image or glyph visual.
 *
 * Transitional convenience wrapper: this creates or updates a scene-owned sampled field and
 * binds it to the visual's `"field"` slot. The bound scale and colormap are applied
 * on the CPU during emit to produce the RGBA texture used by the current first-slice image
 * runtime path. Prefer `dvz_sampled_field()` plus `dvz_visual_set_field()` in new code.
 *
 * @param visual the visual (must be of type IMAGE or GLYPH)
 * @param values scalar F32 pixel data, tightly packed, row-major
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_texture_f32(
    DvzVisual* visual, const float* values, uint32_t width, uint32_t height);


EXTERN_C_OFF
