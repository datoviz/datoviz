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

#include <stddef.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/packet.h"
#include "datoviz/geom/types.h"
#include "scene/annotation.h"
#include "scene/animation.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/enums.h"
#include "scene/field.h"
#include "scene/fly.h"
#include "scene/frame_plan.h"
#include "scene/frame_packets.h"
#include "scene/interaction.h"
#include "scene/overlay.h"
#include "scene/panzoom.h"
#include "scene/plot.h"
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
 * Return the scene-local identity of a scene.
 *
 * DvzId is a fixed-width opaque identity. It is stable for the Datoviz object lifetime and is
 * independent from DRP2 ids, backend handles, and adapter protocol ids. The value is not
 * persistent across scene destruction, process restart, serialization, or replay.
 *
 * @param scene the scene
 * @return the scene-local identity, or DVZ_ID_NONE when scene is NULL
 */
DVZ_EXPORT DvzId dvz_scene_id(const DvzScene* scene);


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
 * Return the scene-local identity of a figure.
 *
 * @param figure the figure
 * @return the scene-local identity, or DVZ_ID_NONE when figure is NULL
 */
DVZ_EXPORT DvzId dvz_figure_id(const DvzFigure* figure);


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
 * Convert a host-window logical pointer position to figure layout coordinates.
 *
 * Raw input events keep backend/window logical coordinates. Scene, panel, and query helpers use
 * figure layout coordinates. When a valid host-window size is supplied, this function maps the
 * position by the figure/window size ratio; otherwise it falls back to the supplied content scale
 * values, or identity scale when no scale is available.
 *
 * @param figure the figure
 * @param window_x pointer x position in host-window logical coordinates
 * @param window_y pointer y position in host-window logical coordinates
 * @param window_width logical host-window width, or zero when unknown
 * @param window_height logical host-window height, or zero when unknown
 * @param content_scale_x horizontal content scale fallback
 * @param content_scale_y vertical content scale fallback
 * @param out_x output x position in figure layout coordinates
 * @param out_y output y position in figure layout coordinates
 * @return whether the output coordinates were written
 */
DVZ_EXPORT bool dvz_figure_window_to_layout(
    const DvzFigure* figure, float window_x, float window_y, float window_width,
    float window_height, float content_scale_x, float content_scale_y, float* out_x,
    float* out_y);


/**
 * Set the figure color pipeline used by app/offscreen rendering.
 *
 * The default is DVZ_COLOR_PIPELINE_LINEAR_SRGB. DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND is an
 * opt-in compatibility mode that blends semantic sRGB colors in display space.
 *
 * @param figure the figure
 * @param pipeline the color pipeline
 */
DVZ_EXPORT void dvz_figure_set_color_pipeline(DvzFigure* figure, DvzColorPipeline pipeline);


/**
 * Return the figure color pipeline used by app/offscreen rendering.
 *
 * @param figure the figure
 * @return the figure color pipeline
 */
DVZ_EXPORT DvzColorPipeline dvz_figure_color_pipeline(const DvzFigure* figure);


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
 * Destroy a retained figure-owned grid layout object.
 *
 * Grid-owned panels are detached from the grid and remain valid as free-placement panels at their
 * last resolved descriptors.
 *
 * @param grid the grid
 */
DVZ_EXPORT void dvz_grid_destroy(DvzGrid* grid);


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
 * Destroy a scene-owned controller.
 *
 * Panels borrowing the controller are detached. Controller payload accessors return borrowed
 * handles and must not be destroyed separately.
 *
 * @param controller the controller
 */
DVZ_EXPORT void dvz_controller_destroy(DvzController* controller);


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
 * Emit an immutable frame artifact from a retained figure.
 *
 * The artifact owns the DRP2 command stream snapshot, frozen upload payload bytes, and split packet
 * spans for one frame. Retained scene mutation is legal immediately after successful artifact
 * creation and affects only later artifacts.
 *
 * @param figure the figure
 * @param caps the capability snapshot (nullable — defaults applied if NULL)
 * @param report output diagnostic report (nullable)
 * @param cfg the emission configuration (nullable — defaults applied if NULL)
 * @return an owned frame artifact, or NULL on failure
 */
DVZ_EXPORT DvzSceneFrameArtifact* dvz_figure_emit_frame(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg);


/**
 * Destroy a frame artifact.
 *
 * @param artifact the frame artifact
 */
DVZ_EXPORT void dvz_scene_frame_artifact_destroy(DvzSceneFrameArtifact* artifact);


/**
 * Return the artifact status.
 *
 * @param artifact the frame artifact
 * @return the artifact status
 */
DVZ_EXPORT DvzSceneFrameArtifactStatus dvz_scene_frame_artifact_status(
    const DvzSceneFrameArtifact* artifact);


/**
 * Return the artifact-owned DRP2 command stream snapshot.
 *
 * @param artifact the frame artifact
 * @return a borrowed immutable stream snapshot, or NULL
 */
DVZ_EXPORT const DvzDrp2CommandStream*
dvz_scene_frame_artifact_stream(const DvzSceneFrameArtifact* artifact);


/**
 * Serialize the artifact stream snapshot to DRP2 JSON.
 *
 * The returned string is owned by the caller and should be released with
 * dvz_drp2_stream_json_destroy().
 *
 * @param artifact the frame artifact
 * @param name optional stream name
 * @return an owned JSON string, or NULL
 */
DVZ_EXPORT char* dvz_scene_frame_artifact_json(
    const DvzSceneFrameArtifact* artifact, const char* name);


/**
 * Return the retained resource version associated with an artifact.
 *
 * @param artifact the frame artifact
 * @return the retained resource version
 */
DVZ_EXPORT uint64_t dvz_scene_frame_artifact_resource_version(
    const DvzSceneFrameArtifact* artifact);


/**
 * Return the frame index associated with an artifact.
 *
 * @param artifact the frame artifact
 * @return the frame index
 */
DVZ_EXPORT uint64_t dvz_scene_frame_artifact_frame_index(const DvzSceneFrameArtifact* artifact);


/**
 * Return one encoded packet span and companion payload arena from the frame artifact.
 *
 * Empty phases return true with NULL packet and zero sizes. Returned spans are borrowed from the
 * artifact and remain valid only until artifact destruction.
 *
 * @param artifact the frame artifact
 * @param kind setup, update, or frame
 * @param packet output borrowed packet pointer
 * @param packet_size output packet byte size
 * @param arena output borrowed payload arena pointer
 * @param arena_size output arena byte size
 * @return whether `kind` is valid and outputs were populated
 */
DVZ_EXPORT bool dvz_scene_frame_artifact_get_packet(
    const DvzSceneFrameArtifact* artifact, DvzDrp2PacketKind kind, const void** packet,
    uint64_t* packet_size, const void** arena, uint64_t* arena_size);


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
 * Return the scene-local identity of a panel.
 *
 * @param panel the panel
 * @return the scene-local identity, or DVZ_ID_NONE when panel is NULL
 */
DVZ_EXPORT DvzId dvz_panel_id(const DvzPanel* panel);


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
 * Set a fixed pixel reservation around one panel's plot area.
 *
 * This is an advanced/manual plot-space override in logical pixels. Bare panels are edge-to-edge by
 * default, while attached axes, colorbars, and legends normally reserve their own bands. Pass NULL
 * to reset the manual reserve to zero.
 *
 * @param panel the panel
 * @param reserve pixel reservation descriptor, or NULL for zero reserve
 * @return whether the reservation was accepted
 */
DVZ_EXPORT bool dvz_panel_set_reserve(DvzPanel* panel, const DvzPanelReserve* reserve);


/**
 * Return one panel's current resolved pixel reservation.
 *
 * The returned reserve includes the manual base reserve plus automatic attached adornment reserves.
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
 * the plot rectangle and reserved adornment bands. Use padding for simple inset spacing; attached
 * adornments normally manage reserve bands themselves. Pass NULL to reset every side to zero.
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
 * The inner rectangle is the panel outer rectangle after padding and before resolved reserve,
 * expressed in figure logical pixels.
 *
 * @param panel the panel
 * @param out output inner rectangle in logical pixels
 * @return whether the rectangle was written
 */
DVZ_EXPORT bool dvz_panel_inner_rect_px(const DvzPanel* panel, DvzRect* out);


/**
 * Return one panel's current plot rectangle in figure pixel coordinates.
 *
 * The plot rectangle is the panel outer rectangle after padding and resolved reserve, expressed in
 * figure logical pixels. Panel query positions remain panel-local logical pixels with origin at the
 * outer panel rectangle.
 *
 * @param panel the panel
 * @param out output plot rectangle in logical pixels
 * @return whether the rectangle was written
 */
DVZ_EXPORT bool dvz_panel_plot_rect_px(const DvzPanel* panel, DvzRect* out);


/**
 * Resolve an immutable snapshot of one panel's current frame geometry and coarse revisions.
 *
 * The returned object owns a copy of the resolved frame info and remains immutable until released.
 * It is an inspection contract only: resolving a frame does not emit, render, or mutate visual
 * resources. Later panel, figure, view, or guide edits produce different revision values in later
 * snapshots, but do not alter snapshots already returned to the caller.
 *
 * @param panel the panel
 * @return owned panel frame snapshot, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzPanelFrameSnapshot* dvz_panel_resolve_frame(DvzPanel* panel);


/**
 * Return a snapshot identity.
 *
 * @param snapshot the panel frame snapshot
 * @return scene-local snapshot identity, or DVZ_ID_NONE when snapshot is NULL
 */
DVZ_EXPORT DvzId dvz_panel_frame_id(const DvzPanelFrameSnapshot* snapshot);


/**
 * Copy immutable panel frame information out of a snapshot.
 *
 * @param snapshot the panel frame snapshot
 * @param out output frame information
 * @return whether the information was copied
 */
DVZ_EXPORT bool dvz_panel_frame_info(
    const DvzPanelFrameSnapshot* snapshot, DvzPanelFrameInfo* out);


/**
 * Retain one panel frame snapshot.
 *
 * @param snapshot the panel frame snapshot
 */
DVZ_EXPORT void dvz_panel_frame_ref(DvzPanelFrameSnapshot* snapshot);


/**
 * Release one panel frame snapshot.
 *
 * @param snapshot the panel frame snapshot
 */
DVZ_EXPORT void dvz_panel_frame_unref(DvzPanelFrameSnapshot* snapshot);


/**
 * Convert one 2D point between explicit panel coordinate spaces.
 *
 * Figure, panel, inner, and plot pixel spaces use logical pixels. Panel pixels are local to the
 * outer panel rectangle and match `dvz_panel_query()` coordinates. DATA coordinates use the
 * current visible data domain. VIEW coordinates are the visual coordinates used by visuals attached
 * with `DVZ_COORD_VIEW`.
 *
 * @param panel the panel
 * @param from source coordinate space
 * @param to destination coordinate space
 * @param in input point
 * @param out output point
 * @return whether the conversion was supported and finite
 */
DVZ_EXPORT bool dvz_panel_transform_point(
    DvzPanel* panel, DvzPanelCoordSpace from, DvzPanelCoordSpace to, const double in[2],
    double out[2]);


/**
 * Convert a position in a pixel or VIEW coordinate space to panel DATA coordinates.
 *
 * @param panel the panel
 * @param from source coordinate space
 * @param in input point
 * @param out_data output data point
 * @return whether the conversion was supported and finite
 */
DVZ_EXPORT bool dvz_panel_position_to_data(
    DvzPanel* panel, DvzPanelCoordSpace from, const double in[2], double out_data[2]);


/**
 * Convert a panel DATA point to a position in another panel coordinate space.
 *
 * @param panel the panel
 * @param to destination coordinate space
 * @param data input data point
 * @param out output point
 * @return whether the conversion was supported and finite
 */
DVZ_EXPORT bool dvz_panel_data_to_position(
    DvzPanel* panel, DvzPanelCoordSpace to, const double data[2], double out[2]);


/**
 * Queue a query at a DATA-coordinate point.
 *
 * @param panel the panel
 * @param x data x coordinate
 * @param y data y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_panel_query_data(
    DvzPanel* panel, double x, double y, const DvzQueryRequest* request);


/**
 * Return the default panel-local placement descriptor.
 *
 * The default anchors a zero-sized widget to the panel top-left corner with no offset. Callers
 * normally override anchors, size, and offsets before resolving.
 *
 * @return default placement descriptor
 */
DVZ_EXPORT DvzPlacement dvz_placement(void);


/**
 * Return a fixed-size panel-corner placement descriptor.
 *
 * @param horizontal horizontal panel anchor
 * @param vertical vertical panel anchor
 * @param width_px widget width in logical pixels
 * @param height_px widget height in logical pixels
 * @param offset_x_px horizontal offset from the anchor in logical pixels
 * @param offset_y_px vertical offset from the anchor in logical pixels
 * @return placement descriptor
 */
DVZ_EXPORT DvzPlacement dvz_placement_panel_corner(
    DvzHorizontalAnchor horizontal, DvzVerticalAnchor vertical, float width_px, float height_px,
    float offset_x_px, float offset_y_px);


/**
 * Resolve a placement to a panel-local rectangle.
 *
 * `panel_rect` and `figure_rect` are expressed in logical figure pixels. Panel-space placements
 * resolve inside the panel; figure-space placements resolve inside the figure and are returned in
 * the panel's local coordinate system.
 *
 * @param placement placement descriptor
 * @param panel_rect panel rectangle in figure pixels
 * @param figure_rect figure rectangle in figure pixels, or NULL to use the panel rectangle
 * @param out output panel-local rectangle
 * @return whether the placement could be resolved
 */
DVZ_EXPORT bool dvz_placement_resolve(
    const DvzPlacement* placement, const DvzRect* panel_rect, const DvzRect* figure_rect,
    DvzRect* out);


/*************************************************************************************************/
/*  Orientation gizmo                                                                            */
/*************************************************************************************************/

/**
 * Return the default orientation-gizmo descriptor.
 *
 * @return default orientation-gizmo descriptor
 */
DVZ_EXPORT DvzOrientationGizmoDesc dvz_orientation_gizmo_desc(void);


/**
 * Create a passive orientation gizmo attached to one source panel.
 *
 * The gizmo observes the source panel's effective rendered orientation and displays it as a
 * fixed-size inset triad with lit mesh arrows, a central hub, and orientation rings.
 *
 * @param panel source panel
 * @param desc descriptor, or NULL for defaults
 * @return the orientation gizmo, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzOrientationGizmo* dvz_orientation_gizmo(
    DvzPanel* panel, const DvzOrientationGizmoDesc* desc);


/**
 * Destroy an orientation gizmo.
 *
 * @param gizmo the orientation gizmo
 */
DVZ_EXPORT void dvz_orientation_gizmo_destroy(DvzOrientationGizmo* gizmo);


/**
 * Set orientation-gizmo visibility.
 *
 * @param gizmo the orientation gizmo
 * @param visible whether the gizmo should be visible
 */
DVZ_EXPORT void dvz_orientation_gizmo_set_visible(DvzOrientationGizmo* gizmo, bool visible);


/*************************************************************************************************/
/*  Reference grid                                                                               */
/*************************************************************************************************/

/**
 * Return the default reference-grid descriptor.
 *
 * @return default reference-grid descriptor
 */
DVZ_EXPORT DvzReferenceGridDesc dvz_reference_grid_desc(void);


/**
 * Create a retained plane-oriented reference grid attached to one panel.
 *
 * The helper lowers to a retained segment visual and does not introduce a renderer path.
 *
 * @param panel panel receiving the grid
 * @param desc descriptor, or NULL for defaults
 * @return the reference grid, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzReferenceGrid* dvz_reference_grid(
    DvzPanel* panel, const DvzReferenceGridDesc* desc);


/**
 * Destroy a reference grid.
 *
 * @param grid the reference grid
 */
DVZ_EXPORT void dvz_reference_grid_destroy(DvzReferenceGrid* grid);


/**
 * Set reference-grid visibility.
 *
 * @param grid the reference grid
 * @param visible whether the grid should be visible
 */
DVZ_EXPORT void dvz_reference_grid_set_visible(DvzReferenceGrid* grid, bool visible);


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
 * Return the scene-local identity of a controller.
 *
 * @param controller the controller
 * @return the scene-local identity, or DVZ_ID_NONE when controller is NULL or destroyed
 */
DVZ_EXPORT DvzId dvz_controller_id(const DvzController* controller);


/**
 * Return the default visual attachment descriptor.
 *
 * @return default visual attachment descriptor
 */
DVZ_EXPORT DvzVisualAttachDesc dvz_visual_attach_desc(void);


/**
 * Return the default future visual transform descriptor.
 *
 * v0.4 accepts only `DVZ_VISUAL_TRANSFORM_NONE` through the descriptor path. Use
 * dvz_visual_set_transform() for the supported affine visual-local transform.
 *
 * @return default visual transform descriptor
 */
DVZ_EXPORT DvzVisualTransformDesc dvz_visual_transform_desc(void);


/**
 * Return the default future visual shader descriptor.
 *
 * v0.4 accepts only `DVZ_VISUAL_SHADER_NONE`; custom visual families and built-in shader
 * replacement are reserved for future releases.
 *
 * @return default visual shader descriptor
 */
DVZ_EXPORT DvzVisualShaderDesc dvz_visual_shader_desc(void);


/**
 * Add a visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 * @param desc per-visual attachment options (z_layer, controller_mode, coord_space, clip_rect,
 *             viewport_rect); pass NULL for defaults (z_layer=0,
 *             controller_mode=DVZ_CONTROLLER_APPLY, coord_space=DVZ_COORD_DATA,
 *             clip_rect=DVZ_VISUAL_CLIP_AUTO, viewport_rect=DVZ_VISUAL_VIEWPORT_AUTO)
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
 * Return the default panel background descriptor.
 *
 * @return default panel background descriptor
 */
DVZ_EXPORT DvzPanelBackgroundDesc dvz_panel_background_desc(void);


/**
 * Set or update a panel background.
 *
 * Backgrounds are rendered as a fixed full-panel visual behind regular visuals. Passing NULL or
 * a descriptor with type DVZ_PANEL_BACKGROUND_NONE clears the current background. Linear gradients
 * use panel-local start and end points in [0, 1]. Background color and gradient float components
 * are display/sRGB-authored semantic colors; RGB is linearized before rendering arithmetic and
 * alpha remains linear. Image backgrounds accept tightly packed RGBA8 sRGB color pixels and stretch
 * them to the panel rectangle.
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
 * @param panel the panel
 * @param color RGBA8 background color
 */
DVZ_EXPORT void dvz_panel_set_background_color(DvzPanel* panel, DvzColor color);


/**
 * Return the default panel border descriptor.
 *
 * The default is a visible one-pixel border inset by half a pixel so it is fully inside the panel.
 *
 * @return default panel border descriptor
 */
DVZ_EXPORT DvzPanelBorderDesc dvz_panel_border_desc(void);


/**
 * Set, update, or clear a fixed panel border.
 *
 * Borders are panel chrome: they are rendered as a fixed screen-space overlay, do not affect plot
 * layout, and do not reserve space. Passing NULL, visible=false, or width_px=0 clears the border.
 *
 * @param panel the panel
 * @param border border descriptor, or NULL to clear
 * @return whether the border was updated
 */
DVZ_EXPORT bool dvz_panel_set_border(DvzPanel* panel, const DvzPanelBorderDesc* border);


/**
 * Clear a panel border.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_clear_border(DvzPanel* panel);


/**
 * Return default Eye-Dome Lighting options.
 *
 * @return EDL descriptor
 */
DVZ_EXPORT DvzEdlDesc dvz_edl_desc(void);


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
 * Return default screen-space ambient occlusion options.
 *
 * @return SSAO descriptor
 */
DVZ_EXPORT DvzSsaoDesc dvz_ssao_desc(void);


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
 * Return default volume-occlusion options.
 *
 * @return volume-occlusion descriptor
 */
DVZ_EXPORT DvzVolumeOcclusionDesc dvz_volume_occlusion_desc(void);


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
 * Return default scene-occlusion options.
 *
 * @return scene-occlusion descriptor
 */
DVZ_EXPORT DvzSceneOcclusionDesc dvz_scene_occlusion_desc(void);


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
 * The first WIP axis slice supports finite linear X/Y domains. Domain endpoints are ordered, not
 * sorted: reversed domains such as `min=10, max=0` are legal and reverse the data-to-visual
 * mapping. Axis geometry is derived from this domain and the panel panzoom extent during frame
 * emission.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param min data minimum
 * @param max data maximum
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_set_domain(DvzPanel* panel, DvzDim dim, double min, double max);


/**
 * Return the default panel 2D view descriptor.
 *
 * @return panel 2D view descriptor
 */
DVZ_EXPORT DvzPanelView2D dvz_panel_view2d(void);


/**
 * Return the default revisioned panel 2D view descriptor.
 *
 * The descriptor owns both fitting policy and optional ordered DATA-domain endpoints. Reversed
 * domains are legal and preserved.
 *
 * @return panel 2D view descriptor
 */
DVZ_EXPORT DvzPanelView2DDesc dvz_panel_view2d_desc(void);


/**
 * Set a panel 2D view policy.
 *
 * The panel view owns fitting, aspect, and padding policy for the panel's source data domains.
 * With DVZ_PANEL_VIEW2D_ASPECT_EQUAL, VIEW and DATA coordinates preserve equal X/Y screen scale
 * under the current plot rectangle. Set source limits with `dvz_panel_set_domain()`.
 *
 * @param panel the panel
 * @param view panel 2D view descriptor; NULL clears the view policy
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_set_view2d(DvzPanel* panel, const DvzPanelView2D* view);


/**
 * Set a revisioned panel 2D view descriptor.
 *
 * Passing NULL clears the active 2D view. Explicit descriptor domains are copied into the retained
 * view and synchronized to panel axes for compatibility.
 *
 * @param panel the panel
 * @param desc panel 2D view descriptor, or NULL to clear
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_panel_set_view2d_desc(DvzPanel* panel, const DvzPanelView2DDesc* desc);


/**
 * Clear a panel 2D view policy without changing the current axis domains.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_clear_view2d(DvzPanel* panel);

/**
 * Return the current resolved panel VIEW extent before panzoom.
 *
 * @param panel the panel
 * @param out output extent as xmin, xmax, ymin, ymax
 * @return whether the extent was written
 */
DVZ_EXPORT bool dvz_panel_view2d_extent(DvzPanel* panel, float out[4]);


/**
 * Copy the current canonical 2D panel view state.
 *
 * @param panel the panel
 * @param out output 2D view state
 * @return whether the state was copied
 */
DVZ_EXPORT bool dvz_panel_view2d_state(const DvzPanel* panel, DvzPanelView2DState* out);


/**
 * Return the default revisioned panel 3D view descriptor.
 *
 * @return panel 3D view descriptor
 */
DVZ_EXPORT DvzPanelView3DDesc dvz_panel_view3d_desc(void);


/**
 * Set a revisioned panel 3D view descriptor.
 *
 * Passing NULL clears the panel-owned camera.
 *
 * @param panel the panel
 * @param desc panel 3D view descriptor, or NULL to clear
 * @return 0 on success, -1 on validation/allocation error
 */
DVZ_EXPORT int dvz_panel_set_view3d_desc(DvzPanel* panel, const DvzPanelView3DDesc* desc);


/**
 * Copy the current canonical 3D panel view state.
 *
 * @param panel the panel
 * @param out output 3D view state
 * @return whether the state was copied
 */
DVZ_EXPORT bool dvz_panel_view3d_state(DvzPanel* panel, DvzPanelView3DState* out);


/**
 * Return the current visible data domain for one panel dimension.
 *
 * The panel's data-to-view policy is combined with the current panzoom extent. The returned
 * endpoints preserve the active domain orientation, so reversed domains return reversed visible
 * endpoints. When no explicit domain has been configured, the default visual domain [-1, +1] is
 * used. Use this for data/pixel conversion; `dvz_panel_transform_point()` wraps the common point
 * conversions.
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
 * Return a panel-owned axis, creating its WIP geometry visual on first use.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @return the panel-owned axis, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzAxis* dvz_panel_axis(DvzPanel* panel, DvzDim dim);


/**
 * Return the default 2D panel axes descriptor.
 *
 * The descriptor enables grid lines in the default X/Y axis styles and leaves labels empty.
 *
 * @return default 2D axes descriptor
 */
DVZ_EXPORT DvzPanelAxes2DDesc dvz_panel_axes_2d_desc(void);


/**
 * Apply common 2D axes defaults to a panel-owned X/Y axis pair.
 *
 * This helper fetches the panel-owned X/Y axes, applies one tick policy to both axes, applies the
 * independent X/Y styles, sets labels, and makes both axes visible. Passing NULL uses
 * dvz_panel_axes_2d_desc().
 *
 * @param panel the panel
 * @param desc axes descriptor, or NULL for defaults
 * @return whether the axes were updated
 */
DVZ_EXPORT bool dvz_panel_set_axes_2d(DvzPanel* panel, const DvzPanelAxes2DDesc* desc);


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
 * Set explicit tick positions and optional labels for one panel-owned axis.
 *
 * Explicit tick values are in panel data coordinates and are rendered in the supplied order. When
 * labels are supplied, Datoviz copies them before this function returns. Passing an empty tick
 * record is valid and renders no ticks, tick labels, or grid lines until explicit ticks are cleared.
 *
 * @param axis the axis
 * @param ticks explicit tick descriptor
 * @return whether the explicit ticks were stored
 */
DVZ_EXPORT bool dvz_axis_set_ticks(DvzAxis* axis, const DvzAxisTicks* ticks);


/**
 * Clear explicit tick positions and labels for one panel-owned axis.
 *
 * The axis returns to its automatic tick policy after this call.
 *
 * @param axis the axis
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_clear_ticks(DvzAxis* axis);


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


/**
 * Attach numeric units to one panel-owned axis.
 *
 * Tick positions remain in panel data coordinates; labels are formatted through the units object.
 *
 * @param axis the axis
 * @param units units object, or NULL to restore plain numeric formatting
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_units(DvzAxis* axis, DvzUnits* units);


/**
 * Attach an absolute datetime formatter to one panel-owned axis.
 *
 * @param axis the axis
 * @param format datetime format, or NULL to restore numeric/unit formatting
 * @return whether the axis was updated
 */
DVZ_EXPORT bool dvz_axis_set_datetime(DvzAxis* axis, DvzDateTimeFormat* format);


/**
 * Map compact data coordinates to an absolute datetime interval.
 *
 * @param axis the axis
 * @param data0 first data coordinate
 * @param data1 second data coordinate
 * @param t0 timestamp corresponding to data0, in microseconds since Unix epoch UTC
 * @param t1 timestamp corresponding to data1, in microseconds since Unix epoch UTC
 * @return whether the mapping was updated
 */
DVZ_EXPORT bool dvz_axis_set_datetime_range(
    DvzAxis* axis, double data0, double data1, DvzTimestamp t0, DvzTimestamp t1);


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
 * Return the scene-local identity of a visual.
 *
 * @param visual the visual
 * @return the scene-local identity, or DVZ_ID_NONE when visual is NULL
 */
DVZ_EXPORT DvzId dvz_visual_id(const DvzVisual* visual);


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
 * Set the future scene-managed visual transform descriptor.
 *
 * v0.4 accepts only NULL or `DVZ_VISUAL_TRANSFORM_NONE`. Scene-managed nonlinear/custom transforms
 * are reserved for future releases and return -1.
 *
 * @param visual the visual
 * @param desc transform descriptor, or NULL to clear the future transform slot
 * @return 0 on success, -1 on validation error or unsupported transform kind
 */
DVZ_EXPORT int
dvz_visual_set_transform_desc(DvzVisual* visual, const DvzVisualTransformDesc* desc);


/**
 * Set the future visual shader descriptor.
 *
 * v0.4 accepts only NULL or `DVZ_VISUAL_SHADER_NONE`. Custom visual families and built-in shader
 * replacement are reserved for future releases and return -1.
 *
 * @param visual the visual
 * @param desc shader descriptor, or NULL to clear the future shader slot
 * @return 0 on success, -1 on validation error or unsupported shader kind
 */
DVZ_EXPORT int dvz_visual_set_shader_desc(DvzVisual* visual, const DvzVisualShaderDesc* desc);


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
 * Return whether a visual has a retained local transform.
 *
 * @param visual the visual
 * @return whether a non-default local transform is retained
 */
DVZ_EXPORT bool dvz_visual_has_transform(const DvzVisual* visual);


/**
 * Copy the retained visual-local transform.
 *
 * When no transform is retained, this writes identity to `out`.
 *
 * @param visual the visual
 * @param out output local model transform
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_get_transform(const DvzVisual* visual, mat4 out);


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
 * Declare the storage format for a visual attribute.
 *
 * The format changes the payload type accepted by `dvz_visual_set_data()` for that attribute.
 * It must be set before dense data or an external buffer is attached. Missing attributes use the
 * family default format; point and pixel `"color"` default to `DVZ_VISUAL_ATTR_FORMAT_RGBA_U8`.
 *
 * `DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32` is currently defined for point and pixel `"color"` and
 * requires a continuous scale bound to the semantic `"color"` slot.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param format requested attribute storage format
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_attr_format(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrFormat format);


/**
 * Return the effective storage format for a visual attribute.
 *
 * Missing attributes return the family default format.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @return the effective attribute storage format
 */
DVZ_EXPORT DvzVisualAttrFormat
dvz_visual_attr_format(const DvzVisual* visual, const char* attr_name);


/**
 * Write attribute data to a visual.
 *
 * First-slice visual families currently accept:
 *
 * | Visual family | Attributes |
 * | --- | --- |
 * | point | `"position"` (vec3f), `"color"` (RGBA8 or scalar float with a color scale), `"diameter_px"` (float pixels), optional `"item_state"` (uint32_t DvzItemStateKind bitfield) |
 * | splat | `"position"` (vec3f), `"color"` (RGBA8), `"sigma"` (vec2f pixels), `"angle"` (float radians) |
 * | pixel | `"position"` (vec3f), `"color"` (RGBA8 or scalar float with a color scale), `"pixel_size_px"` (float pixels), optional `"item_state"` (uint32_t DvzItemStateKind bitfield) |
 * | marker | `"position"` (vec3f), `"color"` (RGBA8), `"diameter_px"` (float pixels), `"angle"` (float radians), `"shape"` (uint32_t DvzMarkerShape), optional `"item_state"` (uint32_t DvzItemStateKind bitfield) |
 * | sphere | `"position"` (vec3f), `"color"` (RGBA8), `"radius"` (float scene units) |
 * | segment | `"position_start"` (vec3f), `"position_end"` (vec3f), `"color"` (RGBA8), `"stroke_width_px"` (float pixels) |
 * | primitive | `"position"` (vec3f), `"color"` (RGBA8), primitive-only `"normal"` (vec3f) |
 * | path | `"position"` (vec3f), `"color"` (RGBA8), optional `"stroke_width_px"` (float pixels) |
 * | mesh | `"position"` (vec3f), optional `"color"` (RGBA8), optional `"normal"` (vec3f), optional `"texcoords"` (vec2f), optional `"instance_transform"` (mat4f, one per instance) |
 * | image | legacy `"position"` (vec3f) + `"texcoords"` (vec2f) corner vertices, or per-item `"position"` (vec3f) + `"extent"` (vec2f) with optional `"tex_rect"` (vec4f) and `"anchor"` (vec2f) |
 * | text | string attribute `"text"` plus per-string `"position"` (vec3f pixels), optional `"anchor"` (vec2f), `"size"` (float pixels), `"color"` (RGBA8), `"angle"` (float radians, positive counter-clockwise in rendered y-up coordinates) |
 * | glyph | `"position"` (vec3f anchor), `"bounds"` (vec4f local pixel bounds), `"texcoords"` (vec4f atlas UV bounds), `"color"` (RGBA8), `"angle"` (float radians, positive counter-clockwise in rendered y-up coordinates) |
 *
 * All configured attributes on one visual must use the same item_count. Retained mutation is legal
 * after frame artifact creation; changes are reflected only in later artifacts.
 *
 * @param visual the visual
 * @param attr_name attribute name (family-specific, e.g. "position", "color")
 * The payload is copied before this function returns. The caller keeps ownership of `data` and may
 * release or reuse it immediately after a successful or failed call.
 *
 * @param data packed data array borrowed for the duration of the call
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
 * Set the active logical item range for a retained visual.
 *
 * The active range is expressed in logical item units:
 * [first_item, first_item + item_count). Changing it affects draw/query/export contribution
 * planning only; it does not upload or rewrite attribute buffers. The point visual is the first
 * supported family for this v0.4 slice.
 *
 * @param visual the visual
 * @param first_item first logical item in the active range
 * @param item_count number of logical items in the active range; zero is valid
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_visual_set_item_range(DvzVisual* visual, uint32_t first_item, uint32_t item_count);


/**
 * Clear the active logical item range and restore full visual participation.
 *
 * @param visual the visual
 */
DVZ_EXPORT void dvz_visual_clear_item_range(DvzVisual* visual);


/**
 * Return the effective logical item range for a retained visual.
 *
 * When no explicit range is active, this returns the full effective range. This first API does not
 * distinguish an explicitly full active range from a cleared range.
 *
 * @param visual the visual
 * @param out output item range
 * @return true on success, false on invalid input or unsupported visual state
 */
DVZ_EXPORT bool dvz_visual_get_item_range(const DvzVisual* visual, DvzItemRange* out);


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
 * Retained mutation is legal after frame artifact creation; changes are reflected only in later
 * artifacts.
 *
 * @param visual the visual
 * Every payload referenced by `updates` is copied before this function returns. The caller keeps
 * ownership of update descriptors and payload pointers and may release or reuse them immediately
 * after a successful or failed call.
 *
 * @param updates attribute update descriptors borrowed for the duration of the call
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
 * [first_item, first_item + item_count) are uploaded on the next emit. Retained mutation is legal
 * after frame artifact creation; changes are reflected only in later artifacts.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * The payload is copied before this function returns. The caller keeps ownership of `data` and may
 * release or reuse it immediately after a successful or failed call.
 *
 * @param data packed array of item_count items borrowed for the duration of the call
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
 * Return the default scene buffer descriptor.
 *
 * @return default scene buffer descriptor
 */
DVZ_EXPORT DvzSceneBufferDesc dvz_scene_buffer_desc(void);


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
 * The payload is copied before this function returns. The caller keeps ownership of `data` and may
 * release or reuse it immediately after a successful or failed call.
 *
 * @param data the packed byte payload borrowed for the duration of the call
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
DVZ_EXPORT const DvzSceneBufferDesc* dvz_scene_buffer_get_desc(const DvzSceneBuffer* buffer);


/**
 * Return the retained scene resource key for a scene buffer.
 *
 * The key is stable for the buffer lifetime and is attached as a DRP2 stream label when a figure is
 * emitted. Advanced runtimes can combine this with `dvz_drp2_stream_label_id()` to register a live
 * external buffer without scanning draw commands.
 *
 * @param buffer the scene buffer
 * @param out output string buffer
 * @param out_size output string capacity
 * @return whether the resource key was written
 */
DVZ_EXPORT bool
dvz_scene_buffer_resource_key(const DvzSceneBuffer* buffer, char* out, size_t out_size);


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
 * Return the default scene compute descriptor.
 *
 * @return default scene compute descriptor
 */
DVZ_EXPORT DvzSceneComputeDesc dvz_scene_compute_desc(void);


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
 * factor, light direction `(-0.45, 0.35, 0.82)`, ambient `0.24`, diffuse `0.82`, specular
 * `0.24`, and shininess `26`.
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
 * base-color factor, light direction `(-0.45, 0.35, 0.82)`, roughness `0.62`, specular `0.34`,
 * metallic `0`, no emissive contribution, and rim contribution `0.10`.
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
 * Return default depth-cue options.
 *
 * @return depth-cue descriptor
 */
DVZ_EXPORT DvzDepthCueDesc dvz_depth_cue_desc(void);


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
 * attribute remains the fill color; `edge_color` and `stroke_width_px` apply when the aspect is
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
 * Create a scene-owned reusable symbol set.
 *
 * Built-in code-SDF symbols are marker-renderable today. Bitmap, SDF, and MSDF source payloads are
 * retained by the same object model; SVG-path import and atlas-backed marker rendering are later
 * parity slices.
 *
 * @param scene the scene
 * @param flags reserved flags
 * @return the symbol set, or NULL on error
 */
DVZ_EXPORT DvzSymbolSet* dvz_symbol_set(DvzScene* scene, uint32_t flags);


/**
 * Register or return a built-in symbol id in one symbol set.
 *
 * Built-in symbol ids are stable within the set and currently match the corresponding
 * `DvzMarkerShape` values so marker `shape` and `symbol` data can share one retained path.
 *
 * @param symbols the symbol set
 * @param builtin the built-in symbol
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DVZ_EXPORT DvzSymbolId dvz_symbol_builtin(DvzSymbolSet* symbols, DvzSymbolBuiltin builtin);


/**
 * Return default image-backed symbol source options.
 *
 * The default row stride is tightly packed. `distance_range_px` is used by SDF/MSDF sources and is
 * ignored for bitmap sources.
 *
 * @return default symbol image descriptor
 */
DVZ_EXPORT DvzSymbolImageDesc dvz_symbol_image_desc(void);


/**
 * Register an RGBA bitmap symbol source in one symbol set.
 *
 * The payload is copied into scene-owned storage. Homogeneous bitmap marker symbol arrays render
 * through a scene-owned atlas texture.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param rgba RGBA8 payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DVZ_EXPORT DvzSymbolId dvz_symbol_bitmap(
    DvzSymbolSet* symbols, const char* name, const void* rgba, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc);


/**
 * Register a single-channel SDF symbol source in one symbol set.
 *
 * The payload is copied into scene-owned storage. `desc->distance_range_px` describes the encoded
 * distance-field range in source pixels when nonzero.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param sdf R8 SDF payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DVZ_EXPORT DvzSymbolId dvz_symbol_sdf(
    DvzSymbolSet* symbols, const char* name, const void* sdf, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc);


/**
 * Register an RGB MSDF symbol source in one symbol set.
 *
 * The payload is copied into scene-owned storage. `desc->distance_range_px` describes the encoded
 * distance-field range in source pixels when nonzero.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param msdf RGB8 MSDF payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DVZ_EXPORT DvzSymbolId dvz_symbol_msdf(
    DvzSymbolSet* symbols, const char* name, const void* msdf, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc);


/**
 * Generate and register an MSDF symbol source from an SVG path string.
 *
 * The generated payload is copied into scene-owned storage through `dvz_symbol_msdf()`.
 * `desc->distance_range_px` controls the generated distance-field pixel range when nonzero. This
 * function returns `DVZ_SYMBOL_ID_INVALID` if Datoviz was built without msdfgen SVG support.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param svg_path SVG path data string
 * @param width generated atlas source width in pixels
 * @param height generated atlas source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DVZ_EXPORT DvzSymbolId dvz_symbol_svg_path(
    DvzSymbolSet* symbols, const char* name, const char* svg_path, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc);


/**
 * Bind a reusable symbol set to a marker visual.
 *
 * Marker visuals accept built-in code-SDF ids and homogeneous bitmap/SDF/MSDF symbol arrays. Mixed
 * built-in/texture-backed or mixed-encoding arrays are rejected for now.
 *
 * @param visual the marker visual
 * @param symbols the symbol set, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_marker_set_symbols(DvzVisual* visual, DvzSymbolSet* symbols);


/**
 * Set every existing marker item to one built-in symbol.
 *
 * This convenience writes the marker's dense `"shape"`/`"symbol"` attribute and requires an
 * existing dense item count, usually from `"position"`.
 *
 * @param visual the marker visual
 * @param builtin the built-in symbol
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_marker_set_symbol(DvzVisual* visual, DvzSymbolBuiltin builtin);


/**
 * Return default marker styling.
 *
 * The default marker style renders filled markers with no stroke. The `color` visual attribute
 * remains the fill color; `edge_color` and `stroke_width_px` apply when the aspect is
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
 * `diameter_px` (float, in pixels), and optional `item_state` (uint32_t DvzItemStateKind bitfield).
 * `dvz_point_set_style()` controls optional edge styling with `edge_color`,
 * `stroke_width_px`, and a filled/stroke/outline aspect.
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
 * `pixel_size_px` (float, in pixels), and optional `item_state` (uint32_t DvzItemStateKind
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
 * `diameter_px` (float in pixels), `angle` (float radians), and `shape`/`symbol` (uint32_t built-in
 * symbol id) attributes. Optional `item_state` (uint32_t DvzItemStateKind bitfield) supports
 * retained hover and selection styling.
 * `position` is the center of the screen-space marker bounding box, `diameter_px` is its width and
 * height, and positive `angle` rotates counter-clockwise in rendered y-up coordinates. Built-in
 * code-SDF shapes include the v0.3 marker vocabulary plus target.
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
 * `color` (RGBA8), and `stroke_width_px` (float width in pixels). Segment caps default to butt at
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
 * `color` (RGBA8), and `stroke_width_px` (float pixels). The first native lowering renders each item
 * through the scene segment stroke pipeline with source item identity preserved.
 *
 * Curved mode omits `vector`; `position`, `color`, and `stroke_width_px` are then interpreted as
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
 * Create a scene-owned semantic graph object.
 *
 * A graph stores user-provided node positions and indexed edges. Layout algorithms are intentionally
 * external to the first public API slice and can update node positions through
 * `dvz_graph_node_positions()`.
 *
 * @param scene the scene
 * @param flags reserved graph flags
 * @return the graph, or NULL on allocation failure
 */
DVZ_EXPORT DvzGraph* dvz_graph(DvzScene* scene, uint32_t flags);


/**
 * Destroy a scene-owned graph object.
 *
 * @param graph the graph
 */
DVZ_EXPORT void dvz_graph_destroy(DvzGraph* graph);


/**
 * Return the default graph edge style descriptor.
 *
 * @return default graph edge style
 */
DVZ_EXPORT DvzGraphEdgeStyle dvz_graph_edge_style(void);


/**
 * Replace the graph node array and reset node style defaults.
 *
 * Existing edges are discarded because their endpoints may no longer be valid. Call
 * `dvz_graph_node_positions()` after this function to set or update semantic node positions.
 *
 * @param graph the graph
 * @param node_count number of nodes
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_graph_node_count(DvzGraph* graph, uint32_t node_count);


/**
 * Update graph node positions without changing node styles or edges.
 *
 * @param graph the graph
 * @param first_node first node index
 * @param node_count number of node positions to update
 * @param positions borrowed node positions
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_graph_node_positions(
    DvzGraph* graph, uint32_t first_node, uint32_t node_count, const dvec3* positions);


/**
 * Replace the graph edge array and reset edge style defaults.
 *
 * Edge endpoints are left at their default values until `dvz_graph_edges()` is called.
 *
 * @param graph the graph
 * @param edge_count number of edges
 * @return 0 on success, -1 on invalid endpoints or allocation failure
 */
DVZ_EXPORT int dvz_graph_edge_count(DvzGraph* graph, uint32_t edge_count);


/**
 * Update graph edge endpoints.
 *
 * Edge endpoints reference node indices in the current graph node array.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edges
 * @param endpoints borrowed packed endpoint array: source0, target0, source1, target1, ...
 * @return 0 on success, -1 on invalid endpoints or allocation failure
 */
DVZ_EXPORT int
dvz_graph_edges(DvzGraph* graph, uint32_t first_edge, uint32_t edge_count,
                const uint32_t* endpoints);


/**
 * Set stable graph node user ids.
 *
 * @param graph the graph
 * @param first_node first node index
 * @param node_count number of nodes
 * @param ids borrowed user-id array
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_graph_node_ids(DvzGraph* graph, uint32_t first_node, uint32_t node_count, const uint64_t* ids);


/**
 * Set stable graph edge user ids.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edges
 * @param ids borrowed user-id array
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_graph_edge_ids(DvzGraph* graph, uint32_t first_edge, uint32_t edge_count,
                   const uint64_t* ids);


/**
 * Configure graph edge rendering.
 *
 * `DVZ_GRAPH_EDGE_MODE_SEGMENT` lowers edges to fast independent segment visuals.
 * `DVZ_GRAPH_EDGE_MODE_PATH` lowers straight edges to high-quality path strokes.
 * `DVZ_GRAPH_EDGE_MODE_BEZIER` lowers edges to tessellated cubic Bezier path strokes.
 *
 * @param graph the graph
 * @param style edge style descriptor
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_graph_set_edge_style(DvzGraph* graph, const DvzGraphEdgeStyle* style);


/**
 * Configure explicit cubic Bezier control points for graph edges.
 *
 * If omitted, Bezier mode derives gentle XY control points from each edge's endpoints.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edge controls to update
 * @param control0 borrowed first control point array
 * @param control1 borrowed second control point array
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_graph_edge_controls(
    DvzGraph* graph, uint32_t first_edge, uint32_t edge_count, const dvec3* control0,
    const dvec3* control1);


/**
 * Set a range of graph node colors.
 *
 * @param graph the graph
 * @param first_node first node index
 * @param node_count number of nodes
 * @param colors borrowed RGBA colors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_graph_node_colors(DvzGraph* graph, uint32_t first_node, uint32_t node_count,
                      const DvzColor* colors);


/**
 * Set a range of graph node sizes in pixels.
 *
 * @param graph the graph
 * @param first_node first node index
 * @param node_count number of nodes
 * @param sizes borrowed node sizes
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_graph_node_sizes(DvzGraph* graph, uint32_t first_node, uint32_t node_count,
                     const float* sizes);


/**
 * Set a range of graph edge colors.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edges
 * @param colors borrowed RGBA colors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_graph_edge_colors(DvzGraph* graph, uint32_t first_edge, uint32_t edge_count,
                      const DvzColor* colors);


/**
 * Set a range of graph edge stroke widths in pixels.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edges
 * @param widths borrowed edge widths
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_graph_edge_widths(DvzGraph* graph, uint32_t first_edge, uint32_t edge_count,
                      const float* widths);


/**
 * Create a scene-owned composite render view for a graph.
 *
 * Graph composites expose `"edges"` and `"nodes"` roles.
 *
 * @param graph the source graph
 * @param flags reserved composite flags
 * @return the composite, or NULL on allocation failure
 */
DVZ_EXPORT DvzComposite* dvz_graph_composite(DvzGraph* graph, uint32_t flags);


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
 * Return the default polygon style descriptor.
 *
 * @return default polygon style
 */
DVZ_EXPORT DvzPolygonStyle dvz_polygon_style(void);


/**
 * Replace all polygon rings from a borrowed descriptor.
 *
 * @param polygon the polygon
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on invalid input or allocation failure
 */
DVZ_EXPORT int dvz_polygon_geometry(DvzPolygon* polygon, const DvzPolygonDesc* desc);


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
 * Set the stable user id associated with a polygon.
 *
 * @param polygon the polygon
 * @param id stable user id
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_id(DvzPolygon* polygon, uint64_t id);


/**
 * Set polygon visibility.
 *
 * @param polygon the polygon
 * @param visible whether the polygon should render
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_visible(DvzPolygon* polygon, bool visible);


/**
 * Apply polygon render style.
 *
 * Flat style setters remain the primary API for common updates. This descriptor is a convenience
 * for applying defaults or several style fields atomically.
 *
 * @param polygon the polygon
 * @param style polygon style descriptor
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_style(DvzPolygon* polygon, const DvzPolygonStyle* style);


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
DVZ_EXPORT int dvz_polygon_stroke_width_px(DvzPolygon* polygon, float width);


/**
 * Configure polygon stroke endpoint caps.
 *
 * @param polygon the polygon
 * @param start_cap cap applied to each ring start
 * @param end_cap cap applied to each ring end
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_stroke_caps(DvzPolygon* polygon, DvzSegmentCap start_cap, DvzSegmentCap end_cap);


/**
 * Configure polygon stroke joins.
 *
 * @param polygon the polygon
 * @param join join style
 * @param miter_limit positive finite miter limit
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_stroke_join(DvzPolygon* polygon, DvzPathJoin join, float miter_limit);


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
 * Set one polygon region's stable user id.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param id stable user id
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_set_region_id(DvzPolygonSet* set, uint32_t polygon_index, uint64_t id);


/**
 * Set a contiguous range of polygon region stable user ids.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param ids borrowed stable user id array
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_ids(
    DvzPolygonSet* set, uint32_t first_polygon, uint32_t polygon_count, const uint64_t* ids);


/**
 * Set one polygon region's visibility.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param visible whether the region should render
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_set_region_visible(DvzPolygonSet* set, uint32_t polygon_index, bool visible);


/**
 * Set a contiguous range of polygon region visibilities.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param visible borrowed visibility array
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_visibilities(
    DvzPolygonSet* set, uint32_t first_polygon, uint32_t polygon_count, const bool* visible);


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
 * Set a contiguous range of polygon region fill colors.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param colors RGBA fill colors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_fill_colors(
    DvzPolygonSet* set, uint32_t first_polygon, uint32_t polygon_count, const DvzColor* colors);


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
 * Set a contiguous range of polygon region stroke colors.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param colors RGBA stroke colors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_stroke_colors(
    DvzPolygonSet* set, uint32_t first_polygon, uint32_t polygon_count, const DvzColor* colors);


/**
 * Set one polygon region's stroke width in pixels.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param width stroke width in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_set_region_stroke_width_px(DvzPolygonSet* set, uint32_t polygon_index, float width);


/**
 * Set a contiguous range of polygon region stroke widths.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param widths stroke widths in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_polygon_set_region_stroke_widths_px(
    DvzPolygonSet* set, uint32_t first_polygon, uint32_t polygon_count, const float* widths);


/**
 * Configure polygon-set stroke endpoint caps.
 *
 * @param set the polygon set
 * @param start_cap cap applied to each ring start
 * @param end_cap cap applied to each ring end
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_set_stroke_caps(DvzPolygonSet* set, DvzSegmentCap start_cap, DvzSegmentCap end_cap);


/**
 * Configure polygon-set stroke joins.
 *
 * @param set the polygon set
 * @param join join style
 * @param miter_limit positive finite miter limit
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_polygon_set_stroke_join(DvzPolygonSet* set, DvzPathJoin join, float miter_limit);


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
 * A path accepts `position` (vec3), `color` (RGBA8), and optional per-point `stroke_width_px`
 * (float, pixels). Without `stroke_width_px`, paths use the primitive line-strip pipeline. With
 * `stroke_width_px`, paths use the scene.path screen-space stroke pipeline.
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
 * Set the sampler filter mode for an image visual.
 *
 * `DVZ_IMAGE_SAMPLING_LINEAR` is the default. `DVZ_IMAGE_SAMPLING_NEAREST` emits a nearest
 * minification and magnification sampler for pixel-exact image rendering.
 *
 * @param visual the image visual
 * @param sampling the image sampler filter mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_image_set_sampling(DvzVisual* visual, DvzImageSampling sampling);


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
 * bound 2D sampled field. Positive `angle` rotates counter-clockwise in rendered y-up coordinates.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_glyph(DvzScene* scene, uint32_t flags);


/**
 * Bind a font atlas to a glyph visual.
 *
 * This configures the atlas sampled field, distance-field encoding, and distance range used by
 * the glyph shader. The atlas remains owned by the font's scene and must outlive the glyph visual.
 *
 * @param visual the glyph visual
 * @param atlas the text atlas
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_glyph_set_atlas(DvzVisual* visual, const DvzTextAtlas* atlas);


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
 * Attach a 2D RGBA8 sRGB-color texture to an image, glyph, or mesh visual.
 *
 * Transitional convenience wrapper: this creates or updates a scene-owned sampled field and
 * binds it to the visual's `"field"` slot for image/glyph visuals or `"texture"` slot for mesh
 * visuals. The owned sampled field uses `DVZ_FIELD_SEMANTIC_COLOR` and
 * `DVZ_COLOR_ROLE_SRGB_COLOR`. Prefer `dvz_sampled_field()` plus `dvz_visual_set_field()` in new
 * code.
 *
 * @param visual the visual (must be of type IMAGE, GLYPH, or MESH)
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
 * binds it to the visual's `"field"` slot. The owned sampled field uses
 * `DVZ_FIELD_SEMANTIC_SCALAR` and `DVZ_COLOR_ROLE_DATA`. The bound scale and colormap are applied
 * on the CPU during emit to produce the RGBA texture used by the current first-slice image runtime
 * path. Prefer `dvz_sampled_field()` plus `dvz_visual_set_field()` in new code.
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
