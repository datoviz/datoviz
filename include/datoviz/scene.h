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
#include "scene/frame_plan.h"
#include "scene/interaction.h"
#include "scene/panzoom.h"
#include "scene/scale.h"
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
 * Update a figure logical size.
 *
 * @param figure the figure
 * @param width width in logical pixels
 * @param height height in logical pixels
 */
DVZ_EXPORT void dvz_figure_resize(DvzFigure* figure, uint32_t width, uint32_t height);


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
 * Return the panzoom controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned panzoom, or NULL
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel);


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


/**
 * Return the arcball controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned arcball, or NULL
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzPanel* panel);



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
 * Set the visual alpha handling mode.
 *
 * This controls which transparency path the scene planner should use for the visual. Use
 * DVZ_ALPHA_BLENDED for ordinary source-over alpha blending and DVZ_ALPHA_WBOIT for weighted
 * blended order-independent transparency.
 *
 * @param visual the visual
 * @param mode the alpha handling mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_alpha_mode(DvzVisual* visual, DvzAlphaMode mode);


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
 * point: `"position"` (vec3f), `"color"` (RGBA8), `"size"` (float)
 * primitive/path: `"position"` (vec3f), `"color"` (RGBA8)
 * mesh: `"position"` (vec3f), optional `"color"` (RGBA8), optional `"normal"` (vec3f)
 * primitive only: `"normal"` (vec3f)
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
 * Override primitive shading parameters.
 *
 * The current primitive/mesh slice uses these parameters only when a visual also has a bound
 * `normal` attribute. The default light direction is `(0, 0, 1)` with ambient `0.2`
 * and diffuse `0.8`.
 *
 * @param visual the visual
 * @param desc the shading descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int
dvz_visual_set_primitive_shading(DvzVisual* visual, const DvzPrimitiveShadingDesc* desc);



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
 * Create a pixel visual.
 *
 * Renders screen-space square sprites with `position` (vec3), `color` (RGBA8), and
 * `size` (float, in pixels). WGSL/WebGPU emission lowers each item to an instanced quad.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_pixel(DvzScene* scene, uint32_t flags);


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
 * `color` (RGBA8, defaulting to opaque white when omitted), optional `normal` (vec3), and
 * optional `"index"` buffer bindings for indexed draws.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags);


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
 * Bind a sampled field via `dvz_visual_set_field()`. The legacy texture convenience wrappers
 * remain available and lower to scene-owned sampled fields internally. Per-item rectangles,
 * anchors, sizes, and color tinting from `spec/scene/visuals/IMAGE.md` are deferred.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzScene* scene, uint32_t flags);


/**
 * Attach a 2D RGBA8 texture to an image visual.
 *
 * Transitional convenience wrapper: this creates or updates a scene-owned sampled field and
 * binds it to the image visual's `"field"` slot. Prefer `dvz_sampled_field()` plus
 * `dvz_visual_set_field()` in new code.
 *
 * @param visual the visual (must be of type IMAGE)
 * @param rgba RGBA8 pixel data, tightly packed, row-major (`width * height * 4` bytes)
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_texture(
    DvzVisual* visual, const void* rgba, uint32_t width, uint32_t height);


/**
 * Attach a 2D scalar F32 texture to an image visual.
 *
 * Transitional convenience wrapper: this creates or updates a scene-owned sampled field and
 * binds it to the image visual's `"field"` slot. The bound scale and colormap are applied
 * on the CPU during emit to produce the RGBA texture used by the current first-slice image
 * runtime path. Prefer `dvz_sampled_field()` plus `dvz_visual_set_field()` in new code.
 *
 * @param visual the visual (must be of type IMAGE)
 * @param values scalar F32 pixel data, tightly packed, row-major
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_texture_f32(
    DvzVisual* visual, const float* values, uint32_t width, uint32_t height);


EXTERN_C_OFF
