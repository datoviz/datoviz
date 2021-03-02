#include "test_canvas.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/context.h"
#include "../include/datoviz/controls.h"
#include "../src/vklite_utils.h"
#include "utils.h"


typedef struct TestParticle TestParticle;



struct TestParticle
{
    vec3 pos;
    vec3 vel;
    vec4 color;
};



/*************************************************************************************************/
/*  Canvas buffer upload                                                                         */
/*************************************************************************************************/

int test_canvas_transfer_buffer(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    VkDeviceSize size = 16;
    DvzBufferRegions br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);
    DvzBufferRegions br2 = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);
    AT(br.offsets[0] < br2.offsets[0]);

    uint8_t* data = calloc(size, sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;

    // Upload a buffer.
    dvz_upload_buffers(canvas, br, 0, size, data);
    dvz_app_run(app, 3);

    // Copy.
    dvz_copy_buffers(canvas, br, 0, br2, 0, size);
    dvz_app_run(app, 3);

    // Download a buffer.
    uint8_t* data2 = calloc(size, sizeof(uint8_t));
    dvz_download_buffers(canvas, br2, 0, size, data2);

    // Compare.
    dvz_app_run(app, 3);
    AT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);
    TEST_END
}



int test_canvas_transfer_texture(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;

    VkDeviceSize size = 16 * 16 * 4;
    uint8_t* data = calloc(size, sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++)
        data[i] = (uint8_t)(i % 256);

    uvec3 offset = {0};
    uvec3 shape = {16, 16, 1};

    DvzTexture* tex = dvz_ctx_texture(ctx, 2, shape, VK_FORMAT_R8G8B8A8_UNORM);
    DvzTexture* tex2 = dvz_ctx_texture(ctx, 2, shape, VK_FORMAT_R8G8B8A8_UNORM);

    // Upload.
    dvz_upload_texture(canvas, tex, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size, data);
    dvz_app_run(app, 3);

    // Copy.
    dvz_copy_texture(canvas, tex, offset, tex2, offset, shape, size);
    dvz_app_run(app, 3);

    // Download.
    uint8_t* data2 = calloc(size, sizeof(uint8_t));
    dvz_download_texture(canvas, tex2, offset, shape, size, data2);

    // Compare.
    dvz_app_run(app, 3);
    AT(memcmp(data, data2, size) == 0);

    FREE(data);
    FREE(data2);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas 1                                                                                     */
/*************************************************************************************************/

static void _frame_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug(
        "canvas #%d, frame callback #%d, time %.6f, interval %.6f", //
        canvas->obj.id, ev.u.f.idx, ev.u.f.time, ev.u.f.interval);
}

static void _key_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("key code %d", ev.u.k.key_code);
}

int test_canvas_1(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    ASSERT(canvas->window != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->window->app != NULL);

    uvec2 size = {0};

    // Framebuffer size.
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);
    log_debug("canvas framebuffer size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    // Screen size.
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    log_debug("canvas screen size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _frame_callback, NULL);

    dvz_app_run(app, 8);

    // Send a mock key press event.
    dvz_event_callback(canvas, DVZ_EVENT_KEY_PRESS, 0, DVZ_EVENT_MODE_SYNC, _key_callback, NULL);
    dvz_event_key_press(canvas, DVZ_KEY_A, 0);

    // Second canvas.
    log_debug("global clock elapsed %.6f interval %.6f", app->clock.elapsed, app->clock.interval);
    log_debug(
        "local clock elapsed %.6f interval %.6f", canvas->clock.elapsed, canvas->clock.interval);

    // Second canvas.
    ASSERT(canvas->window->app != NULL);
    DvzCanvas* canvas2 = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    ASSERT(canvas->window->app != NULL);
    ASSERT(canvas2->window != NULL);
    ASSERT(canvas2->app != NULL);
    dvz_canvas_clear_color(canvas, 1, 0, 0);
    dvz_canvas_clear_color(canvas2, 0, 1, 0);
    dvz_app_run(app, 5);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas 2                                                                                     */
/*************************************************************************************************/

static void _init_callback(DvzCanvas* canvas, DvzEvent ev) { log_debug("init event for canvas"); }

static void _wheel_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("wheel %.3f", ev.u.w.dir[1]);
}

static void _button_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("clicked %d mods %d", ev.u.b.button, ev.u.b.modifiers);
}

static void _cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    dvz_canvas_clear_color(canvas, x, 0, y);
}

static void _timer_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_trace("timer callback #%d time %.3f", ev.u.t.idx, ev.u.t.time);
    float x = exp(-.01 * (float)ev.u.t.idx);
    dvz_canvas_clear_color(canvas, x, 0, 0);
}

int test_canvas_2(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    dvz_event_callback(canvas, DVZ_EVENT_INIT, 0, DVZ_EVENT_MODE_SYNC, _init_callback, NULL);
    dvz_event_callback(canvas, DVZ_EVENT_KEY_PRESS, 0, DVZ_EVENT_MODE_SYNC, _key_callback, NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_WHEEL, 0, DVZ_EVENT_MODE_SYNC, _wheel_callback, NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_PRESS, 0, DVZ_EVENT_MODE_SYNC, _button_callback, NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _cursor_callback, NULL);

    // dvz_event_callback(canvas, DVZ_EVENT_TIMER, .05, DVZ_EVENT_MODE_SYNC, _timer_callback,
    // NULL);

    dvz_app_run(app, N_FRAMES);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle                                                                              */
/*************************************************************************************************/

static void _make_triangle2(DvzCanvas* canvas, TestVisual* visual, const char* suffix)
{
    visual->gpu = canvas->gpu;
    visual->n_vertices = 3;
    visual->renderpass = &canvas->renderpass;
    visual->framebuffers = &canvas->framebuffers;
    test_triangle(visual, suffix);
    canvas->user_data = visual;
}

static void _triangle_refill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    // Take the first command buffers, which corresponds to the default canvas render command//
    // buffer.
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);

    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, visual->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    dvz_cmd_draw(cmds, idx, 0, visual->n_vertices);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

// Triangle canvas
int test_canvas_3(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, &visual);

    dvz_app_run(app, N_FRAMES);
    dvz_graphics_destroy(&visual.graphics);
    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with push constant                                                           */
/*************************************************************************************************/

static vec3 push_vec; // NOTE: not thread-safe

static void _triangle_push_refill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);
    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;

    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, visual->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, //
            visual->framebuffers->attachments[0]->width,
            visual->framebuffers->attachments[0]->height, //
            0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, visual->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);

    // Push constants.
    dvz_cmd_push(
        cmds, idx, &visual->graphics.slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vec3), push_vec);

    dvz_cmd_draw(cmds, idx, 0, 3);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

static void _push_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    push_vec[0] = x;
    push_vec[1] = y;
    push_vec[2] = 1;
    dvz_canvas_to_refill(canvas);
}

static void _wait(DvzCanvas* canvas, DvzEvent ev) { dvz_sleep(50); }

int test_canvas_4(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "_push");
    canvas->user_data = &visual;

    // Create the slots.
    dvz_graphics_push(&visual.graphics, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);

    // Create the bindings.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Triangle buffer.
    visual.buffer = dvz_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    dvz_buffer_size(&visual.buffer, size);
    dvz_buffer_usage(&visual.buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_memory(
        &visual.buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_create(&visual.buffer);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    dvz_buffer_upload(&visual.buffer, 0, size, data);

    visual.br.buffer = &visual.buffer;
    visual.br.size = size;
    visual.br.count = 1;

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_push_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _push_cursor_callback, NULL);

    dvz_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with vertex buffer update                                                    */
/*************************************************************************************************/

static void _vertex_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];

    TestVisual* visual = ev.user_data;
    TestVertex* data = (TestVertex*)visual->data;
    for (uint32_t i = 0; i < 3; i++)
    {
        data[i].color[0] = x;
        data[i].color[1] = y;
        data[i].color[2] = 1;
    }
    dvz_upload_buffers(canvas, visual->br, 0, 3 * sizeof(TestVertex), data);
}

int test_canvas_5(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "");
    canvas->user_data = &visual;
    visual.data = calloc(3, sizeof(TestVertex));

    // Create the bindings.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    visual.br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    dvz_upload_buffers(canvas, visual.br, 0, 3 * sizeof(TestVertex), data);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, &visual);

    // Cursor callback.
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _vertex_cursor_callback, &visual);

    dvz_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    FREE(visual.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with uniform buffer update                                                   */
/*************************************************************************************************/

vec4 vec = {1, 0, 1, 1};

static void _uniform_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    vec[0] = x;
    vec[1] = y;
    vec[2] = 1;
    vec[3] = 1;
}

static void _uniform_frame_callback(DvzCanvas* canvas, DvzEvent ev)
{
    TestVisual* visual = ev.user_data;
    dvz_upload_buffers(canvas, visual->br_u, 0, sizeof(vec4), vec);
}

int test_canvas_6(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);
    uint32_t img_count = canvas->swapchain.img_count;
    ASSERT(img_count > 0);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "_ubo");
    canvas->user_data = &visual;
    visual.data = calloc(3, sizeof(TestVertex));

    // Create the slots.
    dvz_graphics_slot(&visual.graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Uniform buffer.
    visual.br_u =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, img_count, sizeof(vec4));
    ASSERT(visual.br_u.aligned_size >= visual.br_u.size);
    dvz_upload_buffers(canvas, visual.br_u, 0, sizeof(vec4), vec);

    // Create the bindings.
    ASSERT(img_count > 0);
    visual.bindings = dvz_bindings(&visual.graphics.slots, img_count);
    ASSERT(visual.br_u.buffer != VK_NULL_HANDLE);
    dvz_bindings_buffer(&visual.bindings, 0, visual.br_u);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    visual.br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    dvz_upload_buffers(canvas, visual.br, 0, size, data);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, &visual);

    // Cursor callback.
    // WARNING: UNIFORM_MAPPABLE must be updated at every frame!
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _uniform_cursor_callback, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _uniform_frame_callback, &visual);

    dvz_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    FREE(visual.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with compute                                                                 */
/*************************************************************************************************/

static void _triangle_compute_refill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);
    TestVisual* visual = ev.user_data;

    uint32_t idx = ev.u.rf.img_idx;
    dvz_cmd_begin(cmds, idx);

    dvz_cmd_compute(cmds, idx, visual->compute, (uvec3){3, 1, 1});

    dvz_cmd_begin_renderpass(cmds, idx, visual->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, visual->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    dvz_cmd_draw(cmds, idx, 0, 3);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

int test_canvas_7(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    canvas->user_data = &visual;

    // Create compute object.
    DvzBindings bindings = {0};
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/test_triangle.comp.spv", SPIRV_DIR);
        visual.compute = dvz_ctx_compute(gpu->context, path);
        dvz_compute_slot(visual.compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        bindings = dvz_bindings(&visual.compute->slots, 1);
        dvz_bindings_buffer(&bindings, 0, visual.br);
        dvz_bindings_update(&bindings);
        dvz_compute_bindings(visual.compute, &bindings);
        dvz_compute_create(visual.compute);
    }

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_compute_refill, &visual);

    dvz_app_run(app, N_FRAMES);

    dvz_graphics_destroy(&visual.graphics);
    dvz_bindings_destroy(&bindings);
    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with compute                                                                 */
/*************************************************************************************************/

static void _triangle_compute(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    DvzGpu* gpu = canvas->gpu;
    DvzCommands* cmds = (DvzCommands*)ev.user_data;
    ASSERT(dvz_obj_is_created(&cmds->obj));

    DvzSubmit submit = dvz_submit(canvas->gpu);
    dvz_submit_commands(&submit, cmds);

    // HACK: hard synchronization barrier for testing purposes.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_submit_send(&submit, 0, NULL, 0);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_COMPUTE);
}

int test_canvas_8(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    canvas->user_data = &visual;

    // Create compute object.
    DvzBindings bindings = {0};
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/test_triangle.comp.spv", SPIRV_DIR);
        visual.compute = dvz_ctx_compute(gpu->context, path);
        dvz_compute_slot(visual.compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        bindings = dvz_bindings(&visual.compute->slots, 1);
        dvz_bindings_buffer(&bindings, 0, visual.br);
        dvz_bindings_update(&bindings);
        dvz_compute_bindings(visual.compute, &bindings);
        dvz_compute_create(visual.compute);
    }

    // DvzSemaphores compute_finished = dvz_semaphores(gpu, 2);

    DvzCommands* cmds = dvz_container_alloc(&canvas->commands);
    *cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_COMPUTE, 1);
    dvz_cmd_begin(cmds, 0);
    dvz_cmd_compute(cmds, 0, visual.compute, (uvec3){3, 1, 1});
    dvz_cmd_end(cmds, 0);
    ASSERT(dvz_obj_is_created(&cmds->obj));

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, &visual);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _triangle_compute, cmds);

    dvz_app_run(app, N_FRAMES);

    dvz_graphics_destroy(&visual.graphics);
    dvz_bindings_destroy(&bindings);
    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Triangles depth                                                                              */
/*************************************************************************************************/

int test_canvas_depth(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);
    uint32_t img_count = canvas->swapchain.img_count;
    ASSERT(img_count > 0);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;

    visual.graphics = dvz_graphics(gpu);
    DvzGraphics* graphics = &visual.graphics;
    dvz_graphics_renderpass(graphics, visual.renderpass, 0);
    dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_triangle%s.vert.spv", SPIRV_DIR, "");
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle%s.frag.spv", SPIRV_DIR, "");
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(graphics, 0, sizeof(TestVertex));
    dvz_graphics_vertex_attr(
        graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));

    canvas->user_data = &visual;

    // Create the bindings.
    ASSERT(img_count > 0);
    visual.bindings = dvz_bindings(&visual.graphics.slots, img_count);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Triangle buffer.
    const uint32_t N = 3 * 1000;
    visual.n_vertices = N;
    visual.data = calloc(N, sizeof(TestVertex));
    VkDeviceSize size = N * sizeof(TestVertex);
    visual.br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);
    float x = 0;
    float y = 0;
    float l = .075;
    float z = 0;
    TestVertex *v0, *v1, *v2;
    uint32_t j = 0;
    for (uint32_t i = 0; i < N / 3; i++)
    {
        v0 = &((TestVertex*)visual.data)[3 * i + 0];
        v1 = &((TestVertex*)visual.data)[3 * i + 1];
        v2 = &((TestVertex*)visual.data)[3 * i + 2];

        x = .75 * (-1 + 2 * dvz_rand_float());
        y = .75 * (-1 + 2 * dvz_rand_float());

        // The following should work even if the depth buffer is not working.
        // j = i < N / 6 ? 0 : 1;

        // The following checks the depth buffer.
        j = i % 2;

        // red background, green foreground
        z = j == 0 ? .75 : .25; // j == 0, .75 = background, .25 = foreground

        v0->pos[0] = x - l;
        v0->pos[1] = y - l;
        v0->pos[2] = z;
        v0->color[j] = .2;
        v0->color[3] = 1;

        v1->pos[0] = x + l;
        v1->pos[1] = y - l;
        v1->pos[2] = z;
        v1->color[j] = .5;
        v1->color[3] = 1;

        v2->pos[0] = x + 0;
        v2->pos[1] = y + l;
        v2->pos[2] = z;
        v2->color[j] = .8;
        v2->color[j + 1] = .3;
        v2->color[3] = 1;
    }
    dvz_upload_buffers(canvas, visual.br, 0, size, visual.data);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, &visual);

    dvz_app_run(app, N_FRAMES);
    destroy_visual(&visual);
    FREE(visual.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with vertex buffer update                                                    */
/*************************************************************************************************/

static void _append_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    TestVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    const uint32_t N = 3 + ev.u.t.idx;
    visual->n_vertices = 3 * N;
    TestVertex* data = calloc(visual->n_vertices, sizeof(TestVertex));
    float t = 0, t2 = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)N;
        t2 = (i + 1) / (float)N;

        data[3 * i + 0].color[0] = 1;
        data[3 * i + 0].color[3] = 1;

        data[3 * i + 1].color[1] = 1;
        data[3 * i + 1].color[3] = 1;
        data[3 * i + 1].pos[0] = .5 * cos(M_2PI * t);
        data[3 * i + 1].pos[1] = .5 * sin(M_2PI * t);

        data[3 * i + 2].color[2] = 1;
        data[3 * i + 2].color[3] = 1;
        data[3 * i + 2].pos[0] = .5 * cos(M_2PI * t2);
        data[3 * i + 2].pos[1] = .5 * sin(M_2PI * t2);
    }
    FREE(visual->data);
    visual->data = data;
    VkDeviceSize size = visual->n_vertices * sizeof(TestVertex);
    visual->br = dvz_ctx_buffers(canvas->gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);
    dvz_upload_buffers(canvas, visual->br, 0, size, data);
    // NOTE: important, we need to refill the canvas after the vertex count has changed.
    dvz_canvas_to_refill(canvas);
}

int test_canvas_append(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.n_vertices = 3;
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "");
    canvas->user_data = &visual;
    visual.data = calloc(3, sizeof(TestVertex));

    // Create the bindings.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    visual.br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    dvz_upload_buffers(canvas, visual.br, 0, size, data);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_TIMER, .1, DVZ_EVENT_MODE_SYNC, _append_callback, &visual);

    dvz_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    FREE(visual.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas with particles                                                                        */
/*************************************************************************************************/

typedef struct TestParticleUniform TestParticleUniform;
typedef struct TestParticleCompute TestParticleCompute;

struct TestParticleUniform
{
    vec4 pos;
    float dt;
};

struct TestParticleCompute
{
    DvzCommands* cmds;
    DvzFences fence;
    DvzBufferRegions br;
    bool is_running;
};

static void _particle_frame(DvzCanvas* canvas, DvzEvent ev)
{
    TestVisual* visual = (TestVisual*)ev.user_data;
    TestParticleUniform* data_u = visual->data_u;
    data_u->dt = (float)canvas->clock.interval;
    dvz_upload_buffers(canvas, visual->br_u, 0, sizeof(TestParticleUniform), visual->data_u);

    // Here we submit tasks to the compute queue independently of the main render loop.
    // We submit a new task as soon as the old one finishes.
    TestParticleCompute* tpc = visual->user_data;
    log_debug("frame #%d running %d", canvas->frame_idx, tpc->is_running);
    if (tpc->is_running && dvz_fences_ready(&tpc->fence, 0))
    {
        log_debug("compute task finished");
        tpc->is_running = false;
    }
    if (!tpc->is_running)
    {
        // Copy storage buffer to vertex buffer.
        log_debug("enqueue copy from storage buffer to vertex buffer");
        dvz_copy_buffers(canvas, tpc->br, 0, visual->br, 0, visual->br.size);

        // Send the command buffer.
        log_debug("submit new compute command");
        DvzSubmit submit = dvz_submit(visual->gpu);
        dvz_submit_commands(&submit, tpc->cmds);
        dvz_submit_send(&submit, 0, &tpc->fence, 0);
        // ASSERT(!dvz_fences_ready(&tpc->fence, 0));
        tpc->is_running = true;
    }
}

static void _particle_cursor(DvzCanvas* canvas, DvzEvent ev)
{
    TestVisual* visual = (TestVisual*)ev.user_data;
    TestParticleUniform* data_u = visual->data_u;
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = -1 + 2 * ev.u.m.pos[0] / (double)size[0];
    double y = -1 + 2 * ev.u.m.pos[1] / (double)size[1];

    data_u->pos[0] = x;
    data_u->pos[1] = y;
}

static void _particle_refill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);

    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;
    dvz_cmd_begin(cmds, idx);

    // int32_t nn = (int32_t)visual->n_vertices;
    // dvz_cmd_push_constants(
    //     cmds, idx, visual->compute->slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int32_t),
    //     &nn);
    // dvz_cmd_compute(cmds, idx, visual->compute, (uvec3){visual->n_vertices, 1, 1});

    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, //
            canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, visual->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    dvz_cmd_draw(cmds, idx, 0, visual->n_vertices);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

int test_canvas_particles(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    AT(canvas != NULL);

    TestVisual* visual = calloc(1, sizeof(TestVisual));
    visual->gpu = canvas->gpu;
    visual->renderpass = &canvas->renderpass;
    visual->framebuffers = &canvas->framebuffers;

    // Create graphics pipeline.
    visual->graphics = dvz_graphics(gpu);
    ASSERT(visual->renderpass != NULL);
    DvzGraphics* graphics = &visual->graphics;
    char path[1024];
    // Graphics pipeline.
    {
        dvz_graphics_renderpass(graphics, visual->renderpass, 0);
        dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);
        snprintf(path, sizeof(path), "%s/test_marker.vert.spv", SPIRV_DIR);
        dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
        snprintf(path, sizeof(path), "%s/test_marker.frag.spv", SPIRV_DIR);
        dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
        dvz_graphics_vertex_binding(graphics, 0, sizeof(TestParticle));
        dvz_graphics_vertex_attr(
            graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestParticle, pos));
        dvz_graphics_vertex_attr(
            graphics, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestParticle, vel));
        dvz_graphics_vertex_attr(
            graphics, 0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestParticle, color));
    }

    // Create the buffer.
    const uint32_t n = 20000;
    visual->n_vertices = n;
    VkDeviceSize size = n * sizeof(TestParticle);
    // Vertex buffer.
    visual->br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);

    TestParticleCompute tpc = {0};
    // Struct holding some pointers for compute command buffer submission.
    {
        tpc.br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_STORAGE, 1, size);
        tpc.fence = dvz_fences(gpu, 1, true);
        visual->user_data = calloc(1, sizeof(TestParticleCompute));
    }

    // Generate some data.
    {
        visual->data = calloc(n, sizeof(TestParticle));
        for (uint32_t i = 0; i < n; i++)
        {
            ((TestParticle*)visual->data)[i].pos[0] = .2 * dvz_rand_normal();
            ((TestParticle*)visual->data)[i].pos[1] = .2 * dvz_rand_normal();
            ((TestParticle*)visual->data)[i].vel[0] = .05 * dvz_rand_normal();
            ((TestParticle*)visual->data)[i].vel[1] = .05 * dvz_rand_normal();
            ((TestParticle*)visual->data)[i].color[0] = dvz_rand_float();
            ((TestParticle*)visual->data)[i].color[1] = dvz_rand_float();
            ((TestParticle*)visual->data)[i].color[2] = dvz_rand_float();
            ((TestParticle*)visual->data)[i].color[3] = .5;
        }
        // Vertex buffer
        dvz_upload_buffers(canvas, visual->br, 0, size, visual->data);
        // Copy in the storage buffer
        dvz_upload_buffers(canvas, tpc.br, 0, size, visual->data);
        // WARNING: it's okay to free the pointer here, dvz_upload_buffers() is normally lazy
        // and does not make a copy of the pointer, *except* when the app is not running, like
        // here.
        FREE(visual->data);
    }

    // Create the graphics bindings.
    {
        // Create the bindings.
        visual->bindings = dvz_bindings(&visual->graphics.slots, 1);
        dvz_bindings_update(&visual->bindings);

        // Create the graphics pipeline.
        dvz_graphics_create(&visual->graphics);
    }

    // Compute resources.
    DvzBindings bindings = {0};
    {
        // Create compute object.
        snprintf(path, sizeof(path), "%s/test_particle.comp.spv", SPIRV_DIR);
        visual->compute = dvz_ctx_compute(gpu->context, path);

        // Slots
        dvz_compute_slot(visual->compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // vertex buffer
        dvz_compute_slot(visual->compute, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // UBO
        dvz_compute_push(visual->compute, 0, sizeof(int32_t), VK_SHADER_STAGE_COMPUTE_BIT);

        // Uniform buffer.
        visual->br_u = dvz_ctx_buffers(
            gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
            sizeof(TestParticleUniform));
        visual->data_u = calloc(1, sizeof(TestParticleUniform));

        // Create the bindings.
        bindings = dvz_bindings(&visual->compute->slots, canvas->swapchain.img_count);
        dvz_bindings_buffer(&bindings, 0, tpc.br);
        dvz_bindings_buffer(&bindings, 1, visual->br_u);
        dvz_bindings_update(&bindings);

        dvz_compute_bindings(visual->compute, &bindings);
        dvz_compute_create(visual->compute);
    }

    // Compute command buffer?
    DvzCommands* cmds = dvz_container_alloc(&canvas->commands);
    int32_t nn = (int32_t)visual->n_vertices;
    {
        *cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_COMPUTE, 1);
        dvz_cmd_begin(cmds, 0);
        dvz_cmd_push(
            cmds, 0, &visual->compute->slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int32_t),
            &nn);
        dvz_cmd_compute(cmds, 0, visual->compute, (uvec3){visual->n_vertices, 1, 1});
        dvz_cmd_end(cmds, 0);
        tpc.cmds = cmds;
        memcpy(visual->user_data, &tpc, sizeof(TestParticleCompute));
    }

    // Callbacks.
    {
        canvas->user_data = visual;
        dvz_event_callback(
            canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _particle_refill, visual);
        dvz_event_callback(
            canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _particle_frame, visual);
        dvz_event_callback(
            canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _particle_cursor, visual);
    }

    dvz_app_run(app, N_FRAMES);

    FREE(visual->data_u);
    FREE(visual->user_data);
    dvz_fences_destroy(&tpc.fence);
    destroy_visual(visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas offscreen                                                                             */
/*************************************************************************************************/

int test_canvas_offscreen(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_OFFSCREEN);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _frame_callback, NULL);

    dvz_app_run(app, 10);

    // // Send a mock key press event.
    // dvz_event_callback(canvas, DVZ_EVENT_KEY, 0, DVZ_EVENT_MODE_SYNC, _key_callback, NULL);
    // dvz_event_key(canvas, DVZ_KEY_PRESS, DVZ_KEY_A);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas GUI                                                                                   */
/*************************************************************************************************/

static void _gui_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_SLIDER_FLOAT)
    {
        float* value = ev.u.g.control->value;
        log_info("value is %.3f", *value);
    }
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_SLIDER_INT)
    {
        int* value = ev.u.g.control->value;
        log_info("value is %d", *value);
    }
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_CHECKBOX)
    {
        bool* value = ev.u.g.control->value;
        log_info("value is %s", (*value) ? "checked" : "unchecked");
    }
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_BUTTON)
    {
        log_info("button pressed");
    }
}

int test_canvas_gui_1(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);
    AT(canvas != NULL);

    // Make a simple GUI.
    DvzGui* gui = dvz_gui(canvas, "Hello world", 0);

    dvz_gui_label(gui, "label", "hello world!");
    dvz_gui_checkbox(gui, "my checkbox", true);
    dvz_gui_slider_float(gui, "my slider 1", 0.0f, 1.0f, .5);
    dvz_gui_slider_float2(gui, "my slider bis", 0.0f, 1.0f, (vec2){.25, .75});
    dvz_gui_slider_int(gui, "my slider 2", 10, 20, 10);
    dvz_gui_input_float(gui, "enter a float", 1, 10, 0);
    dvz_gui_textbox(gui, "textbox", "some text");
    dvz_gui_button(gui, "my button", 0);
    dvz_gui_colormap(gui, DVZ_CMAP_VIRIDIS);

    // dvz_gui_demo(gui);

    dvz_event_callback(canvas, DVZ_EVENT_GUI, 0, DVZ_EVENT_MODE_SYNC, _gui_callback, NULL);

    dvz_app_run(app, N_FRAMES);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas screencast                                                                            */
/*************************************************************************************************/

static void _screencast_callback(DvzCanvas* canvas, DvzEvent ev)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/screencast_%02d.ppm", ARTIFACTS_DIR, (int)ev.u.sc.idx);
    log_info("screencast frame #%d %d %s", ev.u.sc.idx, ev.u.sc.rgba[0], path);
    dvz_write_ppm(path, ev.u.sc.width, ev.u.sc.height, ev.u.sc.rgba);
    FREE(ev.u.sc.rgba);
}

int test_canvas_screencast(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _cursor_callback, NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_SCREENCAST, 0, DVZ_EVENT_MODE_SYNC, _screencast_callback, NULL);

    dvz_screencast(canvas, 1. / 30, false);

    dvz_app_run(app, N_FRAMES);
    TEST_END
}
