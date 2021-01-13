#include "test_graphics.h"
#include "../include/visky/colormaps.h"
#include "../include/visky/graphics.h"
#include "../include/visky/mesh.h"
#include "../src/mesh_loader.h"
#include "utils.h"



/*************************************************************************************************/
/*  Graphics utils                                                                               */
/*************************************************************************************************/

typedef struct TestGraphics TestGraphics;

struct TestGraphics
{
    VklCanvas* canvas;
    VklGraphics* graphics;
    VklBufferRegions br_vert;
    VklBufferRegions br_index;
    VklBufferRegions br_mvp;
    VklBufferRegions br_viewport;
    VklBufferRegions br_params;
    VklTexture* texture;
    VklBindings bindings;
    VklInteract interact;
    VklMVP mvp;
    vec3 eye, center, up;

    VklArray vertices;
    VklArray indices;
    float param;
    void* data;
};

static void _graphics_refill(VklCanvas* canvas, VklEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    VklCommands* cmds = ev.u.rf.cmds[0];
    VklBufferRegions* br = &tg->br_vert;
    VklBufferRegions* br_index = &tg->br_index;
    VklBindings* bindings = &tg->bindings;
    VklGraphics* graphics = tg->graphics;
    uint32_t idx = ev.u.rf.img_idx;

    // Commands.
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, *br, 0);
    if (br_index->buffer != NULL)
        vkl_cmd_bind_index_buffer(cmds, idx, *br_index, 0);
    vkl_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);
    if (graphics->pipeline != VK_NULL_HANDLE)
    {
        if (br_index->buffer != VK_NULL_HANDLE)
        {
            log_debug("draw indexed %d", tg->indices.item_count);
            vkl_cmd_draw_indexed(cmds, idx, 0, 0, tg->indices.item_count);
        }
        else
        {
            log_debug("draw non-indexed %d", tg->vertices.item_count);
            vkl_cmd_draw(cmds, idx, 0, tg->vertices.item_count);
        }
    }
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

static void _common_bindings(TestGraphics* tg)
{
    VklGpu* gpu = tg->graphics->gpu;
    VklGraphics* graphics = tg->graphics;

    // Create the bindings.
    tg->bindings = vkl_bindings(&graphics->slots, tg->canvas->swapchain.img_count);

    // Binding resources.
    tg->br_mvp = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_UNIFORM_MAPPABLE, tg->canvas->swapchain.img_count,
        sizeof(VklMVP));
    tg->br_viewport =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklViewport));

    // Upload MVP.
    glm_mat4_identity(tg->mvp.model);
    glm_mat4_identity(tg->mvp.view);
    glm_mat4_identity(tg->mvp.proj);
    vkl_upload_buffers(tg->canvas, tg->br_mvp, 0, sizeof(VklMVP), &tg->mvp);

    // Bindings
    vkl_bindings_buffer(&tg->bindings, 0, tg->br_mvp);
    vkl_bindings_buffer(&tg->bindings, 1, tg->br_viewport);
}

static void _interact_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    TestGraphics* tg = ev.user_data;
    ASSERT(tg != NULL);

    vkl_interact_update(&tg->interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    vkl_upload_buffers(canvas, tg->br_mvp, 0, sizeof(VklMVP), &tg->interact.mvp);
}



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MESH VKL_MESH_CUBE



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define INIT_GRAPHICS(type, flags)                                                                \
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);                                                      \
    VklGpu* gpu = vkl_gpu(app, 0);                                                                \
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, VKL_CANVAS_FLAGS_FPS);           \
    VklGraphics* graphics = vkl_graphics_builtin(canvas, type, flags);

#define BEGIN_DATA(type, n, user_data)                                                            \
    TestGraphics tg = {0};                                                                        \
    tg.canvas = canvas;                                                                           \
    tg.graphics = graphics;                                                                       \
    tg.up[1] = 1;                                                                                 \
    tg.eye[2] = 3;                                                                                \
    tg.vertices = vkl_array_struct(0, sizeof(type));                                              \
    tg.indices = vkl_array_struct(0, sizeof(VklIndex));                                           \
    VklGraphicsData data = vkl_graphics_data(graphics, &tg.vertices, &tg.indices, user_data);     \
    vkl_graphics_alloc(&data, n);                                                                 \
    uint32_t item_count = n;                                                                      \
    uint32_t vertex_count = tg.vertices.item_count;                                               \
    uint32_t index_count = tg.indices.item_count;                                                 \
    tg.br_vert =                                                                                  \
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_VERTEX, 1, vertex_count * sizeof(type));    \
    if (index_count > 0)                                                                          \
        tg.br_index = vkl_ctx_buffers(                                                            \
            gpu->context, VKL_BUFFER_TYPE_INDEX, 1, index_count * sizeof(VklIndex));              \
    type* vertices = tg.vertices.data;

#define END_DATA                                                                                  \
    ASSERT(item_count > 0);                                                                       \
    ASSERT(vertex_count > 0);                                                                     \
    ASSERT(index_count == 0 || index_count > 0);                                                  \
    ASSERT(vertices != NULL);                                                                     \
    vkl_upload_buffers(                                                                           \
        canvas, tg.br_vert, 0, vertex_count* tg.vertices.item_size, tg.vertices.data);            \
    if (index_count > 0)                                                                          \
        vkl_upload_buffers(                                                                       \
            canvas, tg.br_index, 0, index_count* tg.indices.item_size, tg.indices.data);

#define BINDINGS_PARAMS                                                                           \
    _common_bindings(&tg);                                                                        \
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);                            \
    vkl_bindings_update(&tg.bindings);

#define BINDINGS_NO_PARAMS                                                                        \
    _common_bindings(&tg);                                                                        \
    vkl_bindings_update(&tg.bindings);

#define RUN                                                                                       \
    vkl_event_callback(canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _graphics_refill, &tg);  \
    vkl_app_run(app, N_FRAMES);                                                                   \
    vkl_array_destroy(&tg.vertices);                                                              \
    vkl_array_destroy(&tg.indices);



/*************************************************************************************************/
/*  Misc graphics tests */
/*************************************************************************************************/

static void _graphics_points_wheel_callback(VklCanvas* canvas, VklEvent ev)
{
    TestGraphics* tg = ev.user_data;

    // Update point size.
    tg->param += ev.u.w.dir[1] * .5;
    tg->param = CLIP(tg->param, 1, 100);
    vkl_upload_buffers(canvas, tg->br_params, 0, sizeof(VklGraphicsPointParams), &tg->param);
}

static void _graphics_update_mvp(VklCanvas* canvas, VklEvent ev)
{
    TestGraphics* tg = ev.user_data;

    // Update MVP.
    tg->mvp.model[0][0] = .1 * tg->param;
    tg->mvp.model[1][1] = .1 * tg->param;
    vkl_upload_buffers(canvas, tg->br_mvp, 0, sizeof(VklMVP), &tg->mvp);
}

int test_graphics_dynamic(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS, 0)
    BEGIN_DATA(VklVertex, 10000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    END_DATA

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg.br_mvp = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(VklMVP));
    tg.br_viewport = vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, 16);
    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsPointParams));

    // Upload MVP.
    glm_mat4_identity(tg.mvp.model);
    glm_mat4_identity(tg.mvp.view);
    glm_mat4_identity(tg.mvp.proj);
    vkl_upload_buffers(canvas, tg.br_mvp, 0, sizeof(VklMVP), &tg.mvp);

    // Upload params.
    tg.param = 5.0f;
    VklGraphicsPointParams params = {.point_size = tg.param};
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsPointParams), &params);

    // Bindings
    vkl_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    vkl_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    vkl_bindings_update(&tg.bindings);

    vkl_event_callback(
        canvas, VKL_EVENT_MOUSE_WHEEL, 0, VKL_EVENT_MODE_SYNC, _graphics_points_wheel_callback,
        &tg);
    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _graphics_update_mvp, &tg);

    RUN;
    TEST_END
}



static void _graphics_3D_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(ev.type = VKL_EVENT_FRAME);
    TestGraphics* tg = ev.user_data;
    vec3 axis = {0};
    axis[1] = 1;
    glm_rotate_make(tg->mvp.model, ev.u.f.time, axis);
    vkl_mvp_camera(canvas->viewport, tg->eye, tg->center, (vec2){.1, 100}, &tg->mvp);

    vkl_upload_buffers(canvas, tg->br_mvp, 0, sizeof(VklMVP), &tg->mvp);
}

int test_graphics_3D(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS, VKL_GRAPHICS_FLAGS_DEPTH_TEST)
    BEGIN_DATA(VklVertex, 3, NULL)
    {
        // Top red
        vertices[0].pos[0] = 0;
        vertices[0].pos[1] = .5;
        vertices[0].color[0] = 255;
        vertices[0].color[3] = 255;

        // Bottom left green
        vertices[1].pos[0] = -.5;
        vertices[1].pos[1] = -.5;
        vertices[1].color[1] = 255;
        vertices[1].color[3] = 255;

        // Bottom right blue
        vertices[2].pos[0] = +.5;
        vertices[2].pos[1] = -.5;
        vertices[2].color[2] = 255;
        vertices[2].color[3] = 255;
    }
    END_DATA

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg.br_mvp = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(VklMVP));
    tg.br_viewport = vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, 16);
    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsPointParams));

    // Upload params.
    tg.param = 200.0f;
    VklGraphicsPointParams params = {.point_size = tg.param};
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsPointParams), &params);

    // Bindings
    vkl_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    vkl_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    vkl_bindings_update(&tg.bindings);

    vkl_event_callback(
        canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _graphics_3D_callback, &tg);

    RUN;
    TEST_END
}



int test_graphics_depth(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_MESH, 0)
    const uint32_t N = 1000;
    BEGIN_DATA(VklGraphicsMeshVertex, N * 3, NULL)
    _depth_vertices(N, vertices, false);
    END_DATA

    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsMeshParams));

    _common_bindings(&tg);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    for (uint32_t i = 1; i <= 4; i++)
        vkl_bindings_texture(
            &tg.bindings, VKL_USER_BINDING + i, gpu->context->color_texture.texture);
    vkl_bindings_update(&tg.bindings);

    VklGraphicsMeshParams params = default_graphics_mesh_params(tg.eye);
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsMeshParams), &params);

    RUN;
    TEST_END
}



/*************************************************************************************************/
/*  Basic graphics tests                                                                         */
/*************************************************************************************************/

int test_graphics_points(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS, 0)
    BEGIN_DATA(VklVertex, 10000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    END_DATA

    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsPointParams));
    BINDINGS_PARAMS

    tg.param = 5.0f;
    VklGraphicsPointParams params = {.point_size = tg.param};
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsPointParams), &params);

    RUN;
    TEST_END
}



int test_graphics_lines(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_LINES, 0)
    BEGIN_DATA(VklVertex, 100, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        float t = (float)(i / 2) / (float)vertex_count;
        vertices[i].pos[0] = .75 * (-1 + 4 * t);
        vertices[i].pos[1] = .75 * (-1 + (i % 2 == 0 ? 0 : 2));
        vkl_colormap_scale(VKL_CMAP_RAINBOW, t, 0, .5, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    TEST_END
}



int test_graphics_line_strip(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_LINE_STRIP, 0)
    BEGIN_DATA(VklVertex, 1000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        float t = (float)i / (float)vertex_count;
        vertices[i].pos[0] = -1 + 2 * t;
        vertices[i].pos[1] = .5 * sin(8 * M_2PI * t);
        vkl_colormap_scale(VKL_CMAP_RAINBOW, t, 0, 1, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    TEST_END
}



int test_graphics_triangles(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TRIANGLES, 0)
    const uint32_t N = 100;
    BEGIN_DATA(VklVertex, N * 3, NULL)

    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[3 * i].pos)
        RAND_COLOR(vertices[3 * i].color)
        vertices[3 * i].pos[2] = 0;
        vertices[3 * i].color[3] = rand_byte();

        // Copy the 2 other points per triangle.
        glm_vec3_copy(vertices[3 * i].pos, vertices[3 * i + 1].pos);
        glm_vec3_copy(vertices[3 * i].pos, vertices[3 * i + 2].pos);
        memcpy(vertices[3 * i + 1].color, vertices[3 * i].color, sizeof(cvec4));
        memcpy(vertices[3 * i + 2].color, vertices[3 * i].color, sizeof(cvec4));

        // Shift the points.
        float ms = .1 * rand_float();
        vertices[3 * i + 0].pos[0] -= ms;
        vertices[3 * i + 1].pos[0] += ms;
        vertices[3 * i + 0].pos[1] -= ms;
        vertices[3 * i + 1].pos[1] -= ms;
        vertices[3 * i + 2].pos[1] += ms;
    }

    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    TEST_END
}



int test_graphics_triangle_strip(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TRIANGLE_STRIP, 0)
    BEGIN_DATA(VklVertex, 50, NULL)
    float m = .05;
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        float t = (float)i / (float)(vertex_count - 1);
        float a = M_2PI * t;
        vertices[i].pos[0] = (.75 + (i % 2 == 0 ? +m : -m)) * cos(a);
        vertices[i].pos[1] = (.75 + (i % 2 == 0 ? +m : -m)) * sin(a);
        vkl_colormap_scale(VKL_CMAP_HSV, t, 0, 1, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    TEST_END
}



int test_graphics_triangle_fan(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TRIANGLE_FAN, 0)
    BEGIN_DATA(VklVertex, 20, NULL)
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        float t = (float)i / (float)(vertex_count - 1);
        float a = M_2PI * t;
        vertices[i].pos[0] = .75 * cos(a);
        vertices[i].pos[1] = .75 * sin(a);
        vkl_colormap_scale(VKL_CMAP_HSV, t, 0, 1, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    TEST_END
}



/*************************************************************************************************/
/*  Agg marker tests                                                                             */
/*************************************************************************************************/

int test_graphics_marker(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_MARKER, 0)
    BEGIN_DATA(VklGraphicsMarkerVertex, 1000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
        vertices[i].color[3] = 196;
        vertices[i].size = 20 + rand_float() * 50;
        vertices[i].marker = VKL_MARKER_DISC;
    }
    END_DATA

    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsPointParams));
    BINDINGS_PARAMS

    VklGraphicsMarkerParams params = {0};
    params.edge_color[0] = 1;
    params.edge_color[1] = 1;
    params.edge_color[2] = 1;
    params.edge_color[3] = 1;
    params.edge_width = 2;
    // params.enable_depth
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsMarkerParams), &params);

    RUN;
    TEST_END
}



/*************************************************************************************************/
/*  Agg segment tests                                                                            */
/*************************************************************************************************/

static void _resize(VklCanvas* canvas, VklEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    vkl_upload_buffers(canvas, tg->br_viewport, 0, sizeof(VklViewport), &canvas->viewport);
}

int test_graphics_segment(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_SEGMENT, 0)
    const uint32_t N = 16;
    BEGIN_DATA(VklGraphicsSegmentVertex, 4 * N, NULL)

    VklGraphicsSegmentVertex vertex = {0};
    for (uint32_t i = 0; i < N; i++)
    {
        float t = (float)i / (float)N;
        float x = .75 * (-1 + 2 * t);
        float y = .75;
        vertex.P0[0] = vertex.P1[0] = x;
        vertex.P0[1] = y;
        vertex.P1[1] = -y;
        vertex.linewidth = 5 + 30 * t;
        vkl_colormap_scale(VKL_CMAP_RAINBOW, t, 0, 1, vertex.color);
        vertex.cap0 = vertex.cap1 = i % VKL_CAP_COUNT;
        vkl_graphics_append(&data, &vertex);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    vkl_event_callback(canvas, VKL_EVENT_RESIZE, 0, VKL_EVENT_MODE_SYNC, _resize, &tg);
    RUN;
    TEST_END
}



/*************************************************************************************************/
/*  Text tests                                                                                   */
/*************************************************************************************************/

int test_graphics_text(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TEXT, 0)
    const uint32_t N = 26;
    const char str[] = "Hello world!";
    const uint32_t offset = strlen(str);

    // Font atlas
    VklFontAtlas* atlas = &gpu->context->font_atlas;

    VklGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;

    // 26 letters in a circle.
    BEGIN_DATA(VklGraphicsTextVertex, (N + offset), atlas)
    float t = 0;
    float a = 0, x = 0, y = 0;
    VklGraphicsTextItem item = {0};
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)N;
        a = M_2PI * t;
        x = .75 * cos(a);
        y = .75 * sin(a);
        item.vertex.pos[0] = x;
        item.vertex.pos[1] = y;
        item.vertex.angle = -a;
        item.font_size = 30;
        char s[2] = {0};
        s[0] = (char)(65 + i);
        item.string = s;
        vkl_colormap_scale(VKL_CMAP_HSV, t, 0, 1, item.vertex.color);

        vkl_graphics_append(&data, &item);
    }

    // Hello world
    item.glyph_colors = calloc(offset, sizeof(cvec4));
    for (uint32_t i = 0; i < offset; i++)
    {
        vkl_colormap_scale(VKL_CMAP_RAINBOW, i, 0, offset, item.glyph_colors[i]);
    }
    item.vertex.pos[0] = 0;
    item.vertex.pos[1] = 0;
    item.vertex.angle = 0;
    item.font_size = 50;
    item.string = str;
    vkl_graphics_append(&data, &item);
    FREE(item.glyph_colors);

    END_DATA

    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsTextParams));
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsTextParams), &params);

    _common_bindings(&tg);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    vkl_bindings_texture(&tg.bindings, VKL_USER_BINDING + 1, atlas->texture);
    vkl_bindings_update(&tg.bindings);

    vkl_event_callback(canvas, VKL_EVENT_RESIZE, 0, VKL_EVENT_MODE_SYNC, _resize, &tg);

    RUN;
    TEST_END
}



/*************************************************************************************************/
/*  Image tests                                                                                  */
/*************************************************************************************************/

int test_graphics_image(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_IMAGE, 0)

    // Vertices.
    uint32_t n = 6;
    TestGraphics tg = {0};
    tg.canvas = canvas;
    tg.graphics = graphics;
    tg.vertices = vkl_array_struct(n, sizeof(VklGraphicsImageVertex));
    ASSERT(tg.vertices.item_count == n);
    tg.br_vert = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_VERTEX, 1,
        tg.vertices.item_count * sizeof(VklGraphicsImageVertex));
    float x = 1;
    VklGraphicsImageVertex vertices[] = {
        {{-x, -x, 0}, {0, 1}}, //
        {{+x, -x, 0}, {1, 1}}, //
        {{+x, +x, 0}, {1, 0}}, //
        {{+x, +x, 0}, {1, 0}}, //
        {{-x, +x, 0}, {0, 0}}, //
        {{-x, -x, 0}, {0, 1}}, //
    };
    vkl_upload_buffers(
        canvas, tg.br_vert, 0, tg.vertices.item_count * sizeof(VklGraphicsImageVertex), vertices);

    // Parameters.
    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsImageParams));
    VklGraphicsImageParams params = {0};
    params.tex_coefs[0] = 1;
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsImageParams), &params);

    // Texture.
    const uint32_t nt = 16;
    VklTexture* texture =
        vkl_ctx_texture(gpu->context, 2, (uvec3){nt, nt, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    cvec4* tex_data = calloc(nt * nt, sizeof(cvec4));
    for (uint32_t i = 0; i < nt * nt; i++)
    {
        tex_data[i][i % 3] = 255;
        tex_data[i][3] = 255;
    }
    vkl_upload_texture(
        canvas, texture, VKL_ZERO_OFFSET, VKL_ZERO_OFFSET, nt * nt * sizeof(cvec4), tex_data);

    // Bindings.
    _common_bindings(&tg);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    for (uint32_t i = 1; i <= 4; i++)
        vkl_bindings_texture(&tg.bindings, VKL_USER_BINDING + i, texture);
    vkl_bindings_update(&tg.bindings);

    RUN;
    FREE(tex_data)
    TEST_END
}



/*************************************************************************************************/
/*  Volume image tests                                                                           */
/*************************************************************************************************/

static void _graphics_volume_callback(VklCanvas* canvas, VklEvent ev)
{
    TestGraphics* tg = ev.user_data;
    float dx = ev.u.f.interval;
    for (uint32_t i = 0; i < 6; i++)
        ((VklGraphicsVolumeVertex*)tg->vertices.data)[i].uvw[2] += .1 * dx;
    vkl_upload_buffers(
        canvas, tg->br_vert, 0, tg->vertices.item_count * sizeof(VklGraphicsVolumeVertex),
        tg->vertices.data);
}

int test_graphics_volume_slice(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_VOLUME_SLICE, 0)

    // Vertices.
    uint32_t n = 6;
    TestGraphics tg = {0};
    tg.canvas = canvas;
    tg.graphics = graphics;
    tg.vertices = vkl_array_struct(n, sizeof(VklGraphicsVolumeVertex));
    ASSERT(tg.vertices.item_count == n);
    tg.br_vert = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_VERTEX, 1, tg.vertices.item_count * tg.vertices.item_size);
    float x = .5;
    VklGraphicsVolumeVertex vertices[] = {
        {{-x, -x, 0}, {0, 1, 0.5}}, //
        {{+x, -x, 0}, {1, 1, 0.5}}, //
        {{+x, +x, 0}, {1, 0, 0.5}}, //
        {{+x, +x, 0}, {1, 0, 0.5}}, //
        {{-x, +x, 0}, {0, 0, 0.5}}, //
        {{-x, -x, 0}, {0, 1, 0.5}}, //
    };
    memcpy(tg.vertices.data, vertices, sizeof(vertices));
    vkl_upload_buffers(
        canvas, tg.br_vert, 0, tg.vertices.item_count * tg.vertices.item_size, tg.vertices.data);

    // Parameters.
    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsVolumeParams));
    VklGraphicsVolumeParams params = {0};
    params.cmap = VKL_CMAP_HSV;

    params.cmap_coefs[0][0] = 0.0;
    params.cmap_coefs[0][1] = .333;
    params.cmap_coefs[0][2] = .666;
    params.cmap_coefs[0][3] = 1.0;

    params.cmap_coefs[1][0] = 0;
    params.cmap_coefs[1][1] = .2;
    params.cmap_coefs[1][2] = .8;
    params.cmap_coefs[1][3] = 1;

    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsVolumeParams), &params);

    // Texture.
    const uint32_t nt = 12;
    VklTexture* texture =
        vkl_ctx_texture(gpu->context, 3, (uvec3){nt, nt, nt}, VK_FORMAT_R8_UNORM);
    // WARNING: nearest filter causes visual artifacts when sampling from a 3D texture close to the
    // boundaries between different values
    vkl_texture_filter(texture, VKL_FILTER_MAX, VK_FILTER_NEAREST);
    vkl_texture_address_mode(texture, VKL_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkl_texture_address_mode(texture, VKL_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkl_texture_address_mode(texture, VKL_TEXTURE_AXIS_W, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    uint8_t* tex_data = calloc(nt * nt * nt, sizeof(uint8_t));
    for (uint32_t i = 0; i < nt * nt * nt; i++)
        tex_data[i] = i % 256;
    vkl_upload_texture(
        canvas, texture, VKL_ZERO_OFFSET, VKL_ZERO_OFFSET, //
        nt * nt * nt * sizeof(uint8_t), tex_data);

    // Bindings.1
    _common_bindings(&tg);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    vkl_bindings_texture(&tg.bindings, VKL_USER_BINDING + 1, gpu->context->color_texture.texture);
    vkl_bindings_texture(&tg.bindings, VKL_USER_BINDING + 2, texture);
    vkl_bindings_update(&tg.bindings);

    // Interactivity.
    tg.interact = vkl_interact_builtin(canvas, VKL_INTERACT_ARCBALL);
    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _interact_callback, &tg);
    vkl_event_callback(
        canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _graphics_volume_callback, &tg);

    RUN;
    FREE(tex_data)
    TEST_END
}



/*************************************************************************************************/
/*  Mesh tests                                                                                   */
/*************************************************************************************************/

static VklMesh _graphics_mesh_example(VklMeshType type)
{
    switch (type)
    {
    case VKL_MESH_SURFACE:;
        const uint32_t N = 250;
        uint32_t col_count = N + 1;
        uint32_t row_count = 2 * N + 1;
        uint32_t point_count = col_count * row_count;
        float* heights = calloc(point_count, sizeof(float));
        float w = 1;
        float x, y, z;
        for (uint32_t i = 0; i < row_count; i++)
        {
            x = (float)i / (row_count - 1);
            x = -w + 2 * w * x;
            for (uint32_t j = 0; j < col_count; j++)
            {
                y = (float)j / (col_count - 1);
                y = -w + 2 * w * y;
                z = .25 * sin(10 * x) * cos(10 * y);
                heights[col_count * i + j] = z;
            }
        }
        VklMesh mesh = vkl_mesh_surface(row_count, col_count, heights);
        FREE(heights);
        return mesh;
        break;

    case VKL_MESH_CUBE:
        return vkl_mesh_cube();

    case VKL_MESH_SPHERE:
        return vkl_mesh_sphere(100, 100);

    case VKL_MESH_CYLINDER:
        return vkl_mesh_cylinder(100);

    case VKL_MESH_CONE:
        return vkl_mesh_cone(100);

    case VKL_MESH_SQUARE:
        return vkl_mesh_square();

    case VKL_MESH_DISC:
        return vkl_mesh_disc(100);

    case VKL_MESH_OBJ:;
        char path[1024];
        snprintf(path, sizeof(path), "%s/mesh/%s", DATA_DIR, "brain.obj");
        return vkl_mesh_obj(path);

    default:
        break;
    }

    return (VklMesh){0};
}

int test_graphics_mesh(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_MESH, 0)

    TestGraphics tg = {0};
    tg.canvas = canvas;
    tg.eye[2] = 3;
    tg.up[1] = 1;
    tg.graphics = graphics;
    VklMesh mesh = _graphics_mesh_example(MESH);

    tg.vertices = mesh.vertices;
    tg.indices = mesh.indices;
    uint32_t vertex_count = tg.vertices.item_count;
    uint32_t index_count = tg.indices.item_count;
    tg.br_vert = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_VERTEX, 1, vertex_count * sizeof(VklGraphicsMeshVertex));
    if (index_count > 0)
        tg.br_index = vkl_ctx_buffers(
            gpu->context, VKL_BUFFER_TYPE_INDEX, 1, index_count * sizeof(VklIndex));

    vkl_upload_buffers(
        canvas, tg.br_vert, 0, vertex_count * tg.vertices.item_size, tg.vertices.data);
    if (index_count > 0)
        vkl_upload_buffers(
            canvas, tg.br_index, 0, index_count * tg.indices.item_size, tg.indices.data);

    // Texture.
    VklTexture* texture =
        vkl_ctx_texture(gpu->context, 2, (uvec3){2, 2, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    cvec4 tex_data[] = {
        {255, 0, 0, 255}, //
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 0, 255},
    };
    vkl_upload_texture(
        canvas, texture, VKL_ZERO_OFFSET, VKL_ZERO_OFFSET, sizeof(tex_data), tex_data);

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg.br_mvp = vkl_ctx_buffers(
        gpu->context, VKL_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(VklMVP));
    tg.br_viewport =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklViewport));

    // Parameters.
    VklGraphicsMeshParams params = default_graphics_mesh_params(tg.eye);

    tg.br_params =
        vkl_ctx_buffers(gpu->context, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsMeshParams));
    vkl_upload_buffers(canvas, tg.br_params, 0, sizeof(VklGraphicsMeshParams), &params);

    // Bindings
    vkl_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    vkl_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    vkl_bindings_buffer(&tg.bindings, VKL_USER_BINDING, tg.br_params);
    for (uint32_t i = 1; i <= 4; i++)
        vkl_bindings_texture(&tg.bindings, VKL_USER_BINDING + i, texture);
    vkl_bindings_update(&tg.bindings);

    // Interactivity.
    tg.interact = vkl_interact_builtin(canvas, VKL_INTERACT_ARCBALL);
    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _interact_callback, &tg);

    RUN;
    TEST_END
}
