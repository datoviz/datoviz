#include <visky/visky.h>

int main()
{
    log_set_level_env();
    const uint32_t n = 20;

    // Create the interface to the GPU.
    VkyGpu gpu = vky_create_device(0, NULL);

    // Initialize the GPU properly without a surface (NULL pointer).
    vky_prepare_gpu(&gpu, NULL);

    // Create the storage buffer with the initial numbers, and that will be modified in-place
    // by the compute shader.
    VkDeviceSize size = n * sizeof(float);
    VkyBuffer buffer = vky_create_buffer(
        &gpu, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkyBufferRegion buffer_region = vky_allocate_buffer(&buffer, size);

    // Fill the buffer with numbers.
    float numbers[n];
    printf("Input : ");
    for (uint32_t i = 0; i < n; i++)
    {
        numbers[i] = (float)i + 1;
        printf("%2.0f ", numbers[i]);
    }
    printf("\n");
    vky_upload_buffer(buffer_region, 0, size, numbers);

    // Create the resource layout for the compute pipeline: we pass the buffer as a storage buffer
    // to the shader.
    VkyResourceLayout resource_layout = vky_create_resource_layout(&gpu, 1);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the pipeline.
    VkyComputePipeline pipeline =
        vky_create_compute_pipeline(&gpu, "pow2.comp.spv", resource_layout);

    // Bind the storage buffer to the pipeline..
    VkyBufferRegion* resources[] = {&buffer_region};
    vky_bind_resources(pipeline.resource_layout, pipeline.descriptor_sets, (void**)resources);

    // Create the compute command buffer making the computation on the GPU using the filled storage
    // buffer.
    vky_begin_compute(&gpu);
    vky_compute(&pipeline, n, 1, 1);
    vky_end_compute(
        &gpu, 0, NULL,
        NULL); // 0 and NULL as we don't need to synchronize with the graphics queue.

    // Submit the compute command buffer to the GPU and wait for its completion.
    vky_compute_submit(&gpu);
    vky_compute_wait(&gpu);

    // Copy the modified buffer back to the CPU, overwriting the original `numbers` array.
    vky_download_buffer(&buffer_region, numbers);

    // Display the results.
    printf("Output: ");
    for (uint32_t i = 0; i < n; i++)
    {
        printf("%2.0f ", numbers[i]);
    }
    printf("\n");

    // Destroy the resources.
    vky_destroy_compute_pipeline(&pipeline);
    vky_destroy_buffer(&buffer);
    vky_destroy_device(&gpu);

    return 0;
}
