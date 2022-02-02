/*************************************************************************************************/
/*  Testing graphics                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_graphics.h"
#include "board.h"
#include "colormaps.h"
#include "context.h"
#include "fileio.h"
#include "graphics.h"
#include "host.h"
#include "pipe.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

#define GRAPHICS_BEGIN                                                                            \
    ASSERT(suite != NULL);                                                                        \
                                                                                                  \
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);                                                   \
                                                                                                  \
    DvzGpu* gpu = dvz_gpu_best(host);                                                             \
    _default_queues(gpu, false);                                                                  \
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});          \
    dvz_gpu_create(gpu, 0);                                                                       \
                                                                                                  \
    DvzContext* ctx = dvz_context(gpu);                                                           \
    ASSERT(ctx != NULL);                                                                          \
                                                                                                  \
    DvzBoard board = dvz_board(gpu, WIDTH, HEIGHT, 0);                                            \
    dvz_board_create(&board);

#define GRAPHICS_MVP                                                                              \
    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);                   \
    ASSERT(dat_mvp != NULL);                                                                      \
    DvzMVP mvp = {0};                                                                             \
    glm_mat4_identity(mvp.model);                                                                 \
    glm_mat4_identity(mvp.view);                                                                  \
    glm_mat4_identity(mvp.proj);                                                                  \
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

#define GRAPHICS_VIEWPORT                                                                         \
    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);         \
    ASSERT(dat_viewport != NULL);                                                                 \
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);                                   \
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);

#define GRAPHICS_SCREENSHOT(name)                                                                 \
    uint8_t* rgb = dvz_board_alloc(&board);                                                       \
    dvz_board_download(&board, board.size, rgb);                                                  \
    char imgpath[1024];                                                                           \
    snprintf(imgpath, sizeof(imgpath), "%s/graphics_%s.png", ARTIFACTS_DIR, (name));              \
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);                                                   \
    dvz_board_free(&board);

#define GRAPHICS_END                                                                              \
    dvz_dat_destroy(dat_vertex);                                                                  \
    dvz_dat_destroy(dat_mvp);                                                                     \
    dvz_dat_destroy(dat_viewport);                                                                \
    dvz_pipe_destroy(&pipe);                                                                      \
    dvz_board_destroy(&board);                                                                    \
    dvz_context_destroy(ctx);                                                                     \
    dvz_gpu_destroy(gpu);                                                                         \
    dvz_host_destroy(host);



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TstSuite* suite)
{
    // Create the board and context
    GRAPHICS_BEGIN

    // Create the graphics.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe, 1);
    dvz_graphics_builtin(&board.renderpass, graphics, DVZ_GRAPHICS_POINT, 0);

    const uint32_t n = 50;

    // Create the dats.
    DvzDat* dat_vertex =
        dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, n * sizeof(DvzGraphicsPointVertex), 0);
    ASSERT(dat_vertex != NULL);

    // Slots 0 and 1.
    GRAPHICS_MVP
    GRAPHICS_VIEWPORT

    // Create the bindings.
    dvz_pipe_vertex(&pipe, dat_vertex);
    dvz_pipe_dat(&pipe, 0, dat_mvp);
    dvz_pipe_dat(&pipe, 1, dat_viewport);
    dvz_pipe_create(&pipe);

    // Upload the data.
    DvzGraphicsPointVertex* data = calloc(n, sizeof(DvzGraphicsPointVertex));
    double t = 0;
    double aspect = WIDTH / (double)HEIGHT;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        data[i].pos[0] = .5 * cos(M_2PI * t);
        data[i].pos[1] = aspect * .5 * sin(M_2PI * t);

        data[i].size = 50;

        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), data[i].color);
        data[i].color[3] = 128;
    }
    dvz_dat_upload(dat_vertex, 0, n * sizeof(DvzGraphicsPointVertex), data, true);
    FREE(data)

    // Commands.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_board_begin(&board, &cmds, 0);
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, n);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    GRAPHICS_SCREENSHOT("point")

    // Destruction
    GRAPHICS_END

    return 0;
}



int test_graphics_triangle(TstSuite* suite)
{
    // Create the board and context
    GRAPHICS_BEGIN

    // Create the graphics.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe, 1);
    dvz_graphics_builtin(&board.renderpass, graphics, DVZ_GRAPHICS_TRIANGLE, 0);

    // Create the dats.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ASSERT(dat_vertex != NULL);

    // Slots 0 and 1.
    GRAPHICS_MVP
    GRAPHICS_VIEWPORT

    // Create the bindings.
    dvz_pipe_vertex(&pipe, dat_vertex);
    dvz_pipe_dat(&pipe, 0, dat_mvp);
    dvz_pipe_dat(&pipe, 1, dat_viewport);
    dvz_pipe_create(&pipe);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    dvz_dat_upload(dat_vertex, 0, sizeof(data), data, true);

    // Commands.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_board_begin(&board, &cmds, 0);
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, 3);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    GRAPHICS_SCREENSHOT("triangle")

    // Destruction
    GRAPHICS_END

    return 0;
}



int test_graphics_line_list(TstSuite* suite)
{
    // Create the board and context
    GRAPHICS_BEGIN

    // Create the graphics.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe, 1);
    dvz_graphics_builtin(&board.renderpass, graphics, DVZ_GRAPHICS_LINE, 0);

    // Vertex count and params.
    uint32_t n = 4 * 16;
    DvzSize size = 2 * n * sizeof(DvzVertex);
    ASSERT(n > 0);

    // Create the dats.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    ASSERT(dat_vertex != NULL);

    // Slots 0 and 1.
    GRAPHICS_MVP
    GRAPHICS_VIEWPORT

    // Create the bindings.
    dvz_pipe_vertex(&pipe, dat_vertex);
    dvz_pipe_dat(&pipe, 0, dat_mvp);
    dvz_pipe_dat(&pipe, 1, dat_viewport);
    dvz_pipe_create(&pipe);

    // Upload the triangle data.
    DvzVertex* data = (DvzVertex*)calloc(size, 1);
    double t = 0, r = .75;
    double aspect = board.width / (float)board.height;
    AT(aspect > 0);
    for (uint32_t i = 0; i < n; i++)
    {
        t = .5 * i / (double)n;
        data[2 * i].pos[0] = r * cos(M_2PI * t);
        data[2 * i].pos[1] = aspect * r * sin(M_2PI * t);

        data[2 * i + 1].pos[0] = -data[2 * i].pos[0];
        data[2 * i + 1].pos[1] = -data[2 * i].pos[1];

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, data[2 * i].color);
        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, data[2 * i + 1].color);
    }
    dvz_dat_upload(dat_vertex, 0, size, data, true);

    // Commands.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_board_begin(&board, &cmds, 0);
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, 2 * n);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    GRAPHICS_SCREENSHOT("line_list")

    // Destruction
    GRAPHICS_END

    FREE(data);
    return 0;
}



int test_graphics_line_strip(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_triangle_list(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_triangle_strip(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_triangle_fan(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_marker(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_segment(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_path(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_text(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_image_1(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_image_cmap(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_volume_slice(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_volume_1(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}



int test_graphics_mesh(TstSuite* suite)
{
    ASSERT(suite != NULL);

    return 0;
}
