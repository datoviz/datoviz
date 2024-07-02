/*************************************************************************************************/
/*  Axis                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/axis.h"
#include "_cglm.h"
#include "_macros.h"
#include "datoviz.h"
#include "datoviz_types.h"
#include "scene/atlas.h"
#include "scene/font.h"
#include "scene/scene.h"
#include "scene/visuals/glyph.h"
#include "scene/visuals/segment.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MINOR_TICKS_PER_INTERVAL 4



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



static inline uint32_t _minor_tick_count(uint32_t tick_count)
{
    return (tick_count - 1) * MINOR_TICKS_PER_INTERVAL;
}



static inline void
set_groups(DvzAxis* axis, uint32_t glyph_count, uint32_t tick_count, uint32_t* group_size)
{
    ANN(axis);
    ANN(group_size);
    ANN(axis->glyph);
    ANN(axis->segment);

    ASSERT(glyph_count > 0);
    ASSERT(tick_count > 0);
    ASSERT(glyph_count >= tick_count);

    _check_groups(glyph_count, tick_count, group_size);

    dvz_visual_groups(axis->glyph, tick_count, group_size);

    uint32_t n_major = axis->glyph->group_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    dvz_segment_alloc(axis->segment, n_total);
    dvz_glyph_alloc(axis->glyph, glyph_count);
}



/*************************************************************************************************/
/*  Tick computation                                                                             */
/*************************************************************************************************/

static inline vec3*
make_tick_positions(DvzAxis* axis, double dmin, double dmax, double* values, vec3 p0, vec3 p1)
{
    ANN(axis);
    ANN(values);
    ANN(axis->glyph);
    ASSERT(dmin < dmax);

    uint32_t tick_count = axis->glyph->group_count;

    // axis->p0 corresponds to axis->dmin
    // axis->p1 corresponds to axis->dmax
    double denom = 1. / (dmax - dmin);
    ASSERT(denom > 0);
    double d = 0;
    double a = 0; // rescaled value between 0 and 1

    float px = p1[0] - p0[0];
    float py = p1[1] - p0[1];
    float pz = p1[2] - p0[2];

    vec3* tick_positions = (vec3*)calloc(tick_count, sizeof(vec3));
    for (uint32_t i = 0; i < tick_count; i++)
    {
        d = values[i];
        a = (d - dmin) * denom;

        tick_positions[i][0] = p0[0] + px * a;
        tick_positions[i][1] = p0[1] + py * a;
        tick_positions[i][2] = p0[2] + pz * a;
    }

    return tick_positions;
}



/*************************************************************************************************/
/*  Tick functions                                                                               */
/*************************************************************************************************/

static inline void set_segment_pos(DvzAxis* axis, vec3* positions)
{
    ANN(axis);
    ANN(axis->glyph);

    DvzVisual* segment = axis->segment;
    ANN(segment);

    uint32_t n_major = axis->glyph->group_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Concatenation of major and minor ticks.
    vec3* pos = (vec3*)calloc(n_total, sizeof(vec3));
    memcpy(pos, positions, n_major * sizeof(vec3));

    // Generate the minor ticks.
    uint32_t major = 0;
    uint32_t minor = 0;
    vec3* target = &pos[n_major];
    float dx = (positions[1][0] - positions[0][0]) / (MINOR_TICKS_PER_INTERVAL + 1);
    float dy = (positions[1][1] - positions[0][1]) / (MINOR_TICKS_PER_INTERVAL + 1);
    float dz = (positions[1][2] - positions[0][2]) / (MINOR_TICKS_PER_INTERVAL + 1);
    for (uint32_t i = 0; i < n_minor; i++)
    {
        major = i / MINOR_TICKS_PER_INTERVAL;
        minor = i % MINOR_TICKS_PER_INTERVAL;
        target[i][0] = positions[major][0] + (minor + 1) * dx;
        target[i][1] = positions[major][1] + (minor + 1) * dy;
        target[i][2] = positions[major][2] + (minor + 1) * dz;
    }

    dvz_segment_position(segment, 0, n_total, pos, pos, 0);
    FREE(pos);
}



static inline void set_segment_color(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->glyph);

    DvzVisual* segment = axis->segment;
    ANN(segment);

    uint32_t n_major = axis->glyph->group_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Colors of the major and minor ticks.
    cvec4* colors = (cvec4*)calloc(n_total, sizeof(cvec4));
    for (uint32_t i = 0; i < n_major; i++)
    {
        memcpy(colors[i], axis->color_major, sizeof(cvec4));
    }
    for (uint32_t i = 0; i < n_minor; i++)
    {
        memcpy(colors[n_major + i], axis->color_minor, sizeof(cvec4));
    }

    dvz_segment_color(segment, 0, n_total, colors, 0);
    FREE(colors);
}



static inline void set_segment_shift(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->glyph);

    DvzVisual* segment = axis->segment;
    ANN(segment);

    uint32_t n_major = axis->glyph->group_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Vector pointing from p0 to p1.
    vec3 u = {0};
    glm_vec3_sub(axis->tick_spec.p1, axis->tick_spec.p0, u);
    glm_vec3_normalize(u);

    // Vector pointing from p0 to p2.
    vec3 v = {0};
    glm_vec3_copy(axis->tick_spec.vector, v);
    glm_vec3_normalize(v);

    // NOTE: this only works in 2D.
    // Rotation.
    float a = -u[1];
    float b = +u[0];

    // Find the right orientation.
    float det = u[0] * v[1] - u[1] * v[0];
    if (det > 0)
    {
        a = -a;
        b = -b;
    }

    // Tick length.
    float major_length = axis->tick_length[2];
    float minor_length = axis->tick_length[3];

    // Major and minor ticks.
    vec4* shift = (vec4*)calloc(n_total, sizeof(vec4));
    for (uint32_t i = 0; i < n_major; i++)
    {
        shift[i][2] = a * major_length;
        shift[i][3] = b * major_length;
    }
    for (uint32_t i = 0; i < n_minor; i++)
    {
        shift[n_major + i][2] = a * minor_length;
        shift[n_major + i][3] = b * minor_length;
    }
    // NOTE: this only works in 2D. In 3D, need to use end positions and shift=0.
    dvz_segment_shift(segment, 0, n_total, shift, 0);
    FREE(shift);
}



static inline void set_segment_width(DvzAxis* axis)
{
    ANN(axis);
    ANN(axis->glyph);

    DvzVisual* segment = axis->segment;
    ANN(segment);

    uint32_t n_major = axis->glyph->group_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Widths of the major and minor ticks.
    float* width = (float*)calloc(n_total, sizeof(float));
    for (uint32_t i = 0; i < n_major; i++)
    {
        width[i] = axis->tick_width[2]; // major
    }
    for (uint32_t i = 0; i < n_minor; i++)
    {
        width[n_major + i] = axis->tick_width[3]; // minor
    }

    dvz_segment_linewidth(segment, 0, n_total, width, 0);
    FREE(width);
}



/*************************************************************************************************/
/*  Glyph functions                                                                              */
/*************************************************************************************************/

// NOTE: size of positions array is group_count=tick_count
static inline void set_glyph_pos(DvzAxis* axis, vec3* positions)
{
    ANN(axis);

    DvzVisual* glyph = axis->glyph;
    ANN(glyph);

    uint32_t glyph_count = glyph->item_count;
    uint32_t group_count = glyph->group_count;
    uint32_t* group_size = glyph->group_sizes;

    ASSERT(glyph_count > 0);
    ASSERT(group_count > 0);
    ANN(group_size);
    ANN(positions);

    vec3* pos =
        _repeat_group(sizeof(vec3), glyph_count, group_count, group_size, (void*)positions, false);
    dvz_glyph_position(glyph, 0, glyph_count, pos, 0);
    FREE(pos);
}



static inline void set_glyph_anchor(DvzAxis* axis)
{
    ANN(axis);

    DvzVisual* glyph = axis->glyph;
    ANN(glyph);

    uint32_t glyph_count = glyph->item_count;
    ASSERT(glyph_count > 0);

    // NOTE: the axis currently only supports a uniform vec2 anchor.
    vec2* anchors = (vec2*)_repeat(glyph_count, sizeof(vec2), (void*)axis->anchor);
    dvz_glyph_anchor(glyph, 0, glyph_count, anchors, 0);
    FREE(anchors);
}



static inline void set_glyph_color(DvzAxis* axis)
{
    ANN(axis);

    uint32_t glyph_count = axis->glyph->item_count;
    ASSERT(glyph_count > 0);
    DvzVisual* glyph = axis->glyph;

    // NOTE: the axis currently only supports a uniform vec2 anchor.
    cvec4* colors = (cvec4*)_repeat(glyph_count, sizeof(cvec4), (void*)axis->color_glyph);
    dvz_glyph_color(glyph, 0, glyph_count, colors, 0);
    FREE(colors);
}



static inline void set_glyphs(DvzAxis* axis, const char* glyphs, uint32_t* index)
{
    // NOTE: glyphs is the concatenation of all group strings, without trailing zeros
    ANN(axis);
    ANN(glyphs);
    ANN(index);

    // Set the size and shift properties of the glyph vsual by using the font to compute the
    // layout.
    vec4* xywh = dvz_font_ascii(axis->font, glyphs);

    uint32_t glyph_count = axis->glyph->item_count;
    uint32_t group_count = axis->glyph->group_count;
    uint32_t* group_size = axis->glyph->group_sizes;
    ASSERT(glyph_count > 0);
    ASSERT(group_count > 0);
    ANN(group_size);

    // Prepare a copy of the string with all glyphs concatenated, but without the spaces
    // between the groups.
    vec4* xywh_trimmed = (vec4*)calloc(glyph_count, sizeof(vec4));
    char* glyphs_trimmed = (char*)calloc(glyph_count, sizeof(char));

    float x0 = 0.0;
    uint32_t idx = 0;
    uint32_t k = 0;
    for (uint32_t i = 0; i < group_count; i++)
    {
        idx = index[i];
        x0 = xywh[idx][0];
        for (uint32_t j = 0; j < group_size[i]; j++)
        {
            // NOTE: remove the x0 offset for each group.
            xywh_trimmed[k][0] = xywh[idx + j][0] - x0;
            xywh_trimmed[k][1] = xywh[idx + j][1];
            xywh_trimmed[k][2] = xywh[idx + j][2];
            xywh_trimmed[k][3] = xywh[idx + j][3];

            glyphs_trimmed[k] = glyphs[idx + j];
            k++;
        }
    }
    ASSERT(k == glyph_count);

    dvz_glyph_xywh(axis->glyph, 0, glyph_count, xywh_trimmed, axis->offset, 0);
    FREE(xywh);

    dvz_glyph_ascii(axis->glyph, glyphs_trimmed);
    FREE(glyphs_trimmed);
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
    // DvzAtlasFont af = dvz_atlas_export("Roboto_Medium", "data/fonts/Roboto_Medium_atlas.bin");
    DvzAtlasFont af = dvz_atlas_import("Roboto_Medium", "Roboto_Medium_atlas");
    axis->atlas = af.atlas;
    axis->font = af.font;

    // DEBUG
    // dvz_atlas_png(axis->atlas, "atlas.png");

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



void dvz_axis_panel(DvzAxis* axis, DvzPanel* panel)
{
    ANN(axis);
    ANN(panel);

    dvz_panel_visual(panel, axis->segment);
    dvz_panel_visual(panel, axis->glyph);
}



int dvz_axis_direction(DvzAxis* axis, DvzMVP* mvp)
{
    ANN(axis);
    // TODO
    // returns 0 for horizontal, 1 for vertical. depends on the intersection or not
    // of two projected boxes with maximal label length
    return 0;
}



void dvz_axis_update(DvzAxis* axis)
{
    ANN(axis);

    dvz_visual_update(axis->segment);
    dvz_visual_update(axis->glyph);
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
/*  Visual properties                                                                            */
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
    axis->tick_width[0] = lim;
    axis->tick_width[1] = grid;
    axis->tick_width[2] = major;
    axis->tick_width[3] = minor;
}



void dvz_axis_length(DvzAxis* axis, float lim, float grid, float major, float minor)
{
    ANN(axis);
    axis->tick_length[0] = lim;
    axis->tick_length[1] = grid;
    axis->tick_length[2] = major;
    axis->tick_length[3] = minor;
}



void dvz_axis_color(DvzAxis* axis, cvec4 glyph, cvec4 lim, cvec4 grid, cvec4 major, cvec4 minor)
{
    ANN(axis);
    memcpy(axis->color_glyph, glyph, sizeof(cvec4));
    memcpy(axis->color_lim, lim, sizeof(cvec4));
    memcpy(axis->color_grid, grid, sizeof(cvec4));
    memcpy(axis->color_major, major, sizeof(cvec4));
    memcpy(axis->color_minor, minor, sizeof(cvec4));
}



void dvz_axis_anchor(DvzAxis* axis, vec2 anchor)
{
    ANN(axis);
    glm_vec2_copy(anchor, axis->anchor);
}



void dvz_axis_offset(DvzAxis* axis, vec2 offset)
{
    ANN(axis);
    glm_vec2_copy(offset, axis->offset);
}



/*************************************************************************************************/
/*  MVP                                                                                          */
/*************************************************************************************************/

void dvz_axis_mvp(DvzAxis* axis, DvzMVP* mvp, dvec2 range_data, vec2 range_ndc)
{
    ANN(axis);
    ANN(mvp);

    // Compute q0=mvp*p0 and q1=mvp*p1.
    vec4 q0, q1;
    glm_vec3_copy(axis->p0_ref, q0);
    glm_vec3_copy(axis->p1_ref, q1);
    q0[3] = 1;
    q1[3] = 1;

    dvz_mvp_apply(mvp, q0, q0);
    dvz_mvp_apply(mvp, q1, q1);

    // glm_vec4_print(q0, stdout);
    // glm_vec4_print(q1, stdout);

    // Direction vector, from p0 to p1.
    vec3 u;
    glm_vec3_sub(axis->p1_ref, axis->p0_ref, u);
    ASSERT(glm_vec3_norm(u) > 0);
    glm_vec3_normalize(u);

    double dmin = axis->tick_spec.dmin;
    double dmax = axis->tick_spec.dmax;

    double p0_ = glm_vec3_dot(axis->tick_spec.p0, u);
    double p1_ = glm_vec3_dot(axis->tick_spec.p1, u);
    double q0_ = glm_vec3_dot(q0, u);
    double q1_ = glm_vec3_dot(q1, u);

    double denom = 1. / (q1_ - q0_);
    range_data[0] = dmin + (dmax - dmin) * (p0_ - q0_) * denom;
    range_data[1] = dmin + (dmax - dmin) * (p1_ - q0_) * denom;

    range_ndc[0] = -1 + 2 * (p0_ - q0_) * denom;
    range_ndc[1] = -1 + 2 * (p1_ - q0_) * denom;
}



/*************************************************************************************************/
/*  Ticks and glyphs                                                                             */
/*************************************************************************************************/

DvzTickSpec dvz_tick_spec(
    vec3 p0, vec3 p1, vec3 vector,                                 // positions in NDC
    double dmin, double dmax, uint32_t tick_count, double* values, // tick positions and values
    uint32_t glyph_count, char* glyphs, uint32_t* index, uint32_t* length) // tick labels
{
    ASSERT(dmin < dmax);
    ASSERT(tick_count > 0);
    ASSERT(glyph_count > 0);

    ANN(values);
    ANN(glyphs);
    ANN(index);
    ANN(length);

    DvzTickSpec spec = {
        .p0 = {p0[0], p0[1], p0[2]},
        .p1 = {p1[0], p1[1], p1[2]},
        .vector = {vector[0], vector[1], vector[2]},
        .dmin = dmin,
        .dmax = dmax,
        .tick_count = tick_count,
        .values = values,
        .glyph_count = glyph_count,
        .glyphs = glyphs,
        .index = index,
        .length = length,
    };
    return spec;
}



void dvz_axis_ticks(DvzAxis* axis, DvzTickSpec* tick_spec)
{
    ANN(axis);
    memcpy(&axis->tick_spec, tick_spec, sizeof(DvzTickSpec));

    // HACK: copy p0, p1 to reference values if they have not been set before.
    if (glm_vec3_norm(axis->p0_ref) == 0)
        glm_vec3_copy(tick_spec->p0, axis->p0_ref);
    if (glm_vec3_norm(axis->p1_ref) == 0)
        glm_vec3_copy(tick_spec->p1, axis->p1_ref);

    // Allocation.
    set_groups(axis, tick_spec->glyph_count, tick_spec->tick_count, tick_spec->length);

    // Segment.
    set_segment_width(axis);

    // Tick positions.
    vec3* tick_positions = make_tick_positions(
        axis, tick_spec->dmin, tick_spec->dmax, tick_spec->values, tick_spec->p0, tick_spec->p1);
    set_glyph_pos(axis, tick_positions);
    set_glyph_anchor(axis);
    set_segment_pos(axis, tick_positions);
    FREE(tick_positions);

    // Tick width and length.
    set_segment_width(axis);
    set_segment_shift(axis);

    // Colors.
    set_segment_color(axis);
    set_glyph_color(axis);

    // NOTE: set_groups() needs to be called BEFORE this function, which calls dvz_glyph_xywh().
    // The latter uses the group information to compute the width of each group, necessary
    // to compute the anchor relative to each group's size in the vertex shader.
    set_glyphs(axis, tick_spec->glyphs, tick_spec->index);

    dvz_axis_update(axis);
}
