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
 * Show information about a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_print(DvzShape* shape);



/**
 * Destroy a shape.
 *
 * @param shape the shape
 */
DVZ_EXPORT void dvz_shape_destroy(DvzShape* shape);



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

/**
 * Create a square shape.
 *
 * @param color the square color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_square(cvec4 color);



/**
 * Create a disc shape.
 *
 * @param count the number of points along the disc's border
 * @param color the disc color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_disc(uint32_t count, cvec4 color);



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

/**
 * Create a cube shape.
 *
 * @param colors the colors of the six faces
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_cube(cvec4* colors);



/**
 * Create a sphere shape.
 *
 * @param rows the number of rows
 * @param cols the number of columns
 * @param color the sphere color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, cvec4 color);



/**
 * Create a cone shape.
 *
 * @param count the number of points along the disc's border
 * @param color the cone color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_cone(uint32_t count, cvec4 color);



/**
 * Create a cylinder shape.
 *
 * @param count the number of points along the cylinder's border
 * @param color the cylinder color
 * @returns the shape
 */
DVZ_EXPORT DvzShape dvz_shape_cylinder(uint32_t count, cvec4 color);



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
 * @param basic the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_position(DvzVisual* basic, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the vertex colors.
 *
 * @param basic the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_basic_color(DvzVisual* basic, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param basic the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_basic_alloc(DvzVisual* basic, uint32_t item_count);



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
 * @param pixel the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_pixel_position(DvzVisual* pixel, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the pixel colors.
 *
 * @param pixel the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param pixel the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_pixel_alloc(DvzVisual* pixel, uint32_t item_count);



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
 * @param point the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_position(DvzVisual* point, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the point colors.
 *
 * @param point the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_color(DvzVisual* point, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Set the point sizes.
 *
 * @param point the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the sizes of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_point_size(DvzVisual* point, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param pixel the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_point_alloc(DvzVisual* point, uint32_t item_count);



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
 * @param marker the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D positions of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_position(DvzVisual* marker, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Set the marker sizes.
 *
 * @param marker the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_size(DvzVisual* marker, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the marker angles.
 *
 * @param point the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the angles of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_angle(DvzVisual* marker, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the marker colors.
 *
 * @param marker the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_marker_color(DvzVisual* marker, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Set the marker edge color.
 *
 * @param visual the visual
 * @param value the edge color
 */
DVZ_EXPORT void dvz_marker_edge_color(DvzVisual* visual, cvec4 value);



/**
 * Set the marker edge width.
 *
 * @param visual the visual
 * @param value the edge width
 */
DVZ_EXPORT void dvz_marker_edge_width(DvzVisual* visual, float value);



/**
 * Set the marker's texture.
 *
 * @param visual the visual
 * @param tex the texture ID
 * @param sampler the sampler ID
 */
DVZ_EXPORT void dvz_marker_tex(DvzVisual* visual, DvzId tex, DvzId sampler);



/**
 * Set the texture scale.
 *
 * @param visual the visual
 * @param value the texture scale
 */
DVZ_EXPORT void dvz_marker_tex_scale(DvzVisual* visual, float value);



/**
 * Allocate memory for a visual.
 *
 * @param pixel the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_marker_alloc(DvzVisual* marker, uint32_t item_count);



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
 * @param segment the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param initial the initial 3D positions of the segments
 * @param terminal the terminal 3D positions of the segments
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_segment_position(
    DvzVisual* segment, uint32_t first, uint32_t count, vec3* initial, vec3* terminal, int flags);



/**
 * Set the segment shift.
 *
 * @param segment the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_shift(DvzVisual* segment, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 * Set the segment colors.
 *
 * @param segment the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the colors of the items to update
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Set the segment line widths.
 *
 * @param segment the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the segment line widths
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_segment_linewidth(
    DvzVisual* segment, uint32_t first, uint32_t count, float* values, int flags);



/**
 * Set the segment cap types.
 *
 * @param segment the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param initial the initial segment cap types
 * @param terminal the terminal segment cap types
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_segment_cap(
    DvzVisual* segment, uint32_t first, uint32_t count, //
    DvzCapType* initial, DvzCapType* terminal, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param segment the visual
 * @param item_count the total number of items to allocate for this visual
 */
DVZ_EXPORT void dvz_segment_alloc(DvzVisual* segment, uint32_t item_count);



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
 * @param vertex_count the total number of points across all paths
 * @param positions the path point positions
 * @param path_count the number of different paths
 * @param path_lengths the number of points in each path
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_path_position(
    DvzVisual* visual, uint32_t vertex_count, vec3* positions, //
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
dvz_path_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Set the path line width.
 *
 * @param visual the visual
 * @param value the line width
 */
DVZ_EXPORT void dvz_path_linewidth(DvzVisual* visual, float value);



/**
 * Allocate memory for a visual.
 *
 * @param visual the visual
 * @param item_count the total number of points to allocate for this visual
 */
DVZ_EXPORT void dvz_path_alloc(DvzVisual* visual, uint32_t total_vertex_count);



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
 * @param pixel the visual
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
 * Set the glyph sizes.
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
 * @param cors the x,y,w,h texture coordinates
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_glyph_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags);



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
dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * Set the glyph group size.
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
 * Set the glyph background color.
 *
 * @param visual the visual
 * @param bgcolor the background color
 */
DVZ_EXPORT void dvz_glyph_bgcolor(DvzVisual* visual, vec4 bgcolor);



/**
 * Assign a texture to a glyph visual.
 *
 * @param visual the visual
 * @param tex the texture ID
 */
DVZ_EXPORT void dvz_glyph_texture(DvzVisual* visual, DvzId tex);



/**
 * Associate an atlas with a glyph visual.
 *
 * @param visual the visual
 * @param atlas the atlas
 */
DVZ_EXPORT void dvz_glyph_atlas(DvzVisual* visual, DvzAtlas* atlas);



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
 * @param flags the flags
 */
DVZ_EXPORT void dvz_glyph_xywh(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, vec2 offset, int flags);



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
 * @param image the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param ul_lr the 2D positions of the upper-left and lower-right corners (vec4 x0,y0,x1,y1)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_position(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 * Set the image texture coordinates.
 *
 * @param image the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param ul_lr the tex coordinates of the upper-left and lower-right corners (vec4 u0,v0,u1,v1)
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_image_texcoords(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 * Assign a texture to an image visual.
 *
 * @param visual the visual
 * @param tex the texture ID
 * @param filter the texture filtering mode
 * @param address_mode the texture address mode
 */
DVZ_EXPORT void dvz_image_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Allocate memory for a visual.
 *
 * @param image the visual
 * @param item_count the total number of images to allocate for this visual
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* image, uint32_t item_count);



/**
 * Create a 2D texture to be used in an image visual.
 *
 * @param batch the batch
 * @param format the texture format
 * @param width the texture width
 * @param height the texture height
 * @param data the texture data to upload
 * @returns the texture ID
 */
DVZ_EXPORT DvzId
dvz_tex_image(DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, void* data);



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
 * @param mesh the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the 3D vertex positions
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_mesh_position(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



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
dvz_mesh_color(DvzVisual* mesh, uint32_t first, uint32_t count, cvec4* values, int flags);



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
dvz_mesh_texcoords(DvzVisual* mesh, uint32_t first, uint32_t count, vec4* values, int flags);



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
void dvz_mesh_normal(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 * Assign a 2D texture to a mesh visual.
 *
 * @param visual the visual
 * @param tex the texture ID
 * @param filter the texture filtering mode
 * @param address_mode the texture address mode
 */
DVZ_EXPORT void dvz_mesh_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Set the mesh indices.
 *
 * @param mesh the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param values the face indices (three vertex indices per triangle)
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_mesh_index(DvzVisual* mesh, uint32_t first, uint32_t count, DvzIndex* values);



/**
 * Allocate memory for a visual.
 *
 * @param image the visual
 * @param vertex_count the number of vertices
 * @param index_count the number of indices
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* mesh, uint32_t vertex_count, uint32_t index_count);



/**
 * Set the mesh light position.
 *
 * @param mesh the mesh
 * @param pos the light position
 */
DVZ_EXPORT void dvz_mesh_light_pos(DvzVisual* mesh, vec4 pos);



/**
 * Set the mesh light parameters.
 *
 * @param mesh the mesh
 * @param pos the light parameters (vec4 ambient, diffuse, specular, exponent)
 */
DVZ_EXPORT void dvz_mesh_light_params(DvzVisual* mesh, vec4 params);



/**
 * Create a mesh out of a shape.
 *
 * @param batch the batch
 * @param shape the shape
 * @param flags the visual creation flags
 * @returns the mesh
 */
DVZ_EXPORT DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape, int flags);



/*************************************************************************************************/
/*  Fake sphere                                                                                  */
/*************************************************************************************************/

/**
 * Create a fake sphere visual.
 *
 * @param batch the batch
 * @param flags the visual creation flags
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_fake_sphere(DvzBatch* batch, int flags);



/**
 * Set the fake sphere positions.
 *
 * @param marker the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param pos the 3D positions of the sphere centers
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_fake_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* pos, int flags);



/**
 * Set the fake sphere colors.
 *
 * @param marker the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param color the sphere colors
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_fake_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* color, int flags);



/**
 * Set the fake sphere sizes.
 *
 * @param marker the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param size the radius of the spheres
 * @param flags the data update flags
 */
DVZ_EXPORT void
dvz_fake_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* size, int flags);



/**
 * Allocate memory for a visual.
 *
 * @param pixel the visual
 * @param item_count the total number of spheres to allocate for this visual
 */
DVZ_EXPORT void dvz_fake_sphere_alloc(DvzVisual* visual, uint32_t item_count);



/**
 * Set the sphere light position.
 *
 * @param mesh the mesh
 * @param pos the light position
 */
DVZ_EXPORT void dvz_fake_sphere_light_pos(DvzVisual* visual, vec3 pos);



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
 * Allocate memory for a visual.
 *
 * @param pixel the visual
 * @param item_count the total number of volumes to allocate for this visual
 */
DVZ_EXPORT void dvz_volume_alloc(DvzVisual* volume, uint32_t item_count);



/**
 * Assign a 3D texture to a volume visual.
 *
 * @param visual the visual
 * @param tex the texture ID
 * @param filter the texture filtering mode
 * @param address_mode the texture address mode
 */
DVZ_EXPORT void dvz_volume_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Set the volume size.
 *
 * @param visual the visual
 * @param w the texture width
 * @param h the texture height
 * @param d the texture depth
 */
DVZ_EXPORT void dvz_volume_size(DvzVisual* visual, float w, float h, float d);



/**
 * Create a 3D texture to be used in a volume visual.
 *
 * @param batch the batch
 * @param format the texture format
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth
 * @param data the texture data to upload
 * @returns the texture ID
 */
DVZ_EXPORT DvzId dvz_tex_volume(
    DvzBatch* batch, DvzFormat format, //
    uint32_t width, uint32_t height, uint32_t depth, void* data);



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
 * @param slice the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param p0 the 3D positions of the upper-left corner
 * @param p1 the 3D positions of the upper-right corner
 * @param p2 the 3D positions of the lower-left corner
 * @param p3 the 3D positions of the lower-right corner
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_slice_position(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* p0, vec3* p1, vec3* p2, vec3* p3, int flags);



/**
 * Set the slice texture coordinates.
 *
 * @param slice the visual
 * @param first the index of the first item to update
 * @param count the number of items to update
 * @param uvw0 the 3D texture coordinates of the upper-left corner
 * @param uvw1 the 3D texture coordinates of the upper-right corner
 * @param uvw2 the 3D texture coordinates of the lower-left corner
 * @param uvw3 the 3D texture coordinates of the lower-right corner
 * @param flags the data update flags
 */
DVZ_EXPORT void dvz_slice_texcoords(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags);



/**
 * Assign a texture to a slice visual.
 *
 * @param visual the visual
 * @param tex the texture ID
 * @param filter the texture filtering mode
 * @param address_mode the texture address mode
 */
DVZ_EXPORT void dvz_slice_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 * Allocate memory for a visual.
 *
 * @param image the visual
 * @param item_count the total number of slices to allocate for this visual
 */
DVZ_EXPORT void dvz_slice_alloc(DvzVisual* slice, uint32_t item_count);



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
