#include <visky/vklite.h>

static int test_vklite_compute()
{
    const uint32_t n = 20;

    VkyGpu gpu = vky_create_device(0, NULL);
    vky_prepare_gpu(&gpu, NULL);

    VkDeviceSize size = n * sizeof(float);
    VkyBuffer buffer = vky_create_buffer(
        &gpu, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkyBufferRegion buffer_region = vky_allocate_buffer(&buffer, size);

    float numbers[n];
    for (uint32_t i = 0; i < n; i++)
    {
        numbers[i] = (float)i + 1;
    }
    vky_upload_buffer(buffer_region, 0, size, numbers);

    VkyResourceLayout resource_layout = vky_create_resource_layout(&gpu, 1);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    VkyComputePipeline pipeline =
        vky_create_compute_pipeline(&gpu, "pow2.comp.spv", resource_layout);

    VkyBufferRegion* resources[] = {&buffer_region};
    vky_bind_resources(pipeline.resource_layout, pipeline.descriptor_sets, (void**)resources);

    vky_begin_compute(&gpu);
    vky_compute(&pipeline, n, 1, 1);
    vky_end_compute(&gpu, 0, NULL, NULL);
    vky_compute_submit(&gpu);
    vky_compute_wait(&gpu);

    vky_download_buffer(&buffer_region, numbers);

    for (uint32_t i = 0; i < n; i++)
    {
        if (numbers[i] != 2 * (i + 1))
            return 1;
    }

    vky_destroy_compute_pipeline(&pipeline);
    vky_destroy_buffer(&buffer);
    vky_destroy_device(&gpu);

    return 0;
}



static void fcb(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);
    vky_end_render_pass(cmd_buf, canvas);
}

static int test_vklite_blank(VkyCanvas* canvas)
{
    // log_set_level_env();
    canvas->cb_fill_command_buffer = fcb;
    // vky_run_app(canvas->app);
    // vky_destroy_app(app);
    return 0;
}
