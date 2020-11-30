#include "../include/visky/graphics.h"
#include "utils.h"


#define N_FRAMES 10



/*************************************************************************************************/
/*  Graphics utils                                                                               */
/*************************************************************************************************/

typedef struct TestGraphics TestGraphics;

struct TestGraphics
{
    VklGraphics* graphics;
    VklBufferRegions br_vert;
    VklBufferRegions br_mvp;
    VklBufferRegions br_viewport;
    VklBufferRegions br_params;
    VklTexture* texture;
    VklBindings bindings;

    uint32_t vertex_count;
    VkDeviceSize size;
    void* data;
};

static void _graphics_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    VklCommands* cmds = ev.u.rf.cmds[0];
    VklBufferRegions* br = &tg->br_vert;
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
    vkl_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);
    vkl_cmd_draw(cmds, idx, 0, tg->vertex_count);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define INIT_GRAPHICS(type)                                                                       \
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);                                                      \
    VklGpu* gpu = vkl_gpu(app, 0);                                                                \
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);                                 \
    VklGraphics* graphics = vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS);

#define BEGIN_DATA(type, n)                                                                       \
    TestGraphics tg = {.graphics = graphics};                                                     \
    tg.vertex_count = (n);                                                                        \
    VkDeviceSize size = tg.vertex_count * sizeof(type);                                           \
    tg.br_vert = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);               \
    type* data = calloc(tg.vertex_count, sizeof(type));                                           \
    for (uint32_t i = 0; i < tg.vertex_count; i++)                                                \
    {

#define END_DATA                                                                                  \
    }                                                                                             \
    vkl_upload_buffers(gpu->context, &tg.br_vert, 0, size, data);

#define RANDN_POS(x)                                                                              \
    x[0] = .25 * randn();                                                                         \
    x[1] = .25 * randn();                                                                         \
    x[2] = 0;

#define RAND_COLOR(x)                                                                             \
    x[0] = rand_byte();                                                                           \
    x[1] = rand_byte();                                                                           \
    x[2] = rand_byte();                                                                           \
    x[3] = 255;



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

static int vklite2_graphics_points(VkyTestContext* context)
{
    INIT_GRAPHICS(VKL_GRAPHICS_POINTS)
    BEGIN_DATA(VklVertex, 10000)
    RANDN_POS(data[i].pos)
    RAND_COLOR(data[i].color)
    END_DATA

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots);

    // TODO: ubos
    tg.br_mvp = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    tg.br_viewport = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    tg.texture = vkl_ctx_texture(gpu->context, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    vkl_bindings_buffer(&tg.bindings, 0, &tg.br_mvp);
    vkl_bindings_buffer(&tg.bindings, 1, &tg.br_viewport);
    // TODO: color
    // vkl_bindings_texture(&tg.bindings, 2, tg.texture->image, tg.texture->sampler);
    vkl_bindings_create(&tg.bindings, 1);
    vkl_bindings_update(&tg.bindings);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _graphics_refill, &tg);

    vkl_app_run(app, 0);

    FREE(data);
    TEST_END
}
