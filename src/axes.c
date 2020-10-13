#include "axes.h"
#include "../external/exwilk.h"
#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static VkyAxesTickRange _get_ticks(double dmin, double dmax, VkyAxesContext context)
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



/*************************************************************************************************/
/*  Axes visuals                                                                                 */
/*************************************************************************************************/

static void _tick_bake(VkyVisual* visual)
{
    VkyData* data = &visual->data;

    uint32_t nv = 4 * data->item_count;
    uint32_t ni = 6 * data->item_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data->vertex_count = nv;
    data->index_count = ni;

    if (data->items == NULL)
    {
        return;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    // log_trace("allocating vertices and indices");
    VkyAxesTickVertex* vertices = calloc(nv, sizeof(VkyAxesTickVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));
    const VkyAxesTickVertex* items = (const VkyAxesTickVertex*)data->items;

    for (uint32_t i = 0; i < data->item_count; i++)
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

    data->vertices = vertices;
    data->need_free_vertices = true;
    data->indices = indices;
    data->need_free_indices = true;
}


static void _invert_color(vec4 color)
{
    glm_vec3_subs(color, 1, color);
    glm_vec3_negate(color);
}


static bool _is_dark_mode(VkyScene* scene)
{
    return (scene->clear_color.rgb[0] == 0) && //
           (scene->clear_color.rgb[1] == 0) && //
           (scene->clear_color.rgb[2] == 0);
}


#define VISCOEF(x) ((int)((axes->visibility_flags & (x)) != 0))


void vky_axes_set_tick_params(VkyAxes* axes)
{
    VkyVisual* visual = axes->tick_visual;

    float dpif = visual->scene->canvas->dpi_factor;
    VkyAxesTickParams params = {0};

    // Axes margins.
    glm_vec4_copy(axes->panel->margins, params.margins);

    // Tick line widths.
    params.linewidths[0] = dpif * VKY_AXES_TICK_LINEWIDTH_MINOR;
    params.linewidths[1] = dpif * VKY_AXES_TICK_LINEWIDTH_MAJOR;
    params.linewidths[2] = dpif * VKY_AXES_TICK_LINEWIDTH_GRID;
    params.linewidths[3] = dpif * VKY_AXES_TICK_LINEWIDTH_LIM;

    // Tick colors.
    // Minor.
    params.colors[0][0] = VKY_AXES_TICK_COLOR_R;
    params.colors[0][1] = VKY_AXES_TICK_COLOR_G;
    params.colors[0][2] = VKY_AXES_TICK_COLOR_B;
    params.colors[0][3] = VKY_AXES_TICK_COLOR_A * VISCOEF(VKY_AXES_TICK_MINOR);

    // Major.
    params.colors[1][0] = VKY_AXES_TICK_COLOR_R;
    params.colors[1][1] = VKY_AXES_TICK_COLOR_G;
    params.colors[1][2] = VKY_AXES_TICK_COLOR_B;
    params.colors[1][3] = VKY_AXES_TICK_COLOR_A * VISCOEF(VKY_AXES_TICK_MAJOR);

    // Grid.
    params.colors[2][0] = VKY_AXES_GRID_COLOR_R;
    params.colors[2][1] = VKY_AXES_GRID_COLOR_G;
    params.colors[2][2] = VKY_AXES_GRID_COLOR_B;
    params.colors[2][3] = VKY_AXES_GRID_COLOR_A * VISCOEF(VKY_AXES_TICK_GRID);

    // Lim.
    params.colors[3][0] = VKY_AXES_LIM_COLOR_R;
    params.colors[3][1] = VKY_AXES_LIM_COLOR_G;
    params.colors[3][2] = VKY_AXES_LIM_COLOR_B;
    params.colors[3][3] = VKY_AXES_LIM_COLOR_A * VISCOEF(VKY_AXES_TICK_LIM);

    // HACK: handle dark mode. RGB = 1 - RGB
    if (_is_dark_mode(visual->scene))
    {
        for (uint32_t i = 0; i < 4; i++)
        {
            _invert_color(params.colors[i]);
        }
    }

    // User line widths.
    params.user_linewidths[0] = dpif * axes->user.linewidths[0];
    params.user_linewidths[1] = dpif * axes->user.linewidths[1];
    params.user_linewidths[2] = dpif * axes->user.linewidths[2];
    params.user_linewidths[3] = dpif * axes->user.linewidths[3];

    // User colors.
    glm_mat4_copy(axes->user.colors, params.user_colors);
    params.user_colors[0][3] *= VISCOEF(VKY_AXES_TICK_USER_0);
    params.user_colors[1][3] *= VISCOEF(VKY_AXES_TICK_USER_1);
    params.user_colors[2][3] *= VISCOEF(VKY_AXES_TICK_USER_2);
    params.user_colors[3][3] *= VISCOEF(VKY_AXES_TICK_USER_3);

    // Tick lengths in pixels.
    params.tick_lengths[0] = dpif * VKY_AXES_TICK_LENGTH_MINOR;
    params.tick_lengths[1] = dpif * VKY_AXES_TICK_LENGTH_MAJOR;

    // Tick line cap type.
    params.cap = VKY_CAP_SQUARE;

    // Params.
    vky_visual_params(visual, sizeof(VkyAxesTickParams), &params);
}


void vky_axes_toggle_tick(VkyAxes* axes, int tick_element)
{
    axes->visibility_flags ^= 1UL << (int)round(log2(tick_element));
    vky_axes_set_tick_params(axes);
}


static void _tick_visual(VkyScene* scene, VkyAxes* axes)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AXES_TICK);
    axes->tick_visual = visual;

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

    vky_axes_set_tick_params(axes);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        scene->canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout,
        resource_layout, (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = _tick_bake;
}


void vky_axes_set_text_params(VkyAxes* axes)
{
    VkyVisual* visual = axes->text_visual;

    // Font texture.
    VkyTexture* texture = vky_get_font_texture(visual->scene->canvas->gpu);

    // UBO params.
    int width = (int)texture->params.width;
    int height = (int)texture->params.height;
    float glyph_height = VKY_AXES_FONT_SIZE * visual->scene->canvas->dpi_factor;
    float glyph_width = glyph_height / height * width * 6 / 16.;
    VkyAxesTextParams params = {
        VKY_FONT_TEXTURE_SHAPE,
        {width, height},
        {glyph_width, glyph_height},
        {VKY_AXES_TEXT_COLOR_R, VKY_AXES_TEXT_COLOR_G, VKY_AXES_TEXT_COLOR_B,
         VKY_AXES_TEXT_COLOR_A},
    };
    // HACK: dark mode
    if (_is_dark_mode(visual->scene))
    {
        _invert_color(params.color);
    }
    vky_visual_params(visual, sizeof(VkyAxesTextParams), &params);
}


static void _text_bake(VkyVisual* visual)
{
    VkyData* data = &visual->data;

    ASSERT(data->items != NULL); // TODO: support allocation with no upload by specifying a max
                                 // number of glyphs per string
    ASSERT(data->item_count > 0);

    // Input text as array of VkyAxesTextData.
    const VkyAxesTextData* text = (const VkyAxesTextData*)data->items;

    // Count the number of glyphs.
    uint32_t glyph_count = 0;
    for (uint32_t i = 0; i < data->item_count; i++)
    { // vertex_count is the number of strings here
        glyph_count += text[i].string_len;
    }

    uint32_t nv = glyph_count;
    ASSERT(nv > 0);

    data->vertex_count = nv * 4;
    data->index_count = 0; // we don't use the index buffer

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    VkyAxesTextVertex* vertices = calloc(data->vertex_count, sizeof(VkyAxesTextVertex));

    uint32_t k = 0;
    VkyAxesTextVertex vertex = {0};
    // Go through all strings.
    for (uint32_t i = 0; i < data->item_count; i++)
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

    data->vertices = vertices;
    data->need_free_vertices = true;
    data->indices = NULL;
}


static void _text_visual(VkyScene* scene, VkyAxes* axes)
{
    VkyCanvas* canvas = scene->canvas;
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AXES_TEXT);
    axes->text_visual = visual;

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

    vky_axes_set_text_params(axes);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, vky_get_font_texture(visual->scene->canvas->gpu));

    visual->cb_bake_data = _text_bake;
}


static void _make_vertices(
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

    ASSERT(axes->xticks.vmin < axes->xticks.vmax);
    ASSERT(axes->yticks.vmin < axes->yticks.vmax);

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

    double alphax = axes->panzoom_box.pos_ll[0];
    double betax = axes->panzoom_box.pos_ur[0];
    double alphay = axes->panzoom_box.pos_ll[1];
    double betay = axes->panzoom_box.pos_ur[1];

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



/*************************************************************************************************/
/*  Main axes functions                                                                          */
/*************************************************************************************************/

VkyAxes* vky_axes_init(VkyPanel* panel, VkyAxes2DParams params)
{
    log_trace("axes init");
    VkyScene* scene = panel->scene;
    VkyCanvas* canvas = scene->canvas;
    VkyAxes* axes = calloc(1, sizeof(VkyAxes));
    axes->panel = panel;
    axes->enable_panzoom = params.enable_panzoom;

    axes->visibility_flags = VKY_AXES_TICK_ALL;
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
    axes->panzoom_outer = vky_panzoom_init();
    axes->panzoom_inner = vky_panzoom_init();

    // Create the visuals.
    _tick_visual(scene, axes);
    _text_visual(scene, axes);

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
        (VkyBox2D){{axes->xscale.vmin, axes->yscale.vmin}, {axes->xscale.vmax, axes->yscale.vmax}};
    vky_axes_compute_ticks(axes);
    vky_axes_update_visuals(axes);

    return axes;
}


void vky_axes_reset(VkyAxes* axes)
{
    axes->xscale = axes->xscale_orig;
    axes->yscale = axes->yscale_orig;
    axes->panzoom_box =
        (VkyBox2D){{axes->xscale.vmin, axes->yscale.vmin}, {axes->xscale.vmax, axes->yscale.vmax}};

    vky_panzoom_reset(axes->panzoom_outer);
    if (axes->panzoom_inner != NULL)
        vky_panzoom_reset(axes->panzoom_inner);

    vky_axes_compute_ticks(axes);
    vky_axes_update_visuals(axes);
}


void vky_axes_panzoom_update(VkyAxes* axes)
{
    if (!axes->enable_panzoom)
        return;
    // Reset lim_reached.
    axes->panzoom_inner->lim_reached[0] = false;
    axes->panzoom_inner->lim_reached[1] = false;

    // Main panel panzoom update, inner viewport.
    vky_panzoom_update(axes->panel, axes->panzoom_inner, VKY_VIEWPORT_INNER);

    // Now, lim_reached may have been set to true. In this case, we need to freeze the axes
    // panzoom as well.
    axes->panzoom_outer->lim_reached[0] = axes->panzoom_inner->lim_reached[0];
    axes->panzoom_outer->lim_reached[1] = axes->panzoom_inner->lim_reached[1];

    // We update the axes panzoom, outer viewport
    vky_panzoom_update(axes->panel, axes->panzoom_outer, VKY_VIEWPORT_OUTER);

    // Possibly trigger a tick recompute after panzoom.
    if (vky_axes_refill_needed(axes))
    {
        vky_axes_rescale(axes);
        vky_axes_compute_ticks(axes);
        vky_axes_update_visuals(axes);
    }
}


bool vky_axes_refill_needed(VkyAxes* axes)
{
    VkyPanzoom* panzoom = axes->panzoom_inner;
    // VkyPanzoom* axpanzoom = axes->panzoom;

    int32_t xlevel = (int)round(log2(panzoom->zoom[0]));
    int32_t ylevel = (int)round(log2(panzoom->zoom[1]));

    double zxlevel = pow(2, xlevel);
    double zylevel = pow(2, ylevel);

    int32_t xoffset = (int)round(panzoom->camera_pos[0] * zxlevel);
    int32_t yoffset = (int)round(panzoom->camera_pos[1] * zylevel);

    bool trigger =
        (abs(axes->xdyad.level - xlevel) >= (int32_t)VKY_AXES_DYAD_TRIGGER ||
         abs(axes->ydyad.level - ylevel) >= (int32_t)VKY_AXES_DYAD_TRIGGER ||
         abs(axes->xdyad.offset - xoffset) >= (int32_t)VKY_AXES_DYAD_TRIGGER ||
         abs(axes->ydyad.offset - yoffset) >= (int32_t)VKY_AXES_DYAD_TRIGGER);

    // Force trigger on panzoom reset.
    VkyMouse* mouse = axes->panel->scene->canvas->event_controller->mouse;
    if (mouse->cur_state == VKY_MOUSE_STATE_DOUBLE_CLICK)
    {
        trigger = true;
    }

    return trigger;
}


void vky_axes_rescale(VkyAxes* axes)
{
    VkyPanzoom* panzoom = axes->panzoom_inner;
    VkyPanzoom* axpanzoom = axes->panzoom_outer;

    int32_t xlevel = (int)round(log2(panzoom->zoom[0]));
    int32_t ylevel = (int)round(log2(panzoom->zoom[1]));

    double zxlevel = pow(2, xlevel);
    double zylevel = pow(2, ylevel);

    int32_t xoffset = (int)round(panzoom->camera_pos[0] * zxlevel);
    int32_t yoffset = (int)round(panzoom->camera_pos[1] * zylevel);

    // Axes extent.
    axes->panzoom_box = vky_panzoom_get_box(axes->panel, panzoom, VKY_VIEWPORT_INNER);

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
}


void vky_axes_compute_ticks(VkyAxes* axes)
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
    axes->xticks = _get_ticks(axes->xscale.vmin, axes->xscale.vmax, context);
    context.coord = 1;
    axes->yticks = _get_ticks(axes->yscale.vmin, axes->yscale.vmax, context);
}


void vky_axes_update_visuals(VkyAxes* axes)
{
    // Generate the tick vertices.
    uint32_t vertex_count, text_vertex_count;
    if (axes->xticks.vmin >= axes->xticks.vmax || axes->yticks.vmin >= axes->yticks.vmax)
        return;
    _make_vertices(axes, &vertex_count, axes->tick_data, &text_vertex_count, axes->text_data);

    // Bake them for the segment visual and upload to the GPU.
    ASSERT(vertex_count > 0);
    ASSERT(VKY_AXES_MAX_VERTICES >= vertex_count);

    // log_trace("update axes visual %d", vertex_count);
    axes->tick_visual->data.item_count = vertex_count;
    axes->tick_visual->data.items = axes->tick_data;
    vky_visual_data_raw(axes->tick_visual);

    // log_trace("update axes text visual %d", text_vertex_count);
    axes->text_visual->data.item_count = text_vertex_count;
    axes->text_visual->data.items = axes->text_data;
    vky_visual_data_raw(axes->text_visual);
}



/*************************************************************************************************/
/*  Axes range                                                                                   */
/*************************************************************************************************/

VkyBox2D vky_axes_get_range(VkyAxes* axes)
{
    ASSERT(axes != NULL);
    ASSERT(axes->panel != NULL);

    VkyAxesTransform tr = vky_axes_transform(axes->panel, VKY_CDS_PANZOOM, VKY_CDS_DATA);
    VkyBox2D box = {0};
    vky_axes_transform_apply(&tr, (dvec2){-1, -1}, box.pos_ll);
    vky_axes_transform_apply(&tr, (dvec2){+1, +1}, box.pos_ur);
    return box;
}


#define PBOX(x)                                                                                   \
    printf("%f %f %f %f\n", (x).pos_ll[0], (x).pos_ll[1], (x).pos_ur[0], (x).pos_ur[1]);



void vky_axes_set_initial_range(VkyAxes* axes, VkyBox2D box)
{
    axes->xscale_orig.vmin = axes->xscale.vmin = box.pos_ll[0];
    axes->yscale_orig.vmin = axes->yscale.vmin = box.pos_ll[1];
    axes->xscale_orig.vmax = axes->xscale.vmax = box.pos_ur[0];
    axes->yscale_orig.vmax = axes->yscale.vmax = box.pos_ur[1];
    vky_axes_compute_ticks(axes);
    vky_axes_update_visuals(axes);
}


void vky_axes_set_range(VkyAxes* axes, VkyBox2D box, bool refill)
{
    ASSERT(axes != NULL);
    ASSERT(axes->panel != NULL);

    VkyPanzoom* panzoom = axes->panzoom_inner;
    ASSERT(panzoom != NULL);

    VkyPanzoom* axpanzoom = axes->panzoom_outer;
    ASSERT(axpanzoom != NULL);

    bool update_x = box.pos_ll[0] < box.pos_ur[0];
    bool update_y = box.pos_ll[1] < box.pos_ur[1];

    if (!update_x && !update_y)
        return;

    VkyBox2D box_inner = {0};
    VkyAxesTransform tr_inner = {0};
    VkyAxesTransform tr_outer = {0};

    // Update the inner panzoom.
    tr_inner = vky_axes_transform(axes->panel, VKY_CDS_DATA, VKY_CDS_GPU);
    vky_axes_transform_apply(&tr_inner, box.pos_ll, box_inner.pos_ll);
    vky_axes_transform_apply(&tr_inner, box.pos_ur, box_inner.pos_ur);
    vky_panzoom_set_box(panzoom, VKY_VIEWPORT_INNER, box_inner);

    // Update the outer panzoom.
    tr_outer = vky_axes_transform_interp(
        box_inner.pos_ll, (dvec2){-1, -1}, box_inner.pos_ur, (dvec2){+1, +1});
    if (!refill && update_x)
    {
        axpanzoom->camera_pos[0] = tr_outer.shift[0];
        axpanzoom->zoom[0] = tr_outer.scale[0];
    }
    if (!refill && update_y)
    {
        axpanzoom->camera_pos[1] = tr_outer.shift[1];
        axpanzoom->zoom[1] = tr_outer.scale[1];
    }

    // Possibly trigger a tick recompute after panzoom.
    if (refill || vky_axes_refill_needed(axes))
    {
        vky_axes_rescale(axes);
        vky_axes_compute_ticks(axes);
        vky_axes_update_visuals(axes);
    }
}



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VkyAxesTransform vky_axes_transform_inv(VkyAxesTransform tr)
{
    ASSERT(tr.scale[0] != 0);
    ASSERT(tr.scale[1] != 0);

    VkyAxesTransform tri = {0};
    tri.scale[0] = 1. / tr.scale[0];
    tri.scale[1] = 1. / tr.scale[1];
    tri.shift[0] = -tr.scale[0] * tr.shift[0];
    tri.shift[1] = -tr.scale[1] * tr.shift[1];
    return tri;
}

VkyAxesTransform vky_axes_transform_mul(VkyAxesTransform tr0, VkyAxesTransform tr1)
{
    VkyAxesTransform trm = {0};
    trm.scale[0] = tr0.scale[0] * tr1.scale[0];
    trm.scale[1] = tr0.scale[1] * tr1.scale[1];
    trm.shift[0] = tr0.shift[0] + tr1.shift[0] / tr0.scale[0];
    trm.shift[1] = tr0.shift[1] + tr1.shift[1] / tr0.scale[1];
    return trm;
}

VkyAxesTransform vky_axes_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout)
{
    VkyAxesTransform tr = {0};
    tr.scale[0] = tr.scale[1] = 1;
    if (qin[0] != pin[0])
        tr.scale[0] = (qout[0] - pout[0]) / (qin[0] - pin[0]);
    if (qin[1] != pin[1])
        tr.scale[1] = (qout[1] - pout[1]) / (qin[1] - pin[1]);
    if (qout[0] != pout[0])
        tr.shift[0] = (pin[0] * qout[0] - pout[0] * qin[0]) / (qout[0] - pout[0]);
    if (qout[1] != pout[1])
        tr.shift[1] = (pin[1] * qout[1] - pout[1] * qin[1]) / (qout[1] - pout[1]);
    return tr;
}

void vky_axes_transform_apply(VkyAxesTransform* tr, dvec2 in, dvec2 out)
{
    if (tr->scale[0] != 0)
        out[0] = tr->scale[0] * (in[0] - tr->shift[0]);
    if (tr->scale[1] != 0)
        out[1] = tr->scale[1] * (in[1] - tr->shift[1]);
}

VkyAxesTransform vky_axes_transform(VkyPanel* panel, VkyCDS source, VkyCDS target)
{
    VkyAxesTransform tr = {{1, 1}, {0, 0}}; // identity
    dvec2 NDC0 = {-1, -1};
    dvec2 NDC1 = {+1, +1};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    VkyPanzoom* panzoom = NULL;

    if (panel->controller_type == VKY_CONTROLLER_AXES_2D)
    {
        VkyAxes* axes = (VkyAxes*)panel->controller;
        ll[0] = axes->xscale_orig.vmin;
        ll[1] = axes->yscale_orig.vmin;
        ur[0] = axes->xscale_orig.vmax;
        ur[1] = axes->yscale_orig.vmax;
        panzoom = axes->panzoom_inner;
    }
    else if (panel->controller_type == VKY_CONTROLLER_PANZOOM)
    {
        panzoom = (VkyPanzoom*)panel->controller;
    }
    else
    {
        log_error("controller other than axes 2D and panzoom not yet supported");
        return tr;
    }

    VkyViewport viewport = panel->viewport;

    if (source == target)
    {
        return tr;
    }
    else if (source > target)
    {
        return vky_axes_transform_inv(vky_axes_transform(panel, target, source));
    }
    else if (target - source >= 2)
    {
        for (uint32_t k = source; k <= target - 1; k++)
        {
            tr = vky_axes_transform_mul(tr, vky_axes_transform(panel, (VkyCDS)k, (VkyCDS)(k + 1)));
        }
    }
    else if (target - source == 1)
    {
        switch (source)
        {

        case VKY_CDS_DATA:
            // linear normalization based on axes range
            ASSERT(target == VKY_CDS_GPU);
            {
                tr = vky_axes_transform_interp(ll, NDC0, ur, NDC1);
            }
            break;

        case VKY_CDS_GPU:
            // apply panzoom
            ASSERT(target == VKY_CDS_PANZOOM);
            {
                dvec2 p = {panzoom->camera_pos[0], panzoom->camera_pos[1]};
                dvec2 s = {panzoom->zoom[0], panzoom->zoom[1]};
                tr.scale[0] = s[0];
                tr.scale[1] = s[1];
                tr.shift[0] = p[0]; // / s[0];
                tr.shift[1] = p[1]; // / s[1];
            }
            break;

        case VKY_CDS_PANZOOM:
            // using inner viewport
            ASSERT(target == VKY_CDS_PANEL);
            {
                // Margins.
                double cw = panel->scene->canvas->size.framebuffer_width;
                double ch = panel->scene->canvas->size.framebuffer_height;
                cw *= viewport.w;
                ch *= viewport.h;
                double mt = 2 * panel->margins[0] / ch;
                double mr = 2 * panel->margins[1] / cw;
                double mb = 2 * panel->margins[2] / ch;
                double ml = 2 * panel->margins[3] / cw;

                tr = vky_axes_transform_interp(
                    NDC0, (dvec2){-1 + ml, -1 + mb}, NDC1, (dvec2){+1 - mr, +1 - mt});
            }
            break;

        case VKY_CDS_PANEL:
            // multiply by canvas size
            ASSERT(target == VKY_CDS_CANVAS_NDC);
            {
                // From outer to inner viewport.
                ll[0] = -1 + 2 * viewport.x;
                ll[1] = +1 - 2 * (viewport.y + viewport.h);
                ur[0] = -1 + 2 * (viewport.x + viewport.w);
                ur[1] = +1 - 2 * viewport.y;

                tr = vky_axes_transform_interp(NDC0, ll, NDC1, ur);
            }
            break;

        case VKY_CDS_CANVAS_NDC:
            // multiply by canvas size
            ASSERT(target == VKY_CDS_CANVAS_PX);
            {
                double w = panel->scene->canvas->size.window_width;
                double h = panel->scene->canvas->size.window_height;

                tr = vky_axes_transform_interp(NDC0, (dvec2){0, h}, NDC1, (dvec2){w, 0});
            }
            break;

        default:
            log_error("unknown coordinate systems");
            break;
        }
    }
    return tr;
}
