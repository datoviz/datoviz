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
#include "scene/arcball.h"
#include "scene/enums.h"
#include "scene/frame_plan.h"
#include "scene/interaction.h"
#include "scene/panzoom.h"
#include "scene/text.h"
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
 * Destroy a figure.
 *
 * @param figure the figure
 */
DVZ_EXPORT void dvz_figure_destroy(DvzFigure* figure);


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



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 * Create a panel inside a figure.
 *
 * @param figure the figure
 * @param desc panel position and size in normalized [0, 1] figure coordinates
 * @return the panel
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc);


/**
 * Destroy a panel.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);


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
 * Attach an arcball controller to a panel and connect it to an input router.
 *
 * Rotate: left-drag. Double-click: reset.
 *
 * @param panel the panel
 * @param router input router to subscribe to (may be NULL to create without connecting)
 * @param flags DvzArcballFlags bitmask
 */
DVZ_EXPORT void dvz_panel_set_arcball(DvzPanel* panel, DvzInputRouter* router, int flags);



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
 * Write attribute data to a visual.
 *
 * First-slice visual families currently accept:
 * point: `"position"` (vec3f), `"color"` (RGBA8), `"size"` (float)
 * primitive/path: `"position"` (vec3f), `"color"` (RGBA8)
 * image: `"position"` (vec3f), `"texcoords"` (vec2f)
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



/*************************************************************************************************/
/*  Visual family constructors                                                                   */
/*************************************************************************************************/

/**
 * Create a point visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzScene* scene, uint32_t flags);


/**
 * Create a primitive visual.
 *
 * Renders raw GPU primitives (point lists, line lists/strips, triangle lists/strips) with
 * built-in pass-through shaders. Accepts `position` (vec3) and `color` (RGBA8) attributes.
 *
 * @param scene the scene
 * @param topology primitive topology, fixed at construction time
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_primitive(
    DvzScene* scene, DvzPrimitiveTopology topology, uint32_t flags);


/**
 * Create a path visual.
 *
 * First-slice scope: a path is a convenience wrapper over the primitive line-strip
 * pipeline. Accepts `position` (vec3) and `color` (RGBA8) attributes and always uses
 * `DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP`.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_path(DvzScene* scene, uint32_t flags);


/**
 * Create an image visual.
 *
 * First-slice scope: one textured quad per visual. Accepts `position` (vec3, 4 corner
 * vertices in TRIANGLE_STRIP order: TL, BL, TR, BR) and `texcoords` (vec2, matching UVs).
 * Attach the texture bytes via `dvz_visual_set_texture`. Per-item rectangles, anchors,
 * sizes, and color tinting from `spec/scene/visuals/IMAGE.md` are deferred.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzScene* scene, uint32_t flags);


/**
 * Attach a 2D RGBA8 texture to an image visual.
 *
 * The pixel data must remain valid until emit time (borrowed pointer, scene-side ownership
 * matches `dvz_visual_set_data`). One texture per visual; calling again replaces it.
 *
 * @param visual the visual (must be of type IMAGE)
 * @param rgba RGBA8 pixel data, tightly packed, row-major (`width * height * 4` bytes)
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_texture(
    DvzVisual* visual, const void* rgba, uint32_t width, uint32_t height);


EXTERN_C_OFF
