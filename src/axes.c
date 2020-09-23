#include "axes.h"
#include "../external/exwilk.h"
#include "../include/visky/utils.h"
#include "../include/visky/visky.h"

// BEGIN_INCL_NO_WARN
// #include <stb_image.h>
// END_INCL_NO_WARN



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

VkyAxesTickRange vky_axes_get_ticks(double dmin, double dmax, VkyAxesContext context)
{
    // log_trace("get tick %.4f %.4f\n", dmin, dmax);
    double dpi_factor = context.dpi_factor;
    int32_t m = (int)ceil(
        VKY_AXES_DEFAULT_TICK_COUNT * context.viewport_size[context.coord] /
        (1800.0 * dpi_factor));
    // log_trace("launching Extended Wilkinson algorithm for tick location");
    R result = wilk_ext(dmin, dmax, m, 0, context);
    return (VkyAxesTickRange){result.lmin, result.lmax, result.lstep, 0, 0, 0, result.f};
}


dvec2s vky_axes_normalize_pos(VkyAxes* axes, dvec2s pos)
{
    double xmin = axes->xscale.vmin;
    double xmax = axes->xscale.vmax;

    double ymin = axes->yscale.vmin;
    double ymax = axes->yscale.vmax;

    double x = -1.0 + 2 * (pos.x - xmin) / (xmax - xmin);
    double y = -1.0 + 2 * (pos.y - ymin) / (ymax - ymin);

    return (dvec2s){x, y};
}


void vky_axes_set_box(VkyAxes* axes, VkyAxesBox box) {}


VkyAxesBox vky_axes_get_box(VkyAxes* axes)
{
    // Normalization of the ticks to convert into NDC coordinates.
    VkyAxesScale xscale = axes->xscale_orig;
    VkyAxesScale yscale = axes->yscale_orig;

    double alphax = axes->panzoom_box.xmin;
    double betax = axes->panzoom_box.xmax;
    double alphay = axes->panzoom_box.ymin;
    double betay = axes->panzoom_box.ymax;

    double mx = xscale.vmin;
    double Mx = xscale.vmax;
    double my = yscale.vmin;
    double My = yscale.vmax;

    double ux = mx + .5 * (Mx - mx) * (alphax + 1.0);
    double vx = mx + .5 * (Mx - mx) * (betax + 1.0);
    double uy = my + .5 * (My - my) * (alphay + 1.0);
    double vy = my + .5 * (My - my) * (betay + 1.0);

    double ax = 2.0 / (vx - ux);
    double ay = 2.0 / (vy - uy);
    double bx = .5 * (vx + ux);
    double by = .5 * (vy + uy);

    VkyAxesBox box = {0};
    box.xmin = -1 / ax + bx;
    box.xmax = +1 / ax + bx;
    box.ymin = -1 / ay + by;
    box.ymax = +1 / ay + by;

    return box;
}


void vky_axes_reset(VkyAxes* axes)
{
    // call vky_axes_set_box()
}


void vky_axes_register_visual(VkyAxes* axis, VkyVisual* visual) {}



/*************************************************************************************************/
/*  Axes tick/grid visual                                                                        */
/*************************************************************************************************/

static VkyData vky_axes_tick_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 4 * data.item_count;
    uint32_t ni = 6 * data.item_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    // log_trace("allocating vertices and indices");
    VkyAxesTickVertex* vertices = calloc(nv, sizeof(VkyAxesTickVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));
    const VkyAxesTickVertex* items = (const VkyAxesTickVertex*)data.items;

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            vertices[4 * i + j].tick = items[i].tick;
            vertices[4 * i + j].coord_level = items[i].coord_level;
            ASSERT(4 * i + j < nv);
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}

VkyVisual* vky_axes_create_tick_visual(VkyScene* scene, VkyAxes* axes)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AXES_TICK);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(scene->canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "axes.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "axes.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(scene->canvas->gpu, 0, sizeof(VkyAxesTickVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32_SFLOAT, offsetof(VkyAxesTickVertex, tick));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8_UINT, offsetof(VkyAxesTickVertex, coord_level));

    float dpif = scene->canvas->dpi_factor;
    VkyAxesTickParams params = {0};

    // Axes margins.
    glm_vec4_copy(axes->panel->margins, params.margins);

    // Tick line widths.
    params.linewidths[0] = dpif * VKY_AXES_TICK_LINEWIDTH_MINOR;
    params.linewidths[1] = dpif * VKY_AXES_TICK_LINEWIDTH_MAJOR;
    params.linewidths[2] = dpif * VKY_AXES_TICK_LINEWIDTH_GRID;
    params.linewidths[3] = dpif * VKY_AXES_TICK_LINEWIDTH_LIM;

    // Tick colors.
    params.colors[0][0] = VKY_AXES_TICK_COLOR_R;
    params.colors[0][1] = VKY_AXES_TICK_COLOR_G;
    params.colors[0][2] = VKY_AXES_TICK_COLOR_B;
    params.colors[0][3] = VKY_AXES_TICK_COLOR_A;

    params.colors[1][0] = VKY_AXES_TICK_COLOR_R;
    params.colors[1][1] = VKY_AXES_TICK_COLOR_G;
    params.colors[1][2] = VKY_AXES_TICK_COLOR_B;
    params.colors[1][3] = VKY_AXES_TICK_COLOR_A;

    params.colors[2][0] = VKY_AXES_GRID_COLOR_R;
    params.colors[2][1] = VKY_AXES_GRID_COLOR_G;
    params.colors[2][2] = VKY_AXES_GRID_COLOR_B;
    params.colors[2][3] = VKY_AXES_GRID_COLOR_A;

    params.colors[3][0] = VKY_AXES_LIM_COLOR_R;
    params.colors[3][1] = VKY_AXES_LIM_COLOR_G;
    params.colors[3][2] = VKY_AXES_LIM_COLOR_B;
    params.colors[3][3] = VKY_AXES_LIM_COLOR_A;

    // User line widths.
    params.user_linewidths[0] = dpif * axes->user.linewidths[0];
    params.user_linewidths[1] = dpif * axes->user.linewidths[1];
    params.user_linewidths[2] = dpif * axes->user.linewidths[2];
    params.user_linewidths[3] = dpif * axes->user.linewidths[3];

    // User colors.
    glm_mat4_copy(axes->user.colors, params.user_colors);

    // Tick lengths in pixels.
    params.tick_lengths[0] = dpif * VKY_AXES_TICK_LENGTH_MINOR;
    params.tick_lengths[1] = dpif * VKY_AXES_TICK_LENGTH_MAJOR;

    // Tick line cap type.
    params.cap = VKY_CAP_SQUARE;

    // Params.
    vky_visual_params(visual, sizeof(VkyAxesTickParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        scene->canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout,
        resource_layout, (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_axes_tick_bake;

    return visual;
}



/*************************************************************************************************/
/*  Axes text visual                                                                             */
/*************************************************************************************************/

static VkyData vky_axes_text_bake(VkyVisual* visual, VkyData data)
{

    ASSERT(data.items != NULL); // TODO: support allocation with no upload by specifying a max
                                // number of glyphs per string
    ASSERT(data.item_count > 0);

    // Input text as array of VkyAxesTextData.
    const VkyAxesTextData* text = (const VkyAxesTextData*)data.items;

    // Count the number of glyphs.
    uint32_t glyph_count = 0;
    for (uint32_t i = 0; i < data.item_count; i++)
    { // vertex_count is the number of strings here
        glyph_count += text[i].string_len;
    }

    uint32_t nv = glyph_count;
    ASSERT(nv > 0);

    data.vertex_count = nv * 4;
    data.index_count = 0; // we don't use the index buffer

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    VkyAxesTextVertex* vertices = calloc(data.vertex_count, sizeof(VkyAxesTextVertex));

    uint32_t k = 0;
    VkyAxesTextVertex vertex = {0};
    // Go through all strings.
    for (uint32_t i = 0; i < data.item_count; i++)
    {
        uint32_t str_len = text[i].string_len;
        // Compute the anchor to align with the dot.
        dvec2s anchor = (text[i].coord == 0) ? (dvec2s){0, VKY_AXES_TICK_ANCHOR_X}
                                             : (dvec2s){VKY_AXES_TICK_ANCHOR_Y, 0};
        // For each string, go through the chars.
        for (uint32_t j = 0; j < str_len; j++)
        {
            char c[] = {text[i].string[j]};
            uint32_t ci = strcspn(VKY_TEXT_CHARS, c);
            vertex = (VkyAxesTextVertex){
                text[i].tick,
                text[i].coord,
                {anchor.x, anchor.y},
                {ci, j, str_len, i}, // char, charIdx, strLen, strIdx
            };
            for (uint32_t u = 0; u < 4; u++)
            {
                vertices[4 * k + u] = vertex;
            }
            k++;
        }
    }

    data.vertices = vertices;
    data.indices = NULL;

    return data;
}

VkyVisual* vky_axes_create_text_visual(VkyScene* scene, VkyAxes* axes)
{
    VkyCanvas* canvas = scene->canvas;
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AXES_TEXT);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "axes_text.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "axes_text.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyAxesTextVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32_SFLOAT, offsetof(VkyAxesTextVertex, tick));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32_UINT, offsetof(VkyAxesTextVertex, coord));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyAxesTextVertex, anchor));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VK_FORMAT_R16G16B16A16_UINT, offsetof(VkyAxesTextVertex, glyph));

    // Font texture.
    VkyTexture* texture = vky_get_font_texture(canvas->gpu);

    // UBO params.
    int width = (int)texture->params.width;
    int height = (int)texture->params.height;
    float glyph_height = VKY_AXES_FONT_SIZE * canvas->dpi_factor;
    float glyph_width = glyph_height / height * width * 6 / 16.;
    VkyAxesTextParams params = {
        VKY_FONT_TEXTURE_SHAPE,
        {width, height},
        {glyph_width, glyph_height},
        {VKY_AXES_TEXT_COLOR_R, VKY_AXES_TEXT_COLOR_G, VKY_AXES_TEXT_COLOR_B,
         VKY_AXES_TEXT_COLOR_A},
    };
    vky_visual_params(visual, sizeof(VkyAxesTextParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, texture);

    visual->cb_bake_data = vky_axes_text_bake;

    return visual;
}



/*************************************************************************************************/
/*  Axes panzoom functions                                                                       */
/*************************************************************************************************/

VkyAxesBox vky_panzoom_get_box(VkyPanzoom* panzoom)
{
    VkyAxesBox box = {
        -1 / panzoom->zoom[0] + panzoom->camera_pos[0],
        +1 / panzoom->zoom[0] + panzoom->camera_pos[0],
        -1 / panzoom->zoom[1] + panzoom->camera_pos[1],
        +1 / panzoom->zoom[1] + panzoom->camera_pos[1],
    };
    return box;
}

void vky_axes_panzoom_update(VkyAxes* axes, VkyPanzoom* panzoom, bool force_trigger)
{
    VkyPanzoom* axpanzoom = axes->panzoom;

    int32_t xlevel = (int)round(log2(panzoom->zoom[0]));
    int32_t ylevel = (int)round(log2(panzoom->zoom[1]));

    double zxlevel = pow(2, xlevel);
    double zylevel = pow(2, ylevel);

    int32_t xoffset = (int)round(panzoom->camera_pos[0] * zxlevel);
    int32_t yoffset = (int)round(panzoom->camera_pos[1] * zylevel);

    bool trigger =
        (force_trigger || abs(axes->xdyad.level - xlevel) >= (int32_t)VKY_AXES_DYAD_TRIGGER ||
         abs(axes->ydyad.level - ylevel) >= (int32_t)VKY_AXES_DYAD_TRIGGER ||
         abs(axes->xdyad.offset - xoffset) >= (int32_t)VKY_AXES_DYAD_TRIGGER ||
         abs(axes->ydyad.offset - yoffset) >= (int32_t)VKY_AXES_DYAD_TRIGGER);

    // Force trigger on panzoom reset.
    VkyMouse* mouse = axes->panel->scene->canvas->event_controller->mouse;
    if (mouse->cur_state == VKY_MOUSE_STATE_DOUBLE_CLICK)
    {
        trigger = true;
    }

    if (!trigger)
        return;
    log_trace("axes panzoom update: recompute new tick extents");

    // Axes extent.
    axes->panzoom_box = vky_panzoom_get_box(panzoom);

    // Reset the axes panzoom.
    axpanzoom->camera_pos[0] = 0;
    axpanzoom->camera_pos[1] = 0;
    axpanzoom->zoom[0] = 1;
    axpanzoom->zoom[1] = 1;

    axes->xdyad = (VkyAxesDyad){xlevel, xoffset};
    axes->ydyad = (VkyAxesDyad){ylevel, yoffset};

    // Center of the original view.
    double xc = .5 * (axes->xscale_orig.vmin + axes->xscale_orig.vmax);
    double yc = .5 * (axes->yscale_orig.vmin + axes->yscale_orig.vmax);

    // Size of the original view.
    double w = .5 * (axes->xscale_orig.vmax - axes->xscale_orig.vmin);
    double h = .5 * (axes->yscale_orig.vmax - axes->yscale_orig.vmin);

    // Update xscale and yscale.
    xc += xoffset * .5 * (axes->xscale_orig.vmax - axes->xscale_orig.vmin) / zxlevel;
    yc += yoffset * .5 * (axes->yscale_orig.vmax - axes->yscale_orig.vmin) / zylevel;

    axes->xscale.vmin = xc - w / zxlevel;
    axes->xscale.vmax = xc + w / zxlevel;

    axes->yscale.vmin = yc - h / zylevel;
    axes->yscale.vmax = yc + h / zylevel;

    // Update the axes: recompute the ticks and update the axes visual.
    vky_axes_update(axes);
}

void vky_axes_set_range(VkyAxes* axes, double xmin, double xmax, double ymin, double ymax)
{
    ASSERT(axes != NULL);
    ASSERT(axes->panel != NULL);

    VkyPanzoom* panzoom = ((VkyControllerAxes2D*)axes->panel->controller)->panzoom;
    ASSERT(panzoom != NULL);

    VkyPanzoom* axpanzoom = axes->panzoom;
    ASSERT(axpanzoom != NULL);

    bool update_x = xmin < xmax;
    bool update_y = ymin < ymax;

    if (update_x)
    {
        log_trace("update x axis");

        axes->xscale.vmin = xmin;
        axes->xscale.vmax = xmax;

        axes->xscale_orig.vmin = xmin;
        axes->xscale_orig.vmax = xmax;
    }

    if (update_y)
    {
        log_trace("update y axis");

        axes->yscale.vmin = ymin;
        axes->yscale.vmax = ymax;

        axes->yscale_orig.vmin = ymin;
        axes->yscale_orig.vmax = ymax;
    }

    if (update_x || update_y)
    {
        vky_axes_panzoom_update(axes, panzoom, true);
        vky_axes_update(axes);
    }
}



/*************************************************************************************************/
/*  Main axes functions                                                                          */
/*************************************************************************************************/

VKY_INLINE void _get_tick_format(VkyAxes* axes, int32_t axis, char* fmt)
{
    VkyTickFormat vtf = {0};
    if (axis == 0)
        vtf = axes->xticks.format;
    else
        vtf = axes->yticks.format;

    strcpy(fmt, "%.XF"); // [2] = precision, [3] = f or e
    sprintf(&fmt[2], "%d", vtf.precision);
    switch (vtf.format_type)
    {
    case VKY_TICK_FORMAT_DECIMAL:
        fmt[3] = 'f';
        break;
    case VKY_TICK_FORMAT_SCIENTIFIC:
        fmt[3] = 'e';
        break;
    default:
        break;
    }
}

static void _tick_formatter(VkyAxes* axes, int32_t axis, double value, char* out_text)
{
    char tick_format[16] = {0};
    _get_tick_format(axes, axis, tick_format);
    snprintf(out_text, VKY_AXES_MAX_GLYPHS_PER_TICK, tick_format, value);
}

static void _tick_formatter_x(VkyAxes* axes, double value, char* out_text)
{
    _tick_formatter(axes, 0, value, out_text);
}

static void _tick_formatter_y(VkyAxes* axes, double value, char* out_text)
{
    _tick_formatter(axes, 1, value, out_text);
}



VkyAxes* vky_axes_init(VkyPanel* panel, VkyAxes2DParams params)
{
    log_trace("axes init");
    VkyScene* scene = panel->scene;
    VkyCanvas* canvas = scene->canvas;
    VkyAxes* axes = calloc(1, sizeof(VkyAxes));
    axes->panel = panel;
    axes->xscale = axes->xscale_orig = params.xscale;
    axes->yscale = axes->yscale_orig = params.yscale;

    // Formatters can be changed by the user.
    axes->xtick_fmt = params.xtick_fmt != NULL ? params.xtick_fmt : _tick_formatter_x;
    axes->ytick_fmt = params.ytick_fmt != NULL ? params.ytick_fmt : _tick_formatter_y;

    // User axes
    axes->user = params.user;

    // Update the margins.
    glm_vec4_scale(params.margins, canvas->dpi_factor, panel->margins);

    // Initialize the Panzoom instance.
    axes->panzoom = vky_panzoom_init();

    // Create the visuals.
    axes->tick_visual = vky_axes_create_tick_visual(scene, axes);
    axes->text_visual = vky_axes_create_text_visual(scene, axes);

    // Initialize the buffers.
    axes->tick_data = calloc(VKY_AXES_MAX_VERTICES, sizeof(VkyAxesTickVertex));
    axes->text_data = calloc(VKY_AXES_MAX_STRINGS, sizeof(VkyAxesTextData));
    axes->str_buffer = calloc(VKY_AXES_MAX_GLYPHS, sizeof(char));

    // Pre-allocate the vertex/index buffers.
    vky_allocate_vertex_buffer(
        axes->tick_visual,
        VKY_AXES_MAX_VERTICES * 4 * axes->tick_visual->pipeline.vertex_layout.stride);
    vky_allocate_index_buffer(axes->tick_visual, VKY_AXES_MAX_VERTICES * 6 * sizeof(VkyIndex));
    vky_allocate_vertex_buffer(
        axes->text_visual, VKY_AXES_MAX_GLYPHS * axes->text_visual->pipeline.vertex_layout.stride);

    // Initialize the axes visual.
    axes->panzoom_box =
        (VkyAxesBox){axes->xscale.vmin, axes->xscale.vmax, axes->yscale.vmin, axes->yscale.vmax};
    vky_axes_update(axes);

    return axes;
}



void vky_axes_make_vertices(
    VkyAxes* axes, uint32_t* vertex_count, VkyAxesTickVertex* vertices,
    uint32_t* text_vertex_count, VkyAxesTextData* text_data)
{

    // Compute the total number of ticks.
    const uint32_t CACHE = VKY_AXES_CACHE_LEN;
    const uint32_t NP = VKY_AXES_DYAD_TRIGGER;
    const uint32_t MIT = VKY_AXES_MINOR_TICKS_COUNT;
    const int32_t NX =
        1 + (int32_t)round((axes->xticks.vmax - axes->xticks.vmin) / axes->xticks.step);
    const int32_t NY =
        1 + (int32_t)round((axes->yticks.vmax - axes->yticks.vmin) / axes->yticks.step);
    ASSERT(NX >= 1);
    ASSERT(NY >= 1);

    ASSERT(NP >= 1);
    ASSERT(CACHE >= 1);
    ASSERT(2 * NP + 1 == CACHE);

    // Size of the initial view and tick intervals.
    double w = axes->xscale.vmax - axes->xscale.vmin;
    ASSERT(w >= 0);
    double h = axes->yscale.vmax - axes->yscale.vmin;
    ASSERT(h >= 0);
    double dw = (axes->xticks.vmax - axes->xticks.vmin) / (NX - 1);
    ASSERT(dw >= 0);
    double dh = (axes->yticks.vmax - axes->yticks.vmin) / (NY - 1);
    ASSERT(dh >= 0);

    // How much the ticks should be extended to cover the cache.
    int32_t Lxmin = ceil((axes->xticks.vmin - axes->xscale.vmin + NP * w) / dw);
    int32_t Lymin = ceil((axes->yticks.vmin - axes->yscale.vmin + NP * h) / dh);

    int32_t Lxmax = ceil((axes->xscale.vmax + NP * w - axes->xticks.vmax) / dw);
    int32_t Lymax = ceil((axes->yscale.vmax + NP * h - axes->yticks.vmax) / dh);

    // Major and minor ticks, cache included.
    const int32_t NXC = Lxmin + NX + Lxmax + 1;
    const int32_t NYC = Lymin + NY + Lymax + 1;
    const int32_t NxC = (NXC - 1) * ((int32_t)MIT - 1);
    const int32_t NyC = (NYC - 1) * ((int32_t)MIT - 1);

    double* TX = calloc((size_t)NXC, sizeof(double));
    double* TY = calloc((size_t)NYC, sizeof(double));
    double* Tx = calloc((size_t)NxC, sizeof(double));
    double* Ty = calloc((size_t)NyC, sizeof(double));

    // Minor ticks, major ticks, grid, lims, user.
    const int64_t N = 2 * NXC + 2 * NYC + NxC + NyC + 2 + (int32_t)axes->user.tick_count;
    *vertex_count = N;
    *text_vertex_count = (uint32_t)(NXC + NYC);

    // k : -Lxmin ... 0 ... NX-1 NX+0 NX+1 ..... NX + Lxmax
    int32_t offset;

    char* str = axes->str_buffer;

    // Normalization of the ticks to convert into NDC coordinates.
    VkyAxesScale xscale = axes->xscale_orig;
    VkyAxesScale yscale = axes->yscale_orig;

    double alphax = axes->panzoom_box.xmin;
    double betax = axes->panzoom_box.xmax;
    double alphay = axes->panzoom_box.ymin;
    double betay = axes->panzoom_box.ymax;

    double mx = xscale.vmin;
    double Mx = xscale.vmax;
    double my = yscale.vmin;
    double My = yscale.vmax;

    double ux = mx + .5 * (Mx - mx) * (alphax + 1.0);
    double vx = mx + .5 * (Mx - mx) * (betax + 1.0);
    double uy = my + .5 * (My - my) * (alphay + 1.0);
    double vy = my + .5 * (My - my) * (betay + 1.0);

    double ax = 2.0 / (vx - ux);
    double ay = 2.0 / (vy - uy);
    double bx = .5 * (vx + ux);
    double by = .5 * (vy + uy);

    // printf("ax=%.4f, bx=%.4f, ay=%.4f, by=%.4f\n", ax, bx, ay, by);

    // Major xticks.
    offset = 0;
    for (int32_t k = -Lxmin; k <= NX + Lxmax; k++)
    {
        double tick = axes->xticks.vmin + dw * k;
        TX[Lxmin + k] = tick;
        offset++;

        uint32_t char_offset = (uint32_t)offset * VKY_AXES_MAX_GLYPHS_PER_TICK;
        ASSERT(axes->xtick_fmt != NULL);
        axes->xtick_fmt(axes, tick, &str[char_offset]);
        uint32_t ns = strlen(&str[char_offset]);
        ASSERT(ns < VKY_AXES_MAX_GLYPHS_PER_TICK);
        text_data[Lxmin + k] = (VkyAxesTextData){
            ax * (tick - bx),
            0,
            ns,
            &str[char_offset],
        };
    }
    ASSERT(offset == NXC);

    // Major yticks.
    offset = 0;
    for (int32_t k = -Lymin; k <= NY + Lymax; k++)
    {
        double tick = axes->yticks.vmin + dh * k;
        TY[Lymin + k] = tick;
        offset++;

        uint32_t char_offset = (uint32_t)(offset + NXC) * VKY_AXES_MAX_GLYPHS_PER_TICK;
        ASSERT(axes->ytick_fmt != NULL);
        axes->ytick_fmt(axes, tick, &str[char_offset]);
        uint32_t ns = strlen(&str[char_offset]);
        ASSERT(ns < VKY_AXES_MAX_GLYPHS_PER_TICK);
        text_data[NXC + Lymin + k] = (VkyAxesTextData){
            ay * (tick - by),
            1,
            ns,
            &str[char_offset],
        };
    }
    ASSERT(offset == NYC);

    // NOTE: we clean the rest of the text vertex buffer as we draw everything.
    for (int32_t i = NXC + NYC; i < (int32_t)(VKY_AXES_MAX_STRINGS); i++)
    {
        text_data[i] = (VkyAxesTextData){0};
    }

    // Minor xticks.
    offset = 0;
    for (int32_t k = -Lxmin; k < NX + Lxmax; k++)
    { // go through all major ticks incl cache
        for (int32_t j = 0; j < (int32_t)MIT - 1; j++)
        { // add the minor ticks
            double u = (j + 1) * dw / MIT;
            ASSERT(0 < u && u < dw);
            Tx[offset + j] = TX[Lxmin + k] + u;
        }
        offset += (int32_t)(MIT - 1);
    }
    ASSERT(offset == NxC);

    // Minor yticks.
    offset = 0;
    for (int32_t k = -Lymin; k < NY + Lymax; k++)
    { // go through all major ticks incl cache
        for (int32_t j = 0; j < (int32_t)MIT - 1; j++)
        { // add the minor ticks
            double u = (j + 1) * dh / MIT;
            ASSERT(0 < u && u < dh);
            Ty[offset + j] = TY[Lymin + k] + u;
        }
        offset += (int32_t)(MIT - 1);
    }
    ASSERT(offset == NyC);

    // Fill the vertices array.
    offset = 0;
    int32_t m = 0;

    // Major xticks.
    m = NXC;
    for (int32_t i = 0; i < m; i++)
    {
        vertices[offset + 2 * i + 0] = (VkyAxesTickVertex){ax * (TX[i] - bx), 1}; // major xtick
        vertices[offset + 2 * i + 1] = (VkyAxesTickVertex){ax * (TX[i] - bx), 2}; // x grid
    }
    offset += 2 * m;

    // Major yticks.
    m = NYC;
    for (int32_t i = 0; i < m; i++)
    {
        vertices[offset + 2 * i + 0] = (VkyAxesTickVertex){ay * (TY[i] - by), 5}; // major ytick
        vertices[offset + 2 * i + 1] = (VkyAxesTickVertex){ay * (TY[i] - by), 6}; // y grid
    }
    offset += 2 * m;

    // Minor xticks.
    m = NxC;
    for (int32_t i = 0; i < m; i++)
    {
        vertices[offset + i] = (VkyAxesTickVertex){ax * (Tx[i] - bx), 0};
    }
    offset += m;

    // Minor yticks.
    m = NyC;
    for (int32_t i = 0; i < m; i++)
    {
        vertices[offset + i] = (VkyAxesTickVertex){ay * (Ty[i] - by), 4};
    }
    offset += m;

    ASSERT(offset == N - axes->user.tick_count - 2);

    // TODO: log scale

    // Lims
    vertices[N - axes->user.tick_count - 2] = (VkyAxesTickVertex){-1, 3};
    vertices[N - axes->user.tick_count - 1] = (VkyAxesTickVertex){-1, 7};

    // User ticks
    double a, b;
    for (uint32_t i = 0; i < axes->user.tick_count; i++)
    {
        a = axes->user.levels[i] <= 3 ? ax : ay;
        b = axes->user.levels[i] <= 3 ? bx : by;
        vertices[N - axes->user.tick_count + i] =
            (VkyAxesTickVertex){a * (axes->user.ticks_ndc[i] - b), 8 + axes->user.levels[i]};
    }

    free(TX);
    free(TY);
    free(Tx);
    free(Ty);
}



void vky_axes_update(VkyAxes* axes)
{
    memset(axes->tick_data, 0, VKY_AXES_MAX_VERTICES);
    memset(axes->text_data, 0, VKY_AXES_MAX_STRINGS);
    memset(axes->str_buffer, 0, VKY_AXES_MAX_GLYPHS);

    // Get the axes context.
    VkyCanvas* canvas = axes->tick_visual->scene->canvas;
    const VkyAxesTextParams* params = (const VkyAxesTextParams*)axes->text_visual->params;
    float glyph_width = params->glyph_size[0];
    float glyph_height = params->glyph_size[1];
    float viewport_width = axes->panel->viewport.w * canvas->size.framebuffer_width;
    float viewport_height = axes->panel->viewport.h * canvas->size.framebuffer_height;

    ASSERT(viewport_width > 0);
    ASSERT(viewport_height > 0);

    // Find the ticks.
    VkyAxesContext context = {
        0,
        {glyph_width, glyph_height},
        {viewport_width, viewport_height},
        canvas->dpi_factor,
        false};
    axes->xticks = vky_axes_get_ticks(axes->xscale.vmin, axes->xscale.vmax, context);
    context.coord = 1;
    axes->yticks = vky_axes_get_ticks(axes->yscale.vmin, axes->yscale.vmax, context);

    // Generate the tick vertices.
    uint32_t vertex_count, text_vertex_count;
    vky_axes_make_vertices(
        axes, &vertex_count, axes->tick_data, &text_vertex_count, axes->text_data);

    // Bake them for the segment visual and upload to the GPU.
    ASSERT(vertex_count > 0);
    ASSERT(VKY_AXES_MAX_VERTICES >= vertex_count);

    // log_trace("update axes visual %d", vertex_count);
    vky_visual_upload(axes->tick_visual, (VkyData){vertex_count, axes->tick_data});

    // log_trace("update axes text visual %d", text_vertex_count);
    vky_visual_upload(axes->text_visual, (VkyData){text_vertex_count, axes->text_data});
}
