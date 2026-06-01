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
#include "datoviz/geom/types.h"
#include "scene/annotation.h"
#include "scene/animation.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/enums.h"
#include "scene/field.h"
#include "scene/fly.h"
#include "scene/frame_plan.h"
#include "scene/interaction.h"
#include "scene/orbit_camera.h"
#include "scene/overlay.h"
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
 * Set the scene font defaults used by text objects without an explicit font.
 *
 * The scene copies the descriptor values, but string pointers remain borrowed and must outlive
 * any later default-font resolution that uses them.
 *
 * @param scene the scene
 * @param defaults font defaults, or NULL for dvz_font_defaults()
 */
DVZ_EXPORT void dvz_scene_set_font_defaults(DvzScene* scene, const DvzFontDefaults* defaults);


/**
 * Return the scene font defaults.
 *
 * @param scene the scene
 * @return the scene font defaults, or dvz_font_defaults() when scene is NULL
 */
DVZ_EXPORT DvzFontDefaults dvz_scene_font_defaults(const DvzScene* scene);


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
 * Return the scene that owns a figure.
 *
 * @param figure the figure
 * @return the owning scene, or NULL
 */
DVZ_EXPORT DvzScene* dvz_figure_scene(DvzFigure* figure);


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
 * Create a retained grid layout object owned by a figure.
 *
 * Rows and columns default to weight-based sizing with weight 1.0. Grid margins and gutters
 * default to zero logical pixels.
 *
 * @param figure the figure
 * @param rows number of rows
 * @param cols number of columns
 * @return the grid, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGrid* dvz_figure_grid(DvzFigure* figure, uint32_t rows, uint32_t cols);


/**
 * Set fixed logical-pixel margins around one grid.
 *
 * @param grid the grid
 * @param margins grid margins, or NULL for zero margins
 * @return whether the margins were accepted
 */
DVZ_EXPORT bool dvz_grid_set_margins(DvzGrid* grid, const DvzPanelReserve* margins);


/**
 * Set fixed logical-pixel gutters between grid columns and rows.
 *
 * @param grid the grid
 * @param x_px horizontal gutter in logical pixels
 * @param y_px vertical gutter in logical pixels
 * @return whether the gutters were accepted
 */
DVZ_EXPORT bool dvz_grid_set_gutter(DvzGrid* grid, float x_px, float y_px);


/**
 * Set one grid column size.
 *
 * @param grid the grid
 * @param col zero-based column index
 * @param mode size mode
 * @param value weight or fixed logical-pixel size
 * @return whether the size was accepted
 */
DVZ_EXPORT bool
dvz_grid_col_size(DvzGrid* grid, uint32_t col, DvzGridSizeMode mode, float value);


/**
 * Set one grid row size.
 *
 * @param grid the grid
 * @param row zero-based row index
 * @param mode size mode
 * @param value weight or fixed logical-pixel size
 * @return whether the size was accepted
 */
DVZ_EXPORT bool
dvz_grid_row_size(DvzGrid* grid, uint32_t row, DvzGridSizeMode mode, float value);


/**
 * Resolve one grid cell into a normalized figure-space panel rectangle.
 *
 * @param grid the grid
 * @param width figure width in logical pixels
 * @param height figure height in logical pixels
 * @param cell zero-based cell and span
 * @param out output normalized panel rectangle
 * @return whether the cell was resolved
 */
DVZ_EXPORT bool dvz_grid_resolve(
    const DvzGrid* grid, uint32_t width, uint32_t height, DvzGridCell cell,
    DvzPanelDesc* out);


/**
 * Create a grid-owned panel for one cell.
 *
 * @param grid the grid
 * @param row zero-based row index
 * @param col zero-based column index
 * @return the panel, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzPanel* dvz_grid_panel(DvzGrid* grid, uint32_t row, uint32_t col);


/**
 * Create a grid-owned panel spanning contiguous cells.
 *
 * @param grid the grid
 * @param row zero-based origin row index
 * @param col zero-based origin column index
 * @param row_span number of rows covered by the panel
 * @param col_span number of columns covered by the panel
 * @return the panel, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzPanel* dvz_grid_panel_span(
    DvzGrid* grid, uint32_t row, uint32_t col, uint32_t row_span, uint32_t col_span);


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
 * Create a scene-owned controller state link.
 *
 * Links propagate selected semantic state components from a source controller to a distinct target
 * controller in the same scene. The first supported link mode is one-way propagation between
 * controllers of the same family.
 *
 * @param scene the scene
 * @param source the source controller
 * @param target the target controller
 * @param components bitmask of DvzControllerLinkComponent values
 * @param mode link propagation mode
 * @return the scene-owned link handle, or NULL on validation error
 */
DVZ_EXPORT DvzControllerLink* dvz_controller_link(
    DvzScene* scene, DvzController* source, DvzController* target, uint32_t components,
    DvzControllerLinkMode mode);


/**
 * Destroy a scene-owned controller state link.
 *
 * @param link the link
 */
DVZ_EXPORT void dvz_controller_link_destroy(DvzControllerLink* link);


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
 * Return the orbit-camera payload of an orbit-camera controller.
 *
 * @param controller the controller
 * @return the borrowed orbit-camera payload, or NULL for the wrong family
 */
DVZ_EXPORT DvzOrbitCamera* dvz_controller_orbit_camera(DvzController* controller);


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
 * Execute queued query requests for one figure through the DRP2 runtime.
 *
 * @param figure the figure
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of requests that were consumed from the scene queues
 */
DVZ_EXPORT uint32_t dvz_figure_process_queries(
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
 * Update a panel rectangle in normalized figure coordinates.
 *
 * Changing the descriptor updates panel viewport/scissor state on the next emit and marks
 * layout-dependent adornments dirty.
 *
 * @param panel the panel
 * @param desc panel position and size in normalized [0, 1] figure coordinates
 * @return whether the descriptor was accepted
 */
DVZ_EXPORT bool dvz_panel_set_desc(DvzPanel* panel, DvzPanelDesc desc);


/**
 * Create a panel that fills the whole figure.
 *
 * @param figure the figure
 * @return the panel
 */
DVZ_EXPORT DvzPanel* dvz_panel_full(DvzFigure* figure);


/**
 * Return the default panel layout reservation.
 *
 * The default is zero on every side so plot panels remain edge-to-edge unless callers opt in.
 *
 * @return default panel layout reservation
 */
DVZ_EXPORT DvzPanelLayoutReserve dvz_panel_layout_reserve(void);


/**
 * Set a fixed pixel reservation around one panel's plot area.
 *
 * Reservations are in logical pixels and remain stable across figure/window resizes. Pass NULL to
 * reset every side to zero.
 *
 * @param panel the panel
 * @param reserve pixel reservation descriptor, or NULL for zero reserve
 * @return whether the reservation was accepted
 */
DVZ_EXPORT bool dvz_panel_set_reserve(DvzPanel* panel, const DvzPanelReserve* reserve);


/**
 * Return one panel's fixed pixel reservation.
 *
 * @param panel the panel
 * @param out output pixel reservation
 * @return whether the reservation was written
 */
DVZ_EXPORT bool dvz_panel_get_reserve(const DvzPanel* panel, DvzPanelReserve* out);


/**
 * Set a fixed pixel padding inside one panel's outer rectangle.
 *
 * Padding is applied before reserves are resolved: the padded inner panel rectangle contains both
 * the plot rectangle and reserved adornment bands. Pass NULL to reset every side to zero.
 *
 * @param panel the panel
 * @param padding pixel padding descriptor, or NULL for zero padding
 * @return whether the padding was accepted
 */
DVZ_EXPORT bool dvz_panel_set_padding(DvzPanel* panel, const DvzPanelReserve* padding);


/**
 * Return one panel's fixed pixel padding.
 *
 * @param panel the panel
 * @param out output pixel padding
 * @return whether the padding was written
 */
DVZ_EXPORT bool dvz_panel_get_padding(const DvzPanel* panel, DvzPanelReserve* out);


/**
 * Return one panel's current inner rectangle in figure pixel coordinates.
 *
 * The inner rectangle is the panel rectangle after padding and before resolved reserve.
 *
 * @param panel the panel
 * @param out output inner rectangle in logical pixels
 * @return whether the rectangle was written
 */
DVZ_EXPORT bool dvz_panel_inner_rect_px(const DvzPanel* panel, DvzRect* out);


/**
 * Return one panel's current plot rectangle in figure pixel coordinates.
 *
 * The plot rectangle is the panel rectangle after padding and resolved reserve.
 *
 * @param panel the panel
 * @param out output plot rectangle in logical pixels
 * @return whether the rectangle was written
 */
DVZ_EXPORT bool dvz_panel_plot_rect_px(const DvzPanel* panel, DvzRect* out);


/**
 * Reserve visual-space room around one panel's plot area for future adornments.
 *
 * Compatibility bridge: reservations are accepted in panel visual-space units and converted to
 * fixed logical pixels at the panel's current size. Prefer dvz_panel_set_reserve() for new code.
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
 * Route an input router through one panel's bound controllers.
 *
 * The panel supplies viewport-local context before events reach scene-owned controllers, so shared
 * controllers do not own a single canonical viewport.
 *
 * @param panel the panel
 * @param router input router to subscribe to, or NULL to disconnect
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_connect_input(DvzPanel* panel, DvzInputRouter* router);


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
 * Add all generated visual roles of a composite to a panel.
 *
 * The composite is realized before attachment. Generated visuals are then attached to the panel
 * with the same semantics as dvz_panel_add_visual().
 *
 * @param panel the panel
 * @param composite the composite
 * @param desc attachment options applied to the composite roles
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_add_composite(
    DvzPanel* panel, DvzComposite* composite, const DvzVisualAttachDesc* desc);


/**
 * Set or update a panel background.
 *
 * Backgrounds are rendered as a fixed full-panel visual behind regular visuals. Passing NULL or
 * a descriptor with type DVZ_PANEL_BACKGROUND_NONE clears the current background. Linear gradients
 * use panel-local start and end points in [0, 1]. Image backgrounds accept tightly packed RGBA8
 * pixels and stretch them to the panel rectangle.
 *
 * @param panel the panel
 * @param background the background descriptor, or NULL to clear
 * @return whether the background was updated
 */
DVZ_EXPORT bool
dvz_panel_set_background(DvzPanel* panel, const DvzPanelBackgroundDesc* background);


/**
 * Clear a panel background.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_clear_background(DvzPanel* panel);


/**
 * Set or update a uniform-color background for a panel.
 *
 * Internally creates a fullscreen-quad visual attached behind regular visuals with
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
 * Show or hide the panel-owned visual bounds overlay.
 *
 * The overlay is a generated wireframe segment visual in visual space. It follows the panel
 * controller and is rebuilt from visible, controller-applied visuals before frame emission.
 *
 * @param panel the panel
 * @param visible whether bounds boxes should be shown
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_set_bounds_visible(DvzPanel* panel, bool visible);


/**
 * Return whether the panel-owned visual bounds overlay is enabled.
 *
 * @param panel the panel
 * @return whether bounds boxes should be shown
 */
DVZ_EXPORT bool dvz_panel_bounds_visible(const DvzPanel* panel);


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
 * Return the default axis tick policy.
 *
 * @return default axis tick policy
 */
DVZ_EXPORT DvzAxisTickPolicy dvz_axis_tick_policy(void);


/**
 * Return the default axis line and text style.
 *
 * @return default axis style
 */
DVZ_EXPORT DvzAxisStyle dvz_axis_style(void);


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
 * Set the line and text style for one panel-owned axis.
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
 * Set the retained visual-local transform.
 *
 * The transform is stored on the visual and applies to every panel attachment before panel
 * controller/view transforms.
 *
 * @param visual the visual
 * @param transform local model transform
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_transform(DvzVisual* visual, mat4 transform);


/**
 * Clear the retained visual-local transform back to identity.
 *
 * @param visual the visual
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_clear_transform(DvzVisual* visual);


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
 *        optional `"item_state"` (uint32_t DvzItemStateKind bitfield)
 * splat: `"position"` (vec3f), `"color"` (RGBA8), `"sigma"` (vec2f pixels),
 *        `"angle"` (float radians)
 * pixel: `"position"` (vec3f), `"color"` (RGBA8), `"pixel_size"` (float pixels),
 *        optional `"item_state"` (uint32_t DvzItemStateKind bitfield)
 * marker: `"position"` (vec3f), `"color"` (RGBA8), `"diameter"` (float pixels),
 *         `"angle"` (float radians), `"shape"` (uint32_t DvzMarkerShape),
 *         optional `"item_state"` (uint32_t DvzItemStateKind bitfield)
 * sphere: `"position"` (vec3f), `"color"` (RGBA8), `"radius"` (float scene units)
 * segment: `"position_start"` (vec3f), `"position_end"` (vec3f), `"color"` (RGBA8),
 *          `"stroke_width"` (float pixels)
 * primitive: `"position"` (vec3f), `"color"` (RGBA8)
 * path: `"position"` (vec3f), `"color"` (RGBA8), optional `"stroke_width"` (float pixels)
 * mesh: `"position"` (vec3f), optional `"color"` (RGBA8), optional `"normal"` (vec3f),
 *       optional `"texcoords"` (vec2f), optional `"instance_transform"` (mat4f, one per instance)
 * primitive only: `"normal"` (vec3f)
 * image: legacy `"position"` (vec3f) + `"texcoords"` (vec2f) corner vertices, or
 *        per-item `"position"` (vec3f) + `"extent"` (vec2f) with optional `"tex_rect"`
 *        (vec4f) and `"anchor"` (vec2f)
 * text: string attribute `"text"` plus per-string `"position"` (vec3f pixels), optional
 *       `"anchor"` (vec2f), `"size"` (float pixels), `"color"` (RGBA8), `"angle"` (float radians)
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
 * Return a read-only view of retained dense visual attribute data.
 *
 * Attribute aliases are resolved with the same storage-name rules as dvz_visual_set_data().
 * Buffer-backed, field-backed, metadata-only, and missing attributes do not expose dense data
 * through this first-slice view.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param out output data view
 * @return 0 when dense data is available, -1 otherwise
 */
DVZ_EXPORT int dvz_visual_data(
    const DvzVisual* visual, const char* attr_name, DvzVisualDataView* out);


/**
 * Return the retained visual-space bounding box of one visual.
 *
 * The box is computed from CPU-retained dense visual attributes and family-specific geometry state.
 * It does not require an emitted frame or a live graphics backend. Buffer-backed attributes do not
 * expose CPU-side bounds in this first slice.
 *
 * @param visual the visual
 * @param out output bounding box
 * @return 0 when bounds are available, -1 otherwise
 */
DVZ_EXPORT int dvz_visual_bounds(const DvzVisual* visual, DvzBounds* out);


/**
 * Return one visual's bounds in the coordinate space of one panel attachment.
 *
 * `DVZ_BOUNDS_SPACE_VISUAL` returns the retained visual-space AABB. `DVZ_BOUNDS_SPACE_SCREEN`
 * projects the visual-space box through the panel attachment MVP and returns pixel bounds relative
 * to the figure.
 *
 * @param panel the panel
 * @param visual visual attached to the panel
 * @param space target bounds space
 * @param out output bounding box
 * @return 0 when bounds are available, -1 otherwise
 */
DVZ_EXPORT int dvz_panel_visual_bounds(
    const DvzPanel* panel, const DvzVisual* visual, DvzBoundsSpace space, DvzBounds* out);


/**
 * Return the union of all visible visual bounds attached to one panel.
 *
 * @param panel the panel
 * @param space target bounds space
 * @param out output bounding box
 * @return 0 when at least one visible visual has bounds, -1 otherwise
 */
DVZ_EXPORT int dvz_panel_bounds(const DvzPanel* panel, DvzBoundsSpace space, DvzBounds* out);


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
 * Create an experimental scene-owned compute pass.
 *
 * The compute pass owns no backend handles. It stores shader source, dispatch dimensions, and
 * buffer bindings that are lowered into DRP2 before figure render passes.
 *
 * @param scene the scene
 * @param desc the compute descriptor
 * @return the compute pass, or NULL on error
 */
DVZ_EXPORT DvzSceneCompute*
dvz_scene_compute(DvzScene* scene, const DvzSceneComputeDesc* desc);


/**
 * Destroy a scene-owned compute pass and detach it from all figures.
 *
 * @param compute the compute pass
 */
DVZ_EXPORT void dvz_scene_compute_destroy(DvzSceneCompute* compute);


/**
 * Set the dispatch size for a scene compute pass.
 *
 * @param compute the compute pass
 * @param x workgroup count in X
 * @param y workgroup count in Y
 * @param z workgroup count in Z
 * @return true on success, false on error
 */
DVZ_EXPORT bool
dvz_scene_compute_set_dispatch(DvzSceneCompute* compute, uint32_t x, uint32_t y, uint32_t z);


/**
 * Bind a scene buffer to one compute shader binding.
 *
 * The buffer must advertise `DVZ_SCENE_BUFFER_USAGE_STORAGE`. The first slice supports storage
 * buffers only. Ranges are passed through to the DRP2 bind group.
 *
 * @param compute the compute pass
 * @param binding shader binding index
 * @param buffer scene buffer, or NULL to clear the binding
 * @param access read or read-write access
 * @param byte_offset byte offset into the buffer
 * @param byte_size bound byte range, or 0 for the remaining buffer range
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_scene_compute_set_buffer(
    DvzSceneCompute* compute, uint32_t binding, DvzSceneBuffer* buffer,
    DvzSceneComputeAccess access, uint64_t byte_offset, uint64_t byte_size);


/**
 * Attach a scene compute pass to a figure.
 *
 * Attached compute passes are emitted before the figure render passes.
 *
 * @param figure the figure
 * @param compute the compute pass
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_figure_add_compute(DvzFigure* figure, DvzSceneCompute* compute);


/**
 * Detach a scene compute pass from a figure.
 *
 * @param figure the figure
 * @param compute the compute pass
 * @return true on success, false on error
 */
DVZ_EXPORT bool dvz_figure_remove_compute(DvzFigure* figure, DvzSceneCompute* compute);


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
 * Replace a visual's index buffer with copied 32-bit index data.
 *
 * This convenience path creates a scene-owned index buffer, copies the index payload into it, and
 * binds it to the visual's `"index"` slot. Use `dvz_scene_buffer()` plus
 * `dvz_visual_set_buffer()` instead when the index buffer must be shared across several visuals.
 *
 * @param visual the primitive or mesh visual
 * @param indices index array
 * @param index_count number of indices
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_visual_set_index_data(DvzVisual* visual, const DvzIndex* indices, uint32_t index_count);


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
 * Return default Phong visual material options.
 *
 * The descriptor uses `DVZ_MATERIAL_MODEL_PHONG` with the same defaults as
 * `dvz_material_desc()`.
 *
 * @return default Phong material descriptor
 */
DVZ_EXPORT DvzMaterialDesc dvz_phong_material_desc(void);


/**
 * Return default standard visual material options.
 *
 * The descriptor uses `DVZ_MATERIAL_MODEL_STANDARD` with opaque alpha, full opacity, a white
 * base-color factor, light direction `(0, 0, 1)`, roughness `0.5`, specular `0.5`, metallic `0`,
 * no emissive contribution, and no rim contribution.
 *
 * @return default standard material descriptor
 */
DVZ_EXPORT DvzMaterialDesc dvz_standard_material_desc(void);


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
 * attribute remains the fill color; `edge_color` and `stroke_width` apply when the aspect is
 * `DVZ_SHAPE_ASPECT_STROKE` or `DVZ_SHAPE_ASPECT_OUTLINE`.
 *
 * @return default point style descriptor
 */
DVZ_EXPORT DvzPointStyleDesc dvz_point_style_desc(void);


/**
 * Configure circular point fill/stroke styling.
 *
 * Pass NULL to restore the default filled/no-stroke style. The aspect is exclusive:
 * `DVZ_SHAPE_ASPECT_FILLED` draws only the fill, `DVZ_SHAPE_ASPECT_STROKE` draws only the edge,
 * and `DVZ_SHAPE_ASPECT_OUTLINE` draws the fill with an edge.
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
 * remains the fill color; `edge_color` and `stroke_width` apply when the aspect is
 * `DVZ_SHAPE_ASPECT_STROKE` or `DVZ_SHAPE_ASPECT_OUTLINE`.
 *
 * @return default marker style descriptor
 */
DVZ_EXPORT DvzMarkerStyle dvz_marker_style(void);


/**
 * Configure marker fill/stroke styling.
 *
 * Pass NULL to restore the default filled/no-stroke style. The aspect is exclusive:
 * `DVZ_SHAPE_ASPECT_FILLED` draws only the fill, `DVZ_SHAPE_ASPECT_STROKE` draws only the edge,
 * and `DVZ_SHAPE_ASPECT_OUTLINE` draws the fill with an edge.
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
 * `diameter` (float, in pixels), and optional `item_state` (uint32_t DvzItemStateKind bitfield).
 * `dvz_point_set_style()` controls optional edge styling with `edge_color`,
 * `stroke_width`, and a filled/stroke/outline aspect.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzScene* scene, uint32_t flags);


/**
 * Create a Gaussian splat visual.
 *
 * Renders one screen-facing Gaussian footprint per item with `position` (vec3), `color` (RGBA8),
 * `sigma` (vec2, screen pixels), and `angle` (float radians). The first implementation uses
 * center depth, depth test on, depth writes off through alpha blending, and no sorting or
 * projected 3D covariance.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_splat(DvzScene* scene, uint32_t flags);


/**
 * Create a pixel visual.
 *
 * Renders screen-space square sprites with `position` (vec3), `color` (RGBA8),
 * `pixel_size` (float, in pixels), and optional `item_state` (uint32_t DvzItemStateKind
 * bitfield). WGSL/WebGPU emission lowers each item to an instanced quad.
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
 * DvzMarkerShape) attributes. Optional `item_state` (uint32_t DvzItemStateKind bitfield)
 * supports retained hover and selection styling.
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
 * Return default vector styling.
 *
 * The default style renders a tail-anchored vector with a triangular end cap. `scale` multiplies
 * each dense `vector` attribute before endpoint derivation.
 *
 * @return default vector style descriptor
 */
DVZ_EXPORT DvzVectorStyle dvz_vector_style(void);


/**
 * Create a vector visual.
 *
 * Straight vector items accept dense `position` (vec3 tail/anchor), `vector` (vec3 displacement),
 * `color` (RGBA8), and `stroke_width` (float pixels). The first native lowering renders each item
 * through the scene segment stroke pipeline with source item identity preserved.
 *
 * Curved mode omits `vector`; `position`, `color`, and `stroke_width` are then interpreted as
 * path points, optionally grouped by dvz_vector_set_subpaths().
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_vector(DvzScene* scene, uint32_t flags);


/**
 * Configure vector styling.
 *
 * The first slice supports visual-wide scale, anchor, endpoint caps, and path join settings.
 * Passing NULL restores the defaults.
 *
 * @param visual the vector visual
 * @param style style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_vector_set_style(DvzVisual* visual, const DvzVectorStyle* style);


/**
 * Set explicit curved-vector subpath lengths for a vector visual.
 *
 * When unset in curved mode, all points belong to one open vector path. Lengths are consumed in
 * order and must sum to the vector visual's path-point count at emission time.
 *
 * @param visual the vector visual
 * @param subpath_count number of subpaths
 * @param lengths point count for each subpath
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_vector_set_subpaths(
    DvzVisual* visual, uint32_t subpath_count, const uint32_t* lengths);


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
 * `texcoords` (vec2), optional `instance_transform` (mat4, one per instance), and optional
 * `"index"` buffer bindings for indexed draws. Binding an RGBA8 2D sampled field to the
 * `"texture"` slot enables the first retained textured-mesh shader path.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags);


/**
 * Upload a CPU geometry object into a mesh visual.
 *
 * Geometry positions, colors, normals, texture coordinates, and optional triangle indices are
 * copied into the visual's retained attribute and index buffers.
 *
 * @param visual the mesh visual
 * @param geometry the CPU geometry object
 * @return 0 on success, -1 on invalid input or upload failure
 */
DVZ_EXPORT int dvz_mesh_set_geometry(DvzVisual* visual, const DvzGeometry* geometry);


/**
 * Create a scene-owned semantic polygon object.
 *
 * A polygon represents one filled region with one outer ring and optional hole rings.
 *
 * @param scene the scene
 * @param flags reserved polygon flags
 * @return the polygon, or NULL on allocation failure
 */
DVZ_EXPORT DvzPolygon* dvz_polygon(DvzScene* scene, uint32_t flags);


/**
 * Destroy a scene-owned polygon object and release its copied ring data.
 *
 * @param polygon the polygon
 */
DVZ_EXPORT void dvz_polygon_destroy(DvzPolygon* polygon);


/**
 * Replace all polygon rings from a borrowed descriptor.
 *
 * @param polygon the polygon
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on invalid input or allocation failure
 */
DVZ_EXPORT int dvz_polygon_set_geometry(DvzPolygon* polygon, const DvzPolygonDesc* desc);


/**
 * Replace the polygon outer ring while preserving existing holes.
 *
 * @param polygon the polygon
 * @param count number of outer ring vertices
 * @param xy borrowed XY vertex array
 * @return 0 on success, -1 on invalid input or allocation failure
 */
DVZ_EXPORT int dvz_polygon_outer(DvzPolygon* polygon, uint32_t count, const dvec2* xy);


/**
 * Append or replace one polygon hole ring.
 *
 * Passing hole_index equal to the current hole count appends a new hole. Passing a smaller index
 * replaces that hole.
 *
 * @param polygon the polygon
 * @param hole_index hole index to replace, or current hole count to append
 * @param count number of hole ring vertices
 * @param xy borrowed XY vertex array
 * @return 0 on success, -1 on invalid input or allocation failure
 */
DVZ_EXPORT int
dvz_polygon_hole(DvzPolygon* polygon, uint32_t hole_index, uint32_t count, const dvec2* xy);


/**
 * Set the polygon fill color.
 *
 * @param polygon the polygon
 * @param color RGBA fill color
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_fill_color(DvzPolygon* polygon, const DvzColor color);


/**
 * Set the polygon stroke color.
 *
 * @param polygon the polygon
 * @param color RGBA stroke color
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_stroke_color(DvzPolygon* polygon, const DvzColor color);


/**
 * Set the polygon stroke width in pixels.
 *
 * @param polygon the polygon
 * @param width stroke width in pixels
 * @return 0 on success, -1 on invalid input
 */
DVZ_EXPORT int dvz_polygon_stroke_width(DvzPolygon* polygon, float width);


/**
 * Create a scene-owned composite render view for a polygon.
 *
 * @param polygon the source polygon
 * @param flags reserved composite flags
 * @return the composite, or NULL on allocation failure
 */
DVZ_EXPORT DvzComposite* dvz_polygon_composite(DvzPolygon* polygon, uint32_t flags);


/**
 * Create a scene-owned semantic polygon set object.
 *
 * A polygon set stores several independent polygon regions. Each region has its own rings and
 * style.
 *
 * @param scene the scene
 * @param flags reserved polygon-set flags
 * @return the polygon set, or NULL on allocation failure
 */
DVZ_EXPORT DvzPolygonSet* dvz_polygon_set(DvzScene* scene, uint32_t flags);


/**
 * Destroy a scene-owned polygon set object.
 *
 * @param set the polygon set
 */
DVZ_EXPORT void dvz_polygon_set_destroy(DvzPolygonSet* set);


/**
 * Append one polygon region to a polygon set.
 *
 * @param set the polygon set
 * @param desc borrowed polygon descriptor
 * @return the polygon index, or UINT32_MAX on error
 */
DVZ_EXPORT uint32_t dvz_polygon_set_add(DvzPolygonSet* set, const DvzPolygonDesc* desc);


/**
 * Replace one polygon region's rings.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_geometry(
    DvzPolygonSet* set, uint32_t polygon_index, const DvzPolygonDesc* desc);


/**
 * Set one polygon region's fill color.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param color RGBA fill color
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_fill_color(
    DvzPolygonSet* set, uint32_t polygon_index, const DvzColor color);


/**
 * Set one polygon region's stroke color.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param color RGBA stroke color
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_stroke_color(
    DvzPolygonSet* set, uint32_t polygon_index, const DvzColor color);


/**
 * Set one polygon region's stroke width in pixels.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param width stroke width in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_set_region_stroke_width(DvzPolygonSet* set, uint32_t polygon_index, float width);


/**
 * Create a scene-owned composite render view for a polygon set.
 *
 * @param set the polygon set
 * @param flags reserved composite flags
 * @return the composite, or NULL on allocation failure
 */
DVZ_EXPORT DvzComposite* dvz_polygon_set_composite(DvzPolygonSet* set, uint32_t flags);


/**
 * Destroy a scene-owned composite render view.
 *
 * @param composite the composite
 */
DVZ_EXPORT void dvz_composite_destroy(DvzComposite* composite);


/**
 * Return the number of generated visuals owned by a composite.
 *
 * @param composite the composite
 * @return generated visual count
 */
DVZ_EXPORT uint32_t dvz_composite_visual_count(const DvzComposite* composite);


/**
 * Return a generated visual by role index.
 *
 * @param composite the composite
 * @param index role index
 * @return the generated visual, or NULL when out of range
 */
DVZ_EXPORT DvzVisual* dvz_composite_visual_at(DvzComposite* composite, uint32_t index);


/**
 * Return a generated visual by role name.
 *
 * Polygon composites currently expose "fill" and "stroke" roles.
 *
 * @param composite the composite
 * @param role role name
 * @return the generated visual, or NULL when absent
 */
DVZ_EXPORT DvzVisual* dvz_composite_visual(DvzComposite* composite, const char* role);


/**
 * Create a path visual.
 *
 * A path accepts `position` (vec3), `color` (RGBA8), and optional per-point `stroke_width`
 * (float, pixels). Without `stroke_width`, paths use the primitive line-strip pipeline. With
 * `stroke_width`, paths use the scene.path screen-space stroke pipeline.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_path(DvzScene* scene, uint32_t flags);


/**
 * Configure caps applied to each open subpath endpoint.
 *
 * Arrow caps, dashes, per-item cap attributes, and closed subpaths are deferred. Supported first
 * slice caps are none, round, triangle-in, triangle-out, square, and butt.
 *
 * @param visual the path visual
 * @param start_cap cap applied to each open subpath start
 * @param end_cap cap applied to each open subpath end
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_path_set_caps(DvzVisual* visual, DvzSegmentCap start_cap, DvzSegmentCap end_cap);


/**
 * Configure the join style and miter limit for stroked path corners.
 *
 * `DVZ_PATH_JOIN_MITER` falls back to bevel when the miter length exceeds `miter_limit` times
 * the local stroke width. Round and bevel joins ignore the limit.
 *
 * @param visual the path visual
 * @param join the path join style
 * @param miter_limit positive finite miter limit
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_path_set_join(DvzVisual* visual, DvzPathJoin join, float miter_limit);


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
 * Create a labels visual.
 *
 * Labels visuals render integer sampled fields with categorical scale metadata. They use the same
 * image placement attributes as image visuals: `position`, `extent`, `anchor`, and `tex_rect`.
 * Bind the integer sampled field with `dvz_visual_set_field(labels, "field", field)` and the
 * categorical scale with `dvz_visual_set_scale(labels, "labels", scale)`.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_labels(DvzScene* scene, uint32_t flags);


/**
 * Set the global opacity multiplier on a labels visual.
 *
 * @param visual the labels visual
 * @param opacity opacity multiplier in [0, 1]
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_set_opacity(DvzVisual* visual, float opacity);


/**
 * Set the transparent background label ID on a labels visual.
 *
 * @param visual the labels visual
 * @param label_id background label ID
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_set_background(DvzVisual* visual, DvzCategoryId label_id);


/**
 * Set the selected label ID on a labels visual.
 *
 * @param visual the labels visual
 * @param label_id selected label ID
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_set_selected(DvzVisual* visual, DvzCategoryId label_id);


/**
 * Clear the selected label ID on a labels visual.
 *
 * @param visual the labels visual
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_clear_selected(DvzVisual* visual);


/**
 * Set the hidden label IDs on a labels visual.
 *
 * @param visual the labels visual
 * @param ids hidden label IDs, or NULL when count is 0
 * @param count hidden label ID count
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_labels_set_hidden(DvzVisual* visual, const DvzCategoryId* ids, uint32_t count);


/**
 * Configure boundary rendering on a labels visual.
 *
 * @param visual the labels visual
 * @param enabled whether boundary rendering is enabled
 * @param width_px boundary width in pixels
 * @param color boundary color
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_labels_set_boundary(DvzVisual* visual, bool enabled, float width_px, DvzColor color);


/**
 * Set the deterministic fallback-color seed on a labels visual.
 *
 * @param visual the labels visual
 * @param seed fallback-color seed
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_set_fallback_seed(DvzVisual* visual, uint32_t seed);


/**
 * Set the first-slice axis for a 3D labels visual.
 *
 * @param visual the labels visual
 * @param axis slice axis
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis);


/**
 * Set the first-slice position for a 3D labels visual.
 *
 * @param visual the labels visual
 * @param position normalized slice position in [0, 1]
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_labels_set_slice_position(DvzVisual* visual, double position);


/**
 * Return the retained labels state for inspection.
 *
 * @param visual the labels visual
 * @return the labels state, or NULL on error
 */
DVZ_EXPORT const DvzLabelsState* dvz_labels_state(const DvzVisual* visual);


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
