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
    VklBufferRegions br;
    VklBindings bindings;

    uint32_t vertex_count;
    VkDeviceSize size;
    void* data;
};

static void _graphics_points_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    VklCommands* cmds = ev.u.rf.cmds[0];
    VklBufferRegions* br = &tg->br;
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
/*  Graphics tests                                                                               */
/*************************************************************************************************/

static int vklite2_graphics_points(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    VklGraphics* graphics = vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS);
    ASSERT(graphics != NULL);

    TestGraphics tg = {0};
    tg.graphics = graphics;
    tg.vertex_count = 10000;
    VkDeviceSize size = tg.vertex_count * sizeof(VklVertex);
    tg.br = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);
    VklVertex* data = calloc(tg.vertex_count, sizeof(VklVertex));
    for (uint32_t i = 0; i < tg.vertex_count; i++)
    {
        data[i].pos[0] = .25 * randn();
        data[i].pos[1] = .25 * randn();
        data[i].pos[2] = 0;
        data[i].color[0] = rand_byte();
        data[i].color[1] = rand_byte();
        data[i].color[2] = rand_byte();
        data[i].color[3] = 128;
    }
    vkl_upload_buffers(gpu->context, &tg.br, 0, size, data);

    // Create the bindings.
    tg.bindings = vkl_bindings(&graphics->slots);
    vkl_bindings_create(&tg.bindings, 1);
    vkl_bindings_update(&tg.bindings);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _graphics_points_refill, &tg);

    vkl_app_run(app, N_FRAMES);

    FREE(data);
    TEST_END
}
