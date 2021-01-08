#ifndef VKL_AXES_HEADER
#define VKL_AXES_HEADER


#include "../external/exwilk.h"
#include "../include/visky/scene.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklAxesTicks VklAxesTicks;



/*************************************************************************************************/
/*  Axes structs                                                                                 */
/*************************************************************************************************/

struct VklAxesTicks
{
    double vmin, vmax;        // range values
    double lmin, lmax, lstep; // tick range and interval
    VklTickFormat format;
    uint32_t value_count;
    double* values; // from lmin to lmax by lstep
    char* labels;
};



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

static void _tick_format(double value, char* out_text) { snprintf(out_text, 16, "%.1f", value); }

static void _axes_ticks(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    VklCanvas* canvas = controller->panel->grid->canvas;

    VklArray* arr = &axes->ticks[coord];
    VklArray* text = &axes->text[coord];
    char* buf = axes->buf[coord];

    // Find the ticks given the range.
    double vmin = axes->range[coord][0];
    double vmax = axes->range[coord][1];
    double vlen = vmax - vmin;
    ASSERT(vlen > 0);

    double vmin0 = vmin - vlen;
    double vmax0 = vmax + vlen;
    ASSERT(vmin0 < vmax0);

    // TODO: improve
    int32_t N = 3 * 12;
    // TODO
    // VklAxesContext context = {0};
    // context.glyph_size[0] = 12;
    // context.glyph_size[1] = 12;
    // context.coord = coord;
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    // context.viewport_size[0] = size[0];
    // context.viewport_size[1] = size[1];
    R r = {0}; // wilk_ext(vmin0, vmax0, N, false, context);
    vmin0 = r.lmin;
    vmax0 = r.lmax;
    double dv = r.lstep;
    N = (int32_t)ceil((vmax0 - vmin0) / dv);
    ASSERT(N > 0);
    vkl_array_resize(arr, (uint32_t)N);
    vkl_array_resize(text, (uint32_t)N);

    double v = 0;
    for (uint32_t i = 0; i < (uint32_t)N; i++)
    {
        v = vmin0 + dv * i;
        ((float*)arr->data)[i] = v;
        ((char**)text->data)[i] = &buf[16 * i];
        _tick_format(v, &buf[16 * i]);
    }
}

static void _axes_upload(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(controller->visual_count == 2);

    VklVisual* visual = controller->visuals[coord];
    VklArray* arr = &axes->ticks[coord];

    uint32_t N = arr->item_count;
    float* ticks = arr->data;
    char** text = axes->text[coord].data;
    ASSERT(axes->text[coord].item_count == N);

    // TODO: more minor ticks between the major ticks.
    float lim[] = {-1};
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, ticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);
    vkl_visual_data(visual, VKL_PROP_TEXT, 0, N, text);
}

static void _axes_ticks_init(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        // Init structures.
        // TODO: constants
        axes->buf[coord] = calloc(128 * 16, sizeof(char)); // max ticks * max glyphs per tick
        axes->ticks[coord] = vkl_array(0, VKL_DTYPE_FLOAT);
        axes->text[coord] = vkl_array(0, VKL_DTYPE_STR);

        // Set the initial range.
        axes->range[coord][0] = -1;
        axes->range[coord][1] = +1;

        // Compute the ticks for these ranges.
        _axes_ticks(controller, coord);

        // Upload the data.
        _axes_upload(controller, coord);
    }
}

static void _axes_collision(VklController* controller, bool* update)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(update != NULL);

    // TODO
    // set update depending on whether there is a collision on the labels on that axis
    update[0] = (controller->panel->grid->canvas->frame_idx % 5000) == 0;
    update[1] = (controller->panel->grid->canvas->frame_idx % 5000) == 0;
}

static void _axes_range(VklController* controller, VklAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // set axes->range depending on coord
    VklTransform tr = {0};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    dvec2 pos_ll = {0};
    dvec2 pos_ur = {0};
    tr = vkl_transform(controller->panel, VKL_CDS_PANZOOM, VKL_CDS_GPU);
    vkl_transform_apply(&tr, ll, pos_ll);
    vkl_transform_apply(&tr, ur, pos_ur);
    axes->range[coord][0] = pos_ll[coord];
    axes->range[coord][1] = pos_ur[coord];
    // log_info("%.3f %.3f", axes->range[coord][0], axes->range[coord][1]);
}

static void _axes_callback(VklController* controller, VklEvent ev)
{
    ASSERT(controller != NULL);
    _default_controller_callback(controller, ev);
    if (!controller->interacts[0].is_active)
        return;
    VklCanvas* canvas = controller->panel->grid->canvas;

    // Check label collision
    // DEBUG
    // bool update[2] = {true, true}; // whether X and Y axes must be updated or not
    bool update[2] = {false, false}; // whether X and Y axes must be updated or not
    // _axes_collision(controller, update);
    // if (!update[0] && !update[1])
    //     return;

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        if (!update[coord])
            continue;
        _axes_range(controller, coord);
        _axes_ticks(controller, coord);
        _axes_upload(controller, coord);
        canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    }
}

static void _add_axes(VklController* controller)
{
    ASSERT(controller != NULL);
    VklPanel* panel = controller->panel;
    ASSERT(panel != NULL);
    panel->controller = controller;
    VklContext* ctx = panel->grid->canvas->gpu->context;
    ASSERT(ctx != NULL);

    vkl_panel_margins(panel, (vec4){25, 25, 100, 100});

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        VklVisual* visual = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, (int)coord);
        vkl_controller_visual(controller, visual);
        visual->priority = VKL_MAX_VISUAL_PRIORITY;

        visual->clip[0] = VKL_VIEWPORT_OUTER;
        visual->clip[1] = coord == 0 ? VKL_VIEWPORT_OUTER_BOTTOM : VKL_VIEWPORT_OUTER_LEFT;

        visual->transform[0] = visual->transform[1] =
            coord == 0 ? VKL_TRANSFORM_AXIS_X : VKL_TRANSFORM_AXIS_Y;

        // Text params.
        VklFontAtlas* atlas = vkl_font_atlas(ctx);
        ASSERT(strlen(atlas->font_str) > 0);
        vkl_visual_texture(visual, VKL_SOURCE_TYPE_FONT_ATLAS, 1, atlas->texture);

        VklGraphicsTextParams params = {0};
        params.grid_size[0] = (int32_t)atlas->rows;
        params.grid_size[1] = (int32_t)atlas->cols;
        params.tex_size[0] = (int32_t)atlas->width;
        params.tex_size[1] = (int32_t)atlas->height;
        vkl_visual_data_buffer(visual, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);
    }
    // Add the axes data.
    _axes_ticks_init(controller);
}

static void _axes_destroy(VklController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == VKL_CONTROLLER_AXES_2D);
    VklAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t i = 0; i < 2; i++)
    {
        vkl_array_destroy(&axes->ticks[i]);
        vkl_array_destroy(&axes->text[i]);
        FREE(axes->buf[i]);
    }
}



static VklAxesTicks vkl_ticks(double vmin, double vmax, VklAxesContext ctx)
{
    // TODO: better choice for the initial number of labels
    R r = wilk_ext(vmin, vmax, 12, ctx);
    ASSERT(r.lstep > 0);
    ASSERT(r.lmin < r.lmax);

    VklAxesTicks out = {0};
    out.format = r.f;
    out.vmin = vmin;
    out.vmax = vmax;
    out.lmin = r.lmin;
    out.lmax = r.lmax;
    out.lstep = r.lstep;

    // Generate the values between lmin and lmax.
    double x = 0;
    uint32_t n = floor(1 + (r.lmax - r.lmin) / r.lstep);
    out.value_count = n;
    out.values = calloc(n, sizeof(double));
    ASSERT(n >= 2);
    ASSERT(r.lmin + (n - 1) * r.lstep <= r.lmax);
    ASSERT(r.lmin + n * r.lstep >= r.lmax);
    for (uint32_t i = 0; i < n; i++)
    {
        x = r.lmin + i * r.lstep;
        ASSERT(x <= r.lmax);
        out.values[i] = x;
    }
    ctx.labels = calloc(n * MAX_GLYPHS_PER_TICK, sizeof(char));
    make_labels(r.f, r.lmin, r.lmax, r.lstep, ctx);
    out.labels = ctx.labels;
    return out;
}


static void vkl_ticks_destroy(VklAxesTicks* ticks)
{
    ASSERT(ticks!=NULL);
    FREE(ticks->values);
    FREE(ticks->labels);
}



#endif
