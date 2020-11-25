#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../src/vklite2_utils.h"

#include "utils.h"


#define N_FRAMES 10

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

static int vklite2_canvas_1(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

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
    vkl_event_key(canvas, VKL_KEY_PRESS, VKL_KEY_A);

    // Second canvas.
    log_debug("global clock elapsed %.6f interval %.6f", app->clock.elapsed, app->clock.interval);
    log_debug(
        "local clock elapsed %.6f interval %.6f", canvas->clock.elapsed, canvas->clock.interval);

    // Second canvas.
    VklCanvas* canvas2 = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
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

static int vklite2_canvas_2(VkyTestContext* context)
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
    visual->renderpass = &canvas->renderpasses[0];
    visual->framebuffers = &canvas->framebuffers;
    test_triangle(visual, suffix);
    canvas->user_data = visual;
}

static void _triangle_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);

    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpasses[0], &canvas->framebuffers);
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

// Triangle canvas
static int vklite2_canvas_3(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &visual);

    vkl_app_run(app, N_FRAMES);

    vkl_graphics_destroy(&visual.graphics);
    destroy_visual(&visual);
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
    vkl_cmd_push_constants(
        cmds, idx, visual->graphics.slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vec3), push_vec);

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

static int vklite2_canvas_4(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpasses[0];
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "_push");
    canvas->user_data = &visual;

    // Create the slots.
    visual.slots = vkl_slots(gpu);
    vkl_slots_push_constant(&visual.slots, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);
    vkl_slots_create(&visual.slots);
    vkl_graphics_slots(&visual.graphics, &visual.slots);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.slots);
    vkl_bindings_create(&visual.bindings, 1);
    vkl_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual.graphics);

    // Triangle buffer.
    visual.buffer = vkl_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(VklVertex);
    vkl_buffer_size(&visual.buffer, size);
    vkl_buffer_usage(&visual.buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkl_buffer_memory(
        &visual.buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(&visual.buffer);

    // Upload the triangle data.
    VklVertex data[3] = {
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
    VklVertex* data = (VklVertex*)visual->data;
    for (uint32_t i = 0; i < 3; i++)
    {
        data[i].color[0] = x;
        data[i].color[1] = y;
        data[i].color[2] = 1;
    }
    vkl_buffer_regions_upload(canvas->gpu->context, &visual->br, 0, 3 * sizeof(VklVertex), data);
}

static int vklite2_canvas_5(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpasses[0];
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "");
    canvas->user_data = &visual;
    visual.data = calloc(3, sizeof(VklVertex));

    // Create the slots.
    visual.slots = vkl_slots(gpu);
    vkl_slots_create(&visual.slots);
    vkl_graphics_slots(&visual.graphics, &visual.slots);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.slots);
    vkl_bindings_create(&visual.bindings, 1);
    vkl_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(VklVertex);
    visual.br = vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    VklVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    vkl_buffer_regions_upload(canvas->gpu->context, &visual.br, 0, size, data);

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
    vkl_buffer_regions_upload(canvas->gpu->context, &visual->br_u, 0, sizeof(vec4), vec);
}

static int vklite2_canvas_6(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);
    uint32_t img_count = canvas->swapchain.img_count;
    ASSERT(img_count > 0);

    TestVisual visual = {0};
    visual.gpu = canvas->gpu;
    visual.renderpass = &canvas->renderpasses[0];
    visual.framebuffers = &canvas->framebuffers;
    _triangle_graphics(&visual, "_ubo");
    canvas->user_data = &visual;
    visual.data = calloc(3, sizeof(VklVertex));

    // Create the slots.
    visual.slots = vkl_slots(gpu);
    vkl_slots_binding(&visual.slots, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkl_slots_create(&visual.slots);
    vkl_graphics_slots(&visual.graphics, &visual.slots);

    // Uniform buffer.
    visual.br_u =
        vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, img_count, sizeof(vec4));
    ASSERT(visual.br_u.aligned_size >= visual.br_u.size);
    vkl_buffer_regions_upload(canvas->gpu->context, &visual.br_u, 0, sizeof(vec4), vec);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.slots);
    ASSERT(visual.br_u.buffer != VK_NULL_HANDLE);
    vkl_bindings_create(&visual.bindings, img_count);
    vkl_bindings_buffer(&visual.bindings, 0, &visual.br_u);
    vkl_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(VklVertex);
    visual.br = vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    VklVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(visual.data, data, sizeof(data));
    vkl_buffer_regions_upload(canvas->gpu->context, &visual.br, 0, size, data);

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

static int vklite2_canvas_7(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    canvas->user_data = &visual;

    // Create compute object.
    VklSlots slots = vkl_slots(gpu);
    VklBindings bindings = vkl_bindings(&slots);
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/spirv/test_triangle.comp.spv", DATA_DIR);
        visual.compute = vkl_new_compute(gpu->context, path);
        vkl_slots_binding(&slots, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vkl_slots_create(&slots);
        vkl_compute_slots(visual.compute, &slots);
        vkl_bindings_create(&bindings, 1);
        vkl_bindings_buffer(&bindings, 0, &visual.br);
        vkl_bindings_update(&bindings);
        vkl_compute_bindings(visual.compute, &bindings);
        vkl_compute_create(visual.compute);
    }

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_compute_refill, &visual);

    vkl_app_run(app, N_FRAMES);

    vkl_graphics_destroy(&visual.graphics);
    vkl_bindings_destroy(&bindings);
    vkl_slots_destroy(&slots);
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

static int vklite2_canvas_8(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    _make_triangle2(canvas, &visual, "");
    canvas->user_data = &visual;

    // Create compute object.
    VklSlots slots = vkl_slots(gpu);
    VklBindings bindings = vkl_bindings(&slots);
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/spirv/test_triangle.comp.spv", DATA_DIR);
        visual.compute = vkl_new_compute(gpu->context, path);
        vkl_slots_binding(&slots, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vkl_slots_create(&slots);
        vkl_compute_slots(visual.compute, &slots);
        vkl_bindings_create(&bindings, 1);
        vkl_bindings_buffer(&bindings, 0, &visual.br);
        vkl_bindings_update(&bindings);
        vkl_compute_bindings(visual.compute, &bindings);
        vkl_compute_create(visual.compute);
    }

    INSTANCE_NEW(VklSemaphores, compute_finished, canvas->semaphores, canvas->max_semaphores)
    *compute_finished = vkl_semaphores(gpu, 2);

    INSTANCE_NEW(VklCommands, cmds, canvas->commands, canvas->max_commands)
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
    vkl_slots_destroy(&slots);
    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas with particles                                                                        */
/*************************************************************************************************/

static void _particle_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);

    TestVisual* visual = (TestVisual*)ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;
    vkl_cmd_begin(cmds, idx);

    vkl_cmd_compute(cmds, idx, visual->compute, (uvec3){visual->n_vertices, 1, 1});

    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpasses[0], &canvas->framebuffers);
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

static int vklite2_canvas_particles(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    // Sync mode only to set things up.
    vkl_transfer_mode(canvas->gpu->context, VKL_TRANSFER_MODE_SYNC);

    TestVisual* visual = calloc(1, sizeof(TestVisual));
    visual->gpu = canvas->gpu;
    visual->renderpass = &canvas->renderpasses[0];
    visual->framebuffers = &canvas->framebuffers;

    visual->graphics = vkl_graphics(gpu);
    ASSERT(visual->renderpass != NULL);
    VklGraphics* graphics = &visual->graphics;

    vkl_graphics_renderpass(graphics, visual->renderpass, 0);
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/test_marker.vert.spv", DATA_DIR);
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/spirv/test_marker.frag.spv", DATA_DIR);
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    vkl_graphics_vertex_binding(graphics, 0, sizeof(TestParticle));
    vkl_graphics_vertex_attr(
        graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestParticle, pos));
    vkl_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestParticle, vel));
    vkl_graphics_vertex_attr(
        graphics, 0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestParticle, color));

    // Create the buffer.
    const uint32_t n = 10000;
    visual->n_vertices = n;
    VkDeviceSize size = n * sizeof(TestParticle);
    visual->br = vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    visual->data = calloc(n, sizeof(TestParticle));
    for (uint32_t i = 0; i < n; i++)
    {
        ((TestParticle*)visual->data)[i].pos[0] = .25 * randn();
        ((TestParticle*)visual->data)[i].pos[1] = .25 * randn();
        ((TestParticle*)visual->data)[i].vel[0] = .25 * randn();
        ((TestParticle*)visual->data)[i].vel[1] = .25 * randn();
        ((TestParticle*)visual->data)[i].color[0] = rand_float();
        ((TestParticle*)visual->data)[i].color[1] = rand_float();
        ((TestParticle*)visual->data)[i].color[2] = rand_float();
        ((TestParticle*)visual->data)[i].color[3] = 1;
    }
    vkl_buffer_regions_upload(canvas->gpu->context, &visual->br, 0, size, visual->data);
    FREE(visual->data);

    // Create the slots.
    visual->slots = vkl_slots(gpu);
    vkl_slots_create(&visual->slots);
    vkl_graphics_slots(&visual->graphics, &visual->slots);

    // Create the bindings.vkl_slots_destroy(&slots);
    visual->bindings = vkl_bindings(&visual->slots);
    vkl_bindings_create(&visual->bindings, 1);
    vkl_bindings_update(&visual->bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual->graphics);



    // Create compute object.
    VklSlots slots = vkl_slots(gpu);
    VklBindings bindings = vkl_bindings(&slots);
    snprintf(path, sizeof(path), "%s/spirv/test_particle.comp.spv", DATA_DIR);
    visual->compute = vkl_new_compute(gpu->context, path);
    vkl_slots_binding(&slots, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_slots_create(&slots);
    vkl_compute_slots(visual->compute, &slots);
    vkl_bindings_create(&bindings, 1);
    vkl_bindings_buffer(&bindings, 0, &visual->br);
    vkl_bindings_update(&bindings);
    vkl_compute_bindings(visual->compute, &bindings);
    vkl_compute_create(visual->compute);

    INSTANCE_NEW(VklCommands, cmds, canvas->commands, canvas->max_commands)
    *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_COMPUTE, 1);
    vkl_cmd_begin(cmds, 0);
    vkl_cmd_compute(cmds, 0, visual->compute, (uvec3){n, 1, 1});
    vkl_cmd_end(cmds, 0);
    ASSERT(is_obj_created(&cmds->obj));



    canvas->user_data = visual;
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _particle_refill, visual);

    vkl_transfer_mode(canvas->gpu->context, VKL_TRANSFER_MODE_ASYNC);
    vkl_app_run(app, 0); // DEBUG: N_FRAMES

    vkl_slots_destroy(&slots);
    destroy_visual(visual);
    TEST_END
}
