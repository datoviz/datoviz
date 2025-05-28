/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**************************************************************************************************

 * DATOVIZ PUBLIC API HEADER FILE
 * ==============================
 * 2025-05-28
 * Cyrille Rossant
 * cyrille dot rossant at gmail com

This file exposes the public API of Datoviz, a C/C++ library for high-performance GPU scientific
visualization.

Datoviz is still an early stage library and the API may change at any time.

**************************************************************************************************/



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC
#define DVZ_HEADER_PUBLIC



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "datoviz_app.h"
#include "datoviz_gui.h"
#include "datoviz_keycodes.h"
#include "datoviz_macros.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "datoviz_version.h"
#include "datoviz_visuals.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzArcball DvzArcball;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzAxes DvzAxes;
typedef struct DvzAxis DvzAxis;
typedef struct DvzBatch DvzBatch;
typedef struct DvzBox DvzBox;
typedef struct DvzCamera DvzCamera;
typedef struct DvzFigure DvzFigure;
typedef struct DvzFont DvzFont;
typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzMouse DvzMouse;
typedef struct DvzMVP DvzMVP;
typedef struct DvzOrtho DvzOrtho;
typedef struct DvzPanel DvzPanel;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzParams DvzParams;
typedef struct DvzRef DvzRef;
typedef struct DvzRenderer DvzRenderer;
typedef struct DvzScene DvzScene;
typedef struct DvzServer DvzServer;
typedef struct DvzShape DvzShape;
typedef struct DvzTex DvzTex;
typedef struct DvzTexture DvzTexture;
typedef struct DvzTransform DvzTransform;
typedef struct DvzVisual DvzVisual;



EXTERN_C_ON

/*************************************************************************************************/
/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Run a demo.
 */
DVZ_EXPORT void dvz_demo(void);



/**
 * Demo panel (random scatter plot).
 *
 * @param panel the panel
 * @returns the marker visual
 */
DVZ_EXPORT DvzVisual* dvz_demo_panel_2D(DvzPanel* panel);



/**
 * Demo panel (random scatter plot).
 *
 * @param panel the panel
 * @returns the marker visual
 */
DVZ_EXPORT DvzVisual* dvz_demo_panel_3D(DvzPanel* panel);



/**
 * Return the current version string.
 *
 * @returns the version string
 */
DVZ_EXPORT const char* dvz_version(void);



/**
 * Register an error callback, a C function taking as input a string.
 *
 * @param cb the error callback
 */
DVZ_EXPORT void dvz_error_callback(DvzErrorCallback cb);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Qt                                                                                           */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzQtApp* dvz_qt_app(QApplication* qapp, int flags);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzQtWindow* dvz_qt_window(DvzQtApp* app);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_qt_submit(DvzQtApp* app, DvzBatch* batch);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzBatch* dvz_qt_batch(DvzQtApp* app);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_qt_app_destroy(DvzQtApp* app);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Server                                                                                       */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzServer* dvz_server(int flags);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_server_submit(DvzServer* server, DvzBatch* batch);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzMouse* dvz_server_mouse(DvzServer* server);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzKeyboard* dvz_server_keyboard(DvzServer* server);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void
dvz_server_resize(DvzServer* server, DvzId canvas_id, uint32_t width, uint32_t height);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT uint8_t* dvz_server_grab(DvzServer* server, DvzId canvas_id, int flags);


/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_scene_render(DvzScene* scene, DvzServer* server);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_server_destroy(DvzServer* server);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Scene API                                                                                    */
/*************************************************************************************************/
/*************************************************************************************************/

/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

/**
 * Create a scene.
 *
 * @param batch the batch
 * @returns the scene
 */
DVZ_EXPORT DvzScene* dvz_scene(DvzBatch* batch);



/**
 * Return the batch from a scene.
 *
 * @param scene the scene
 * @returns the batch
 */
DVZ_EXPORT DvzBatch* dvz_scene_batch(DvzScene* scene);



/**
 * Start the event loop and render the scene in a window.
 *
 * @param scene the scene
 * @param app the app
 * @param frame_count the maximum number of frames, 0 for infinite loop
 */
DVZ_EXPORT void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t frame_count);



/**
 * Manually pass a mouse event to the scene.
 *
 * @param scene the scene
 * @param fig the figure
 * @param ev the mouse event
 */
DVZ_EXPORT void dvz_scene_mouse(DvzScene* scene, DvzFigure* fig, DvzMouseEvent* ev);



/**
 * Destroy a scene.
 *
 * @param scene the scene
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

/**
 * Create a mouse object.
 *
 * @returns the mouse
 */
DVZ_EXPORT DvzMouse* dvz_mouse(void);



/**
 * Create a mouse move event.
 *
 * @param mouse the mouse
 * @param pos the cursor position, in pixels
 * @param mods the keyboard modifier flags
 */
DVZ_EXPORT void dvz_mouse_move(DvzMouse* mouse, vec2 pos, int mods);



/**
 * Create a mouse press event.
 *
 * @param mouse the mouse
 * @param button the mouse button (enum int)
 * @param mods the keyboard modifier flags
 */
DVZ_EXPORT void dvz_mouse_press(DvzMouse* mouse, DvzMouseButton button, int mods);



/**
 * Create a mouse release event.
 *
 * @param mouse the mouse
 * @param button the mouse button (enum int)
 * @param mods the keyboard modifier flags
 */
DVZ_EXPORT void dvz_mouse_release(DvzMouse* mouse, DvzMouseButton button, int mods);



/**
 * Create a mouse wheel event.
 *
 * @param mouse the mouse
 * @param dir the mouse wheel direction (x, y)
 * @param mods the keyboard modifier flags
 */
DVZ_EXPORT void dvz_mouse_wheel(DvzMouse* mouse, vec2 dir, int mods);



/**
 * Create a generic mouse event.
 *
 * @param mouse the mouse
 * @param ev the mouse event
 */
DVZ_EXPORT void dvz_mouse_event(DvzMouse* mouse, DvzMouseEvent* ev);



/**
 * Destroy a mouse.
 *
 * @param mouse the mouse
 */
DVZ_EXPORT void dvz_mouse_destroy(DvzMouse* mouse);



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

/**
 * Create a figure, a desktop window with panels and visuals.
 *
 * @param scene the scene
 * @param width the window width
 * @param height the window height
 * @param flags the figure creation flags (not yet stabilized)
 * @returns the figure
 */
DVZ_EXPORT DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, int flags);



/**
 * Return a figure ID.
 *
 * @param figure the figure
 * @returns the figure ID
 */
DVZ_EXPORT DvzId dvz_figure_id(DvzFigure* figure);



/**
 * Resize a figure.
 *
 * @param fig the figure
 * @param width the window width
 * @param height the window height
 */
DVZ_EXPORT void dvz_figure_resize(DvzFigure* fig, uint32_t width, uint32_t height);



/**
 * Get a figure from its id.
 *
 * @param scene the scene
 * @param id the figure id
 * @returns the figure
 */
DVZ_EXPORT DvzFigure* dvz_scene_figure(DvzScene* scene, DvzId id);



/**
 * Update a figure after the composition of the panels and visuals has changed.
 *
 * @param figure the figure
 */
DVZ_EXPORT void dvz_figure_update(DvzFigure* figure);



/**
 * Destroy a figure.
 *
 * @param figure the figure
 */
DVZ_EXPORT void dvz_figure_destroy(DvzFigure* figure);



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 * Create a panel in a figure (partial or complete rectangular portion of a figure).
 *
 * @param fig the figure
 * @param x the x coordinate of the top left corner, in pixels
 * @param y the y coordinate of the top left corner, in pixels
 * @param width the panel width, in pixels
 * @param height the panel height, in pixels
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float width, float height);



/**
 * Set the panel flags
 *
 * @param panel the panel
 * @param flags the panel flags
 */
DVZ_EXPORT void dvz_panel_flags(DvzPanel* panel, int flags);



/**
 * Return the batch from a panel.
 *
 * @param panel the panel
 * @returns the batch
 */
DVZ_EXPORT DvzBatch* dvz_panel_batch(DvzPanel* panel);



/**
 * Set a colored background for a panel.
 *
 * @param panel the panel
 * @param background the colors of the four corners (top-left, top-right, bottom left,
 * bottom-right)
 */
DVZ_EXPORT void dvz_panel_background(DvzPanel* panel, cvec4* background);



/**
 * Get the panel's reference.
 *
 * @param panel the panel
 * @returns the reference
 */
DVZ_EXPORT DvzRef* dvz_panel_ref(DvzPanel* panel);



/**
 * Get the axes.
 *
 * @param panel the panel
 * @returns the axes
 */
DVZ_EXPORT DvzAxes* dvz_panel_axes(DvzPanel* panel);



/**
 * Create 2D axes.
 *
 * @param panel the panel
 * @param xmin xmin
 * @param xmax xmax
 * @param ymin ymin
 * @param ymax ymax
 * @returns the axes
 */
DVZ_EXPORT DvzAxes*
dvz_panel_axes_2D(DvzPanel* panel, double xmin, double xmax, double ymin, double ymax);



/**
 * Return the figure from a panel.
 *
 * @param panel the panel
 * @returns the figure
 */
DVZ_EXPORT DvzFigure* dvz_panel_figure(DvzPanel* panel);



/**
 * Set a panel as a GUI panel.
 *
 * @param panel the panel
 * @param title the GUI dialog title
 * @param flags the GUI dialog flags (unused at the moment)
 */
DVZ_EXPORT void dvz_panel_gui(DvzPanel* panel, const char* title, int flags);



/**
 * Return the default full panel spanning an entire figure.
 *
 * @param fig the figure
 * @returns the panel spanning the entire figure
 */
DVZ_EXPORT DvzPanel* dvz_panel_default(DvzFigure* fig);



/**
 * Assign a transform to a panel.
 *
 * @param panel the panel
 * @param tr the transform
 */
DVZ_EXPORT void dvz_panel_transform(DvzPanel* panel, DvzTransform* tr);



/**
 * Assign a MVP structure to a panel.
 *
 * @param panel the panel
 * @param mvp a pointer to the MVP structure
 */
DVZ_EXPORT void dvz_panel_mvp(DvzPanel* panel, DvzMVP* mvp);



/**
 * Assign the model-view-proj matrices to a panel.
 *
 * @param panel the panel
 * @param model the model matrix
 * @param view the view matrix
 * @param proj the projection matrix
 */
DVZ_EXPORT void dvz_panel_mvpmat(DvzPanel* panel, mat4 model, mat4 view, mat4 proj);



/**
 * Resize a panel.
 *
 * @param panel the panel
 * @param x the x coordinate of the top left corner, in pixels
 * @param y the y coordinate of the top left corner, in pixels
 * @param width the panel width, in pixels
 * @param height the panel height, in pixels
 */
DVZ_EXPORT void dvz_panel_resize(DvzPanel* panel, float x, float y, float width, float height);



/**
 * Set the margins of a panel.
 *
 * @param panel the panel
 * @param top the top margin, in pixels
 * @param right the right margin, in pixels
 * @param bottom the bottom margin, in pixels
 * @param left the left margin, in pixels
 */
DVZ_EXPORT void
dvz_panel_margins(DvzPanel* panel, float top, float right, float bottom, float left);



/**
 * Return whether a point is inside a panel.
 *
 * @param panel the panel
 * @param pos the position
 * @returns true if the position lies within the panel
 */
DVZ_EXPORT bool dvz_panel_contains(DvzPanel* panel, vec2 pos);



/**
 * Return the panel containing a given point.
 *
 * @param figure the figure
 * @param pos the position
 * @returns the panel containing the point, or NULL if there is none
 */
DVZ_EXPORT DvzPanel* dvz_panel_at(DvzFigure* figure, vec2 pos);



/**
 * Set a camera for a panel.
 *
 * @param panel the panel
 * @param flags the camera flags
 * @returns the camera
 */
DVZ_EXPORT DvzCamera* dvz_panel_camera(DvzPanel* panel, int flags);



/**
 * Set panzoom interactivity for a panel.
 *
 * @param panel the panel
 * @param flags the flags
 * @returns the panzoom
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel, int flags);



/**
 * Set ortho interactivity for a panel.
 *
 * @param panel the panel
 * @param flags the flags
 * @returns the ortho
 */
DVZ_EXPORT DvzOrtho* dvz_panel_ortho(DvzPanel* panel, int flags);



/**
 * Set arcball interactivity for a panel.
 *
 * @param panel the panel
 * @param flags the flags
 * @returns the arcball
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzPanel* panel, int flags);



/**
 * Show or hide a panel.
 *
 * @param panel the panel
 * @param is_visible whether to show or hide the panel
 */
DVZ_EXPORT void dvz_panel_show(DvzPanel* panel, bool is_visible);



/**
 * Trigger a panel update.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_update(DvzPanel* panel);



/**
 * Add a visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 * @param flags the flags
 */
DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual, int flags);



/**
 * Remove a visual from a panel.
 *
 * @param panel the panel
 * @param visual the visual
 */
DVZ_EXPORT void dvz_panel_remove(DvzPanel* panel, DvzVisual* visual);



/**
 * Destroy a panel.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Visuals API                                                                                  */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Update a visual after its data has changed.
 *
 * Note: this function is automatically called in the event loop internally, so you should not need
 * to use it in most cases.
 *
 * @param visual the visual
 */
DVZ_EXPORT void dvz_visual_update(DvzVisual* visual);



/**
 * Fix some axes in a visual.
 *
 * @param visual the visual
 * @param fixed_x whether the x axis should be fixed
 * @param fixed_y whether the y axis should be fixed
 * @param fixed_z whether the z axis should be fixed
 */
DVZ_EXPORT void dvz_visual_fixed(DvzVisual* visual, bool fixed_x, bool fixed_y, bool fixed_z);



/**
 * Declare a dynamic attribute, meaning that it is stored in a separate dat rather than being
 * interleaved with the other attributes in the same vertex buffer.
 *
 * @param visual the visual
 * @param attr_idx the attribute index
 * @param binding_idx the binding index (0 = common vertex buffer, use 1 or 2, 3... for each
 * different independent dat)
 */
DVZ_EXPORT void dvz_visual_dynamic(DvzVisual* visual, uint32_t attr_idx, uint32_t binding_idx);



/**
 * Set the visual clipping.
 *
 * @param visual the visual
 * @param clip the viewport clipping
 */
DVZ_EXPORT void dvz_visual_clip(DvzVisual* visual, DvzViewportClip clip);



/**
 * Set the visual depth.
 *
 * @param visual the visual
 * @param depth_test whether to activate the depth test
 */
DVZ_EXPORT void dvz_visual_depth(DvzVisual* visual, DvzDepthTest depth_test);



/**
 * Set the visibility of a visual.
 *
 * @param visual the visual
 * @param is_visible the visual visibility
 */
DVZ_EXPORT void dvz_visual_show(DvzVisual* visual, bool is_visible);



/*************************************************************************************************/
/*  Visual fixed pipeline                                                                        */
/*************************************************************************************************/

/**
 * Set the primitive topology of a visual.
 *
 * @param visual the visual
 * @param primitive the primitive topology
 */
DVZ_EXPORT void dvz_visual_primitive(DvzVisual* visual, DvzPrimitiveTopology primitive);


/**
 * Set the blend type of a visual.
 *
 * @param visual the visual
 * @param blend_type the blend type
 */
DVZ_EXPORT void dvz_visual_blend(DvzVisual* visual, DvzBlendType blend_type);



/**
 * Set the polygon mode of a visual.
 *
 * @param visual the visual
 * @param polygon_mode the polygon mode
 */
DVZ_EXPORT void dvz_visual_polygon(DvzVisual* visual, DvzPolygonMode polygon_mode);



/**
 * Set the cull mode of a visual.
 *
 * @param visual the visual
 * @param cull_mode the cull mode
 */
DVZ_EXPORT void dvz_visual_cull(DvzVisual* visual, DvzCullMode cull_mode);



/**
 * Set the front face mode of a visual.
 *
 * @param visual the visual
 * @param front_face the front face mode
 */
DVZ_EXPORT void dvz_visual_front(DvzVisual* visual, DvzFrontFace front_face);



/**
 * Set a push constant of a visual.
 *
 * @param visual the visual
 * @param shader_stages the shader stage flags
 * @param offset the offset, in bytes
 * @param size the size, in bytes
 */
DVZ_EXPORT void dvz_visual_push(
    DvzVisual* visual, DvzShaderStageFlags shader_stages, DvzSize offset, DvzSize size);



/**
 * Set a specialization constant of a visual.
 *
 * @param visual the visual
 * @param shader the shader type
 * @param idx the specialization constant index
 * @param size the size, in bytes, of the value passed to this function
 * @param value a pointer to the value to use for that specialization constant
 */
DVZ_EXPORT void dvz_visual_specialization(
    DvzVisual* visual, DvzShaderType shader, uint32_t idx, DvzSize size, void* value);



/*************************************************************************************************/
/*  Visual declaration                                                                           */
/*************************************************************************************************/

/**
 * Set the shader SPIR-V code of a visual.
 *
 * @param visual the visual
 * @param type the shader type
 * @param size the size, in bytes, of the SPIR-V buffer
 * @param buffer a pointer to the SPIR-V buffer
 */
DVZ_EXPORT void
dvz_visual_spirv(DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer);



/**
 * Set the shader SPIR-V name of a visual.
 *
 * @param visual the visual
 * @param name the built-in resource name of the shader (_vert and _frag are appended)
 */
DVZ_EXPORT void dvz_visual_shader(DvzVisual* visual, const char* name);



/**
 * Resize a visual allocation.
 *
 * @param visual the visual
 * @param item_count the number of items
 * @param vertex_count the number of vertices
 * @param index_count the number of indices (0 if there is no index buffer)
 */
DVZ_EXPORT void dvz_visual_resize(
    DvzVisual* visual, uint32_t item_count, uint32_t vertex_count, uint32_t index_count);



/**
 * Set groups in a visual.
 *
 * @param visual the visual
 * @param group_count the number of groups
 * @param group_sizes the size of each group
 */
DVZ_EXPORT void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes);



/**
 * Declare a visual attribute.
 *
 * @param visual the visual
 * @param attr_idx the attribute index
 * @param offset the attribute offset within the vertex buffer, in bytes
 * @param item_size the attribute size, in bytes
 * @param format the attribute data format
 * @param flags the attribute flags
 */
DVZ_EXPORT void dvz_visual_attr(
    DvzVisual* visual, uint32_t attr_idx, DvzSize offset, DvzSize item_size, //
    DvzFormat format, int flags);



/**
 * Declare a visual binding.
 *
 * @param visual the visual
 * @param binding_idx the binding index
 * @param stride the binding stride, in bytes
 */
DVZ_EXPORT void dvz_visual_stride(DvzVisual* visual, uint32_t binding_idx, DvzSize stride);



/**
 * Declare a visual slot.
 *
 * @param visual the visual
 * @param slot_idx the slot index
 * @param type the slot type
 */
DVZ_EXPORT void dvz_visual_slot(DvzVisual* visual, uint32_t slot_idx, DvzSlotType type);



/**
 * Declare a set of visual parameters.
 *
 * @param visual the visual
 * @param slot_idx the slot index of the uniform buffer storing the parameter values
 * @param size the size, in bytes, of that uniform buffer
 */
DVZ_EXPORT DvzParams* dvz_visual_params(DvzVisual* visual, uint32_t slot_idx, DvzSize size);



/**
 * Bind a dat to a visual slot.
 *
 * @param visual the visual
 * @param slot_idx the slot index
 * @param dat the dat ID
 */
// TODO: add 'DvzSize offset' ?
DVZ_EXPORT void dvz_visual_dat(DvzVisual* visual, uint32_t slot_idx, DvzId dat);



/**
 * Bind a tex to a visual slot.
 *
 * @param visual the visual
 * @param slot_idx the slot index
 * @param tex the tex ID
 * @param sampler the sampler ID
 * @param offset the texture offset
 */
DVZ_EXPORT void
dvz_visual_tex(DvzVisual* visual, uint32_t slot_idx, DvzId tex, DvzId sampler, uvec3 offset);



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

/**
 * Allocate a visual.
 *
 * @param visual the visual
 * @param item_count the number of items
 * @param vertex_count the number of vertices
 * @param index_count the number of indices
 */
DVZ_EXPORT void dvz_visual_alloc(
    DvzVisual* visual, uint32_t item_count, uint32_t vertex_count, uint32_t index_count);



/**
 * Set a visual transform.
 *
 * @param visual the visual
 * @param tr the transform
 * @param vertex_attr the vertex attribute on which the transform applies to
 */
DVZ_EXPORT void dvz_visual_transform(DvzVisual* visual, DvzTransform* tr, uint32_t vertex_attr);



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

/**
 * Set visual data.
 *
 * @param visual the visual
 * @param attr_idx the attribute index
 * @param first the index of the first item to set
 * @param count the number of items to set
 * @param data a pointer to the data buffer
 */
DVZ_EXPORT void
dvz_visual_data(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



/**
 * Set visual data as quads.
 *
 * @param visual the visual
 * @param attr_idx the attribute index
 * @param first the index of the first item to set
 * @param count the number of items to set
 * @param tl_br a pointer to a buffer of vec4 with the 2D coordinates of the top-left and
 *              bottom-right quad corners
 */
DVZ_EXPORT void dvz_visual_quads(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, vec4* tl_br);



/**
 * Set the visual index data.
 *
 * @param visual the visual
 * @param first the index of the first index to set
 * @param count the number of indices
 * @param data a pointer to a buffer of DvzIndex (uint32_t) values with the indices
 */
DVZ_EXPORT void
dvz_visual_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* data);



/**
 * Set a visual parameter value.
 *
 * @param visual the visual
 * @param slot_idx the slot index
 * @param attr_idx the index of the parameter attribute within the params structure
 * @param item a pointer to the value to use for that parameter
 */
DVZ_EXPORT void
dvz_visual_param(DvzVisual* visual, uint32_t slot_idx, uint32_t attr_idx, void* item);



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

/**
 * Create a texture.
 *
 * @param batch the batch
 * @param dims the number of dimensions in the texture
 * @param flags the texture creation flags
 * @returns the texture
 */
DVZ_EXPORT DvzTexture* dvz_texture(DvzBatch* batch, DvzTexDims dims, int flags);



/**
 * Set the texture shape.
 *
 * @param texture the texture
 * @param width the width
 * @param height the height
 * @param depth the depth
 */
DVZ_EXPORT void
dvz_texture_shape(DvzTexture* texture, uint32_t width, uint32_t height, uint32_t depth);



/**
 * Set the texture format.
 *
 * @param texture the texture
 * @param format the format
 */
DVZ_EXPORT void dvz_texture_format(DvzTexture* texture, DvzFormat format);



/**
 * Set the texture's associated sampler's filter (nearest or linear).
 *
 * @param texture the texture
 * @param filter the filter
 */
DVZ_EXPORT void dvz_texture_filter(DvzTexture* texture, DvzFilter filter);



/**
 * Set the texture's associated sampler's address mode.
 *
 * @param texture the texture
 * @param address_mode the address mode
 */
DVZ_EXPORT void dvz_texture_address_mode(DvzTexture* texture, DvzSamplerAddressMode address_mode);



/**
 * Upload all or part of the the texture data.
 *
 * @param texture the texture
 * @param xoffset the x offset inside the texture
 * @param yoffset the y offset inside the texture
 * @param zoffset the z offset inside the texture
 * @param width the width of the uploaded image
 * @param height the height of the uploaded image
 * @param depth the depth of the uploaded image
 * @param size the size of the data buffer
 * @param data the data buffer
 */
DVZ_EXPORT void dvz_texture_data(
    DvzTexture* texture, uint32_t xoffset, uint32_t yoffset, uint32_t zoffset, //
    uint32_t width, uint32_t height, uint32_t depth, DvzSize size, void* data);



/**
 * Create the texture once set.
 *
 * @param texture the texture
 */
DVZ_EXPORT void dvz_texture_create(DvzTexture* texture);



/**
 * Destroy a texture.
 *
 * @param texture the texture
 */
DVZ_EXPORT void dvz_texture_destroy(DvzTexture* texture);



/**
 * Create a 1D texture.
 *
 * @param batch the batch
 * @param format the texture format
 * @param filter the filter
 * @param address_mode the address mode
 * @param width the texture width
 * @param data the texture data to upload
 * @param flags the texture creation flags
 * @returns the texture
 */
DVZ_EXPORT DvzTexture* dvz_texture_1D(
    DvzBatch* batch, DvzFormat format, DvzFilter filter, DvzSamplerAddressMode address_mode,
    uint32_t width, void* data, int flags);



/**
 * Create a 2D texture to be used in an image visual.
 *
 * @param batch the batch
 * @param format the texture format
 * @param filter the filter
 * @param address_mode the address mode
 * @param width the texture width
 * @param height the texture height
 * @param data the texture data to upload
 * @param flags the texture creation flags
 * @returns the texture
 */
DVZ_EXPORT DvzTexture* dvz_texture_2D(
    DvzBatch* batch, DvzFormat format, DvzFilter filter, DvzSamplerAddressMode address_mode,
    uint32_t width, uint32_t height, void* data, int flags);



/**
 * Create a 3D texture to be used in a volume visual.
 *
 * @param batch the batch
 * @param format the texture format
 * @param filter the filter
 * @param address_mode the address mode
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth
 * @param data the texture data to upload
 * @param flags the texture creation flags
 * @returns the texture
 */
DVZ_EXPORT DvzTexture* dvz_texture_3D(
    DvzBatch* batch, DvzFormat format, DvzFilter filter, DvzSamplerAddressMode address_mode, //
    uint32_t width, uint32_t height, uint32_t depth, void* data, int flags);



/*************************************************************************************************/
/*  Colormap functions                                                                           */
/*************************************************************************************************/

/**
 * Fetch a color from a colormap and a value (either 8-bit or float, depending on DVZ_COLOR_CVEC4).
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] color the fetched color
 */
DVZ_EXPORT void dvz_colormap(DvzColormap cmap, uint8_t value, DvzColor color);



/**
 * Fetch a color from a colormap and a value (8-bit version).
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] color the fetched color
 */
DVZ_EXPORT void dvz_colormap_8bit(DvzColormap cmap, uint8_t value, cvec4 color);



/**
 * Fetch a color from a colormap and an interpolated value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] color the fetched color
 */
DVZ_EXPORT void
dvz_colormap_scale(DvzColormap cmap, float value, float vmin, float vmax, DvzColor color);



/**
 * Fetch colors from a colormap and an array of values.
 *
 * @param cmap the colormap
 * @param count the number of values
 * @param values pointer to the array of float numbers
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] out (array) the fetched colors
 */
DVZ_EXPORT void dvz_colormap_array(
    DvzColormap cmap, uint32_t count, float* values, float vmin, float vmax, DvzColor* out);



/**
 * Generate an SDF from an SVG path.
 *
 * @param svg_path the SVG path
 * @param width the width of the generated SDF, in pixels
 * @param height the height of the generated SDF, in pixels
 * @returns the generated texture as RGB floats
 */
DVZ_EXPORT float* dvz_sdf_from_svg(const char* svg_path, uint32_t width, uint32_t height);



/**
 * Generate a multichannel SDF from an SVG path.
 *
 * @param svg_path the SVG path
 * @param width the width of the generated SDF, in pixels
 * @param height the height of the generated SDF, in pixels
 * @returns the generated texture as RGB floats
 */
DVZ_EXPORT float* dvz_msdf_from_svg(const char* svg_path, uint32_t width, uint32_t height);



/**
 * Convert an SDF float texture to a byte texture.
 *
 * @param sdf the SDF float texture
 * @param width the width of the texture
 * @param height the height of the texture
 * @returns the byte texture
 */
DVZ_EXPORT uint8_t* dvz_sdf_to_rgb(float* sdf, uint32_t width, uint32_t height);



/**
 * Convert a multichannel SDF float texture to a byte texture.
 *
 * @param sdf the SDF float texture
 * @param width the width of the texture
 * @param height the height of the texture
 * @returns the byte texture
 */
DVZ_EXPORT uint8_t* dvz_msdf_to_rgb(float* sdf, uint32_t width, uint32_t height);



/**
 * Convert an RGB byte texture to an RGBA one.
 *
 * @param count the number of pixels (and NOT the number of bytes) in the byte texture
 * @param rgb the RGB texture
 * @param rgba the returned RGBA texture
 */
DVZ_EXPORT void dvz_rgb_to_rgba_char(uint32_t count, uint8_t* rgb, uint8_t* rgba);



/**
 * Convert an RGB float texture to an RGBA one.
 *
 * @param count the number of pixels (and NOT the number of bytes) in the float texture
 * @param rgb the RGB texture
 * @param rgba the returned RGBA texture
 */
DVZ_EXPORT void dvz_rgb_to_rgba_float(uint32_t count, float* rgb, float* rgba);



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

/**
 * Compute face normals.
 *
 * @param vertex_count number of vertices
 * @param index_count number of indices (triple of the number of faces)
 * @param pos array of vec3 positions
 * @param index pos array of uint32_t indices
 * @param[out] normal (array) the vec3 normals (to be overwritten by this function)
 */
DVZ_EXPORT void dvz_compute_normals(
    uint32_t vertex_count, uint32_t index_count, vec3* pos, DvzIndex* index, vec3* normal);



/**
 * Recompute the face normals.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_normals(DvzShape* shape);



/**
 * Merge several shapes.
 *
 * @param shape the merged shape
 * @param count the number of shapes to merge
 * @param shapes the shapes to merge
 */
DVZ_EXPORT void dvz_shape_merge(DvzShape* shape, uint32_t count, DvzShape** shapes);



/**
 * Show information about a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_print(DvzShape* shape);



/**
 * Return the number of vertices of a shape.
 *
 * @param shape the shape
 * @returns the number of vertices
 */
DVZ_EXPORT uint32_t dvz_shape_vertex_count(DvzShape* shape);



/**
 * Return the number of index of a shape.
 *
 * @param shape the shape
 * @returns the number of index
 */
DVZ_EXPORT uint32_t dvz_shape_index_count(DvzShape* shape);



/**
 * Convert an indexed shape to a non-indexed one by duplicating the vertex values according
 * to the indices.
 *
 * This is used by the mesh wireframe option, as a given vertex may have distinct barycentric
 * coordinates depending on its index.
 *
 * @param shape the shape
 * @param flags the flags
 */
DVZ_EXPORT void dvz_shape_unindex(DvzShape* shape, int flags);



/**
 * Destroy a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_destroy(DvzShape* shape);



/*************************************************************************************************/
/*  Shape transforms                                                                             */
/*************************************************************************************************/

/**
 * Start a transformation sequence.
 *
 * @param shape the shape
 * @param first the first vertex to modify
 * @param count the number of vertices to modify
 */
DVZ_EXPORT void dvz_shape_begin(DvzShape* shape, uint32_t first, uint32_t count);



/**
 * Append a scaling transform to a shape.
 *
 * @param shape the shape
 * @param scale the scaling factors
 */
DVZ_EXPORT void dvz_shape_scale(DvzShape* shape, vec3 scale);



/**
 * Append a translation to a shape.
 *
 * @param shape the shape
 * @param translate the translation vector
 */
DVZ_EXPORT void dvz_shape_translate(DvzShape* shape, vec3 translate);



/**
 * Append a rotation to a shape.
 *
 * @param shape the shape
 * @param angle the rotation angle
 * @param axis the rotation axis
 */
DVZ_EXPORT void dvz_shape_rotate(DvzShape* shape, float angle, vec3 axis);



/**
 * Append an arbitrary transformation.
 *
 * @param shape the shape
 * @param transform the transform mat4 matrix
 */
DVZ_EXPORT void dvz_shape_transform(DvzShape* shape, mat4 transform);



/**
 * Compute the rescaling factor to renormalize a shape.
 *
 * @param shape the shape
 * @param flags the rescaling flags
 * @param[out] out_scale the computed scaling factors
 */
DVZ_EXPORT float dvz_shape_rescaling(DvzShape* shape, int flags, vec3 out_scale);



/**
 * Apply the transformation sequence and reset it.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_end(DvzShape* shape);



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

/**
 * Create a square shape.
 *
 * @param shape the shape
 * @param color the square color
 */
DVZ_EXPORT void dvz_shape_square(DvzShape* shape, DvzColor color);



/**
 * Create a disc shape.
 *
 * @param shape the shape
 * @param count the number of points along the disc border
 * @param color the disc color
 */
DVZ_EXPORT void dvz_shape_disc(DvzShape* shape, uint32_t count, DvzColor color);



/**
 * Create a sector shape.
 *
 * @param shape the shape
 * @param count the number of points along the sector border
 * @param angle_start the initial angle
 * @param angle_stop the final angle
 * @param color the sector color
 */
DVZ_EXPORT void dvz_shape_sector(
    DvzShape* shape, uint32_t count, float angle_start, float angle_stop, DvzColor color);



/**
 * Create a histogram shape.
 *
 * @param shape the shape
 * @param count the number of bars
 * @param heights the height of each bar
 * @param color the sector color
 */
DVZ_EXPORT void
dvz_shape_histogram(DvzShape* shape, uint32_t count, float* heights, DvzColor color);



/**
 * Create a polygon shape using the simple earcut polygon triangulation algorithm.
 *
 * @param shape the shape
 * @param count the number of points along the polygon border
 * @param points the points 2D coordinates
 * @param color the polygon color
 */
DVZ_EXPORT
void dvz_shape_polygon(DvzShape* shape, uint32_t count, const dvec2* points, DvzColor color);



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

/**
 * Create an empty shape.
 *
 * @returns the shape
 */
DVZ_EXPORT DvzShape* dvz_shape(void);



/**
 * Create a grid shape.
 *
 * @param shape the shape
 * @param row_count number of rows
 * @param col_count number of cols
 * @param heights a pointer to row_count*col_count height values (floats)
 * @param colors a pointer to row_count*col_count color values (DvzColor: cvec4 or vec4)
 * @param o the origin
 * @param u the unit vector parallel to each column
 * @param v the unit vector parallel to each row
 * @param flags the grid creation flags
 */
DVZ_EXPORT
void dvz_shape_surface(
    DvzShape* shape, uint32_t row_count, uint32_t col_count, //
    float* heights, DvzColor* colors,                        //
    vec3 o, vec3 u, vec3 v, int flags);



/**
 * Create a cube shape.
 *
 * @param shape the shape
 * @param colors the colors of the six faces
 */
DVZ_EXPORT void dvz_shape_cube(DvzShape* shape, DvzColor* colors);



/**
 * Create a sphere shape.
 *
 * @param shape the shape
 * @param rows the number of rows
 * @param cols the number of columns
 * @param color the sphere color
 */
DVZ_EXPORT void dvz_shape_sphere(DvzShape* shape, uint32_t rows, uint32_t cols, DvzColor color);



/**
 * Create a cylinder shape.
 *
 * @param shape the shape
 * @param count the number of points along the cylinder border
 * @param color the cylinder color
 */
DVZ_EXPORT void dvz_shape_cylinder(DvzShape* shape, uint32_t count, DvzColor color);



/**
 * Create a cone shape.
 *
 * @param shape the shape
 * @param count the number of points along the disc border
 * @param color the cone color
 */
DVZ_EXPORT void dvz_shape_cone(DvzShape* shape, uint32_t count, DvzColor color);



/**
 * Create a 3D arrow using a cylinder and cone.
 *
 * The total length is 1.
 *
 * @param shape the shape
 * @param count the number of sides to the shaft and head
 * @param head_length the length of the head
 * @param head_radius the radius of the head
 * @param shaft_radius the radius of the shaft
 * @param color the arrow color
 */
DVZ_EXPORT void dvz_shape_arrow(
    DvzShape* shape, uint32_t count, float head_length, float head_radius, float shaft_radius,
    DvzColor color);



/**
 * Create a torus shape.
 *
 * The radius of the ring is 0.5.
 *
 * @param shape the shape
 * @param count_radial the number of points around the ring
 * @param count_tubular the number of points in each cross-section
 * @param tube_radius the radius of the tube.
 * @param color the torus color
 */
DVZ_EXPORT void dvz_shape_torus(
    DvzShape* shape, uint32_t count_radial, uint32_t count_tubular, float tube_radius,
    DvzColor color);



/**
 * Create a tetrahedron.
 *
 * @param shape the shape
 * @param color the color
 */
DVZ_EXPORT void dvz_shape_tetrahedron(DvzShape* shape, DvzColor color);



/**
 * Create a tetrahedron.
 *
 * @param shape the shape
 * @param color the color
 */
DVZ_EXPORT void dvz_shape_hexahedron(DvzShape* shape, DvzColor color);



/**
 * Create a octahedron.
 *
 * @param shape the shape
 * @param color the color
 */
DVZ_EXPORT void dvz_shape_octahedron(DvzShape* shape, DvzColor color);



/**
 * Create a dodecahedron.
 *
 * @param shape the shape
 * @param color the color
 */
DVZ_EXPORT void dvz_shape_dodecahedron(DvzShape* shape, DvzColor color);



/**
 * Create a icosahedron.
 *
 * @param shape the shape
 * @param color the color
 */
DVZ_EXPORT void dvz_shape_icosahedron(DvzShape* shape, DvzColor color);



/**
 * Normalize a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_normalize(DvzShape* shape);



/**
 * Load a .obj shape.
 *
 * @param shape the shape
 * @param file_path the path to the .obj file
 */
DVZ_EXPORT void dvz_shape_obj(DvzShape* shape, const char* file_path);



/**
 * Create a shape out of an array of vertices and faces.
 *
 * @param shape the shape
 * @param vertex_count number of vertices
 * @param positions 3D positions of the vertices
 * @param normals normal vectors (optional, will be otherwise computed automatically)
 * @param colors vertex vectors (optional)
 * @param texcoords texture uv*a coordinates (optional)
 * @param index_count number of indices (3x the number of triangular faces)
 * @param indices vertex indices, three per face
 */
DVZ_EXPORT void dvz_shape_custom(
    DvzShape* shape, uint32_t vertex_count, vec3* positions, vec3* normals, DvzColor* colors,
    vec4* texcoords, uint32_t index_count, DvzIndex* indices);



/*************************************************************************************************/
/*  Font                                                                                         */
/*************************************************************************************************/

/**
 * Load the default atlas and font.
 *
 * @param font_size the font size
 * @param[out] af the returned DvzAtlasFont object with DvzAtlas and DvzFont objects.
 */
DVZ_EXPORT void dvz_atlas_font(float font_size, DvzAtlasFont* af);



/**
 * Destroy an atlas.
 *
 * @param atlas the atlas
 */
DVZ_EXPORT void dvz_atlas_destroy(DvzAtlas* atlas);



/**
 * Create a font.
 *
 * @param ttf_size size in bytes of a TTF font raw buffer
 * @param ttf_bytes TTF font raw buffer
 * @returns the font
 */
DVZ_EXPORT DvzFont* dvz_font(unsigned long ttf_size, unsigned char* ttf_bytes);



/**
 * Set the font size.
 *
 * @param font the font
 * @param size the font size
 */
DVZ_EXPORT void dvz_font_size(DvzFont* font, double size);



/**
 * Compute the shift of each glyph in a Unicode string, using the Freetype library.
 *
 * @param font the font
 * @param length the number of glyphs
 * @param codepoints the Unicode codepoints of the glyphs
 * @param xywh an array of (x,y,w,h) shifts
 */
DVZ_EXPORT void
dvz_font_layout(DvzFont* font, uint32_t length, const uint32_t* codepoints, vec4* xywh);



/**
 * Compute the shift of each glyph in an ASCII string, using the Freetype library.
 *
 * @param font the font
 * @param string the ASCII string
 * @param xywh the returned array of (x,y,w,h) shifts
 */
DVZ_EXPORT void dvz_font_ascii(DvzFont* font, const char* string, vec4* xywh);



/**
 * Render a string using Freetype.
 *
 * Note: the caller must free the output after use.
 *
 * @param font the font
 * @param length the number of glyphs
 * @param codepoints the Unicode codepoints of the glyphs
 * @param xywh an array of (x,y,w,h) shifts, returned by dvz_font_layout()
 * @param flags the font flags
 * @param[out] out_size the number of bytes in the returned image
 * @returns an RGBA array allocated by this function and that MUST be freed by the caller
 *
 */
DVZ_EXPORT uint8_t* dvz_font_draw(
    DvzFont* font, uint32_t length, const uint32_t* codepoints, vec4* xywh, int flags,
    uvec2 out_size);



/**
 * Generate a texture with a rendered text.
 *
 * @param font the font
 * @param batch the batch
 * @param length the number of Unicode codepoints
 * @param codepoints the Unicode codepoints
 * @param[out] size the generated texture size
 * @returns the texture
 *
 */
DVZ_EXPORT DvzTexture* dvz_font_texture(
    DvzFont* font, DvzBatch* batch, uint32_t length, uint32_t* codepoints, uvec3 size);



/**
 * Destroy a font.
 *
 * @param font the font
 */
DVZ_EXPORT void dvz_font_destroy(DvzFont* font);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Interactivity API                                                                            */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Animations                                                                                   */
/*************************************************************************************************/

/**
 * Normalize a value in an interval.
 *
 * @param t0 the interval start
 * @param t1 the interval end
 * @param t the value within the interval
 * @returns the normalized value between 0 and 1
 */
DVZ_EXPORT double dvz_resample(double t0, double t1, double t);



/**
 * Apply an easing function to a normalized value.
 *
 * @param easing the easing mode
 * @param t the normalized value
 * @returns the eased value
 */
DVZ_EXPORT double dvz_easing(DvzEasing easing, double t);



/**
 * Generate a 2D circular motion.
 *
 * @param center the circle center
 * @param radius the circle radius
 * @param angle the initial angle
 * @param t the normalized value
 * @param[out] out the 2D position
 */
DVZ_EXPORT void dvz_circular_2D(vec2 center, float radius, float angle, float t, vec2 out);



/**
 * Generate a 3D circular motion.
 *
 * @param center the circle center
 * @param u the first 3D vector defining the plane containing the circle
 * @param v the second 3D vector defining the plane containing the circle
 * @param radius the circle radius
 * @param angle the initial angle
 * @param t the normalized value
 * @param[out] out the 3D position
 */
DVZ_EXPORT void
dvz_circular_3D(vec3 center, vec3 u, vec3 v, float radius, float angle, float t, vec3 out);



/**
 * Make a linear interpolation between two scalar value.
 *
 * @param p0 the first value
 * @param p1 the second value
 * @param t the normalized value
 * @returns the interpolated value
 */
DVZ_EXPORT float dvz_interpolate(float p0, float p1, float t);



/**
 * Make a linear interpolation between two 2D points.
 *
 * @param p0 the first point
 * @param p1 the second point
 * @param t the normalized value
 * @param[out] out the interpolated point
 */
DVZ_EXPORT void dvz_interpolate_2D(vec2 p0, vec2 p1, float t, vec2 out);



/**
 * Make a linear interpolation between two 3D points.
 *
 * @param p0 the first point
 * @param p1 the second point
 * @param t the normalized value
 * @param[out] out the interpolated point
 */
DVZ_EXPORT void dvz_interpolate_3D(vec3 p0, vec3 p1, float t, vec3 out);



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set the initial arcball angles.
 *
 * @param arcball the arcball
 * @param angles the initial angles
 */
DVZ_EXPORT void dvz_arcball_initial(DvzArcball* arcball, vec3 angles);



/**
 * Reset an arcball to its initial position.
 *
 * @param arcball the arcball
 */
DVZ_EXPORT void dvz_arcball_reset(DvzArcball* arcball);



/**
 * Inform an arcball of a panel resize.
 *
 * @param arcball the arcball
 * @param width the panel width
 * @param height the panel height
 */
DVZ_EXPORT void dvz_arcball_resize(DvzArcball* arcball, float width, float height);



/**
 * Set the arcball flags.
 *
 * @param arcball the arcball
 * @param flags the flags
 */
DVZ_EXPORT void dvz_arcball_flags(DvzArcball* arcball, int flags);



/**
 * Add arcball constraints.
 *
 * @param arcball the arcball
 * @param constrain the constrain values
 */
DVZ_EXPORT void dvz_arcball_constrain(DvzArcball* arcball, vec3 constrain);



/**
 * Set the arcball angles.
 *
 * @param arcball the arcball
 * @param angles the angles
 */
DVZ_EXPORT void dvz_arcball_set(DvzArcball* arcball, vec3 angles);



/**
 * Get the current arcball angles.
 *
 * @param arcball the arcball
 * @param[out] out_angles the arcball angles
 */
DVZ_EXPORT void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles);



/**
 * Apply a rotation to an arcball.
 *
 * @param arcball the arcball
 * @param cur_pos the initial position
 * @param last_pos the final position
 */
DVZ_EXPORT void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);



// DVZ_EXPORT void dvz_arcball_pan(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);

// DVZ_EXPORT void dvz_arcball_lock(DvzArcball* arcball, vec3 dir);



/**
 * Return the model matrix of an arcball.
 *
 * @param arcball the arcball
 * @param[out] model the model
 */
DVZ_EXPORT void dvz_arcball_model(DvzArcball* arcball, mat4 model);



/**
 * Finalize arcball position update.
 *
 * @param arcball the arcball
 */
DVZ_EXPORT void dvz_arcball_end(DvzArcball* arcball);



/**
 * Apply an MVP matrix to an arcball (only the model matrix).
 *
 * @param arcball the arcball
 * @param mvp the MVP
 */
DVZ_EXPORT void dvz_arcball_mvp(DvzArcball* arcball, DvzMVP* mvp);



/**
 * Display information about an arcball.
 *
 * @param arcball the arcball
 */
DVZ_EXPORT void dvz_arcball_print(DvzArcball* arcball);



/**
 * Show a GUI with sliders controlling the three arcball angles.
 *
 * @param arcball the arcball
 * @param app the app
 * @param canvas_id the canvas (or figure) ID
 * @param panel the panel
 */
DVZ_EXPORT
void dvz_arcball_gui(DvzArcball* arcball, DvzApp* app, DvzId canvas_id, DvzPanel* panel);



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

/**
 * Set the initial camera parameters.
 *
 * @param camera the camera
 * @param pos the initial position
 * @param lookat the lookat position
 * @param up the up vector
 */
DVZ_EXPORT void dvz_camera_initial(DvzCamera* camera, vec3 pos, vec3 lookat, vec3 up);



/**
 * Reset a camera.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_reset(DvzCamera* camera);



/**
 * Set the camera zrange.
 *
 * @param camera the camera
 * @param near the near value
 * @param far the far value
 */
DVZ_EXPORT void dvz_camera_zrange(DvzCamera* camera, float near, float far);



/**
 * Make an orthographic camera.
 *
 * @param camera the camera
 * @param left the left value
 * @param right the right value
 * @param bottom the bottom value
 * @param top the top value
 */
DVZ_EXPORT void
dvz_camera_ortho(DvzCamera* camera, float left, float right, float bottom, float top);



/**
 * Inform a camera of a panel resize.
 *
 * @param camera the camera
 * @param width the panel width
 * @param height the panel height
 */
DVZ_EXPORT void dvz_camera_resize(DvzCamera* camera, float width, float height);



/**
 * Set a camera position.
 *
 * @param camera the camera
 * @param pos the pos
 */
DVZ_EXPORT void dvz_camera_position(DvzCamera* camera, vec3 pos);



/**
 * Get the camera position.
 *
 * @param camera the camera
 * @param[out] pos the pos
 */
DVZ_EXPORT void dvz_camera_get_position(DvzCamera* camera, vec3 pos);



/**
 * Set a camera lookat position.
 *
 * @param camera the camera
 * @param lookat the lookat position
 */
DVZ_EXPORT void dvz_camera_lookat(DvzCamera* camera, vec3 lookat);



/**
 * Get the camera lookat position.
 *
 * @param camera the camera
 * @param[out] lookat the lookat position
 */
DVZ_EXPORT void dvz_camera_get_lookat(DvzCamera* camera, vec3 lookat);



/**
 * Set a camera up vector.
 *
 * @param camera the camera
 * @param up the up vector
 */
DVZ_EXPORT void dvz_camera_up(DvzCamera* camera, vec3 up);



/**
 * Get the camera up vector.
 *
 * @param camera the camera
 * @param[out] up the up vector
 */
DVZ_EXPORT void dvz_camera_get_up(DvzCamera* camera, vec3 up);



/**
 * Set a camera perspective.
 *
 * @param camera the camera
 * @param fov the field of view angle (in radians)
 */
DVZ_EXPORT void dvz_camera_perspective(DvzCamera* camera, float fov);



/**
 * Return the view and proj matrices of the camera.
 *
 * @param camera the camera
 * @param[out] view the view matrix
 * @param[out] proj the proj matrix
 */
DVZ_EXPORT void dvz_camera_viewproj(DvzCamera* camera, mat4 view, mat4 proj);



/**
 * Apply an MVP to a camera.
 *
 * @param camera the camera
 * @param mvp the MVP
 */
DVZ_EXPORT void dvz_camera_mvp(DvzCamera* camera, DvzMVP* mvp);



/**
 * Display information about a camera.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_print(DvzCamera* camera);



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

/**
 * Create a panzoom object (usually you'd rather use `dvz_panel_panzoom()`).
 *
 * @param width the panel width
 * @param height the panel height
 * @param flags the panzoom creation flags
 * @returns the Panzoom object
 */
DVZ_EXPORT DvzPanzoom* dvz_panzoom(float width, float height, int flags); // inner viewport size



/**
 * Reset a panzoom.
 *
 * @param pz the panzoom
 */
DVZ_EXPORT void dvz_panzoom_reset(DvzPanzoom* pz);



/**
 * Inform a panzoom of a panel resize.
 *
 * @param pz the panzoom
 * @param width the panel width
 * @param height the panel height
 */
DVZ_EXPORT void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height);



/**
 * Set the panzoom flags.
 *
 * @param pz the panzoom
 * @param flags the flags
 */
DVZ_EXPORT void dvz_panzoom_flags(DvzPanzoom* pz, int flags);



/**
 * Apply a pan value to a panzoom.
 *
 * @param pz the panzoom
 * @param pan the pan, in NDC
 */
DVZ_EXPORT void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan);



/**
 * Apply a zoom value to a panzoom.
 *
 * @param pz the panzoom
 * @param zoom the zoom, in NDC
 */
DVZ_EXPORT void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom);



/**
 * Apply a pan shift to a panzoom.
 *
 * @param pz the panzoom
 * @param shift_px the shift value, in pixels
 * @param center_px the center position, in pixels
 */
DVZ_EXPORT void dvz_panzoom_pan_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px);



/**
 * Apply a zoom shift to a panzoom.
 *
 * @param pz the panzoom
 * @param shift_px the shift value, in pixels
 * @param center_px the center position, in pixels
 */
DVZ_EXPORT void dvz_panzoom_zoom_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px);



/**
 * End a panzoom interaction.
 *
 * @param pz the panzoom
 */
DVZ_EXPORT void dvz_panzoom_end(DvzPanzoom* pz);



/**
 * Apply a wheel zoom to a panzoom.
 *
 * @param pz the panzoom
 * @param dir the wheel direction
 * @param center_px the center position, in pixels
 */
DVZ_EXPORT void dvz_panzoom_zoom_wheel(DvzPanzoom* pz, vec2 dir, vec2 center_px);



/**
 * Get the current zoom level.
 *
 * @param pz the panzoom
 * @param dim the dimension
 */
DVZ_EXPORT float dvz_panzoom_level(DvzPanzoom* pz, DvzDim dim);



/**
 * Get the extent box.
 *
 * @param pz the panzoom
 * @param[out] extent the extent box in normalized coordinates
 */
DVZ_EXPORT void dvz_panzoom_extent(DvzPanzoom* pz, DvzBox* extent);



/**
 * Set the extent box.
 *
 * @param pz the panzoom
 * @param extent the extent box
 */
DVZ_EXPORT void dvz_panzoom_set(DvzPanzoom* pz, DvzBox* extent);



/**
 * Apply an MVP matrix to a panzoom.
 *
 * @param pz the panzoom
 * @param mvp the MVP
 */
DVZ_EXPORT void dvz_panzoom_mvp(DvzPanzoom* pz, DvzMVP* mvp);



/**
 * Get x-y bounds.
 *
 * @param pz the panzoom
 * @param ref the ref
 * @param[out] xmin xmin
 * @param[out] xmax xmax
 * @param[out] ymin ymin
 * @param[out] ymax ymax
 */
DVZ_EXPORT void dvz_panzoom_bounds(
    DvzPanzoom* pz, DvzRef* ref, double* xmin, double* xmax, double* ymin, double* ymax);



/**
 * Set x bounds.
 *
 * @param pz the panzoom
 * @param ref the ref
 * @param xmin xmin
 * @param xmax xmax
 */
DVZ_EXPORT void dvz_panzoom_xlim(DvzPanzoom* pz, DvzRef* ref, double xmin, double xmax);



/**
 * Set y bounds.
 *
 * @param pz the panzoom
 * @param ref the ref
 * @param ymin ymin
 * @param ymax ymax
 */
DVZ_EXPORT void dvz_panzoom_ylim(DvzPanzoom* pz, DvzRef* ref, double ymin, double ymax);



/**
 * Register a mouse event to a panzoom.
 *
 * @param pz the panzoom
 * @param ev the mouse event
 * @returns whether the panzoom is affected by the mouse event
 */
DVZ_EXPORT bool dvz_panzoom_mouse(DvzPanzoom* pz, DvzMouseEvent* ev);



/**
 * Destroy a panzoom.
 *
 * @param pz the pz
 */
DVZ_EXPORT void dvz_panzoom_destroy(DvzPanzoom* pz);



/*************************************************************************************************/
/*  Ortho                                                                                        */
/*************************************************************************************************/

/**
 * Reset an ortho.
 *
 * @param ortho the ortho
 */
DVZ_EXPORT void dvz_ortho_reset(DvzOrtho* ortho);



/**
 * Inform an ortho of a panel resize.
 *
 * @param ortho the ortho
 * @param width the panel width
 * @param height the panel height
 */
DVZ_EXPORT void dvz_ortho_resize(DvzOrtho* ortho, float width, float height);



/**
 * Set the ortho flags.
 *
 * @param ortho the ortho
 * @param flags the flags
 */
DVZ_EXPORT void dvz_ortho_flags(DvzOrtho* ortho, int flags);



/**
 * Apply a pan value to an ortho.
 *
 * @param ortho the ortho
 * @param pan the pan, in NDC
 */
DVZ_EXPORT void dvz_ortho_pan(DvzOrtho* ortho, vec2 pan);



/**
 * Apply a zoom value to an ortho.
 *
 * @param ortho the ortho
 * @param zoom the zoom level
 */
DVZ_EXPORT void dvz_ortho_zoom(DvzOrtho* ortho, float zoom);



/**
 * Apply a pan shift to an ortho.
 *
 * @param ortho the ortho
 * @param shift_px the shift value, in pixels
 * @param center_px the center position, in pixels
 */
DVZ_EXPORT void dvz_ortho_pan_shift(DvzOrtho* ortho, vec2 shift_px, vec2 center_px);



/**
 * Apply a zoom shift to an ortho.
 *
 * @param ortho the ortho
 * @param shift_px the shift value, in pixels
 * @param center_px the center position, in pixels
 */
DVZ_EXPORT void dvz_ortho_zoom_shift(DvzOrtho* ortho, vec2 shift_px, vec2 center_px);



/**
 * End an ortho interaction.
 *
 * @param ortho the ortho
 */
DVZ_EXPORT void dvz_ortho_end(DvzOrtho* ortho);



/**
 * Apply a wheel zoom to an ortho.
 *
 * @param ortho the ortho
 * @param dir the wheel direction
 * @param center_px the center position, in pixels
 */
DVZ_EXPORT void dvz_ortho_zoom_wheel(DvzOrtho* ortho, vec2 dir, vec2 center_px);



/**
 * Apply an MVP matrix to an ortho.
 *
 * @param ortho the ortho
 * @param mvp the MVP
 */
DVZ_EXPORT void dvz_ortho_mvp(DvzOrtho* ortho, DvzMVP* mvp);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Axis-related functions                                                                       */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Create a reference frame (wrapping a 3D box representing the data in its original coordinates).
 *
 * @param flags the flags
 * @returns the reference frame
 */
DVZ_EXPORT DvzRef* dvz_ref(int flags);



/**
 * Indicate whether the reference is set on a given axis.
 *
 * @param ref the reference frame
 * @param dim the dimension axis
 * @returns whether the ref is set on this axis.
 */
DVZ_EXPORT bool dvz_ref_is_set(DvzRef* ref, DvzDim dim);



/**
 * Set the range on a given axis.
 *
 * @param ref the reference frame
 * @param dim the dimension axis
 * @param vmin the minimum value
 * @param vmax the maximum value
 */
DVZ_EXPORT void dvz_ref_set(DvzRef* ref, DvzDim dim, double vmin, double vmax);



/**
 * Get the range on a given axis.
 *
 * @param ref the reference frame
 * @param dim the dimension axis
 * @param[out] vmin the minimum value
 * @param[out] vmax the maximum value
 */
DVZ_EXPORT void dvz_ref_get(DvzRef* ref, DvzDim dim, double* vmin, double* vmax);



/**
 * Expand the reference by ensuring it contains the specified range.
 *
 * @param ref the reference frame
 * @param dim the dimension axis
 * @param vmin the minimum value
 * @param vmax the maximum value
 */
DVZ_EXPORT void dvz_ref_expand(DvzRef* ref, DvzDim dim, double vmin, double vmax);



/**
 * Expand the reference by ensuring it contains the specified 2D data.
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 2D positions
 */
DVZ_EXPORT void dvz_ref_expand_2D(DvzRef* ref, uint32_t count, dvec2* pos);



/**
 * Expand the reference by ensuring it contains the specified 3D data.
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 3D positions
 */
DVZ_EXPORT void dvz_ref_expand_3D(DvzRef* ref, uint32_t count, dvec3* pos);



/**
 * Transform 1D data from the reference frame to normalized device coordinates [-1..+1].
 *
 * @param ref the reference frame
 * @param dim which dimension
 * @param count the number of positions
 * @param pos the 1D positions
 * @param[out] pos_tr (array) the transformed positions
 */
DVZ_EXPORT void
dvz_ref_normalize_1D(DvzRef* ref, DvzDim dim, uint32_t count, double* pos, vec3* pos_tr);



/**
 * Transform 2D data from the reference frame to normalized device coordinates [-1..+1].
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 2D positions
 * @param[out] pos_tr (array) the transformed 3D positions
 */
DVZ_EXPORT void dvz_ref_normalize_2D(DvzRef* ref, uint32_t count, dvec2* pos, vec3* pos_tr);



/**
 * Transform 2D data from the reference frame to normalized device coordinates [-1..+1] in 2D.
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 2D positions
 * @param[out] pos_tr (array) the transformed 2D positions
 */
DVZ_EXPORT void dvz_ref_normalize_polygon(DvzRef* ref, uint32_t count, dvec2* pos, dvec2* pos_tr);



/**
 * Transform 3D data from the reference frame to normalized device coordinates [-1..+1].
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 3D positions
 * @param[out] pos_tr (array) the transformed positions
 */
DVZ_EXPORT void dvz_ref_normalize_3D(DvzRef* ref, uint32_t count, dvec3* pos, vec3* pos_tr);



/**
 * Inverse transform from normalized device coordinates [-1..+1] to the reference frame.
 *
 * @param ref the reference frame
 * @param pos_tr the 3D position in normalized device coordinates
 * @param[out] pos the original position
 */
DVZ_EXPORT void dvz_ref_inverse(DvzRef* ref, vec3 pos_tr, dvec3* pos);



/**
 * Destroy a reference frame.
 *
 * @param ref the reference frame
 */
DVZ_EXPORT void dvz_ref_destroy(DvzRef* ref);



EXTERN_C_OFF

#endif
