#include "test_graphics.h"
#include "../include/visky/colormaps.h"
#include "../include/visky/graphics.h"
#include "utils.h"



/*************************************************************************************************/
/*  Graphics utils                                                                               */
/*************************************************************************************************/

typedef struct TestGraphics TestGraphics;

struct TestGraphics
{
    VklGraphics* graphics;
    VklBufferRegions br_vert;
    VklBufferRegions br_index;
    VklBufferRegions br_mvp;
    VklBufferRegions br_viewport;
    VklBufferRegions br_params;
    VklTexture* texture;
    VklBindings bindings;
    VklMVP mvp;
    vec3 eye, center, up;

    VklViewport viewport;
    uint32_t vertex_count;
    uint32_t index_count;
    VkDeviceSize size;
    float param;
    void* data;
};

static void _graphics_refill(VklCanvas* canvas, VklPrivateEvent ev)
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
    vkl_cmd_bind_vertex_buffer(cmds, idx, br, 0);
    if (br_index->buffer != NULL)
        vkl_cmd_bind_index_buffer(cmds, idx, br_index, 0);
    vkl_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);
    if (br_index->buffer != NULL)
        vkl_cmd_draw_indexed(cmds, idx, 0, 0, tg->index_count);
    else
        vkl_cmd_draw(cmds, idx, 0, tg->vertex_count);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

static void _common_bindings(TestGraphics* tg)
{
    VklGpu* gpu = tg->graphics->gpu;
    VklGraphics* graphics = tg->graphics;

    // Create the bindings.
    tg->bindings = vkl_bindings(&graphics->slots, 1);

    // Binding resources.
    tg->br_mvp = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklMVP));
    tg->br_viewport =
        vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklViewport));
    tg->texture = vkl_ctx_texture(gpu->context, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    // Upload MVP.
    glm_mat4_identity(tg->mvp.model);
    glm_mat4_identity(tg->mvp.view);
    glm_mat4_identity(tg->mvp.proj);
    vkl_upload_buffers(gpu->context, tg->br_mvp, 0, sizeof(VklMVP), &tg->mvp);

    // Bindings
    vkl_bindings_buffer(&tg->bindings, 0, tg->br_mvp);
    vkl_bindings_buffer(&tg->bindings, 1, tg->br_viewport);
    vkl_bindings_texture(&tg->bindings, 2, tg->texture);
}



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define INIT_GRAPHICS(type)                                                                       \
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);                                                      \
    VklGpu* gpu = vkl_gpu(app, 0);                                                                \
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);                                 \
    VklGraphics* graphics = vkl_graphics_builtin(canvas, type, 0);

#define BEGIN_DATA(type, n)                                                                       \
    TestGraphics tg = {0};                                                                        \
    tg.graphics = graphics;                                                                       \
    tg.vertex_count = (n);                                                                        \
    VkDeviceSize size = tg.vertex_count * sizeof(type);                                           \
    tg.br_vert = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);               \
    type* data = calloc(tg.vertex_count, sizeof(type));

#define END_DATA vkl_upload_buffers(gpu->context, tg.br_vert, 0, size, data);

#define BINDINGS_PARAMS                                                                           \
    _common_bindings(&tg);                                                                        \
    vkl_bindings_buffer(&tg.bindings, 3, tg.br_params);                                           \
    vkl_bindings_update(&tg.bindings);

#define BINDINGS_NO_PARAMS                                                                        \
    _common_bindings(&tg);                                                                        \
    vkl_bindings_update(&tg.bindings);

#define RUN                                                                                       \
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _graphics_refill, &tg);              \
    vkl_app_run(app, N_FRAMES);                                                                   \
    FREE(data);



/*************************************************************************************************/
/*  Misc graphics tests                                                                          */
/*************************************************************************************************/

static void _graphics_points_wheel_callback(VklCanvas* canvas, VklEvent ev)
{
    VklGpu* gpu = canvas->gpu;
    TestGraphics* tg = ev.user_data;

    // Update point size.
    tg->param += ev.u.w.dir[1] * .5;
    tg->param = CLIP(tg->param, 1, 100);
    vkl_upload_buffers(gpu->context, tg->br_params, 0, sizeof(VklGraphicsPointParams), &tg->param);

    // Update MVP.
    tg->mvp.model[0][0] = .1 * tg->param;
    tg->mvp.model[1][1] = .1 * tg->param;
    vkl_upload_buffers(gpu->context, tg->br_mvp, 0, sizeof(VklMVP), &tg->mvp);
}

int test_graphics_dynamic(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS)
    BEGIN_DATA(VklVertex, 10000)
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        RANDN_POS(data[i].pos)
        RAND_COLOR(data[i].color)
    }
    END_DATA

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots, 1);

    // Binding resources.
    tg.br_mvp = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklMVP));
    tg.br_viewport = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    tg.br_params = vkl_ctx_buffers(
        gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointParams));
    tg.texture = vkl_ctx_texture(gpu->context, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    // Upload MVP.
    glm_mat4_identity(tg.mvp.model);
    glm_mat4_identity(tg.mvp.view);
    glm_mat4_identity(tg.mvp.proj);
    vkl_upload_buffers(gpu->context, tg.br_mvp, 0, sizeof(VklMVP), &tg.mvp);

    // Upload params.
    tg.param = 5.0f;
    VklGraphicsPointParams params = {.point_size = tg.param};
    vkl_upload_buffers(gpu->context, tg.br_params, 0, sizeof(VklGraphicsPointParams), &params);

    // Bindings
    vkl_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    vkl_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    vkl_bindings_texture(&tg.bindings, 2, tg.texture);
    vkl_bindings_buffer(&tg.bindings, 3, tg.br_params);

    vkl_bindings_update(&tg.bindings);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _graphics_refill, &tg);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _graphics_points_wheel_callback, &tg);

    vkl_app_run(app, N_FRAMES);
    FREE(data);
    TEST_END
}



static void _graphics_3D_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    VklGpu* gpu = canvas->gpu;
    TestGraphics* tg = ev.user_data;
    vec3 axis;
    axis[1] = 1;
    glm_rotate_make(tg->mvp.model, .5 * ev.u.t.time, axis);
    vkl_upload_buffers(gpu->context, tg->br_mvp, 0, sizeof(VklMVP), &tg->mvp);
}

int test_graphics_3D(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS)
    BEGIN_DATA(VklVertex, 3)

    // Top red
    data[0].pos[0] = 0;
    data[0].pos[1] = .5;
    data[0].color[0] = 255;
    data[0].color[3] = 255;

    // Bottom left green
    data[1].pos[0] = -.5;
    data[1].pos[1] = -.5;
    data[1].color[1] = 255;
    data[1].color[3] = 255;

    // Bottom right blue
    data[2].pos[0] = +.5;
    data[2].pos[1] = -.5;
    data[2].color[2] = 255;
    data[2].color[3] = 255;

    END_DATA

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots, 1);

    // Binding resources.
    tg.br_mvp = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklMVP));
    tg.br_viewport = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    tg.br_params = vkl_ctx_buffers(
        gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointParams));
    tg.texture = vkl_ctx_texture(gpu->context, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    // Upload MVP.
    glm_mat4_identity(tg.mvp.model);
    glm_mat4_identity(tg.mvp.view);
    glm_mat4_identity(tg.mvp.proj);

    tg.eye[2] = 2;
    tg.up[1] = 1;
    glm_lookat(tg.eye, tg.center, tg.up, tg.mvp.view);
    float ratio = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    glm_perspective(GLM_PI_4, ratio, -1.0f, 1.0f, tg.mvp.proj);

    vkl_upload_buffers(gpu->context, tg.br_mvp, 0, sizeof(VklMVP), &tg.mvp);

    // Upload params.
    tg.param = 50.0f;
    VklGraphicsPointParams params = {.point_size = tg.param};
    vkl_upload_buffers(gpu->context, tg.br_params, 0, sizeof(VklGraphicsPointParams), &params);

    // Bindings
    vkl_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    vkl_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    vkl_bindings_texture(&tg.bindings, 2, tg.texture);
    vkl_bindings_buffer(&tg.bindings, 3, tg.br_params);

    vkl_bindings_update(&tg.bindings);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _graphics_refill, &tg);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1.0 / 60, _graphics_3D_callback, &tg);

    vkl_app_run(app, N_FRAMES);
    FREE(data);
    TEST_END
}



/*************************************************************************************************/
/*  Basic graphics tests                                                                         */
/*************************************************************************************************/

int test_graphics_points(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS)
    BEGIN_DATA(VklVertex, 10000)
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        RANDN_POS(data[i].pos)
        RAND_COLOR(data[i].color)
    }
    END_DATA

    tg.br_params = vkl_ctx_buffers(
        gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointParams));
    BINDINGS_PARAMS

    tg.param = 5.0f;
    VklGraphicsPointParams params = {.point_size = tg.param};
    vkl_upload_buffers(gpu->context, tg.br_params, 0, sizeof(VklGraphicsPointParams), &params);

    RUN TEST_END
}



int test_graphics_lines(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_LINES)
    BEGIN_DATA(VklVertex, 100)
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        float t = (float)(i / 2) / (float)tg.vertex_count;
        data[i].pos[0] = .75 * (-1 + 4 * t);
        data[i].pos[1] = .75 * (-1 + (i % 2 == 0 ? 0 : 2));
        vkl_colormap_scale(VKL_CMAP_RAINBOW, t, 0, .5, data[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN TEST_END
}



int test_graphics_line_strip(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_LINE_STRIP)
    BEGIN_DATA(VklVertex, 1000)
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        float t = (float)i / (float)tg.vertex_count;
        data[i].pos[0] = -1 + 2 * t;
        data[i].pos[1] = .5 * sin(8 * M_2PI * t);
        vkl_colormap_scale(VKL_CMAP_RAINBOW, t, 0, 1, data[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN TEST_END
}



int test_graphics_triangles(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TRIANGLES)
    const uint32_t N = 100;
    BEGIN_DATA(VklVertex, N * 3)

    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(data[3 * i].pos)
        RAND_COLOR(data[3 * i].color)
        data[3 * i].pos[2] = 0;
        data[3 * i].color[3] = rand_byte();

        // Copy the 2 other points per triangle.
        glm_vec3_copy(data[3 * i].pos, data[3 * i + 1].pos);
        glm_vec3_copy(data[3 * i].pos, data[3 * i + 2].pos);
        memcpy(data[3 * i + 1].color, data[3 * i].color, sizeof(cvec4));
        memcpy(data[3 * i + 2].color, data[3 * i].color, sizeof(cvec4));

        // Shift the points.
        float ms = .1 * rand_float();
        data[3 * i + 0].pos[0] -= ms;
        data[3 * i + 1].pos[0] += ms;
        data[3 * i + 0].pos[1] -= ms;
        data[3 * i + 1].pos[1] -= ms;
        data[3 * i + 2].pos[1] += ms;
    }

    END_DATA
    BINDINGS_NO_PARAMS
    RUN TEST_END
}



int test_graphics_triangle_strip(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TRIANGLE_STRIP)
    BEGIN_DATA(VklVertex, 50)
    float m = .05;
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        float t = (float)i / (float)(tg.vertex_count - 1);
        float a = M_2PI * t;
        data[i].pos[0] = (.75 + (i % 2 == 0 ? +m : -m)) * cos(a);
        data[i].pos[1] = (.75 + (i % 2 == 0 ? +m : -m)) * sin(a);
        vkl_colormap_scale(VKL_CMAP_HSV, t, 0, 1, data[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN TEST_END
}



int test_graphics_triangle_fan(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_TRIANGLE_FAN)
    BEGIN_DATA(VklVertex, 20)
    for (uint32_t i = 1; i < tg.vertex_count; i++)
    {
        float t = (float)i / (float)(tg.vertex_count - 1);
        float a = M_2PI * t;
        data[i].pos[0] = .75 * cos(a);
        data[i].pos[1] = .75 * sin(a);
        vkl_colormap_scale(VKL_CMAP_HSV, t, 0, 1, data[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN TEST_END
}



/*************************************************************************************************/
/*  Agg graphics tests                                                                           */
/*************************************************************************************************/

int test_graphics_marker(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_MARKER)
    BEGIN_DATA(VklGraphicsMarkerVertex, 1000)
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        RANDN_POS(data[i].pos)
        RAND_COLOR(data[i].color)
        data[i].color[3] = 196;
        data[i].size = 20 + rand_float() * 50;
        data[i].marker = VKL_MARKER_DISC;
    }
    END_DATA

    tg.br_params = vkl_ctx_buffers(
        gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointParams));
    BINDINGS_PARAMS

    VklGraphicsMarkerParams params = {0};
    params.edge_color[0] = 1;
    params.edge_color[1] = 1;
    params.edge_color[2] = 1;
    params.edge_color[3] = 1;
    params.edge_width = 2;
    // params.enable_depth
    vkl_upload_buffers(gpu->context, tg.br_params, 0, sizeof(VklGraphicsMarkerParams), &params);

    RUN TEST_END
}


static void _segment_resize(VklCanvas* canvas, VklPrivateEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    tg->viewport = vkl_viewport_full(canvas);
    vkl_upload_buffers(
        canvas->gpu->context, tg->br_viewport, 0, sizeof(VklViewport), &tg->viewport);
}

int test_graphics_segment(TestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_SEGMENT)
    const uint32_t N = 16;
    BEGIN_DATA(VklGraphicsSegmentVertex, 4 * N)

    tg.index_count = 6 * N;
    VklIndex* indices = calloc(6 * N, sizeof(VklIndex));
    VkDeviceSize index_buf_size = 6 * N * sizeof(VklIndex);
    tg.br_index = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_INDEX, 1, index_buf_size);

    for (uint32_t i = 0; i < N; i++)
    {
        float t = (float)i / (float)N;
        float x = .75 * (-1 + 2 * t);
        float y = .75;

        for (uint32_t j = 0; j < 4; j++)
        {
            data[4 * i + j].P0[0] = x;
            data[4 * i + j].P0[1] = -y;
            data[4 * i + j].P1[0] = x;
            data[4 * i + j].P1[1] = +y;

            vkl_colormap_scale(VKL_CMAP_RAINBOW, t, 0, 1, data[4 * i + j].color);

            data[4 * i + j].cap0 = data[4 * i + j].cap1 = i % VKL_CAP_COUNT;
            data[4 * i + j].linewidth = 5 + 30 * t;

            ASSERT(4 * i + j < 4 * N);
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;

        ASSERT(6 * i + 5 < 6 * N);
    }
    END_DATA
    BINDINGS_NO_PARAMS

    vkl_upload_buffers(gpu->context, tg.br_index, 0, index_buf_size, indices);
    tg.viewport = vkl_viewport_full(canvas);
    vkl_upload_buffers(gpu->context, tg.br_viewport, 0, sizeof(VklViewport), &tg.viewport);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_RESIZE, 0, _segment_resize, &tg);

    RUN;
    FREE(indices);
    TEST_END
}
