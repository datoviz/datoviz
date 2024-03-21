/*************************************************************************************************/
/* Axis                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_AXIS
#define DVZ_HEADER_AXIS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAxis DvzAxis;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzFont DvzFont;
typedef struct DvzMVP DvzMVP;
typedef struct DvzPanel DvzPanel;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxis
{
    int flags;

    DvzVisual* segment;
    DvzVisual* glyph;
    DvzAtlas* atlas;
    DvzFont* font;

    // Pos.
    double dmin;
    double dmax;
    vec3 p0;
    vec3 p1;
    vec3 p2;
    vec3 p3;

    // Color.
    cvec4 color_glyph;
    cvec4 color_lim;
    cvec4 color_grid;
    cvec4 color_major;
    cvec4 color_minor;

    // Tick width.
    vec4 tick_width;  // lim, grid, major, minor
    vec4 tick_length; // lim, grid, major, minor

    // Glyphs
    uint32_t tick_count; // = group_count
    uint32_t glyph_count;
    uint32_t* group_size;
    vec2 anchor;
    vec2 offset;

    void* user_data;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/


EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzAxis* dvz_axis(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_axis_segment(DvzAxis* axis);



/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_axis_glyph(DvzAxis* axis);



/**
 *
 */
DVZ_EXPORT void dvz_axis_size(DvzAxis* axis, float font_size);


// voir old datoviz params: number of minor ticks, disable grid


/**
 *
 */
DVZ_EXPORT void dvz_axis_width(DvzAxis* axis, float lim, float grid, float major, float minor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_length(DvzAxis* axis, float lim, float grid, float major, float minor);



/**
 *
 */
DVZ_EXPORT void
dvz_axis_color(DvzAxis* axis, cvec4 glyph, cvec4 lim, cvec4 grid, cvec4 major, cvec4 minor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_anchor(DvzAxis* axis, vec2 anchor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_offset(DvzAxis* axis, vec2 offset);



/**
 *
 */
DVZ_EXPORT void dvz_axis_pos(DvzAxis* axis, vec3 p0, vec3 p1, vec3 p2, vec3 p3);



/**
 *
 */
DVZ_EXPORT void dvz_axis_range(DvzAxis* axis, double dmin, double dmax);



/**
 *
 */
DVZ_EXPORT void dvz_axis_set(
    DvzAxis* axis, uint32_t tick_count, double* values, //
    uint32_t glyph_count, char* glyphs, uint32_t* index, uint32_t* length);



/**
 *
 */
DVZ_EXPORT void
dvz_axis_get(DvzAxis* axis, DvzMVP* mvp, dvec2 out_d); // compute dmin, dmax of the visible viewbox



/**
 *
 */
DVZ_EXPORT int dvz_axis_direction(
    DvzAxis* axis,
    DvzMVP* mvp); // returns 0 for horizontal, 1 for vertical. depends on the intersection or not
                  // of two projected boxes with maximal label length



/**
 *
 */
DVZ_EXPORT void dvz_axis_panel(DvzAxis* axis, DvzPanel* panel);



/**
 *
 */
DVZ_EXPORT void dvz_axis_destroy(DvzAxis* axis);



EXTERN_C_OFF

#endif
