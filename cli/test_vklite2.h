static int vklite2_test_app_1(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    ASSERT(app->obj.status == VKL_OBJECT_STATUS_CREATED);
    ASSERT(app->gpus != NULL);
    ASSERT(app->gpu_count >= 1);
    ASSERT(app->gpus[0].name != NULL);
    ASSERT(app->gpus[0].obj.status == VKL_OBJECT_STATUS_INIT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_create(gpu, 0);

    vkl_app_destroy(app);
    return 0;
}

static int vklite2_test_compute_only(VkyTestContext* context)
{
    // VkyApp* app = vky_app();
    // VkyCompute* compute = vky_compute(app->gpu, "compute.spv");
    // VkyBuffer* buffer =
    // VkyCommands* commands = vky_commands(app->gpu, VKY_COMMAND_COMPUTE);
    // vky_cmd_begin(commands);
    // vky_cmd_compute(commands, compute, uvec3 size);
    // vky_cmd_end(commands);
    // VkySubmit* sub = vky_submit(app-> gpu, VKY_QUEUE_COMPUTE);
    // vky_submit_send(sub, NULL);
    // vky_app_destroy(app);
    return 0;
}

static int vklite2_test_offscreen(VkyTestContext* context) { return 0; }

static int vklite2_test_offscreen_gui(VkyTestContext* context) { return 0; }

static int vklite2_test_offscreen_compute(VkyTestContext* context) { return 0; }
