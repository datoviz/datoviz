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

#include "datoviz_enums.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define DATOVIZ_VERSION_MAJOR 0
#define DATOVIZ_VERSION_MINOR 2
#define DATOVIZ_VERSION_PATCH 0

#ifndef DVZ_EXPORT
#if CC_MSVC
#ifdef DVZ_SHARED
#define DVZ_EXPORT __declspec(dllexport)
#else
#define DVZ_EXPORT __declspec(dllimport)
#endif
#define DVZ_INLINE __forceinline
#else
#define DVZ_EXPORT __attribute__((visibility("default")))
#define DVZ_INLINE static inline __attribute((always_inline))
#endif
#endif



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzBatch DvzBatch;
typedef struct DvzScene DvzScene;
typedef struct DvzFigure DvzFigure;
typedef struct DvzPanel DvzPanel;
typedef struct DvzVisual DvzVisual;
typedef struct DvzTransform DvzTransform;
typedef struct DvzMVP DvzMVP;
typedef struct DvzCamera DvzCamera;
typedef struct DvzArcball DvzArcball;
typedef struct DvzPanzoom DvzPanzoom;

typedef struct DvzShape DvzShape;
typedef struct DvzAtlas DvzAtlas;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzShape
{
    DvzShapeType type;
    uint32_t vertex_count;
    uint32_t index_count;
    vec3* pos;
    vec3* normal;
    cvec4* color;
    vec4* texcoords; // u, v, *, a
    DvzIndex* index;
};



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
 * Start the event loop and render the scene in a window.
 *
 * @param scene the scene
 * @param app the app
 * @param n_frames the maximum number of frames, 0 for infinite loop
 */
DVZ_EXPORT void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t n_frames);



/**
 * Destroy a scene.
 *
 * @param scene the scene
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

/**
 * Create a figure, a desktop window with panels and visuals.
 *
 * @param scene the scene
 * @param width the window's width
 * @param height the window's height
 * @param flags the figure creation flags (not yet stabilized)
 * @returns the figure
 */
DVZ_EXPORT DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, int flags);



/**
 * Resize a figure.
 *
 * @param scene the scene
 * @param width the window's width
 * @param height the window's height
 */
DVZ_EXPORT void dvz_figure_resize(DvzFigure* fig, uint32_t width, uint32_t height);



/**
 * Get a figure from its id.
 *
 * @param scene the scene
 * @param id the figure's id
 * @returns the figure
 */
DvzFigure* dvz_scene_figure(DvzScene* scene, DvzId id);



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
 * @param x the x coordinate of the top-left corner, in pixels
 * @param y the y coordinate of the top-left corner, in pixels
 * @param width the panel's width, in pixels
 * @param height the panel's height, in pixels
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float width, float height);



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
 * Resize a panel.
 *
 * @param panel the panel
 * @param x the x coordinate of the top-left corner, in pixels
 * @param y the y coordinate of the top-left corner, in pixels
 * @param width the panel's width, in pixels
 * @param height the panel's height, in pixels
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
 * Return the Camera of a panel, if there is one.
 *
 * @param panel the panel
 * @returns the camera, or NULL if there is none
 */
DVZ_EXPORT DvzCamera* dvz_panel_camera(DvzPanel* panel);


/**
 * Return the Panzoom of a panel, if there is one.
 *
 * @param panel the panel
 * @returns the panzoom, or NULL if there is none
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzScene* scene, DvzPanel* panel);


/**
 * Return the Arcball of a panel, if there is one.
 *
 * @param panel the panel
 * @returns the panzoom, or NULL if there is none
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzScene* scene, DvzPanel* panel);



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
 */
DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual);



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



/*************************************************************************************************/
/*  Colormap functions                                                                           */
/*************************************************************************************************/

/**
 * Fetch a color from a colormap and a value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] color the fetched color
 */
DVZ_EXPORT void dvz_colormap(DvzColormap cmap, uint8_t value, cvec4 color);



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
dvz_colormap_scale(DvzColormap cmap, double value, double vmin, double vmax, cvec4 color);



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_shape_print(DvzShape* shape);



/**
 * Function.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_destroy(DvzShape* shape);



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzShape dvz_shape_square(cvec4 color);



/**
 * Function.
 *
 * @param count the count
 * @param color the color
 * @returns DvzShape
 */
DVZ_EXPORT DvzShape dvz_shape_disc(uint32_t count, cvec4 color);



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzShape dvz_shape_cube(cvec4* colors);



/**
 * Function.
 *
 * @param rows the rows
 * @param cols the cols
 * @param color the color
 * @returns DvzShape
 */
DVZ_EXPORT DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, cvec4 color);



/**
 * Function.
 *
 * @param count the count
 * @param color the color
 * @returns DvzShape
 */
DVZ_EXPORT DvzShape dvz_shape_cone(uint32_t count, cvec4 color);



/**
 * Function.
 *
 * @param count the count
 * @param color the color
 * @returns DvzShape
 */
DVZ_EXPORT DvzShape dvz_shape_cylinder(uint32_t count, cvec4 color);



/*************************************************************************************************/
/*  Basic visual                                                                                 */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_basic(DvzBatch* batch, DvzPrimitiveTopology topology, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_basic_position(DvzVisual* basic, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param basic the basic
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_basic_color(DvzVisual* basic, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param basic the basic
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_basic_alloc(DvzVisual* basic, uint32_t item_count);



/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_pixel(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_pixel_position(DvzVisual* pixel, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param pixel the pixel
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param pixel the pixel
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_pixel_alloc(DvzVisual* pixel, uint32_t item_count);



/*************************************************************************************************/
/*  Point                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_point(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_point_position(DvzVisual* point, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param point the point
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_point_color(DvzVisual* point, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param point the point
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_point_size(DvzVisual* point, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Function.
 *
 * @param point the point
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_point_alloc(DvzVisual* point, uint32_t item_count);



/*************************************************************************************************/
/*  Marker                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_marker(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_marker_mode(DvzVisual* visual, DvzMarkerMode mode);



/**
 * Function.
 *
 * @param visual the visual
 * @param aspect the aspect
 */
DVZ_EXPORT void dvz_marker_aspect(DvzVisual* visual, DvzMarkerAspect aspect);



/**
 * Function.
 *
 * @param visual the visual
 * @param shape the shape
 */
DVZ_EXPORT void dvz_marker_shape(DvzVisual* visual, DvzMarkerShape shape);



/**
 * Function.
 *
 * @param marker the marker
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_marker_position(DvzVisual* marker, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param marker the marker
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_marker_size(DvzVisual* marker, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Function.
 *
 * @param marker the marker
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_marker_angle(DvzVisual* marker, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Function.
 *
 * @param marker the marker
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_marker_color(DvzVisual* marker, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param value the value
 */
DVZ_EXPORT void dvz_marker_edge_color(DvzVisual* visual, cvec4 value);



/**
 * Function.
 *
 * @param visual the visual
 * @param value the value
 */
DVZ_EXPORT void dvz_marker_edge_width(DvzVisual* visual, float value);



/**
 * Function.
 *
 * @param visual the visual
 * @param tex the tex
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_marker_tex(DvzVisual* visual, DvzId tex, DvzId sampler);



/**
 * Function.
 *
 * @param visual the visual
 * @param value the value
 */
DVZ_EXPORT void dvz_marker_tex_scale(DvzVisual* visual, float value);



/**
 * Function.
 *
 * @param marker the marker
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_marker_alloc(DvzVisual* marker, uint32_t item_count);



/*************************************************************************************************/
/*  Segment                                                                                      */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_segment(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_position(
    DvzVisual* segment, uint32_t first, uint32_t count, vec3* initial, vec3* terminal, int flags);



/**
 * Function.
 *
 * @param segment the segment
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_segment_shift(DvzVisual* segment, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Function.
 *
 * @param segment the segment
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param segment the segment
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void dvz_segment_linewidth(
    DvzVisual* segment, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Function.
 *
 * @param segment the segment
 * @param first the first
 * @param count the count
 * @param initial the initial
 * @param terminal the terminal
 * @param flags the flags
 */
DVZ_EXPORT void dvz_segment_cap(
    DvzVisual* segment, uint32_t first, uint32_t count, DvzCapType* initial, DvzCapType* terminal,
    int flags);



/**
 * Function.
 *
 * @param segment the segment
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_segment_alloc(DvzVisual* segment, uint32_t item_count);



/*************************************************************************************************/
/*  Path                                                                                         */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_path(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_path_position(
    DvzVisual* visual, uint32_t vertex_count, vec3* positions, //
    uint32_t path_count, uint32_t* path_lengths, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_path_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param value the value
 */
DVZ_EXPORT void dvz_path_linewidth(DvzVisual* visual, float value);



/**
 * Function.
 *
 * @param visual the visual
 * @param total_vertex_count the total_vertex_count
 */
DVZ_EXPORT void dvz_path_alloc(DvzVisual* visual, uint32_t total_vertex_count);



/*************************************************************************************************/
/*  Glyph                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_glyph(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_axis(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param coords the coords
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_glyph_groupsize(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param bgcolor the bgcolor
 */
DVZ_EXPORT void dvz_glyph_bgcolor(DvzVisual* visual, vec4 bgcolor);



/**
 * Function.
 *
 * @param visual the visual
 * @param tex the tex
 */
DVZ_EXPORT void dvz_glyph_texture(DvzVisual* visual, DvzId tex);



/**
 * Function.
 *
 * @param visual the visual
 * @param atlas the atlas
 */
DVZ_EXPORT void dvz_glyph_atlas(DvzVisual* visual, DvzAtlas* atlas);



/**
 * Function.
 *
 * @param visual the visual
 * @param count the count
 * @param codepoints the codepoints
 */
DVZ_EXPORT void dvz_glyph_unicode(DvzVisual* visual, uint32_t count, uint32_t* codepoints);



/**
 * Function.
 *
 * @param visual the visual
 * @param char the char
 */
DVZ_EXPORT void dvz_glyph_ascii(DvzVisual* visual, const char* string);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param values the values
 * @param offset the offset
 * @param flags the flags
 */
DVZ_EXPORT void dvz_glyph_xywh(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, vec2 offset, int flags);



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_image(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_image_position(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 * Function.
 *
 * @param image the image
 * @param first the first
 * @param count the count
 * @param ul_lr the ul_lr
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_image_texcoords(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param tex the tex
 * @param filter the filter
 * @param address_mode the address_mode
 */
DVZ_EXPORT void dvz_image_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Function.
 *
 * @param image the image
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* image, uint32_t item_count);



/**
 * Function.
 *
 * @param batch the batch
 * @param format the format
 * @param width the width
 * @param height the height
 * @param data the data
 * @returns DvzId
 */
DVZ_EXPORT DvzId
dvz_tex_image(DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, void* data);



/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_mesh(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_mesh_position(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param mesh the mesh
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_mesh_color(DvzVisual* mesh, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * vec4: u, v, <unused>, a
 */
DVZ_EXPORT void
dvz_mesh_texcoords(DvzVisual* mesh, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Function.
 *
 * @param mesh the mesh
 * @param first the first
 * @param count the count
 * @param values the values
 * @param flags the flags
 */
DVZ_EXPORT
void dvz_mesh_normal(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param tex the tex
 * @param filter the filter
 * @param address_mode the address_mode
 */
DVZ_EXPORT void dvz_mesh_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Function.
 *
 * @param mesh the mesh
 * @param first the first
 * @param count the count
 * @param values the values
 */
DVZ_EXPORT void dvz_mesh_index(DvzVisual* mesh, uint32_t first, uint32_t count, DvzIndex* values);



/**
 * Function.
 *
 * @param mesh the mesh
 * @param vertex_count the vertex_count
 * @param index_count the index_count
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* mesh, uint32_t vertex_count, uint32_t index_count);



/**
 * Function.
 *
 * @param mesh the mesh
 * @param pos the pos
 */
DVZ_EXPORT void dvz_mesh_light_pos(DvzVisual* mesh, vec4 pos);



/**
 * Function.
 *
 * @param mesh the mesh
 * @param params the params
 */
DVZ_EXPORT void dvz_mesh_light_params(DvzVisual* mesh, vec4 params);



/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape, int flags);



/*************************************************************************************************/
/*  Fake sphere                                                                                  */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_fake_sphere(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_fake_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* pos, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param color the color
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_fake_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* color, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param first the first
 * @param count the count
 * @param size the size
 * @param flags the flags
 */
DVZ_EXPORT void
dvz_fake_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* size, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_fake_sphere_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Function.
 *
 * @param visual the visual
 * @param pos the pos
 */
DVZ_EXPORT void dvz_fake_sphere_light_pos(DvzVisual* visual, vec3 pos);



/*************************************************************************************************/
/*  Volume                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_volume(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_volume_alloc(DvzVisual* volume, uint32_t item_count);



/**
 * Function.
 *
 * @param visual the visual
 * @param tex the tex
 * @param filter the filter
 * @param address_mode the address_mode
 */
DVZ_EXPORT void dvz_volume_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Function.
 *
 * @param visual the visual
 * @param w the w
 * @param h the h
 * @param d the d
 */
DVZ_EXPORT void dvz_volume_size(DvzVisual* visual, float w, float h, float d);



/**
 * Function.
 *
 * @param batch the batch
 * @param format the format
 * @param width the width
 * @param height the height
 * @param depth the depth
 * @param data the data
 * @returns DvzId
 */
DVZ_EXPORT DvzId dvz_tex_volume(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth,
    void* data);



/*************************************************************************************************/
/*  Slice                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_slice(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_slice_position(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* p0, vec3* p1, vec3* p2, vec3* p3, int flags);



/**
 * Function.
 *
 * @param slice the slice
 * @param first the first
 * @param count the count
 * @param uvw1 the uvw1
 * @param uvw2 the uvw2
 * @param uvw3 the uvw3
 * @param flags the flags
 */
DVZ_EXPORT void dvz_slice_texcoords(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags);



/**
 * Function.
 *
 * @param visual the visual
 * @param tex the tex
 * @param filter the filter
 * @param address_mode the address_mode
 */
DVZ_EXPORT void dvz_slice_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Function.
 *
 * @param slice the slice
 * @param item_count the item_count
 */
DVZ_EXPORT void dvz_slice_alloc(DvzVisual* slice, uint32_t item_count);



/**
 * Function.
 *
 * @param batch the batch
 * @param format the format
 * @param width the width
 * @param height the height
 * @param depth the depth
 * @param data the data
 * @returns DvzId
 */
DVZ_EXPORT DvzId dvz_tex_slice(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth,
    void* data);



/**
 * Function.
 *
 * @param visual the visual
 * @param alpha the alpha
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
 *
 */
DVZ_EXPORT double dvz_resample(double t0, double t1, double t);



/**
 * Function.
 *
 * @param easing the easing
 * @param t the t
 * @returns double
 */
DVZ_EXPORT double dvz_easing(DvzEasing easing, double t);



/**
 * Function.
 *
 * @param center the center
 * @param radius the radius
 * @param angle the angle
 * @param t the t
 * @param out the out
 */
DVZ_EXPORT void dvz_circular_2D(vec2 center, float radius, float angle, float t, vec2 out);



/**
 * Function.
 *
 * @param center the center
 * @param u the u
 * @param v the v
 * @param radius the radius
 * @param angle the angle
 * @param t the t
 * @param out the out
 */
DVZ_EXPORT void
dvz_circular_3D(vec3 center, vec3 u, vec3 v, float radius, float angle, float t, vec3 out);



/**
 * Function.
 *
 * @param p0 the p0
 * @param p1 the p1
 * @param t the t
 * @returns float
 */
DVZ_EXPORT float dvz_interpolate(float p0, float p1, float t);



/**
 * Function.
 *
 * @param p0 the p0
 * @param p1 the p1
 * @param t the t
 * @param out the out
 */
DVZ_EXPORT void dvz_interpolate_2D(vec2 p0, vec2 p1, float t, vec2 out);



/**
 * Function.
 *
 * @param p0 the p0
 * @param p1 the p1
 * @param t the t
 * @param out the out
 */
DVZ_EXPORT void dvz_interpolate_3D(vec3 p0, vec3 p1, float t, vec3 out);



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzArcball* dvz_arcball(float width, float height, int flags); // inner viewport size



/**
 *
 */
DVZ_EXPORT void dvz_arcball_initial(DvzArcball* arcball, vec3 angles);



/**
 * Function.
 *
 * @param pz the pz
 */
DVZ_EXPORT void dvz_arcball_reset(DvzArcball* pz);



/**
 * Function.
 *
 * @param pz the pz
 * @param width the width
 * @param height the height
 */
DVZ_EXPORT void dvz_arcball_resize(DvzArcball* pz, float width, float height);



/**
 * Function.
 *
 * @param pz the pz
 * @param flags the flags
 */
DVZ_EXPORT void dvz_arcball_flags(DvzArcball* pz, int flags);



/**
 * Function.
 *
 * @param pz the pz
 * @param constrain the constrain
 */
DVZ_EXPORT void dvz_arcball_constrain(DvzArcball* pz, vec3 constrain);



/**
 * Function.
 *
 * @param arcball the arcball
 * @param angles the angles
 */
DVZ_EXPORT void dvz_arcball_set(DvzArcball* arcball, vec3 angles);



/**
 * Function.
 *
 * @param arcball the arcball
 * @param out_angles the out_angles
 */
DVZ_EXPORT void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles);



/**
 * Function.
 *
 * @param arcball the arcball
 * @param cur_pos the cur_pos
 * @param last_pos the last_pos
 */
DVZ_EXPORT void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);



// DVZ_EXPORT void dvz_arcball_pan(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);

// DVZ_EXPORT void dvz_arcball_lock(DvzArcball* arcball, vec3 dir);



/**
 * Function.
 *
 * @param arcball the arcball
 * @param model the model
 */
DVZ_EXPORT void dvz_arcball_model(DvzArcball* arcball, mat4 model);



/**
 * Function.
 *
 * @param arcball the arcball
 */
DVZ_EXPORT void dvz_arcball_end(DvzArcball* arcball);



/**
 * Function.
 *
 * @param pz the pz
 * @param mvp the mvp
 */
DVZ_EXPORT void dvz_arcball_mvp(DvzArcball* pz, DvzMVP* mvp);



/**
 * Function.
 *
 * @param arcball the arcball
 */
DVZ_EXPORT void dvz_arcball_print(DvzArcball* arcball);



/**
 * Function.
 *
 * @param pz the pz
 */
DVZ_EXPORT void dvz_arcball_destroy(DvzArcball* pz);



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzCamera* dvz_camera(float width, float height, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_camera_initial(DvzCamera* camera, vec3 pos, vec3 lookat, vec3 up);



/**
 * Function.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_reset(DvzCamera* camera);



/**
 * Function.
 *
 * @param camera the camera
 * @param near the near
 * @param far the far
 */
DVZ_EXPORT void dvz_camera_zrange(DvzCamera* camera, float near, float far);



/**
 * Function.
 *
 * @param camera the camera
 * @param left the left
 * @param right the right
 * @param bottom the bottom
 * @param top the top
 */
DVZ_EXPORT void
dvz_camera_ortho(DvzCamera* camera, float left, float right, float bottom, float top);



/**
 * Function.
 *
 * @param camera the camera
 * @param width the width
 * @param height the height
 */
DVZ_EXPORT void dvz_camera_resize(DvzCamera* camera, float width, float height);



/**
 * Function.
 *
 * @param camera the camera
 * @param pos the pos
 */
DVZ_EXPORT void dvz_camera_position(DvzCamera* camera, vec3 pos);



/**
 * Function.
 *
 * @param camera the camera
 * @param lookat the lookat
 */
DVZ_EXPORT void dvz_camera_lookat(DvzCamera* camera, vec3 lookat);



/**
 * Function.
 *
 * @param camera the camera
 * @param up the up
 */
DVZ_EXPORT void dvz_camera_up(DvzCamera* camera, vec3 up);



/**
 * Function.
 *
 * @param camera the camera
 * @param fov the fov
 */
DVZ_EXPORT void dvz_camera_perspective(DvzCamera* camera, float fov);
// field of view angle (in radians)



/**
 * Function.
 *
 * @param camera the camera
 * @param view the view
 * @param proj the proj
 */
DVZ_EXPORT void dvz_camera_viewproj(DvzCamera* camera, mat4 view, mat4 proj);



/**
 * Function.
 *
 * @param camera the camera
 * @param mvp the mvp
 */
DVZ_EXPORT void dvz_camera_mvp(DvzCamera* camera, DvzMVP* mvp);



/**
 * Function.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_print(DvzCamera* camera);



/**
 * Function.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_destroy(DvzCamera* camera);



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzPanzoom* dvz_panzoom(float width, float height, int flags); // inner viewport size



/**
 *
 */
DVZ_EXPORT void dvz_panzoom_reset(DvzPanzoom* pz);



/**
 * Function.
 *
 * @param pz the pz
 * @param width the width
 * @param height the height
 */
DVZ_EXPORT void dvz_panzoom_resize(DvzPanzoom* pz, float width, float height);



/**
 * Function.
 *
 * @param pz the pz
 * @param flags the flags
 */
DVZ_EXPORT void dvz_panzoom_flags(DvzPanzoom* pz, int flags);



/**
 * Function.
 *
 * @param pz the pz
 * @param xlim the xlim
 */
DVZ_EXPORT void dvz_panzoom_xlim(DvzPanzoom* pz, vec2 xlim); // FLOAT_MIN/MAX=no lim



/**
 * Function.
 *
 * @param pz the pz
 * @param ylim the ylim
 */
DVZ_EXPORT void dvz_panzoom_ylim(DvzPanzoom* pz, vec2 ylim);



/**
 * Function.
 *
 * @param pz the pz
 * @param zlim the zlim
 */
DVZ_EXPORT void dvz_panzoom_zlim(DvzPanzoom* pz, vec2 zlim);



/**
 * Function.
 *
 * @param pz the pz
 * @param pan the pan
 */
DVZ_EXPORT void dvz_panzoom_pan(DvzPanzoom* pz, vec2 pan); // in NDC, gets or sets



/**
 * Function.
 *
 * @param pz the pz
 * @param zoom the zoom
 */
DVZ_EXPORT void dvz_panzoom_zoom(DvzPanzoom* pz, vec2 zoom); // in NDC



/**
 * Function.
 *
 * @param pz the pz
 * @param shift_px the shift_px
 * @param center_px the center_px
 */
DVZ_EXPORT void dvz_panzoom_pan_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px);



/**
 * Function.
 *
 * @param pz the pz
 * @param shift_px the shift_px
 * @param center_px the center_px
 */
DVZ_EXPORT void dvz_panzoom_zoom_shift(DvzPanzoom* pz, vec2 shift_px, vec2 center_px);



/**
 * Function.
 *
 * @param pz the pz
 */
DVZ_EXPORT void dvz_panzoom_end(DvzPanzoom* pz); // end of pan or zoom interaction



/**
 * Function.
 *
 * @param pz the pz
 * @param dir the dir
 * @param center_px the center_px
 */
DVZ_EXPORT void dvz_panzoom_zoom_wheel(DvzPanzoom* pz, vec2 dir, vec2 center_px);



/**
 * Function.
 *
 * @param pz the pz
 * @param xrange the xrange
 */
DVZ_EXPORT void dvz_panzoom_xrange(DvzPanzoom* pz, vec2 xrange);
// if (0, 0), gets the xrange, otherwise sets it



/**
 * Function.
 *
 * @param pz the pz
 * @param yrange the yrange
 */
DVZ_EXPORT void dvz_panzoom_yrange(DvzPanzoom* pz, vec2 yrange);



/**
 * Function.
 *
 * @param pz the pz
 * @param mvp the mvp
 */
DVZ_EXPORT void dvz_panzoom_mvp(DvzPanzoom* pz, DvzMVP* mvp);



/**
 * Function.
 *
 * @param pz the pz
 */
DVZ_EXPORT void dvz_panzoom_destroy(DvzPanzoom* pz);



#endif
