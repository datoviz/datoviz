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
 *
 */
DVZ_EXPORT DvzScene* dvz_scene(DvzBatch* batch);



/**
 *
 */
DVZ_EXPORT void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t n_frames);



/**
 *
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_figure_resize(DvzFigure* fig, uint32_t width, uint32_t height);



/**
 *
 */
DvzFigure* dvz_scene_figure(DvzScene* scene, DvzId id);



/**
 *
 */
DVZ_EXPORT void dvz_figure_destroy(DvzFigure* figure);



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float w, float h);



/**
 *
 */
DVZ_EXPORT DvzPanel* dvz_panel_default(DvzFigure* fig);



/**
 *
 */
DVZ_EXPORT void dvz_panel_transform(DvzPanel* panel, DvzTransform* tr);



/**
 *
 */
DVZ_EXPORT void dvz_panel_resize(DvzPanel* panel, float x, float y, float width, float height);



/**
 *
 */
DVZ_EXPORT void
dvz_panel_margins(DvzPanel* panel, float top, float right, float bottom, float left);



/**
 *
 */
DVZ_EXPORT bool dvz_panel_contains(DvzPanel* panel, vec2 pos);



/**
 *
 */
DVZ_EXPORT DvzPanel* dvz_panel_at(DvzFigure* figure, vec2 pos);



/**
 *
 */
DVZ_EXPORT DvzCamera* dvz_panel_camera(DvzPanel* panel);


/**
 *
 */
DVZ_EXPORT DvzPanzoom* dvz_panel_panzoom(DvzScene* scene, DvzPanel* panel);


/**
 *
 */
DVZ_EXPORT DvzArcball* dvz_panel_arcball(DvzScene* scene, DvzPanel* panel);



/**
 *
 */
DVZ_EXPORT void dvz_panel_update(DvzPanel* panel);



/**
 *
 */
DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual);



/**
 *
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);



/*************************************************************************************************/
/*************************************************************************************************/
/*  Visuals API                                                                                  */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Shape functions                                                                              */
/*************************************************************************************************/

DVZ_EXPORT void dvz_shape_print(DvzShape* shape);

DVZ_EXPORT void dvz_shape_destroy(DvzShape* shape);



/*************************************************************************************************/
/*  2D shapes                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzShape dvz_shape_square(cvec4 color);

DVZ_EXPORT DvzShape dvz_shape_disc(uint32_t count, cvec4 color);



/*************************************************************************************************/
/*  3D shapes                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzShape dvz_shape_cube(cvec4* colors);

DVZ_EXPORT DvzShape dvz_shape_sphere(uint32_t rows, uint32_t cols, cvec4 color);

DVZ_EXPORT DvzShape dvz_shape_cone(uint32_t count, cvec4 color);

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
 *
 */
DVZ_EXPORT void
dvz_basic_color(DvzVisual* basic, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_point_color(DvzVisual* point, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_point_size(DvzVisual* point, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
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
 *
 */
DVZ_EXPORT void dvz_marker_aspect(DvzVisual* visual, DvzMarkerAspect aspect);



/**
 *
 */
DVZ_EXPORT void dvz_marker_shape(DvzVisual* visual, DvzMarkerShape shape);



/**
 *
 */
DVZ_EXPORT void
dvz_marker_position(DvzVisual* marker, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_marker_size(DvzVisual* marker, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_marker_angle(DvzVisual* marker, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_marker_color(DvzVisual* marker, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_marker_edge_color(DvzVisual* visual, cvec4 value);



/**
 *
 */
DVZ_EXPORT void dvz_marker_edge_width(DvzVisual* visual, float value);



/**
 *
 */
DVZ_EXPORT void dvz_marker_tex(DvzVisual* visual, DvzId tex, DvzId sampler);



/**
 *
 */
DVZ_EXPORT void dvz_marker_tex_scale(DvzVisual* visual, float value);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_segment_shift(DvzVisual* segment, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_segment_color(DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_linewidth(
    DvzVisual* segment, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_segment_cap(
    DvzVisual* segment, uint32_t first, uint32_t count, DvzCapType* initial, DvzCapType* terminal,
    int flags);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_path_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_path_linewidth(DvzVisual* visual, float value);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_glyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_axis(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_groupsize(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_bgcolor(DvzVisual* visual, vec4 bgcolor);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_texture(DvzVisual* visual, DvzId tex);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_atlas(DvzVisual* visual, DvzAtlas* atlas);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_unicode(DvzVisual* visual, uint32_t count, uint32_t* codepoints);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_ascii(DvzVisual* visual, const char* string);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_image_texcoords(DvzVisual* image, uint32_t first, uint32_t count, vec4* ul_lr, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_image_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 *
 */
DVZ_EXPORT void dvz_image_alloc(DvzVisual* image, uint32_t item_count);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_mesh_color(DvzVisual* mesh, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 * vec4: u, v, <unused>, a
 */
DVZ_EXPORT void
dvz_mesh_texcoords(DvzVisual* mesh, uint32_t first, uint32_t count, vec4* values, int flags);



/**
 *
 */
DVZ_EXPORT
void dvz_mesh_normal(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_index(DvzVisual* mesh, uint32_t first, uint32_t count, DvzIndex* values);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_alloc(DvzVisual* mesh, uint32_t vertex_count, uint32_t index_count);



/**
 *
 */
DVZ_EXPORT void dvz_mesh_light_pos(DvzVisual* mesh, vec4 pos);



/**
 *
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
 *
 */
DVZ_EXPORT void
dvz_fake_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* color, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_fake_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* size, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_fake_sphere_alloc(DvzVisual* visual, uint32_t item_count);



/**
 *
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
 *
 */
DVZ_EXPORT void dvz_volume_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 *
 */
DVZ_EXPORT void dvz_volume_size(DvzVisual* visual, float w, float h, float d);



/**
 *
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
 *
 */
DVZ_EXPORT void dvz_slice_texcoords(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_slice_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 *
 */
DVZ_EXPORT void dvz_slice_alloc(DvzVisual* slice, uint32_t item_count);



/**
 *
 */
DVZ_EXPORT DvzId dvz_tex_slice(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth,
    void* data);



/**
 *
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

DVZ_EXPORT double dvz_resample(double t0, double t1, double t);

DVZ_EXPORT double dvz_easing(DvzEasing easing, double t);

DVZ_EXPORT void dvz_circular_2D(vec2 center, float radius, float angle, float t, vec2 out);

DVZ_EXPORT void
dvz_circular_3D(vec3 center, vec3 u, vec3 v, float radius, float angle, float t, vec3 out);

DVZ_EXPORT float dvz_interpolate(float p0, float p1, float t);

DVZ_EXPORT void dvz_interpolate_2D(vec2 p0, vec2 p1, float t, vec2 out);

DVZ_EXPORT void dvz_interpolate_3D(vec3 p0, vec3 p1, float t, vec3 out);



#endif