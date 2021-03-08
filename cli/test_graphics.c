#include "test_graphics.h"
#include "../include/datoviz/colormaps.h"
#include "../include/datoviz/context.h"
#include "../include/datoviz/graphics.h"
#include "../include/datoviz/mesh.h"
#include "../src/interact_utils.h"
#include "utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MESH DVZ_MESH_SURFACE



/*************************************************************************************************/
/*  Graphics utils                                                                               */
/*************************************************************************************************/

typedef struct TestGraphics TestGraphics;

struct TestGraphics
{
    DvzCanvas* canvas;
    DvzGraphics* graphics;
    DvzBufferRegions br_vert;
    DvzBufferRegions br_index;
    DvzBufferRegions br_mvp;
    DvzBufferRegions br_viewport;
    DvzBufferRegions br_params;
    DvzTexture* texture;
    DvzBindings bindings;
    DvzInteract interact;
    DvzMVP mvp;
    vec3 eye, center, up;

    DvzArray vertices;
    DvzArray indices;
    float param;
    void* data;
    void* params_data;
};

static void _graphics_refill(DvzCanvas* canvas, DvzEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    DvzCommands* cmds = ev.u.rf.cmds[0];
    DvzBufferRegions* br = &tg->br_vert;
    DvzBufferRegions* br_index = &tg->br_index;
    DvzBindings* bindings = &tg->bindings;
    DvzGraphics* graphics = tg->graphics;
    uint32_t idx = ev.u.rf.img_idx;

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, *br, 0);
    if (br_index->buffer != NULL)
        dvz_cmd_bind_index_buffer(cmds, idx, *br_index, 0);
    dvz_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);
    if (graphics->pipeline != VK_NULL_HANDLE)
    {
        if (br_index->buffer != VK_NULL_HANDLE)
        {
            log_debug("draw indexed %d", tg->indices.item_count);
            dvz_cmd_draw_indexed(cmds, idx, 0, 0, tg->indices.item_count);
        }
        else
        {
            log_debug("draw non-indexed %d", tg->vertices.item_count);
            dvz_cmd_draw(cmds, idx, 0, tg->vertices.item_count);
        }
    }
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

static void _common_bindings(TestGraphics* tg)
{
    DvzGpu* gpu = tg->graphics->gpu;
    DvzGraphics* graphics = tg->graphics;

    // Create the bindings.
    tg->bindings = dvz_bindings(&graphics->slots, tg->canvas->swapchain.img_count);

    // Binding resources.
    tg->br_mvp = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, tg->canvas->swapchain.img_count,
        sizeof(DvzMVP));
    tg->br_viewport =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzViewport));

    // Upload MVP.
    glm_mat4_identity(tg->mvp.model);
    glm_mat4_identity(tg->mvp.view);
    glm_mat4_identity(tg->mvp.proj);
    dvz_upload_buffers(tg->canvas, tg->br_mvp, 0, sizeof(DvzMVP), &tg->mvp);

    // Bindings
    dvz_bindings_buffer(&tg->bindings, 0, tg->br_mvp);
    dvz_bindings_buffer(&tg->bindings, 1, tg->br_viewport);
}

static void _interact_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    TestGraphics* tg = ev.user_data;
    ASSERT(tg != NULL);

    dvz_interact_update(&tg->interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_upload_buffers(canvas, tg->br_mvp, 0, sizeof(DvzMVP), &tg->interact.mvp);
}



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define INIT_GRAPHICS(type, flags)                                                                \
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);                                                      \
    DvzGpu* gpu = dvz_gpu_best(app);                                                              \
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, DVZ_CANVAS_FLAGS_FPS);           \
    DvzGraphics* graphics = dvz_graphics_builtin(canvas, type, flags);

#define BEGIN_DATA(type, n, user_data)                                                            \
    TestGraphics tg = {0};                                                                        \
    tg.canvas = canvas;                                                                           \
    tg.graphics = graphics;                                                                       \
    tg.up[1] = 1;                                                                                 \
    tg.eye[2] = 3;                                                                                \
    tg.vertices = dvz_array_struct(0, sizeof(type));                                              \
    tg.indices = dvz_array_struct(0, sizeof(DvzIndex));                                           \
    DvzGraphicsData data = dvz_graphics_data(graphics, &tg.vertices, &tg.indices, user_data);     \
    dvz_graphics_alloc(&data, n);                                                                 \
    uint32_t item_count = n;                                                                      \
    uint32_t vertex_count = tg.vertices.item_count;                                               \
    uint32_t index_count = tg.indices.item_count;                                                 \
    tg.br_vert =                                                                                  \
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, vertex_count * sizeof(type));    \
    if (index_count > 0)                                                                          \
        tg.br_index = dvz_ctx_buffers(                                                            \
            gpu->context, DVZ_BUFFER_TYPE_INDEX, 1, index_count * sizeof(DvzIndex));              \
    type* vertices = tg.vertices.data;

#define END_DATA                                                                                  \
    ASSERT(item_count > 0);                                                                       \
    ASSERT(vertex_count > 0);                                                                     \
    ASSERT(index_count == 0 || index_count > 0);                                                  \
    ASSERT(vertices != NULL);                                                                     \
    dvz_upload_buffers(                                                                           \
        canvas, tg.br_vert, 0, vertex_count* tg.vertices.item_size, tg.vertices.data);            \
    if (index_count > 0)                                                                          \
        dvz_upload_buffers(                                                                       \
            canvas, tg.br_index, 0, index_count* tg.indices.item_size, tg.indices.data);

#define BINDINGS_PARAMS                                                                           \
    _common_bindings(&tg);                                                                        \
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);                            \
    dvz_bindings_update(&tg.bindings);

#define BINDINGS_NO_PARAMS                                                                        \
    _common_bindings(&tg);                                                                        \
    dvz_bindings_update(&tg.bindings);

#define RUN                                                                                       \
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _graphics_refill, &tg);  \
    dvz_app_run(app, N_FRAMES);                                                                   \
    dvz_array_destroy(&tg.vertices);                                                              \
    dvz_array_destroy(&tg.indices);

// NOTE: avoid screenshot in interactive mode, otherwise the canvas is destroyed *before* taking
// the screenshot, leading to a segfault.
#define SCREENSHOT(name)                                                                          \
    if (N_FRAMES != 0)                                                                            \
    {                                                                                             \
        char screenshot_path[1024];                                                               \
        snprintf(                                                                                 \
            screenshot_path, sizeof(screenshot_path), "%s/docs/images/graphics/%s.png", ROOT_DIR, \
            name);                                                                                \
        dvz_screenshot_file(canvas, screenshot_path);                                             \
    }



/*************************************************************************************************/
/*  Misc graphics tests */
/*************************************************************************************************/

static void _graphics_point_wheel_callback(DvzCanvas* canvas, DvzEvent ev)
{
    TestGraphics* tg = ev.user_data;

    // Update point size.
    tg->param += ev.u.w.dir[1] * .5;
    tg->param = CLIP(tg->param, 1, 100);
    dvz_upload_buffers(canvas, tg->br_params, 0, sizeof(DvzGraphicsPointParams), &tg->param);
}

static void _graphics_update_mvp(DvzCanvas* canvas, DvzEvent ev)
{
    TestGraphics* tg = ev.user_data;

    // Update MVP.
    tg->mvp.model[0][0] = .1 * tg->param;
    tg->mvp.model[1][1] = .1 * tg->param;
    dvz_upload_buffers(canvas, tg->br_mvp, 0, sizeof(DvzMVP), &tg->mvp);
}

int test_graphics_dynamic(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_POINT, 0)
    BEGIN_DATA(DvzVertex, 10000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    END_DATA

    // Create the bindings.
    tg.bindings = dvz_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg.br_mvp = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(DvzMVP));
    tg.br_viewport = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);
    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));

    // Upload MVP.
    glm_mat4_identity(tg.mvp.model);
    glm_mat4_identity(tg.mvp.view);
    glm_mat4_identity(tg.mvp.proj);
    dvz_upload_buffers(canvas, tg.br_mvp, 0, sizeof(DvzMVP), &tg.mvp);

    // Upload params.
    tg.param = 5.0f;
    DvzGraphicsPointParams params = {.point_size = tg.param};
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsPointParams), &params);

    // Bindings
    dvz_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    dvz_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_update(&tg.bindings);

    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_WHEEL, 0, DVZ_EVENT_MODE_SYNC, _graphics_point_wheel_callback,
        &tg);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _graphics_update_mvp, &tg);

    RUN;
    TEST_END
}



static void _graphics_3D_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(ev.type = DVZ_EVENT_FRAME);
    TestGraphics* tg = ev.user_data;
    vec3 axis = {0};
    axis[1] = 1;
    glm_rotate_make(tg->mvp.model, ev.u.f.time, axis);
    dvz_mvp_camera(canvas->viewport, tg->eye, tg->center, (vec2){.1, 100}, &tg->mvp);

    dvz_upload_buffers(canvas, tg->br_mvp, 0, sizeof(DvzMVP), &tg->mvp);
}

int test_graphics_3D(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_POINT, DVZ_GRAPHICS_FLAGS_DEPTH_TEST)
    BEGIN_DATA(DvzVertex, 3, NULL)
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
    tg.bindings = dvz_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg.br_mvp = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(DvzMVP));
    tg.br_viewport = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);
    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));

    // Upload params.
    tg.param = 200.0f;
    DvzGraphicsPointParams params = {.point_size = tg.param};
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsPointParams), &params);

    // Bindings
    dvz_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    dvz_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_update(&tg.bindings);

    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _graphics_3D_callback, &tg);

    RUN;
    TEST_END
}



int test_graphics_depth(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_MESH, 0)
    const uint32_t N = 1000;
    BEGIN_DATA(DvzGraphicsMeshVertex, N * 3, NULL)
    _depth_vertices(N, vertices, false);
    END_DATA

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsMeshParams));

    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_bindings_texture(
            &tg.bindings, DVZ_USER_BINDING + i, gpu->context->color_texture.texture);
    dvz_bindings_update(&tg.bindings);

    DvzGraphicsMeshParams params = default_graphics_mesh_params(tg.eye);
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsMeshParams), &params);

    RUN;
    TEST_END
}



/*************************************************************************************************/
/*  Basic graphics tests                                                                         */
/*************************************************************************************************/

int test_graphics_point(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_POINT, 0)
    BEGIN_DATA(DvzVertex, 10000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    END_DATA

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));
    BINDINGS_PARAMS

    tg.param = 5.0f;
    DvzGraphicsPointParams params = {.point_size = tg.param};
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsPointParams), &params);

    RUN;
    SCREENSHOT("point")
    TEST_END
}



int test_graphics_line(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_LINE, 0)
    BEGIN_DATA(DvzVertex, 100, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        float t = (float)(i / 2) / (float)vertex_count;
        vertices[i].pos[0] = .75 * (-1 + 4 * t);
        vertices[i].pos[1] = .75 * (-1 + (i % 2 == 0 ? 0 : 2));
        dvz_colormap_scale(DVZ_CMAP_RAINBOW, t, 0, .5, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    SCREENSHOT("line")
    TEST_END
}



int test_graphics_line_strip(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_LINE_STRIP, 0)
    BEGIN_DATA(DvzVertex, 1000, NULL)
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        float t = (float)i / (float)vertex_count;
        vertices[i].pos[0] = -1 + 2 * t;
        vertices[i].pos[1] = .5 * sin(8 * M_2PI * t);
        dvz_colormap_scale(DVZ_CMAP_RAINBOW, t, 0, 1, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    SCREENSHOT("line_strip")
    TEST_END
}



int test_graphics_triangle(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_TRIANGLE, 0)
    const uint32_t N = 40; // number of triangles
    BEGIN_DATA(DvzVertex, N * 3, NULL)

    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)N;
        vertices[3 * i].pos[0] = -.75 + 1.5 * t * t;
        vertices[3 * i].pos[1] = +.75 - 1.5 * t;
        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N, vertices[3 * i].color);
        vertices[3 * i].color[3] = 128;

        // Copy the 2 other points per triangle.
        glm_vec3_copy(vertices[3 * i].pos, vertices[3 * i + 1].pos);
        glm_vec3_copy(vertices[3 * i].pos, vertices[3 * i + 2].pos);
        memcpy(vertices[3 * i + 1].color, vertices[3 * i].color, sizeof(cvec4));
        memcpy(vertices[3 * i + 2].color, vertices[3 * i].color, sizeof(cvec4));

        // Shift the points.
        float ms = .02 + .2 * t * t;
        vertices[3 * i + 0].pos[0] -= ms;
        vertices[3 * i + 1].pos[0] += ms;
        vertices[3 * i + 0].pos[1] -= ms;
        vertices[3 * i + 1].pos[1] -= ms;
        vertices[3 * i + 2].pos[1] += ms;
    }

    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    SCREENSHOT("triangle")
    TEST_END
}



int test_graphics_triangle_strip(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_TRIANGLE_STRIP, 0)
    BEGIN_DATA(DvzVertex, 40, NULL)
    float m = .1;
    float y = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        float t = .9 * (float)i / (float)(vertex_count - 1);
        float a = M_2PI * t;
        vertices[i].pos[0] = (.5 + (i % 2 == 0 ? +m : -m)) * cos(a);
        vertices[i].pos[1] = y * (.5 + (i % 2 == 0 ? +m : -m)) * sin(a);
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    SCREENSHOT("triangle_strip")
    TEST_END
}



int test_graphics_triangle_fan(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_TRIANGLE_FAN, 0)
    BEGIN_DATA(DvzVertex, 30, NULL)
    float y = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        float t = (float)i / (float)(vertex_count - 1);
        float a = M_2PI * t;
        vertices[i].pos[0] = .5 * cos(a);
        vertices[i].pos[1] = y * .5 * sin(a);
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, vertices[i].color);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    RUN;
    SCREENSHOT("triangle_fan")
    TEST_END
}



/*************************************************************************************************/
/*  Marker tests                                                                                 */
/*************************************************************************************************/

int test_graphics_marker_1(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_MARKER, 0)
    BEGIN_DATA(DvzGraphicsMarkerVertex, 1000, NULL)

    // Random markers.
    for (uint32_t i = 0; i < vertex_count - DVZ_MARKER_COUNT; i++)
    {
        RANDN_POS(vertices[i].pos)
        vertices[i].pos[1] -= .1;
        RAND_COLOR(vertices[i].color)
        vertices[i].color[3] = dvz_rand_byte();
        vertices[i].size = 10 + 40 * dvz_rand_float();
        vertices[i].marker = DVZ_MARKER_DISC;
    }

    // Top bar with all marker types.
    uint32_t j = 0;
    for (uint32_t i = vertex_count - DVZ_MARKER_COUNT; i < vertex_count; i++)
    {
        j = i - (vertex_count - DVZ_MARKER_COUNT);
        ASSERT(j < DVZ_MARKER_COUNT);

        vertices[i].pos[0] = .9 * (-1 + 2 * j / (float)(DVZ_MARKER_COUNT - 1));
        vertices[i].pos[1] = +.9;
        dvz_colormap_scale(DVZ_CMAP_HSV, j, 0, DVZ_MARKER_COUNT, vertices[i].color);
        vertices[i].color[3] = 255;
        vertices[i].size = 35;
        vertices[i].angle = TO_BYTE(i / (float)(DVZ_MARKER_COUNT - 1));
        vertices[i].marker = (DvzMarkerType)(j);
    }
    END_DATA

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));
    BINDINGS_PARAMS

    DvzGraphicsMarkerParams params = {0};
    params.edge_color[0] = 1;
    params.edge_color[1] = 1;
    params.edge_color[2] = 1;
    params.edge_color[3] = 1;
    params.edge_width = 2;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsMarkerParams), &params);

    RUN;
    SCREENSHOT("marker")
    TEST_END
}



#define SAVE_MARKER(MARKER, NAME)                                                                 \
    ((DvzGraphicsMarkerVertex*)tg.vertices.data)[0].marker = (MARKER);                            \
    dvz_upload_buffers(                                                                           \
        canvas, tg.br_vert, 0, vertex_count* tg.vertices.item_size, tg.vertices.data);            \
    dvz_app_run(app, 3);                                                                          \
    SCREENSHOT(NAME)

int test_graphics_marker_screenshots(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 128, 128, 0);
    dvz_canvas_clear_color(canvas, 1, 1, 1);
    DvzGraphics* graphics = dvz_graphics_builtin(canvas, DVZ_GRAPHICS_MARKER, 0);
    BEGIN_DATA(DvzGraphicsMarkerVertex, 1, NULL)

    vertices[0].marker = 0;
    vertices[0].size = 100;
    vertices[0].color[0] = 124;
    vertices[0].color[1] = 141;
    vertices[0].color[2] = 194;
    vertices[0].color[3] = 255;

    END_DATA

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));
    BINDINGS_PARAMS

    DvzGraphicsMarkerParams params = {0};
    params.edge_color[0] = 0;
    params.edge_color[1] = 0;
    params.edge_color[2] = 0;
    params.edge_color[3] = 1;
    params.edge_width = 5;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsMarkerParams), &params);

    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _graphics_refill, &tg);

    SAVE_MARKER(DVZ_MARKER_DISC, "marker_disc")
    SAVE_MARKER(DVZ_MARKER_ASTERISK, "marker_asterisk")
    SAVE_MARKER(DVZ_MARKER_CHEVRON, "marker_chevron")
    SAVE_MARKER(DVZ_MARKER_CLOVER, "marker_clover")
    SAVE_MARKER(DVZ_MARKER_CLUB, "marker_club")
    SAVE_MARKER(DVZ_MARKER_CROSS, "marker_cross")
    SAVE_MARKER(DVZ_MARKER_DIAMOND, "marker_diamond")
    SAVE_MARKER(DVZ_MARKER_ARROW, "marker_arrow")
    SAVE_MARKER(DVZ_MARKER_ELLIPSE, "marker_ellipse")
    SAVE_MARKER(DVZ_MARKER_HBAR, "marker_hbar")
    SAVE_MARKER(DVZ_MARKER_HEART, "marker_heart")
    SAVE_MARKER(DVZ_MARKER_INFINITY, "marker_infinity")
    SAVE_MARKER(DVZ_MARKER_PIN, "marker_pin")
    SAVE_MARKER(DVZ_MARKER_RING, "marker_ring")
    SAVE_MARKER(DVZ_MARKER_SPADE, "marker_spade")
    SAVE_MARKER(DVZ_MARKER_SQUARE, "marker_square")
    SAVE_MARKER(DVZ_MARKER_TAG, "marker_tag")
    SAVE_MARKER(DVZ_MARKER_TRIANGLE, "marker_triangle")
    SAVE_MARKER(DVZ_MARKER_VBAR, "marker_vbar")

    dvz_array_destroy(&tg.vertices);
    dvz_array_destroy(&tg.indices);
    TEST_END
}



/*************************************************************************************************/
/*  Agg segment tests                                                                            */
/*************************************************************************************************/

static void _resize(DvzCanvas* canvas, DvzEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    dvz_upload_buffers(canvas, tg->br_viewport, 0, sizeof(DvzViewport), &canvas->viewport);
}

int test_graphics_segment(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_SEGMENT, 0)
    const uint32_t N = 16;
    BEGIN_DATA(DvzGraphicsSegmentVertex, 4 * N, NULL)

    DvzGraphicsSegmentVertex vertex = {0};
    for (uint32_t i = 0; i < N; i++)
    {
        float t = (float)i / (float)N;
        float x = .75 * (-1 + 2 * t);
        float y = .75;
        vertex.P0[0] = vertex.P1[0] = x;
        vertex.P0[1] = y;
        vertex.P1[1] = -y;
        vertex.linewidth = 5 + 30 * t;
        dvz_colormap_scale(DVZ_CMAP_RAINBOW, t, 0, 1, vertex.color);
        vertex.cap0 = vertex.cap1 = i % DVZ_CAP_COUNT;
        dvz_graphics_append(&data, &vertex);
    }
    END_DATA
    BINDINGS_NO_PARAMS
    dvz_event_callback(canvas, DVZ_EVENT_RESIZE, 0, DVZ_EVENT_MODE_SYNC, _resize, &tg);
    RUN;
    SCREENSHOT("segment")
    TEST_END
}



/*************************************************************************************************/
/*  Agg path tests                                                                               */
/*************************************************************************************************/

int test_graphics_path(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_PATH, 0)
    const uint32_t N = 1000;
    BEGIN_DATA(DvzGraphicsPathVertex, N, NULL)

    DvzGraphicsPathVertex vertex = {0};
    float t0, t1, t2, t3;
    int32_t i0, i1, i2, i3;
    float d = 1.0 / (float)(N - 1);
    // float y = 0;
    int32_t n = (int32_t)N;
    for (int32_t i = 0; i < n; i++)
    {
        i0 = i >= 1 ? i - 1 : 0;
        i1 = i + 0;
        i2 = i < n - 1 ? i + 1 : n - 1;
        i3 = i < n - 2 ? i + 2 : n - 1;

        t0 = -.9 + 1.8 * i0 * d;
        t1 = -.9 + 1.8 * i1 * d;
        t2 = -.9 + 1.8 * i2 * d;
        t3 = -.9 + 1.8 * i3 * d;

        vertex.p0[0] = t0;
        vertex.p1[0] = t1;
        vertex.p2[0] = t2;
        vertex.p3[0] = t3;

        vertex.p0[1] = .35 * sin(M_2PI * t0 / .9);
        vertex.p1[1] = .35 * sin(M_2PI * t1 / .9);
        vertex.p2[1] = .35 * sin(M_2PI * t2 / .9);
        vertex.p3[1] = .35 * sin(M_2PI * t3 / .9);

        dvz_colormap_scale(DVZ_CMAP_RAINBOW, i, 0, N - 1, vertex.color);
        dvz_graphics_append(&data, &vertex);
    }
    END_DATA

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPathParams));
    BINDINGS_PARAMS

    DvzGraphicsPathParams params = {0};
    params.cap_type = DVZ_CAP_ROUND;
    params.linewidth = 50;
    params.miter_limit = 4;
    params.round_join = DVZ_JOIN_ROUND;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsPathParams), &params);

    dvz_event_callback(canvas, DVZ_EVENT_RESIZE, 0, DVZ_EVENT_MODE_SYNC, _resize, &tg);

    tg.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_PANZOOM);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _interact_callback, &tg);

    RUN;
    SCREENSHOT("path")
    TEST_END
}



/*************************************************************************************************/
/*  Text tests                                                                                   */
/*************************************************************************************************/

int test_graphics_text(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_TEXT, 0)
    const uint32_t N = 26;
    const char str[] = "Hello world!";
    const uint32_t offset = strlen(str);

    // Font atlas
    DvzFontAtlas* atlas = &gpu->context->font_atlas;

    DvzGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;

    // 26 letters in a circle.
    BEGIN_DATA(DvzGraphicsTextVertex, (N + offset), atlas)
    float t = 0;
    float a = 0, x = 0, y = 0;
    DvzGraphicsTextItem item = {0};
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
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, item.vertex.color);

        dvz_graphics_append(&data, &item);
    }

    // Hello world
    item.glyph_colors = calloc(offset, sizeof(cvec4));
    for (uint32_t i = 0; i < offset; i++)
    {
        dvz_colormap_scale(DVZ_CMAP_RAINBOW, i, 0, offset, item.glyph_colors[i]);
    }
    item.vertex.pos[0] = 0;
    item.vertex.pos[1] = 0;
    item.vertex.angle = 0;
    item.font_size = 36;
    item.string = str;
    dvz_graphics_append(&data, &item);
    FREE(item.glyph_colors);

    END_DATA

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsTextParams));
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsTextParams), &params);

    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 1, atlas->texture);
    dvz_bindings_update(&tg.bindings);

    dvz_event_callback(canvas, DVZ_EVENT_RESIZE, 0, DVZ_EVENT_MODE_SYNC, _resize, &tg);

    RUN;
    SCREENSHOT("text")
    TEST_END
}



/*************************************************************************************************/
/*  Image tests                                                                                  */
/*************************************************************************************************/

int test_graphics_image_1(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_IMAGE, 0)

    const uint32_t N = 1;
    BEGIN_DATA(DvzGraphicsImageVertex, N, NULL)
    float x0, x1, z;
    z = 0;
    float w = 2.0 / (float)N;
    for (uint32_t i = 0; i < N; i++)
    {
        x0 = -1 + i * w;
        x1 = -1 + (i + 1) * w;
        DvzGraphicsImageItem item =                              //
            {{x0, x1, z}, {x1, x1, z}, {x1, x0, z}, {x0, x0, z}, //
             {0, 0},      {1, 0},      {1, 1},      {0, 1}};     //
        dvz_graphics_append(&data, &item);
    }
    END_DATA

    // Parameters.
    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsImageParams));
    DvzGraphicsImageParams params = {0};
    params.tex_coefs[0] = 1;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsImageParams), &params);

    // Texture.
    // https://pixabay.com/illustrations/earth-planet-world-globe-space-1617121/
    DvzTexture* texture = _earth_texture(canvas);

    // Bindings.
    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + i, texture);
    dvz_bindings_update(&tg.bindings);

    RUN;
    SCREENSHOT("image")
    TEST_END
}



int test_graphics_image_cmap(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_IMAGE_CMAP, 0)

    const uint32_t N = 1;
    BEGIN_DATA(DvzGraphicsImageVertex, N, NULL)
    float x0, x1, z;
    z = 0;
    float w = 2.0 / (float)N;
    for (uint32_t i = 0; i < N; i++)
    {
        x0 = -1 + i * w;
        x1 = -1 + (i + 1) * w;
        DvzGraphicsImageItem item =                              //
            {{x0, x1, z}, {x1, x1, z}, {x1, x0, z}, {x0, x0, z}, //
             {0, 0},      {1, 0},      {1, 1},      {0, 1}};     //
        dvz_graphics_append(&data, &item);
    }
    END_DATA

    // Parameters.
    tg.br_params = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsImageCmapParams));
    DvzGraphicsImageCmapParams params = {0};
    params.cmap = DVZ_CMAP_HSV;
    // NOTE: reverse order for negative colormap
    params.vrange[0] = 1;
    params.vrange[1] = 0;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsImageCmapParams), &params);

    // Random texture.
    const uint32_t S = 16;
    VkDeviceSize size = S * S * 1;
    uint8_t* tex_data = calloc(size, sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++)
        tex_data[i] = (uint8_t)((i * 12) % 256);
    uvec3 shape = {S, S, 1};
    DvzTexture* tex = dvz_ctx_texture(gpu->context, 2, shape, VK_FORMAT_R8_UNORM);
    dvz_upload_texture(canvas, tex, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size, tex_data);
    FREE(tex_data);

    // Bindings.
    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 1, gpu->context->color_texture.texture);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 2, tex);
    dvz_bindings_update(&tg.bindings);

    RUN;
    SCREENSHOT("image_cmap")
    TEST_END
}



/*************************************************************************************************/
/*  Volume image tests                                                                           */
/*************************************************************************************************/

static void _graphics_volume_callback(DvzCanvas* canvas, DvzEvent ev)
{
    TestGraphics* tg = ev.user_data;
    float dx = ev.u.f.interval;
    for (uint32_t i = 0; i < 6; i++)
        ((DvzGraphicsVolumeSliceVertex*)tg->vertices.data)[i].uvw[2] += .1 * dx;
    dvz_upload_buffers(
        canvas, tg->br_vert, 0, tg->vertices.item_count * sizeof(DvzGraphicsVolumeSliceVertex),
        tg->vertices.data);
}

int test_graphics_volume_slice(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_VOLUME_SLICE, 0)
    const uint32_t N = 8;
    BEGIN_DATA(DvzGraphicsVolumeSliceVertex, N, NULL)
    float x = MOUSE_VOLUME_DEPTH / (float)MOUSE_VOLUME_HEIGHT;
    float y = 1;
    float z = 0;
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = 1 - i / (float)(N - 1);
        z = -1 + 2 * t;
        t = .1 + .8 * t;
        DvzGraphicsVolumeSliceItem item =                        //
            {{-x, -y, z}, {+x, -y, z}, {+x, +y, z}, {-x, +y, z}, //
             {1, 0, t},   {1, 1, t},   {0, 1, t},   {0, 0, t}};  //
        dvz_graphics_append(&data, &item);
    }
    END_DATA

    // Parameters.
    tg.br_params = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsVolumeSliceParams));

    DvzGraphicsVolumeSliceParams params = {0};
    params.cmap = DVZ_CMAP_BONE;
    params.scale = 13;

    // Transfer function for the alpha channel.
    params.x_alpha[0] = .0;
    params.x_alpha[1] = .05;
    params.x_alpha[2] = .051;
    params.x_alpha[3] = 1;

    params.y_alpha[0] = 0;
    params.y_alpha[1] = 0;
    params.y_alpha[2] = .75;
    params.y_alpha[3] = .75;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsVolumeSliceParams), &params);

    // Texture.
    DvzTexture* texture = _mouse_volume(canvas);

    // Bindings.
    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 1, gpu->context->color_texture.texture);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 2, texture);
    dvz_bindings_update(&tg.bindings);

    // Interactivity.
    tg.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _interact_callback, &tg);
    // dvz_event_callback(
    //     canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _graphics_volume_callback, &tg);

    DvzArcball* arcball = &tg.interact.u.a;
    versor q;
    glm_quatv(q, M_PI, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, -M_PI / 8, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, -M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    _arcball_update_mvp(canvas->viewport, arcball, &tg.interact.mvp);

    RUN;
    SCREENSHOT("volume_slice")
    TEST_END
}



/*************************************************************************************************/
/*  Volume tests                                                                                 */
/*************************************************************************************************/

static void _volume_click(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);

    // Mouse coordinates.
    uvec2 pos = {0};
    pos[0] = (uint32_t)ev.u.c.pos[0];
    pos[1] = (uint32_t)ev.u.c.pos[1];

    ivec4 picked = {0};
    dvz_canvas_pick(canvas, pos, picked);
    log_info("picked %d %d %d %d", picked[0], picked[1], picked[2], picked[3]);
}

int test_graphics_volume_1(TestContext* context)
{
    const uint32_t ni = MOUSE_VOLUME_WIDTH;
    const uint32_t nj = MOUSE_VOLUME_HEIGHT;
    const uint32_t nk = MOUSE_VOLUME_DEPTH;

    DvzGraphicsVolumeParams params = {0};
    float c = .005;
    vec4 box_size = {c * ni, c * nj, 1 * c * nk, 0};
    glm_vec4_copy(box_size, params.box_size);
    params.uvw1[0] = 1;
    params.uvw1[1] = 1;
    params.uvw1[2] = 1;

    params.clip[2] = +1;
    params.clip[3] = -.5;

    // params.transfer_xrange[0] = 0;
    // params.transfer_xrange[1] = 1.5;
    params.color_coef = .01;

    vec3 p0 = {-c * ni / 2., -c * nj / 2., -1 * c * nk / 2.};
    vec3 p1 = {+c * ni / 2., +c * nj / 2., +1 * c * nk / 2.};

    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas =
        dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, DVZ_CANVAS_FLAGS_FPS | DVZ_CANVAS_FLAGS_PICK);
    DvzGraphics* graphics = dvz_graphics_builtin(canvas, DVZ_GRAPHICS_VOLUME, 0);

    BEGIN_DATA(DvzGraphicsVolumeVertex, 1, NULL)
    DvzGraphicsVolumeItem item = {{p0[0], p0[1], p0[2]}, {p1[0], p1[1], p1[2]}};
    dvz_graphics_append(&data, &item);
    END_DATA

    // Parameters.
    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsVolumeParams));
    tg.params_data = &params;
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsVolumeParams), &params);

    // 3D texture.
    DvzTexture* tex_density = _mouse_volume(canvas);
    DvzTexture* tex_colors = _mouse_region_colors(canvas);

    // Bindings.
    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 1, tex_density);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 2, tex_colors);
    dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + 3, gpu->context->transfer_texture);
    dvz_bindings_update(&tg.bindings);

    // Interactivity.
    tg.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _interact_callback, &tg);
    dvz_event_callback(canvas, DVZ_EVENT_RESIZE, 0, DVZ_EVENT_MODE_SYNC, _resize, &tg);
    dvz_event_callback(canvas, DVZ_EVENT_MOUSE_CLICK, 0, DVZ_EVENT_MODE_SYNC, _volume_click, &tg);

    DvzArcball* arcball = &tg.interact.u.a;
    versor q;
    glm_quatv(q, M_PI / 2, (vec3){0, 0, 1});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, M_PI, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, M_PI / 6, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, -M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    _arcball_update_mvp(canvas->viewport, arcball, &tg.interact.mvp);

    RUN;
    SCREENSHOT("volume")
    TEST_END
}



/*************************************************************************************************/
/*  Mesh tests                                                                                   */
/*************************************************************************************************/

static DvzMesh _graphics_mesh_example(DvzMeshType type)
{
    switch (type)
    {
    case DVZ_MESH_SURFACE:;
        const uint32_t N = 250;
        uint32_t col_count = N + 1;
        uint32_t row_count = 2 * N + 1;
        uint32_t point_count = col_count * row_count;
        float* heights = calloc(point_count, sizeof(float));
        float w = 1.5;
        float x, y, z;
        for (uint32_t i = 0; i < row_count; i++)
        {
            x = (float)i / (row_count - 1);
            x = -w + 2 * w * x;
            for (uint32_t j = 0; j < col_count; j++)
            {
                y = (float)j / (col_count - 1);
                y = -w + 2 * w * y;
                z = .5 * sin(10 * x) * cos(10 * y);
                z *= exp(-1 * (x * x + y * y));
                heights[col_count * i + j] = z;
            }
        }
        DvzMesh mesh = dvz_mesh_surface(row_count, col_count, heights);
        FREE(heights);
        return mesh;
        break;

    case DVZ_MESH_CUBE:
        return dvz_mesh_cube();

    case DVZ_MESH_SPHERE:
        return dvz_mesh_sphere(100, 100);

    case DVZ_MESH_CYLINDER:
        return dvz_mesh_cylinder(100);

    case DVZ_MESH_CONE:
        return dvz_mesh_cone(100);

    case DVZ_MESH_SQUARE:
        return dvz_mesh_square();

    case DVZ_MESH_DISC:
        return dvz_mesh_disc(100);

    case DVZ_MESH_OBJ:;
        char path[1024];
        snprintf(path, sizeof(path), "%s/mesh/%s", DATA_DIR, "brain.obj");
        return dvz_mesh_obj(path);

    default:
        break;
    }

    return (DvzMesh){0};
}

int test_graphics_mesh(TestContext* context)
{
    INIT_GRAPHICS(DVZ_GRAPHICS_MESH, 0)

    TestGraphics tg = {0};
    tg.canvas = canvas;
    tg.eye[2] = 3;
    tg.up[1] = 1;
    tg.graphics = graphics;
    DvzMesh mesh = _graphics_mesh_example(MESH);

    // Texture.
    DvzTexture* texture = NULL;
    // Square texture.
    if (0)
    {
        texture = dvz_ctx_texture(gpu->context, 2, (uvec3){2, 2, 1}, VK_FORMAT_R8G8B8A8_UNORM);
        cvec4 tex_data[] = {
            {255, 0, 0, 255}, //
            {0, 255, 0, 255},
            {0, 0, 255, 255},
            {255, 255, 0, 255},
        };
        dvz_upload_texture(
            canvas, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, sizeof(tex_data), tex_data);
    }

    // Height map.
    {
        DvzGraphicsMeshVertex* vertices = ((DvzGraphicsMeshVertex*)mesh.vertices.data);
        // Use the colormap texture.
        texture = gpu->context->color_texture.texture;
        float z = 0;
        for (uint32_t i = 0; i < mesh.vertices.item_count; i++)
        {
            z = vertices[i].pos[1];
            z = (z + .18) / .6;
            // Get the coordinates within the colormap texture.
            dvz_colormap_uv(DVZ_CMAP_JET, TO_BYTE(z), vertices[i].uv);
        }
    }

    tg.vertices = mesh.vertices;
    tg.indices = mesh.indices;
    uint32_t vertex_count = tg.vertices.item_count;
    uint32_t index_count = tg.indices.item_count;
    tg.br_vert = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, vertex_count * sizeof(DvzGraphicsMeshVertex));
    if (index_count > 0)
        tg.br_index = dvz_ctx_buffers(
            gpu->context, DVZ_BUFFER_TYPE_INDEX, 1, index_count * sizeof(DvzIndex));

    dvz_upload_buffers(
        canvas, tg.br_vert, 0, vertex_count * tg.vertices.item_size, tg.vertices.data);
    if (index_count > 0)
        dvz_upload_buffers(
            canvas, tg.br_index, 0, index_count * tg.indices.item_size, tg.indices.data);

    // Create the bindings.
    tg.bindings = dvz_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg.br_mvp = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(DvzMVP));
    tg.br_viewport =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzViewport));

    // Parameters.
    DvzGraphicsMeshParams params = default_graphics_mesh_params(tg.eye);

    tg.br_params =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsMeshParams));
    dvz_upload_buffers(canvas, tg.br_params, 0, sizeof(DvzGraphicsMeshParams), &params);

    // Bindings
    dvz_bindings_buffer(&tg.bindings, 0, tg.br_mvp);
    dvz_bindings_buffer(&tg.bindings, 1, tg.br_viewport);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_bindings_texture(&tg.bindings, DVZ_USER_BINDING + i, texture);
    dvz_bindings_update(&tg.bindings);

    // Interactivity.
    tg.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _interact_callback, &tg);

    DvzArcball* arcball = &tg.interact.u.a;
    versor q;
    glm_quatv(q, M_PI / 6, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    arcball->camera.eye[2] = 3;
    _arcball_update_mvp(canvas->viewport, arcball, &tg.interact.mvp);

    RUN;
    SCREENSHOT("mesh")
    TEST_END
}
