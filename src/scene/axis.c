/*************************************************************************************************/
/*  Axis                                                                                         */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/axis.h"
#include "_macros.h"
#include "scene/atlas.h"
#include "scene/colormaps.h"
#include "scene/font.h"
#include "scene/visuals/glyph.h"
#include "scene/visuals/segment.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct AtlasFont AtlasFont;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct AtlasFont
{
    unsigned long ttf_size;
    unsigned char* ttf_bytes;
    DvzAtlas* atlas;
    DvzFont* font;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static AtlasFont _load_font(void)
{
    // Load the font ttf bytes.
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);

    // Create the font.
    DvzFont* font = dvz_font(ttf_size, ttf_bytes);

    // Create the atlas.
    DvzAtlas* atlas = dvz_atlas(ttf_size, ttf_bytes);

    // Generate the atlas.
    dvz_atlas_generate(atlas);

    AtlasFont af = {.ttf_size = ttf_size, .ttf_bytes = ttf_bytes, .atlas = atlas, .font = font};
    return af;
}



/*************************************************************************************************/
/*  Allocation functions                                                                         */
/*************************************************************************************************/

static inline uint32_t _tick_count(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->segment);
    return axis->segment->item_count;
}



static inline uint32_t _glyph_count(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->glyph);
    return axis->glyph->item_count;
}



// NOTE: the caller must FREE the output
static inline void* _repeat(uint32_t item_count, DvzSize item_size, void* value)
{
    void* out = (vec3*)calloc(item_count, item_size);
    for (uint32_t i = 0; i < item_count; i++)
    {
        memcpy((void*)((uint64_t)out + i * item_size), value, item_size);
    }
    return out;
}



// NOTE: the caller must FREE the output
static inline void* _repeat_group(
    DvzSize item_size, uint32_t item_count, uint32_t group_count, uint32_t* group_size,
    void* group_values, bool uniform)
{
    void* out = (vec3*)calloc(item_count, item_size);
    uint32_t k = 0;
    DvzSize item_size_src = uniform ? 0 : item_size;
    for (uint32_t i = 0; i < group_count; i++)
    {
        for (uint32_t j = 0; j < group_size[i]; j++)
        {
            ASSERT(k < item_count);
            memcpy(
                (void*)((uint64_t)out + (k++) * item_size),
                (void*)((uint64_t)group_values + i * item_size_src), //
                item_size);
        }
    }
    ASSERT(k == item_count);
    return out;
}



static inline void
set_groups(DvzAxis* axis, uint32_t glyph_count, uint32_t tick_count, uint32_t* group_size)
{
    ANN(axis);
    ANN(group_size);
    ASSERT(glyph_count > 0);
    ASSERT(tick_count > 0);

    axis->glyph_count = glyph_count;
    axis->tick_count = tick_count;
    axis->group_size = group_size;

    dvz_segment_alloc(axis->segment, tick_count);
    dvz_glyph_alloc(axis->glyph, glyph_count);
}



/*************************************************************************************************/
/*  Tick computation                                                                             */
/*************************************************************************************************/

static inline vec3* make_tick_positions(DvzAxis* axis, double* values)
{
    ANN(axis);
    ANN(values);
    uint32_t tick_count = axis->tick_count;

    // axis->p0 corresponds to axis->dmin
    // axis->p1 corresponds to axis->dmax
    double dmin = axis->dmin;
    double dmax = axis->dmax;
    ASSERT(dmin < dmax);
    double denom = 1. / (dmax - dmin);
    ASSERT(denom > 0);
    double d = 0;
    double a = 0; // rescaled value between 0 and 1

    float px = axis->p1[0] - axis->p0[0];
    float py = axis->p1[1] - axis->p0[1];
    float pz = axis->p1[2] - axis->p0[2];

    vec3* tick_positions = (vec3*)calloc(tick_count, sizeof(vec3));
    for (uint32_t i = 0; i < tick_count; i++)
    {
        d = values[i];
        a = (d - dmin) * denom;

        tick_positions[i][0] = axis->p0[0] + px * a;
        tick_positions[i][1] = axis->p0[1] + py * a;
        tick_positions[i][2] = axis->p0[2] + pz * a;
    }

    return tick_positions;
}



/*************************************************************************************************/
/*  Tick functions                                                                               */
/*************************************************************************************************/

static inline void set_segment_pos(DvzAxis* axis, vec3* positions)
{
    ANN(axis);
    DvzVisual* segment = axis->segment;
    ANN(segment);
    dvz_segment_position(segment, 0, axis->tick_count, positions, positions, 0);
}



static inline void set_segment_color(DvzAxis* axis, cvec4 color)
{
    ANN(axis);
    DvzVisual* segment = axis->segment;
    ANN(segment);
    cvec4* colors = (cvec4*)_repeat(axis->tick_count, sizeof(cvec4), (void*)color);
    dvz_segment_color(segment, 0, axis->tick_count, colors, 0);
    FREE(colors);
}



static inline void set_segment_shift(DvzAxis* axis)
{
    ANN(axis);
    DvzVisual* segment = axis->segment;
    ANN(segment);
    vec4* shift = (vec4*)calloc(axis->tick_count, sizeof(vec4));
    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        // TODO: proper shift.
        shift[i][2] = 0;
        shift[i][3] = 20;
    }
    // NOTE: this only works in 2D. In 3D, need to use end positions and shift=0.
    dvz_segment_shift(segment, 0, axis->tick_count, shift, 0);
    FREE(shift);
}



static inline void set_segment_width(DvzAxis* axis)
{
    ANN(axis);

    float* width = (float*)calloc(axis->tick_count, sizeof(float));
    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        width[i] = axis->width[0]; // TODO: proper ticks
    }
    // TODO
    dvz_segment_linewidth(axis->segment, 0, axis->tick_count, width, 0);
    FREE(width);
}



/*************************************************************************************************/
/*  Glyph functions                                                                              */
/*************************************************************************************************/

// NOTE: size of positions array is group_count=tick_count
static inline void set_glyph_pos(DvzAxis* axis, vec3* positions)
{
    ANN(axis);

    uint32_t glyph_count = axis->glyph_count;
    uint32_t group_count = axis->tick_count;
    uint32_t* group_size = axis->group_size;

    ASSERT(glyph_count > 0);
    ASSERT(group_count > 0);
    ANN(group_size);
    ANN(positions);
    DvzVisual* glyph = axis->glyph;

    vec3* pos =
        _repeat_group(sizeof(vec3), glyph_count, group_count, group_size, (void*)positions, false);
    dvz_glyph_position(glyph, 0, glyph_count, pos, 0);
    FREE(pos);

    vec2* anchor = (vec2*)_repeat(glyph_count, sizeof(vec2), (vec2){-.5, -1.5});
    dvz_glyph_anchor(glyph, 0, glyph_count, anchor, 0);
    FREE(anchor)
}



static inline void set_glyph_color(DvzAxis* axis, cvec4 color)
{
    ANN(axis);

    uint32_t glyph_count = axis->glyph_count;
    uint32_t group_count = axis->tick_count;
    uint32_t* group_size = axis->group_size;

    ASSERT(glyph_count > 0);
    ASSERT(group_count > 0);
    ANN(group_size);
    ANN(color);
    DvzVisual* glyph = axis->glyph;
    cvec4* colors =
        _repeat_group(sizeof(cvec4), glyph_count, group_count, group_size, (void*)color, true);
    dvz_glyph_color(glyph, 0, glyph_count, colors, 0);
    FREE(colors);
}



static inline void set_text(DvzAxis* axis, const char* glyphs)
{
    // NOTE: text is the concatenation of all group strings, without trailing zeros
    ANN(axis);

    // Set the size and shift properties of the glyph vsual by using the font to compute the
    // layout.
    uint32_t n = strnlen(glyphs, 65536); // NOTE: hard-coded maximal text size
    vec4* xywh = dvz_font_ascii(axis->font, glyphs);

    // NOTE: remove the x0 offset for each group.
    uint32_t glyph_count = axis->glyph_count;
    uint32_t group_count = axis->tick_count;
    uint32_t* group_size = axis->group_size;
    ASSERT(glyph_count > 0);
    ASSERT(group_count > 0);
    ANN(group_size);
    float x0 = 0.0;
    uint32_t k = 0;
    for (uint32_t i = 0; i < group_count; i++)
    {
        x0 = xywh[k][0];
        for (uint32_t j = 0; j < group_size[i]; j++)
        {
            ASSERT(k < n);
            xywh[k++][0] -= x0;
        }
    }
    ASSERT(k == glyph_count);

    dvz_glyph_xywh(axis->glyph, 0, n, xywh, (vec2){0, 0}, 0); // TODO: offset
    FREE(xywh);

    dvz_glyph_ascii(axis->glyph, glyphs);
}



/*************************************************************************************************/
/*  General functions                                                                            */
/*************************************************************************************************/

DvzAxis* dvz_axis(DvzBatch* batch, int flags)
{
    DvzAxis* axis = (DvzAxis*)calloc(1, sizeof(DvzAxis));
    axis->flags = flags;

    axis->segment = dvz_segment(batch, 0);
    axis->glyph = dvz_glyph(batch, 0);

    // Load the font and generate the atlas.
    AtlasFont af = _load_font();
    axis->atlas = af.atlas;
    axis->font = af.font;

    // Upload the atlas texture to the glyph visual.
    dvz_glyph_atlas(axis->glyph, axis->atlas);

    return axis;
}



DvzVisual* dvz_axis_segment(DvzAxis* axis)
{
    ANN(axis);
    return axis->segment;
}



DvzVisual* dvz_axis_glyph(DvzAxis* axis)
{
    ANN(axis);
    return axis->glyph;
}



void dvz_axis_destroy(DvzAxis* axis)
{
    ANN(axis);

    dvz_visual_destroy(axis->segment);
    dvz_visual_destroy(axis->glyph);

    dvz_atlas_destroy(axis->atlas);
    dvz_font_destroy(axis->font);

    FREE(axis);
}



/*************************************************************************************************/
/*  Global parameters                                                                            */
/*************************************************************************************************/

void dvz_axis_size(DvzAxis* axis, float font_size)
{
    ANN(axis);
    DvzVisual* glyph = axis->glyph;
    ANN(glyph);

    dvz_font_size(axis->font, font_size);
}



void dvz_axis_width(DvzAxis* axis, float lim, float grid, float major, float minor)
{
    ANN(axis);
    axis->width[0] = lim;
    axis->width[1] = grid;
    axis->width[2] = major;
    axis->width[3] = minor;
}



void dvz_axis_color(DvzAxis* axis, cvec4 lim, cvec4 grid, cvec4 major, cvec4 minor)
{
    ANN(axis);
    memcpy(axis->lim, lim, sizeof(cvec4));
    memcpy(axis->grid, grid, sizeof(cvec4));
    memcpy(axis->major, major, sizeof(cvec4));
    memcpy(axis->minor, minor, sizeof(cvec4));
}



void dvz_axis_pos(DvzAxis* axis, double dmin, double dmax, vec3 p0, vec3 p1, vec3 p2, vec3 p3)
{
    ANN(axis);
    ASSERT(dmin < dmax);

    axis->dmin = dmin;
    axis->dmax = dmax;

    _vec3_copy(p0, axis->p0);
    _vec3_copy(p1, axis->p1);
    _vec3_copy(p2, axis->p2);
    _vec3_copy(p3, axis->p3);
}



/*************************************************************************************************/
/*  Ticks and glyphs                                                                             */
/*************************************************************************************************/

void dvz_axis_set(
    DvzAxis* axis, uint32_t tick_count, double* values, //
    uint32_t glyph_count, char* glyphs, uint32_t* index, uint32_t* length)
{
    ANN(axis);

    // Allocation.
    set_groups(axis, glyph_count, tick_count, length);

    // Segment.
    set_segment_width(axis);

    // Tick positions.
    vec3* tick_positions = make_tick_positions(axis, values);
    set_glyph_pos(axis, tick_positions);
    set_segment_pos(axis, tick_positions);
    set_segment_shift(axis);
    FREE(tick_positions);

    set_segment_color(axis, axis->major); // TODO
    set_glyph_color(axis, axis->major);   // TODO: proper colors

    set_text(axis, glyphs);
}



void dvz_axis_get(DvzAxis* axis, DvzMVP* mvp, vec2 out_d)
{
    ANN(axis);
    // TODO
    // compute dmin, dmax of the visible viewbox
}



int dvz_axis_direction(DvzAxis* axis, DvzMVP* mvp)
{
    ANN(axis);
    // TODO
    // returns 0 for horizontal, 1 for vertical. depends on the intersection or not
    // of two projected boxes with maximal label length
    return 0;
}
