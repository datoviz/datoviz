/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**************************************************************************************************

 * DATOVIZ PUBLIC API HEADER FILE
 * ==============================
 * 2024-07-01
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
#include "datoviz_keycodes.h"
#include "datoviz_macros.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "datoviz_version.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzArcball DvzArcball;
typedef struct DvzAtlas DvzAtlas;
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
DVZ_EXPORT void dvz_scene_mouse(DvzScene* scene, DvzFigure* fig, DvzMouseEvent ev);



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
 * @returns the generated mouse event
 */
DVZ_EXPORT DvzMouseEvent dvz_mouse_move(DvzMouse* mouse, vec2 pos, int mods);



/**
 * Create a mouse press event.
 *
 * @param mouse the mouse
 * @param button the mouse button (enum int)
 * @param mods the keyboard modifier flags
 * @returns the generated mouse event
 */
DVZ_EXPORT DvzMouseEvent dvz_mouse_press(DvzMouse* mouse, DvzMouseButton button, int mods);



/**
 * Create a mouse release event.
 *
 * @param mouse the mouse
 * @param button the mouse button (enum int)
 * @param mods the keyboard modifier flags
 * @returns the generated mouse event
 */
DVZ_EXPORT DvzMouseEvent dvz_mouse_release(DvzMouse* mouse, DvzMouseButton button, int mods);



/**
 * Create a mouse wheel event.
 *
 * @param mouse the mouse
 * @param dir the mouse wheel direction (x, y)
 * @param mods the keyboard modifier flags
 * @returns the generated mouse event
 */
DVZ_EXPORT DvzMouseEvent dvz_mouse_wheel(DvzMouse* mouse, vec2 dir, int mods);



/**
 * Create a generic mouse event.
 *
 * @param mouse the mouse
 * @param ev the mouse event
 */
DVZ_EXPORT void dvz_mouse_event(DvzMouse* mouse, DvzMouseEvent ev);



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
 * Get or create a Reference for a panel.
 *
 * @param panel the panel
 * @param flags the reference creation flags.
 * @returns the reference
 */
DVZ_EXPORT DvzRef* dvz_panel_ref(DvzPanel* panel, int flags);



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
 * @returns the panzoom
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel);



/**
 * Set ortho interactivity for a panel.
 *
 * @param panel the panel
 * @returns the ortho
 */
DVZ_EXPORT DvzOrtho* dvz_panel_ortho(DvzPanel* panel);



/**
 * Set arcball interactivity for a panel.
 *
 * @param panel the panel
 * @returns the arcball
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzPanel* panel);



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
DVZ_EXPORT DvzTexture* dvz_texture_image(
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
DVZ_EXPORT DvzTexture* dvz_texture_volume(
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
 * @returns the RGBA texture
 */
DVZ_EXPORT uint8_t* dvz_rgb_to_rgba_char(uint32_t count, uint8_t* rgb);



/**
 * Convert an RGB float texture to an RGBA one.
 *
 * @param count the number of pixels (and NOT the number of bytes) in the float texture
 * @param rgb the RGB texture
 * @returns the RGBA texture
 */
DVZ_EXPORT float* dvz_rgb_to_rgba_float(uint32_t count, float* rgb);



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
 * @param count the number of shapes to merge
 * @param shapes the shapes to merge
 * @returns the merged shape
 */
DVZ_EXPORT DvzShape dvz_shape_merge(uint32_t count, DvzShape* shapes);



/**
 * Show information about a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_print(DvzShape* shape);



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
 * @param color the square color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_square(DvzColor color);



/**
 * Create a disc shape.
 *
 * @param count the number of points along the disc border
 * @param color the disc color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_disc(uint32_t count, DvzColor color);



/**
 * Create a polygon shape using the simple earcut polygon triangulation algorithm.
 *
 * @param count the number of points along the polygon border
 * @param points the points 2D coordinates
 * @param color the polygon color
 * @returns the shape
 */
DVZ_EXPORT
DvzShape dvz_shape_polygon(uint32_t count, const dvec2* points, DvzColor color);



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

/**
 * Create a grid shape.
 *
 * @param row_count number of rows
 * @param col_count number of cols
 * @param heights a pointer to row_count*col_count height values (floats)
 * @param colors a pointer to row_count*col_count color values (DvzColor: cvec4 or vec4)
 * @param o the origin
 * @param u the unit vector parallel to each column
 * @param v the unit vector parallel to each row
 * @param flags the grid creation flags
 * @returns the shape
 */
DVZ_EXPORT
DvzShape dvz_shape_surface(
    uint32_t row_count, uint32_t col_count, //
    float* heights, DvzColor* colors,       //
    vec3 o, vec3 u, vec3 v, int flags);



/**
 * Create a cube shape.
 *
 * @param colors the colors of the six faces
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_cube(DvzColor* colors);



/**
 * Create a sphere shape.
 *
 * @param rows the number of rows
 * @param cols the number of columns
 * @param color the sphere color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, DvzColor color);



/**
 * Create a cone shape.
 *
 * @param count the number of points along the disc border
 * @param color the cone color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_cone(uint32_t count, DvzColor color);



/**
 * Create a cylinder shape.
 *
 * @param count the number of points along the cylinder border
 * @param color the cylinder color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_cylinder(uint32_t count, DvzColor color);



/**
 * Create a tetrahedron.
 *
 * @param color the color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_tetrahedron(DvzColor color);



/**
 * Create a tetrahedron.
 *
 * @param color the color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_hexahedron(DvzColor color);



/**
 * Create a octahedron.
 *
 * @param color the color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_octahedron(DvzColor color);



/**
 * Create a dodecahedron.
 *
 * @param color the color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_dodecahedron(DvzColor color);



/**
 * Create a icosahedron.
 *
 * @param color the color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_icosahedron(DvzColor color);



/**
 * Normalize a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_normalize(DvzShape* shape);



/**
 * Load a .obj shape.
 *
 * @param file_path the path to the .obj file
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_obj(const char* file_path);



/**
 * Create a shape out of an array of vertices and faces.
 *
 * @param vertex_count number of vertices
 * @param positions 3D positions of the vertices
 * @param normals normal vectors (optional, will be otherwise computed automatically)
 * @param colors vertex vectors (optional)
 * @param texcoords texture uv*a coordinates (optional)
 * @param index_count number of indices (3x the number of triangular faces)
 * @param indices vertex indices, three per face
 */
DVZ_EXPORT DvzShape dvz_shape(
    uint32_t vertex_count, vec3* positions, vec3* normals, DvzColor* colors, vec4* texcoords,
    uint32_t index_count, DvzIndex* indices);



/*************************************************************************************************/
/*  Basic visual                                                                                 */
/*************************************************************************************************/

/**
 * Create a basic visual using the few GPU visual primitives (point, line, triangles).
 *
 * @param batch the batch
 * @param topology the primitive topology
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_basic(DvzBatch* batch, DvzPrimitiveTopology topology, int flags);



/**
 * Set the vertex positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the vertex colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the vertex group index.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the group index of each vertex
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_group(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the point size (for POINT_LIST topology only).
 *
 * @param visual the visual
 * @param size the point size in pixels
 */
DVZ_EXPORT void dvz_basic_size(DvzVisual* visual, float size);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_basic_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/

/**
 * Create a pixel visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_pixel(DvzBatch* batch, int flags);



/**
 * Set the pixel positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_pixel_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the pixel colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_pixel_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the pixel size.
 *
 * @param visual the visual
 * @param size the point size in pixels
 */
DVZ_EXPORT void dvz_pixel_size(DvzVisual* visual, float size);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_pixel_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Point                                                                                        */
/*************************************************************************************************/

/**
 * Create a point visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzBatch* batch, int flags);



/**
 * Set the point positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the point sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_size(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the point colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_point_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Marker                                                                                       */
/*************************************************************************************************/

/**
 * Create a marker visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_marker(DvzBatch* batch, int flags);



/**
 * Set the marker mode.
 *
 * @param visual the visual
 * @param mode the marker mode, one of DVZ_MARKER_MODE_CODE, DVZ_MARKER_MODE_BITMAP,
 * DVZ_MARKER_MODE_SDF, DVZ_MARKER_MODE_MSDF, DVZ_MARKER_MODE_MTSDF.
 */
DVZ_EXPORT void dvz_marker_mode(DvzVisual* visual, DvzMarkerMode mode);



/**
 * Set the marker aspect.
 *
 * @param visual the visual
 * @param aspect the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE,
 * DVZ_MARKER_ASPECT_OUTLINE.
 */
DVZ_EXPORT void dvz_marker_aspect(DvzVisual* visual, DvzMarkerAspect aspect);



/**
 * Set the marker shape.
 *
 * @param visual the visual
 * @param shape the marker shape
 */
DVZ_EXPORT void dvz_marker_shape(DvzVisual* visual, DvzMarkerShape shape);



/**
 * Set the marker positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the marker sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_size(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the marker angles.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the angles of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the marker colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the marker edge color.
 *
 * @param visual the visual
 * @param color the edge color
 */
DVZ_EXPORT void dvz_marker_edgecolor(DvzVisual* visual, DvzColor color);



/**
 * Set the marker edge width.
 *
 * @param visual the visual
 * @param width the edge width
 */
DVZ_EXPORT void dvz_marker_linewidth(DvzVisual* visual, float width);



/**
 * Set the marker texture.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_marker_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the texture scale.
 *
 * @param visual the visual
 * @param scale the texture scale
 */
DVZ_EXPORT void dvz_marker_tex_scale(DvzVisual* visual, float scale);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_marker_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Segment                                                                                      */
/*************************************************************************************************/

/**
 * Create a segment visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_segment(DvzBatch* batch, int flags);



/**
 * Set the segment positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param initial the initial 3D positions of the segments
 * @param terminal the terminal 3D positions of the segments
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_segment_position(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* initial, vec3* terminal, int flags);



/**
 * Set the segment shift.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Set the segment colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the segment line widths.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the segment line widths
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_linewidth(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the segment cap types.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param initial the initial segment cap types
 * @param terminal the terminal segment cap types
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_segment_cap(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    DvzCapType* initial, DvzCapType* terminal, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_segment_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Path                                                                                         */
/*************************************************************************************************/

/**
 * Create a path visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_path(DvzBatch* batch, int flags);



/**
 * Set the path positions. Note: all path point positions must be updated at once for now.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param point_count the total number of points across all paths
 * @param positions the path point positions
 * @param path_count the number of different paths
 * @param path_lengths the number of points in each path
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_path_position(
    DvzVisual* visual, uint32_t first, uint32_t point_count, vec3* positions, //
    uint32_t path_count, uint32_t* path_lengths, int flags);



/**
 * Set the path colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_path_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the path line width (may be variable along a path).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the line width of the vertex, in pixels
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_path_linewidth(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the path cap.
 *
 * @param visual the visual
 * @param cap the cap
 */
DVZ_EXPORT
void dvz_path_cap(DvzVisual* visual, DvzCapType cap);



/**
 * Set the path join.
 *
 * @param visual the visual
 * @param join the join
 */
DVZ_EXPORT void dvz_path_join(DvzVisual* visual, DvzJoinType join);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param total_point_count the total number of points to allocate for this visual
 */
DVZ_EXPORT void dvz_path_alloc(DvzVisual* visual, uint32_t total_point_count);



/*************************************************************************************************/
/*  Font                                                                                         */
/*************************************************************************************************/

/**
 * Load the default atlas and font.
 *
 * @param font_size the font size
 * @returns a DvzAtlasFont struct with DvzAtlas and DvzFont objects.
 */
DVZ_EXPORT DvzAtlasFont dvz_atlas_font(double font_size);



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
/*  Glyph                                                                                        */
/*************************************************************************************************/

/**
 * Create a glyph visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_glyph(DvzBatch* batch, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_glyph_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Set the glyph positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the glyph axes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D axis vectors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_axis(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the glyph sizes, in pixels.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes (width and height) of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph anchors.
 *
 * The anchor should be the same for each glyph in a given string. In addition, it is important to
 * set dvz_glyph_group_size() (the size of each string in pixels) for the anchor computation to be
 * correct.
 *
 * The anchor determines the relationship between the glyph 3D position, and the position of the
 * string bounding box. Each string comes with a local coordinate system extending from (-1, -1)
 * (bottom-left corner) to (+1, +1) (top-right corner), and (0, 0) refers to the center of the
 * string. The anchor is the point, in this local coordinate system, that matches the glyph 3D
 * position. For example, to center a string around the glyph 3D position, use (0, 0) as anchor.
 * To align the string to the right of the glyph 3D position, use (-1, -1) for example.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the anchors (x and y) of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph shifts.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the shifts (x and y) of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param coords the x,y,w,h texture coordinates
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags);



/**
 * Set the glyph group size, in pixels (size of each string).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the glyph group shapes (width and height, in pixels)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_group_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the glyph scaling applied to the size of all individual glyphs.
 *
 * We assume that the scaling is the same within each string (group of glyphs).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the scaling of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_scale(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the glyph angles.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the angles of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the glyph colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the glyph background color.
 *
 * @param visual the visual
 * @param bgcolor the background color
 */
DVZ_EXPORT void dvz_glyph_bgcolor(DvzVisual* visual, DvzColor bgcolor);



/**
 * Assign a texture to a glyph visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_glyph_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Associate an atlas and font with a glyph visual.
 *
 * @param visual the visual
 * @param af the atlas font
 */
DVZ_EXPORT void dvz_glyph_atlas_font(DvzVisual* visual, DvzAtlasFont* af);



/**
 * Set the glyph unicode code points.
 *
 * @param visual the visual
 * @param count the number of glyphs
 * @param codepoints the unicode codepoints
 */
DVZ_EXPORT void dvz_glyph_unicode(DvzVisual* visual, uint32_t count, uint32_t* codepoints);



/**
 * Set the glyph ascii characters.
 *
 * @param visual the visual
 * @param string the characters
 */
DVZ_EXPORT void dvz_glyph_ascii(DvzVisual* visual, const char* string);



/**
 * Set the xywh parameters of each glyph.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the xywh values of each glyph
 * @param offset the xy offsets of each glyph
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_glyph_xywh(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, vec2 offset, int flags);



/**
 * Helper function to easily set multiple strings of the same size and color on a glyph visual.
 *
 * @param visual the visual
 * @param string_count the number of strings
 * @param strings the strings
 * @param positions the positions of each string
 * @param scales the scaling of each string
 * @param color the same color for all strings
 * @param offset the same offset for all strings
 * @param anchor the same anchor for all strings
 */
DVZ_EXPORT
void dvz_glyph_strings(
    DvzVisual* visual, uint32_t string_count, char** strings, vec3* positions, float* scales,
    DvzColor color, vec2 offset, vec2 anchor);



/*************************************************************************************************/
/*  Monoglyph visual                                                                             */
/*************************************************************************************************/

/**
 * Create a monoglyph visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_monoglyph(DvzBatch* batch, int flags);



/**
 * Set the glyph positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_monoglyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the glyph offsets.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the glyph offsets (ivec2 integers: row,column)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_monoglyph_offset(DvzVisual* visual, uint32_t first, uint32_t count, ivec2* values, int flags);



/**
 * Set the glyph colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_monoglyph_color(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the text.
 *
 * @param visual the visual
 * @param first the first item
 * @param text the ASCII test (string length without the null terminal byte = number of glyphs)
 * @param flags the upload flags
 */
DVZ_EXPORT void
dvz_monoglyph_glyph(DvzVisual* visual, uint32_t first, const char* text, int flags);



/**
 * Set the glyph anchor (relative to the glyph size).
 *
 * @param visual the visual
 * @param anchor the anchor
 */
DVZ_EXPORT void dvz_monoglyph_anchor(DvzVisual* visual, vec2 anchor);



/**
 * Set the glyph size (relative to the initial glyph size).
 *
 * @param visual the visual
 * @param size the glyph size
 */
DVZ_EXPORT void dvz_monoglyph_size(DvzVisual* visual, float size);



/**
 * All-in-one function for multiline text.
 *
 * @param visual the visual
 * @param pos the text position
 * @param color the text color
 * @param size the glyph size
 * @param text the text, can contain `\n` new lines
 */
DVZ_EXPORT void
dvz_monoglyph_textarea(DvzVisual* visual, vec3 pos, DvzColor color, float size, const char* text);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_monoglyph_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

/**
 * Create an image visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzBatch* batch, int flags);



/**
 * Set the image positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the top left corner
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the image sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes of each image, in pixels
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the image anchors.
 *
 * The anchor determines the relationship between the image 3D position, and the position of the
 * image on the screen. Each images comes with a local coordinate system extending from (-1, -1)
 * (bottom-left corner) to (+1, +1) (top-right corner), and (0, 0) refers to the center of the
 * image. The anchor is the point, in this local coordinate system, that matches the image 3D
 * position. For example, to center an image around the image 3D position, use (0, 0) as anchor.
 * To align the image to the right and bottom of the image 3D position, so that this position
 * refers to the top-left corner, use (-1, +1) as anchor.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the relative anchors of each image, (0,0 = position pertains to top left corner)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Set the image texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param tl_br the tex coordinates of the top left and bottom right corners (vec4 u0,v0,u1,v1)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* tl_br, int flags);



/**
 * Set the image colors (only when using DVZ_IMAGE_FLAGS_FILL).
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the image colors
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_image_facecolor(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Assign a texture to an image visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_image_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the edge color.
 *
 * @param visual the visual
 * @param color the edge color
 */
DVZ_EXPORT void dvz_image_edgecolor(DvzVisual* visual, DvzColor color);



/**
 * Set the texture coordinates index permutation.
 *
 * @param visual the visual
 * @param ij index permutation
 */
DVZ_EXPORT void dvz_image_permutation(DvzVisual* visual, ivec2 ij);



/**
 * Set the edge width.
 *
 * @param visual the visual
 * @param width the edge width
 */
DVZ_EXPORT void dvz_image_linewidth(DvzVisual* visual, float width);



/**
 * Use a rounded rectangle for images, with a given radius in pixels.
 *
 * @param visual the visual
 * @param radius the rounded corner radius, in pixel
 */
DVZ_EXPORT void dvz_image_radius(DvzVisual* visual, float radius);



/**
 * Specify the colormap when using DVZ_IMAGE_FLAGS_MODE_COLORMAP.
 *
 * Only the following colormaps are available on the GPU at the moment:
 *
 * `CMAP_BINARY`
 * `CMAP_HSV`
 * `CMAP_CIVIDIS`
 * `CMAP_INFERNO`
 * `CMAP_MAGMA`
 * `CMAP_PLASMA`
 * `CMAP_VIRIDIS`
 * `CMAP_AUTUMN`
 * `CMAP_BONE`
 * `CMAP_COOL`
 * `CMAP_COPPER`
 * `CMAP_HOT`
 * `CMAP_SPRING`
 * `CMAP_SUMMER`
 * `CMAP_WINTER`
 * `CMAP_JET`
 *
 * @param visual the visual
 * @param cmap the colormap
 */
DVZ_EXPORT void dvz_image_colormap(DvzVisual* visual, DvzColormap cmap);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of images to allocate for this visual
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/

/**
 * Create a mesh visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzBatch* batch, int flags);



/**
 * Set the mesh vertex positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D vertex positions
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the mesh colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the vertex colors
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags);



/**
 * Set the mesh texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the vertex texture coordinates (vec4 u,v,*,alpha)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Set the mesh normals.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the vertex normal vectors
 * @param flags the data update flags
 */
DVZ_EXPORT
void dvz_mesh_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the isolines values.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the scalar field for which to draw isolines
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_isoline(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the distance between the current vertex to the left edge at corner A, B, or C in triangle
 * ABC.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the distance to the left edge adjacent to each triangle vertex
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_left(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the distance between the current vertex to the right edge at corner A, B, or C in triangle
 * ABC.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the distance to the right edge adjacent to each triangle vertex
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_right(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the contour information for polygon contours.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values for vertex A, B, C, the least significant bit is 1 if the opposite edge is a
 * contour, and the second least significant bit is 1 if the corner is a contour
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_contour(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Assign a 2D texture to a mesh visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_mesh_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the mesh indices.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the face indices (three vertex indices per triangle)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param vertex_count the number of vertices
 * @param index_count the number of indices
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* visual, uint32_t vertex_count, uint32_t index_count);



/**
 * Set the light direction.
 *
 * @param visual the mesh
 * @param idx the light index (0, 1, 2, or 3)
 * @param dir the light direction
 */
DVZ_EXPORT void dvz_mesh_light_dir(DvzVisual* visual, uint32_t idx, vec3 dir);



/**
 * Set the light color.
 *
 * @param visual the mesh
 * @param idx the light index (0, 1, 2, or 3)
 * @param rgba the light color (rgba, but the a component is ignored)
 */
DVZ_EXPORT void dvz_mesh_light_color(DvzVisual* visual, uint32_t idx, DvzColor rgba);



/**
 * Set the light parameters.
 *
 * @param visual the mesh
 * @param idx the light index (0, 1, 2, or 3)
 * @param params the light parameters (vec4 ambient, diffuse, specular, exponent)
 */
DVZ_EXPORT void dvz_mesh_light_params(DvzVisual* visual, uint32_t idx, vec4 params);



/**
 * Set the marker edge color.
 *
 * Note: the alpha component is currently unused.
 *
 * @param visual the mesh
 * @param rgba the rgba components
 */
DVZ_EXPORT void dvz_mesh_edgecolor(DvzVisual* visual, DvzColor rgba);



/**
 * Set the mesh contour linewidth (wireframe or isoline).
 *
 * @param visual the mesh
 * @param linewidth the line width
 */
DVZ_EXPORT void dvz_mesh_linewidth(DvzVisual* visual, float linewidth);



/**
 * Set the number of isolines
 *
 * @param visual the mesh
 * @param count the number of isolines
 */
DVZ_EXPORT void dvz_mesh_density(DvzVisual* visual, uint32_t count);



/**
 * Create a mesh out of a shape.
 *
 * @param batch the batch
 * @param shape the shape
 * @param flags the visual creation flags
 * @returns the mesh
 */
DVZ_EXPORT DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape, int flags);



/**
 * Update a mesh once a shape has been updated.
 *
 * @param visual the mesh
 * @param shape the shape
 */
DVZ_EXPORT void dvz_mesh_reshape(DvzVisual* visual, DvzShape* shape);



/*************************************************************************************************/
/*  Sphere                                                                                  */
/*************************************************************************************************/

/**
 * Create a sphere visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_sphere(DvzBatch* batch, int flags);



/**
 * Set the sphere positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param pos the 3D positions of the sphere centers
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* pos, int flags);



/**
 * Set the sphere colors.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param color the sphere colors
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* color, int flags);



/**
 * Set the sphere sizes.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param size the radius of the spheres
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* size, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of spheres to allocate for this visual
 */
DVZ_EXPORT void dvz_sphere_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Set the sphere light position.
 *
 * @param visual the visual
 * @param pos the light position
 */
DVZ_EXPORT void dvz_sphere_light_pos(DvzVisual* visual, vec3 pos);



/**
 * Set the sphere light parameters.
 *
 * @param visual the visual
 * @param params the light parameters (vec4 ambient, diffuse, specular, exponent)
 */
DVZ_EXPORT void dvz_sphere_light_params(DvzVisual* visual, vec4 params);



/*************************************************************************************************/
/*  Volume                                                                                       */
/*************************************************************************************************/

/**
 * Create a volume visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_volume(DvzBatch* batch, int flags);



/**
 * Assign a 3D texture to a volume visual.
 *
 * @param visual the visual
 * @param texture the 3D texture
 */
DVZ_EXPORT void dvz_volume_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Set the volume bounds.
 *
 * @param visual the visual
 * @param xlim xmin and xmax
 * @param ylim ymin and ymax
 * @param zlim zmin and zmax
 */
DVZ_EXPORT void dvz_volume_bounds(DvzVisual* visual, vec2 xlim, vec2 ylim, vec2 zlim);



/**
 * Set the texture coordinates of two corner points.
 *
 * @param visual the visual
 * @param uvw0 coordinates of one of the corner points
 * @param uvw1 coordinates of one of the corner points
 */
DVZ_EXPORT void dvz_volume_texcoords(DvzVisual* visual, vec3 uvw0, vec3 uvw1);



/**
 * Set the texture coordinates index permutation.
 *
 * @param visual the visual
 * @param ijk index permutation
 */
DVZ_EXPORT void dvz_volume_permutation(DvzVisual* visual, ivec3 ijk);



/**
 * Set the bounding box face index on which to slice (showing the texture itself).
 *
 * @param visual the visual
 * @param face_index -1 to disable, or the face index between 0 and 5 included
 */
DVZ_EXPORT void dvz_volume_slice(DvzVisual* visual, int32_t face_index);



/**
 * Set the volume size.
 *
 * @param visual the visual
 * @param transfer transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor
 */
DVZ_EXPORT void dvz_volume_transfer(DvzVisual* visual, vec4 transfer);



/*************************************************************************************************/
/*  Slice                                                                                        */
/*************************************************************************************************/

/**
 * Create a slice visual (multiple 2D images with slices of a 3D texture).
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_slice(DvzBatch* batch, int flags);



/**
 * Set the slice positions.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param p0 the 3D positions of the top left corner
 * @param p1 the 3D positions of the top right corner
 * @param p2 the 3D positions of the bottom left corner
 * @param p3 the 3D positions of the bottom right corner
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_slice_position(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    vec3* p0, vec3* p1, vec3* p2, vec3* p3, int flags);



/**
 * Set the slice texture coordinates.
 *
 * @param visual the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param uvw0 the 3D texture coordinates of the top left corner
 * @param uvw1 the 3D texture coordinates of the top right corner
 * @param uvw2 the 3D texture coordinates of the bottom left corner
 * @param uvw3 the 3D texture coordinates of the bottom right corner
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_slice_texcoords(
    DvzVisual* visual, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags);



/**
 * Assign a texture to a slice visual.
 *
 * @param visual the visual
 * @param texture the texture
 */
DVZ_EXPORT void dvz_slice_texture(DvzVisual* visual, DvzTexture* texture);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of slices to allocate for this visual
 */
DVZ_EXPORT void dvz_slice_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Create a 3D texture to be used in a slice visual.
 *
 * @param batch the batch
 * @param format the texture format
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth
 * @param data the texture data to upload
 * @returns the texture ID
 */
DVZ_EXPORT DvzId dvz_tex_slice(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth,
    void* data);



/**
 * Set the slice transparency alpha value.
 *
 * @param visual the visual
 * @param alpha the alpha value
 */
DVZ_EXPORT void dvz_slice_alpha(DvzVisual* visual, float alpha);



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
 * Set a camera lookat position.
 *
 * @param camera the camera
 * @param lookat the lookat position
 */
DVZ_EXPORT void dvz_camera_lookat(DvzCamera* camera, vec3 lookat);



/**
 * Set a camera up vector.
 *
 * @param camera the camera
 * @param up the up vector
 */
DVZ_EXPORT void dvz_camera_up(DvzCamera* camera, vec3 up);



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
DVZ_EXPORT bool dvz_panzoom_mouse(DvzPanzoom* pz, DvzMouseEvent ev);



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
dvz_ref_transform1D(DvzRef* ref, DvzDim dim, uint32_t count, double* pos, vec3* pos_tr);



/**
 * Transform 2D data from the reference frame to normalized device coordinates [-1..+1].
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 2D positions
 * @param[out] pos_tr (array) the transformed 3D positions
 */
DVZ_EXPORT void dvz_ref_transform_2D(DvzRef* ref, uint32_t count, dvec2* pos, vec3* pos_tr);



/**
 * Transform 2D data from the reference frame to normalized device coordinates [-1..+1] in 2D.
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 2D positions
 * @param[out] pos_tr (array) the transformed 2D positions
 */
DVZ_EXPORT void dvz_ref_transform_polygon(DvzRef* ref, uint32_t count, dvec2* pos, dvec2* pos_tr);



/**
 * Transform 3D data from the reference frame to normalized device coordinates [-1..+1].
 *
 * @param ref the reference frame
 * @param count the number of positions
 * @param pos the 3D positions
 * @param[out] pos_tr (array) the transformed positions
 */
DVZ_EXPORT void dvz_ref_transform_3D(DvzRef* ref, uint32_t count, dvec3* pos, vec3* pos_tr);



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



/*************************************************************************************************/
/*************************************************************************************************/
/*  GUI functions                                                                                */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Capture a GUI window.
 *
 * @param gui_window the GUI window
 * @param is_captured whether the windows should be captured
 *
 */
DVZ_EXPORT void dvz_gui_window_capture(DvzGuiWindow* gui_window, bool is_captured);



/**
 * Return whether a dialog is being moved.
 *
 * @returns whether the dialog is being moved
 */
DVZ_EXPORT bool dvz_gui_moving(void);



/**
 * Return whether a dialog is being resized
 *
 * @returns whether the dialog is being resized
 */
DVZ_EXPORT bool dvz_gui_resizing(void);



/**
 * Return whether a dialog has just moved.
 *
 * @returns whether the dialog has just moved
 */
DVZ_EXPORT bool dvz_gui_moved(void);



/**
 * Return whether a dialog has just been resized.
 *
 * @returns whether the dialog has just been resized
 */
DVZ_EXPORT bool dvz_gui_resized(void);



/**
 * Return whether a dialog is collapsed.
 *
 * @returns whether the dialog is collapsed
 */
DVZ_EXPORT bool dvz_gui_collapsed(void);



/**
 * Return whether a dialog has just been collapsed or uncollapsed.
 *
 * @returns whether the dialog has just been collapsed or uncollapsed.
 */
DVZ_EXPORT bool dvz_gui_collapse_changed(void);



/**
 * Return whether mouse is dragging.
 *
 * @returns whether the mouse is dragging
 */
DVZ_EXPORT bool dvz_gui_dragging(void);



/**
 * Set the position of the next GUI dialog.
 *
 * @param pos the dialog position
 * @param pivot the pivot
 *
 */
DVZ_EXPORT void dvz_gui_pos(vec2 pos, vec2 pivot);



/**
 * Set a fixed position for a GUI dialog.
 *
 * @param pos the dialog position
 * @param pivot the pivot
 *
 */
DVZ_EXPORT void dvz_gui_fixed(vec2 pos, vec2 pivot);



/**
 * Get the position and size of the current dialog.
 *
 * @param viewport the x, y, w, h values
 *
 */
DVZ_EXPORT void dvz_gui_viewport(vec4 viewport);



/**
 * Set the corner position of the next GUI dialog.
 *
 * @param corner which corner
 * @param pad the pad
 *
 */
DVZ_EXPORT void dvz_gui_corner(DvzCorner corner, vec2 pad);



/**
 * Set the size of the next GUI dialog.
 *
 * @param size the size
 *
 */
DVZ_EXPORT void dvz_gui_size(vec2 size);



/**
 * Set the color of an element.
 *
 * @param type the element type for which to change the color
 * @param color the color
 *
 */
DVZ_EXPORT void dvz_gui_color(int type, DvzColor color);



/**
 * Set the style of an element.
 *
 * @param type the element type for which to change the style
 * @param value the value
 *
 */
DVZ_EXPORT void dvz_gui_style(int type, float value);



/**
 * Set the flags of the next GUI dialog.
 *
 * @param flags the flags
 *
 */
DVZ_EXPORT int dvz_gui_flags(int flags);



/**
 * Set the alpha transparency of the next GUI dialog.
 *
 * @param alpha the alpha transparency value
 *
 */
DVZ_EXPORT void dvz_gui_alpha(float alpha);



/**
 * Start a new dialog.
 *
 * @param title the dialog title
 * @param flags the flags
 */
DVZ_EXPORT void dvz_gui_begin(const char* title, int flags);



/**
 * Add a text item in a dialog.
 *
 * @param fmt the format string
 */
DVZ_EXPORT void dvz_gui_text(const char* fmt, ...);



/**
 * Add a text box in a dialog.
 *
 * @param label the label
 * @param str_len the size of the str buffer
 * @param str the modified string
 * @param flags the flags
 * @returns whether the text has changed
 */
DVZ_EXPORT bool dvz_gui_textbox(const char* label, uint32_t str_len, char* str, int flags);



/**
 * Add a slider.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider(const char* name, float vmin, float vmax, float* value);



/**
 * Add a button.
 *
 * @param name the button name
 * @param width the button width
 * @param height the button height
 * @returns whether the button was pressed
 */
DVZ_EXPORT bool dvz_gui_button(const char* name, float width, float height);



/**
 * Add a checkbox.
 *
 * @param name the button name
 * @param[out] checked whether the checkbox is checked
 * @returns whether the checkbox's state has changed
 */
DVZ_EXPORT bool dvz_gui_checkbox(const char* name, bool* checked);



/**
 * Add a dropdown menu.
 *
 * @param name the menu name
 * @param count the number of menu items
 * @param items the item labels
 * @param[out] selected a pointer to the selected index
 * @param flags the dropdown menu flags
 */
DVZ_EXPORT bool dvz_gui_dropdown(
    const char* name, uint32_t count, const char** items, uint32_t* selected, int flags);



/**
 * Add a progress widget.
 *
 * @param fraction the fraction between 0 and 1
 * @param width the widget width
 * @param height the widget height
 * @param fmt the format string
 */
DVZ_EXPORT void dvz_gui_progress(float fraction, float width, float height, const char* fmt, ...);



/**
 * Add an image in a GUI dialog.
 *
 * @param tex the texture
 * @param width the image width
 * @param height the image height
 */
DVZ_EXPORT void dvz_gui_image(DvzTex* tex, float width, float height);



/**
 * Add a color picker
 *
 * @param name the widget name
 * @param color the color
 * @param flags the widget flags
 */
DVZ_EXPORT bool dvz_gui_colorpicker(const char* name, vec3 color, int flags);



/**
 * Start a new tree node.
 *
 * @param name the widget name
 */
DVZ_EXPORT bool dvz_gui_node(const char* name);



/**
 * Close the current tree node.
 */
DVZ_EXPORT void dvz_gui_pop(void);



/**
 * Close the current tree node.
 */
DVZ_EXPORT bool dvz_gui_clicked(void);



/**
 * Close the current tree node.
 *
 * @param name the widget name
 */
DVZ_EXPORT bool dvz_gui_selectable(const char* name);



/**
 * Display a table with selectable rows.
 *
 * @param name the widget name
 * @param row_count the number of rows
 * @param column_count the number of columns
 * @param labels all cell labels
 * @param selected a pointer to an array of boolean indicated which rows are selected
 * @param flags the Dear ImGui flags
 * @returns whether the row selection has changed (in the selected array)
 */
DVZ_EXPORT bool dvz_gui_table(                                   //
    const char* name, uint32_t row_count, uint32_t column_count, //
    const char** labels, bool* selected, int flags);



/**
 * Display a collapsible tree. Assumes the data is in the right order, with level encoding the
 * depth of each row within the tree.
 *
 * Filtering can be implemented with the "visible" parameter. Note that this function automatically
 * propagates the visibility of each node to all its descendents and ascendents, without modifying
 * in-place the "visible" array.
 *
 * @param count the number of rows
 * @param ids short id of each row
 * @param labels full label of each row
 * @param levels a positive integer indicate
 * @param colors the color of each square in each row
 * @param folded whether each row is currently folded (modified by this function)
 * @param selected whether each row is currently selected (modified by this function)
 * @param visible whether each row is visible (used for filtering)
 * @returns whether the selection has changed
 */
DVZ_EXPORT bool dvz_gui_tree(
    uint32_t count, char** ids, char** labels, uint32_t* levels, DvzColor* colors, bool* folded,
    bool* selected, bool* visible);



/**
 * Show the demo GUI.
 */
DVZ_EXPORT void dvz_gui_demo(void);



/**
 * Stop the creation of the dialog.
 */
DVZ_EXPORT void dvz_gui_end(void);



EXTERN_C_OFF

#endif
