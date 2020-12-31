#include "test_canvas.h"
#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../src/imgui.h"
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
/*  Canvas 1                                                                                     */
/*************************************************************************************************/

static void _frame_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    log_debug(
        "canvas #%d, frame callback #%d, time %.6f, interval %.6f", //
        canvas->obj.id, ev.u.f.idx, ev.u.f.time, ev.u.f.interval);
}

static void _key_callback(VklCanvas* canvas, VklEvent ev)
{
    if (ev.u.k.type == VKL_KEY_PRESS)
        log_debug("key code %d", ev.u.k.key_code);
}

int test_canvas_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    ASSERT(canvas->window != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->window->app != NULL);

    uvec2 size = {0};

    // Framebuffer size.
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    log_debug("canvas framebuffer size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    // Screen size.
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    log_debug("canvas screen size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _frame_callback, NULL);

    vkl_app_run(app, 8);

    // Send a mock key press event.
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _key_callback, NULL);
    vkl_event_key(canvas, VKL_KEY_PRESS, VKL_KEY_A, 0);

    // Second canvas.
    log_debug("global clock elapsed %.6f interval %.6f", app->clock.elapsed, app->clock.interval);
    log_debug(
        "local clock elapsed %.6f interval %.6f", canvas->clock.elapsed, canvas->clock.interval);

    // Second canvas.
    ASSERT(canvas->window->app != NULL);
    VklCanvas* canvas2 = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    ASSERT(canvas->window->app != NULL);
    ASSERT(canvas2->window != NULL);
    ASSERT(canvas2->app != NULL);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 0, 0, 1}});
    vkl_canvas_clear_color(canvas2, (VkClearColorValue){{0, 1, 0, 1}});
    vkl_app_run(app, 5);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas 2                                                                                     */
/*************************************************************************************************/

static void _init_callback(VklCanvas* canvas, VklEvent ev) { log_debug("init event for canvas"); }

static void _wheel_callback(VklCanvas* canvas, VklEvent ev)
{
    log_debug("wheel %.3f", ev.u.w.dir[1]);
}

static void _button_callback(VklCanvas* canvas, VklEvent ev)
{
    if (ev.u.b.type == VKL_MOUSE_PRESS)
        log_debug("clicked %d mods %d", ev.u.b.button, ev.u.b.modifiers);
}

static void _cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{x, 0, y, 1}});
}

static void _timer_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    log_trace("timer callback #%d time %.3f", ev.u.t.idx, ev.u.t.time);
    float x = exp(-.01 * (float)ev.u.t.idx);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{x, 0, 0, 1}});
}

int test_canvas_2(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    vkl_event_callback(canvas, VKL_EVENT_INIT, 0, _init_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _key_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _wheel_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _button_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _cursor_callback, NULL);

    // vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, .05, _timer_callback, NULL);

    vkl_app_run(app, N_FRAMES);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle                                                                              */
/*************************************************************************************************/

static void _make_triangle2(VklCanvas* canvas, TestVisual* visual, const char* suffix)
{
    visual->gpu = canvas->gpu;
    visual->renderpass = &canvas->renderpass;
    visual->framebuffers = &canvas->framebuffers;
    test_triangle(visual, suffix);
    canvas->user_data = visual;
}

static void _triangle_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    // Take the first command buffers, which corresponds to the default canvas render command//
    // buffer.
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);

    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, &visual->br, 0);
    vkl_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    vkl_cmd_draw(cmds, idx, 0, 3);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

static void _presend(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    if (!canvas->app->has_overlay)
        return;
    VklCommands* cmds = ev.user_data;
    ASSERT(cmds != NULL);
    uint32_t idx = canvas->swapchain.img_idx;
    // log_debug("canvas frame %d, swapchain idx %d", canvas->frame_idx, idx);

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(
        cmds, idx, &canvas->renderpass_overlay, &canvas->framebuffers_overlay);
    vkl_imgui_frame(canvas, cmds, idx);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);

    ASSERT(canvas != NULL);
    vkl_submit_commands(&canvas->submit, ev.user_data);
}

// Triangle canvas
int test_canvas_3(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    app->has_overlay = true;
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &visual);

    // ImGUI.
    vkl_imgui_init(canvas);
    VklCommands* cmds = vkl_canvas_commands(canvas, VKL_DEFAULT_QUEUE_RENDER, 0, 0);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_PRE_SEND, 0, _presend, cmds);

    vkl_app_run(app, N_FRAMES);

    vkl_graphics_destroy(&visual.graphics);
    destroy_visual(&visual);
    vkl_imgui_destroy(app);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with push constant                                                           */
/*************************************************************************************************/

static vec3 push_vec; // NOTE: not thread-safe

static void _triangle_push_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);
    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, visual->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, //
            visual->framebuffers->attachments[0]->width,
            visual->framebuffers->attachments[0]->height, //
            0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, &visual->br, 0);
    vkl_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);

    // Push constants.
    vkl_cmd_push(
        cmds, idx, &visual->graphics.slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vec3), push_vec);

    vkl_cmd_draw(cmds, idx, 0, 3);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

static void _push_cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    push_vec[0] = x;
    push_vec[1] = y;
    push_vec[2] = 1;
    vkl_canvas_to_refill(canvas, true);
}

int test_canvas_4(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "_push");
    canvas->user_data = &visual;

    // Create the slots.
    vkl_graphics_push(&visual.graphics, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.graphics.slots, 1);
    vkl_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual.graphics);

    // Triangle buffer.
    visual.buffer = vkl_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    vkl_buffer_size(&visual.buffer, size);
    vkl_buffer_usage(&visual.buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkl_buffer_memory(
        &visual.buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(&visual.buffer);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vkl_buffer_upload(&visual.buffer, 0, size, data);

    visual.br.buffer = &visual.buffer;
    visual.br.size = size;
    visual.br.count = 1;

    // Refill callback
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_push_refill, &visual);

    // Cursor callback.
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _push_cursor_callback, NULL);

    vkl_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with vertex buffer update                                                    */
/*************************************************************************************************/

static void _vertex_cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
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
    vkl_upload_buffers(canvas->gpu->context, visual->br, 0, 3 * sizeof(TestVertex), data);
}

int test_canvas_5(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "");
    canvas->user_data = &visual;
    visual.data = calloc(3, sizeof(TestVertex));

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.graphics.slots, 1);
    vkl_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    visual.br = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    vkl_upload_buffers(canvas->gpu->context, visual.br, 0, size, data);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &visual);

    // Cursor callback.
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _vertex_cursor_callback, &visual);

    vkl_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    FREE(visual.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with uniform buffer update                                                   */
/*************************************************************************************************/

vec4 vec = {1, 0, 1, 1};

static void _uniform_cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    TestVisual* visual = ev.user_data;

    vec[0] = x;
    vec[1] = y;
    vec[2] = 1;
    vec[3] = 1;
    vkl_upload_buffers(canvas->gpu->context, visual->br_u, 0, sizeof(vec4), vec);
}

int test_canvas_6(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
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
    vkl_graphics_slot(&visual.graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Uniform buffer.
    visual.br_u =
        vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, img_count, sizeof(vec4));
    ASSERT(visual.br_u.aligned_size >= visual.br_u.size);
    vkl_upload_buffers(canvas->gpu->context, visual.br_u, 0, sizeof(vec4), vec);

    // Create the bindings.
    ASSERT(img_count > 0);
    visual.bindings = vkl_bindings(&visual.graphics.slots, img_count);
    ASSERT(visual.br_u.buffer != VK_NULL_HANDLE);
    vkl_bindings_buffer(&visual.bindings, 0, visual.br_u);
    vkl_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    visual.br = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    vkl_upload_buffers(canvas->gpu->context, visual.br, 0, size, data);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &visual);

    // Cursor callback.
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _uniform_cursor_callback, &visual);

    vkl_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    FREE(visual.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with compute                                                                 */
/*************************************************************************************************/

static void _triangle_compute_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);
    TestVisual* visual = ev.user_data;

    uint32_t idx = ev.u.rf.img_idx;
    vkl_cmd_begin(cmds, idx);

    vkl_cmd_compute(cmds, idx, visual->compute, (uvec3){3, 1, 1});

    vkl_cmd_begin_renderpass(cmds, idx, visual->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, &visual->br, 0);
    vkl_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    vkl_cmd_draw(cmds, idx, 0, 3);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

int test_canvas_7(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    canvas->user_data = &visual;

    // Create compute object.
    VklBindings bindings = {0};
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/test_triangle.comp.spv", SPIRV_DIR);
        visual.compute = vkl_ctx_compute(gpu->context, path);
        vkl_compute_slot(visual.compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        bindings = vkl_bindings(&visual.compute->slots, 1);
        vkl_bindings_buffer(&bindings, 0, visual.br);
        vkl_bindings_update(&bindings);
        vkl_compute_bindings(visual.compute, &bindings);
        vkl_compute_create(visual.compute);
    }

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_compute_refill, &visual);

    vkl_app_run(app, N_FRAMES);

    vkl_graphics_destroy(&visual.graphics);
    vkl_bindings_destroy(&bindings);
    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with compute                                                                 */
/*************************************************************************************************/

static void _triangle_compute(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    VklGpu* gpu = canvas->gpu;
    VklCommands* cmds = (VklCommands*)ev.user_data;
    ASSERT(is_obj_created(&cmds->obj));

    VklSubmit submit = vkl_submit(canvas->gpu);
    vkl_submit_commands(&submit, cmds);

    // HACK: hard synchronization barrier for testing purposes.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);
    vkl_submit_send(&submit, 0, NULL, 0);
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_COMPUTE);
}

int test_canvas_8(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    canvas->user_data = &visual;

    // Create compute object.
    VklBindings bindings = {0};
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/test_triangle.comp.spv", SPIRV_DIR);
        visual.compute = vkl_ctx_compute(gpu->context, path);
        vkl_compute_slot(visual.compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        bindings = vkl_bindings(&visual.compute->slots, 1);
        vkl_bindings_buffer(&bindings, 0, visual.br);
        vkl_bindings_update(&bindings);
        vkl_compute_bindings(visual.compute, &bindings);
        vkl_compute_create(visual.compute);
    }

    // VklSemaphores compute_finished = vkl_semaphores(gpu, 2);

    VklCommands* cmds = vkl_container_alloc(&canvas->commands);
    *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_COMPUTE, 1);
    vkl_cmd_begin(cmds, 0);
    vkl_cmd_compute(cmds, 0, visual.compute, (uvec3){3, 1, 1});
    vkl_cmd_end(cmds, 0);
    ASSERT(is_obj_created(&cmds->obj));

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &visual);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _triangle_compute, cmds);

    vkl_app_run(app, N_FRAMES);

    vkl_graphics_destroy(&visual.graphics);
    vkl_bindings_destroy(&bindings);
    destroy_visual(&visual);
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
    VklCommands* cmds;
    VklFences fence;
    VklBufferRegions br;
    bool is_running;
};

static void _particle_frame(VklCanvas* canvas, VklPrivateEvent ev)
{
    TestVisual* visual = (TestVisual*)ev.user_data;
    VklContext* ctx = visual->gpu->context;
    TestParticleUniform* data_u = visual->data_u;
    data_u->dt = (float)canvas->clock.interval;

    // This command is slower as it causes a full GPU wait every time we update the uniform buffer
    // because there is only one.
    // vkl_buffer_regions_upload(
    //     canvas->gpu->context, &visual->br_u, 0, sizeof(TestParticleUniform), visual->data_u);

    // This command is slighty faster as there are no waits, the uniform buffer is updated
    // directly in the main event loop, but there are as many copies as there are swapchain
    // images. Only the buffer region corresponding to the current swapchain image is
    // updated, because we're sure that region is not being used by the RENDER queue.
    vkl_upload_buffers_immediate(
        canvas, visual->br_u, false, 0, sizeof(TestParticleUniform), visual->data_u);

    // Here we submit tasks to the compute queue independently of the main render loop.
    // We submit a new task as soon as the old one finishes.
    TestParticleCompute* tpc = visual->user_data;
    log_trace("frame #%d running %d", canvas->frame_idx, tpc->is_running);
    if (tpc->is_running && vkl_fences_ready(&tpc->fence, 0))
    {
        log_trace("compute task finished");
        tpc->is_running = false;
    }
    if (!tpc->is_running)
    {
        // Copy storage buffer to vertex buffer.
        log_trace("enqueue copy from storage buffer to vertex buffer");
        vkl_copy_buffers(ctx, tpc->br, 0, visual->br, 0, visual->br.size);

        // Send the command buffer.
        log_trace("submit new compute command");
        VklSubmit submit = vkl_submit(visual->gpu);
        vkl_submit_commands(&submit, tpc->cmds);
        vkl_submit_send(&submit, 0, &tpc->fence, 0);
        // ASSERT(!vkl_fences_ready(&tpc->fence, 0));
        tpc->is_running = true;
    }
}

static void _particle_cursor(VklCanvas* canvas, VklEvent ev)
{
    TestVisual* visual = (TestVisual*)ev.user_data;
    TestParticleUniform* data_u = visual->data_u;
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = -1 + 2 * ev.u.m.pos[0] / (double)size[0];
    double y = -1 + 2 * ev.u.m.pos[1] / (double)size[1];

    data_u->pos[0] = x;
    data_u->pos[1] = y;
}

static void _particle_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);

    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;
    vkl_cmd_begin(cmds, idx);

    // int32_t nn = (int32_t)visual->n_vertices;
    // vkl_cmd_push_constants(
    //     cmds, idx, visual->compute->slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int32_t),
    //     &nn);
    // vkl_cmd_compute(cmds, idx, visual->compute, (uvec3){visual->n_vertices, 1, 1});

    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, //
            canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, &visual->br, 0);
    vkl_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    vkl_cmd_draw(cmds, idx, 0, visual->n_vertices);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

int test_canvas_particles(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    // Sync mode only to set things up.
    vkl_transfer_mode(canvas->gpu->context, VKL_TRANSFER_MODE_SYNC);

    TestVisual* visual = calloc(1, sizeof(TestVisual));
    visual->gpu = canvas->gpu;
    visual->renderpass = &canvas->renderpass;
    visual->framebuffers = &canvas->framebuffers;

    // Create graphics pipeline.

    visual->graphics = vkl_graphics(gpu);
    ASSERT(visual->renderpass != NULL);
    VklGraphics* graphics = &visual->graphics;
    char path[1024];

    {
        vkl_graphics_renderpass(graphics, visual->renderpass, 0);
        vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);
        snprintf(path, sizeof(path), "%s/test_marker.vert.spv", SPIRV_DIR);
        vkl_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
        snprintf(path, sizeof(path), "%s/test_marker.frag.spv", SPIRV_DIR);
        vkl_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
        vkl_graphics_vertex_binding(graphics, 0, sizeof(TestParticle));
        vkl_graphics_vertex_attr(
            graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestParticle, pos));
        vkl_graphics_vertex_attr(
            graphics, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestParticle, vel));
        vkl_graphics_vertex_attr(
            graphics, 0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestParticle, color));
    }

    // Create the buffer.
    const uint32_t n = 20000;
    visual->n_vertices = n;
    VkDeviceSize size = n * sizeof(TestParticle);
    // Vertex buffer.
    visual->br = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    TestParticleCompute tpc = {0};
    // Struct holding some pointers for compute command buffer submission.
    {
        tpc.br = vkl_ctx_buffers(gpu->context, VKL_DEFAULT_BUFFER_STORAGE, 1, size);
        tpc.fence = vkl_fences(gpu, 1);
        visual->user_data = calloc(1, sizeof(TestParticleCompute));
    }

    // Generate some data.
    {
        visual->data = calloc(n, sizeof(TestParticle));
        for (uint32_t i = 0; i < n; i++)
        {
            ((TestParticle*)visual->data)[i].pos[0] = .2 * randn();
            ((TestParticle*)visual->data)[i].pos[1] = .2 * randn();
            ((TestParticle*)visual->data)[i].vel[0] = .05 * randn();
            ((TestParticle*)visual->data)[i].vel[1] = .05 * randn();
            ((TestParticle*)visual->data)[i].color[0] = rand_float();
            ((TestParticle*)visual->data)[i].color[1] = rand_float();
            ((TestParticle*)visual->data)[i].color[2] = rand_float();
            ((TestParticle*)visual->data)[i].color[3] = .5;
        }
        // Vertex buffer
        vkl_upload_buffers(canvas->gpu->context, visual->br, 0, size, visual->data);
        // Copy in the storage buffer
        vkl_upload_buffers(canvas->gpu->context, tpc.br, 0, size, visual->data);
        FREE(visual->data);
    }

    // Create the graphics bindings.
    {
        // Create the bindings.
        visual->bindings = vkl_bindings(&visual->graphics.slots, 1);
        vkl_bindings_update(&visual->bindings);

        // Create the graphics pipeline.
        vkl_graphics_create(&visual->graphics);
    }

    // Compute resources.
    VklBindings bindings = {0};
    {
        // Create compute object.
        snprintf(path, sizeof(path), "%s/test_particle.comp.spv", SPIRV_DIR);
        visual->compute = vkl_ctx_compute(gpu->context, path);

        // Slots
        vkl_compute_slot(visual->compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // vertex buffer
        vkl_compute_slot(visual->compute, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // UBO
        vkl_compute_push(visual->compute, 0, sizeof(int32_t), VK_SHADER_STAGE_COMPUTE_BIT);

        // Uniform buffer.
        visual->br_u = vkl_ctx_buffers(
            gpu->context, VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
            sizeof(TestParticleUniform));
        visual->data_u = calloc(1, sizeof(TestParticleUniform));

        // Create the bindings.
        bindings = vkl_bindings(&visual->compute->slots, canvas->swapchain.img_count);
        vkl_bindings_buffer(&bindings, 0, tpc.br);
        vkl_bindings_buffer(&bindings, 1, visual->br_u);
        vkl_bindings_update(&bindings);

        vkl_compute_bindings(visual->compute, &bindings);
        vkl_compute_create(visual->compute);
    }

    // Compute command buffer?
    VklCommands* cmds = vkl_container_alloc(&canvas->commands);
    int32_t nn = (int32_t)visual->n_vertices;
    {
        *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_COMPUTE, 1);
        vkl_cmd_begin(cmds, 0);
        vkl_cmd_push(
            cmds, 0, &visual->compute->slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int32_t),
            &nn);
        vkl_cmd_compute(cmds, 0, visual->compute, (uvec3){visual->n_vertices, 1, 1});
        vkl_cmd_end(cmds, 0);
        tpc.cmds = cmds;
        memcpy(visual->user_data, &tpc, sizeof(TestParticleCompute));
    }

    // Callbacks.
    {
        canvas->user_data = visual;
        vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _particle_refill, visual);
        vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _particle_frame, visual);
        vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);
        vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _particle_cursor, visual);
    }

    vkl_transfer_mode(canvas->gpu->context, VKL_TRANSFER_MODE_ASYNC);
    vkl_app_run(app, N_FRAMES);

    FREE(visual->data_u);
    FREE(visual->user_data);
    vkl_fences_destroy(&tpc.fence);
    destroy_visual(visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas offscreen                                                                             */
/*************************************************************************************************/

int test_canvas_offscreen(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_OFFSCREEN);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _frame_callback, NULL);

    vkl_app_run(app, N_FRAMES);

    // // Send a mock key press event.
    // vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _key_callback, NULL);
    // vkl_event_key(canvas, VKL_KEY_PRESS, VKL_KEY_A);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas screencast                                                                            */
/*************************************************************************************************/

static void _screencast_callback(VklCanvas* canvas, VklEvent ev)
{
    log_trace("screencast frame #%d %d", ev.u.s.idx, ev.u.s.rgba[0]);
    // write_ppm("screenshot.ppm", ev.u.s.width, ev.u.s.height, ev.u.s.rgba);
    FREE(ev.u.s.rgba);
}

int test_canvas_screencast(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _cursor_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_SCREENCAST, 0, _screencast_callback, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1., _fps, NULL);

    vkl_screencast(canvas, 1. / 30);

    vkl_app_run(app, N_FRAMES);
    TEST_END
}
