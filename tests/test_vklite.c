/*************************************************************************************************/
/*  Testing vklite                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_vklite.h"
#include "../src/render_utils.h"
#include "../src/vklite_utils.h"
#include "fileio.h"
#include "resources.h"
#include "surface.h"
#include "test.h"
#include "test_gui.h"
#include "testing.h"
#include "testing_utils.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _make_buffer(DvzBuffer* buffer, VkDeviceSize size, VkBufferUsageFlags usage)
{
    dvz_buffer_size(buffer, size);
    dvz_buffer_usage(buffer, usage);
    // dvz_buffer_memory(
    //     buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_vma_usage(buffer, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_buffer_queue_access(buffer, 0);
    dvz_buffer_create(buffer);
}



static inline void _make_vertex_buffer(DvzBuffer* buffer, VkDeviceSize size)
{
    _make_buffer(
        buffer, size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}



static inline void _make_index_buffer(DvzBuffer* buffer, VkDeviceSize size)
{
    _make_buffer(
        buffer, size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}



static void _save_screenshot(DvzFramebuffers* framebuffers, const char* name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", ARTIFACTS_DIR, name);
    log_info("saving screenshot to %s", path);
    DvzImages* images = framebuffers->attachments[0];
    uint8_t* rgb = (uint8_t*)screenshot(images, 1);
    dvz_write_ppm(path, images->shape[0], images->shape[1], rgb);
    FREE(rgb);
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_host(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);

    AT(host->obj.status == DVZ_OBJECT_STATUS_CREATED);
    AT(host->gpus.count >= 1);
    AT(((DvzGpu*)(host->gpus.items[0]))->name != NULL);
    AT(((DvzGpu*)(host->gpus.items[0]))->obj.status == DVZ_OBJECT_STATUS_INIT);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_TRANSFER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_GRAPHICS | DVZ_QUEUE_COMPUTE);
    dvz_gpu_queue(gpu, 2, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    gpu = dvz_gpu_best(host);
    ANN(gpu);
    log_info("Best GPU is %s with %s VRAM", gpu->name, pretty_size(gpu->vram));
    ANN(gpu->name);

    dvz_host_destroy(host);
    return 0;
}



int test_vklite_commands(TstSuite* suite)
{
    ANN(suite);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);
    DvzCommands cmds = dvz_commands(gpu, 0, 3);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_reset(&cmds, 0);
    dvz_cmd_free(&cmds);

    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_buffer_1(TstSuite* suite)
{
    ANN(suite);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzBuffer buffer = dvz_buffer(gpu);
    const VkDeviceSize size = 256;
    _make_buffer(
        &buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    // Send some data to the GPU.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);

    // Recover the data.
    void* data2 = calloc(size, 1);
    dvz_buffer_download(&buffer, 0, size, data2);
    dvz_queue_wait(gpu, 0);

    // Check that the data downloaded from the GPU is the same.
    AT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);

    dvz_buffer_destroy(&buffer);

    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_buffer_resize(TstSuite* suite)
{
    ANN(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzBuffer buffer = dvz_buffer(gpu);
    const VkDeviceSize size = 256;
    _make_buffer(
        &buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    // Map the buffer.
    buffer.mmap = dvz_buffer_map(&buffer, 0, VK_WHOLE_SIZE);

    // Send some data to the GPU.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);
    ANN(buffer.mmap);
    void* old_mmap = buffer.mmap;

    // Resize the buffer.
    // DvzCommands cmds = dvz_commands(gpu, 0, 1);
    // NOTE: this should automatically unmap, delete, create, remap, copy old data to new.
    dvz_buffer_resize(&buffer, 2 * size);
    ASSERT(buffer.size == 2 * size);
    ANN(buffer.mmap);
    ASSERT(buffer.mmap != old_mmap);

    // Recover the data.
    void* data2 = calloc(size, 1);
    dvz_buffer_download(&buffer, 0, size, data2);
    dvz_queue_wait(gpu, 0);

    // Check that the data downloaded from the GPU is the same.
    AT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);

    // Unmap the buffer.
    dvz_buffer_unmap(&buffer);
    buffer.mmap = NULL;
    dvz_buffer_destroy(&buffer);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_load_shader(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    ANN(host);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create a shader module.
    char shader_path[1024] = {0};
    snprintf(shader_path, sizeof(shader_path), "%s/test_pow.comp.spv", SPIRV_DIR);
    VkShaderModule module = create_shader_module_from_file(gpu->device, shader_path);

    // Destroy the shader module.
    vkDestroyShaderModule(gpu->device, module, NULL);

    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_compute(TstSuite* suite)
{
    ANN(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_double.comp.spv", SPIRV_DIR);
    DvzCompute compute = dvz_compute(gpu, path);


    // Create the buffers
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    _make_buffer(
        &buffer, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);

    // Create the slots.
    dvz_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&compute.slots, 1);
    DvzBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings, 0, br);
    dvz_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    dvz_compute_bindings(&compute, &bindings);
    dvz_compute_create(&compute);

    // Command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    dvz_buffer_download(&buffer, 0, size, data2);
    dvz_queue_wait(gpu, 0);
    for (uint32_t i = 0; i < n; i++)
        AT(data2[i] == 2 * data[i]);
    FREE(data);
    FREE(data2);

    dvz_bindings_destroy(&bindings);
    dvz_compute_destroy(&compute);
    dvz_buffer_destroy(&buffer);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_push(TstSuite* suite)
{
    ANN(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_pow.comp.spv", SPIRV_DIR);
    DvzCompute compute = dvz_compute(gpu, path);

    // Create the buffers
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    _make_buffer(
        &buffer, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);

    // Create the slots.
    dvz_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    dvz_compute_push(&compute, 0, sizeof(float), VK_SHADER_STAGE_COMPUTE_BIT);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&compute.slots, 1);
    DvzBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings, 0, br);
    dvz_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    dvz_compute_bindings(&compute, &bindings);
    dvz_compute_create(&compute);

    // Command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    float power = 2.0f;
    dvz_cmd_push(&cmds, 0, &compute.slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &power);
    dvz_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    dvz_buffer_download(&buffer, 0, size, data2);
    dvz_queue_wait(gpu, 0);
    for (uint32_t i = 0; i < n; i++)
        AT(fabs(data2[i] - pow(data[i], power)) < .01);
    FREE(data);
    FREE(data2);

    dvz_bindings_destroy(&bindings);
    dvz_compute_destroy(&compute);
    dvz_buffer_destroy(&buffer);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_images(TstSuite* suite)
{
    ANN(suite);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzImages images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UINT);
    dvz_images_size(&images, (uvec3){16, 16, 1});
    dvz_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(&images, VK_IMAGE_USAGE_STORAGE_BIT);
    dvz_images_memory(&images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_queue_access(&images, 0);
    dvz_images_create(&images);

    dvz_images_destroy(&images);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_sampler(TstSuite* suite)
{
    ANN(suite);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzSampler sampler = dvz_sampler(gpu);
    dvz_sampler_min_filter(&sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(&sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(&sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_create(&sampler);

    dvz_sampler_destroy(&sampler);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_barrier_buffer(TstSuite* suite)
{
    ANN(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Buffers.
    DvzBuffer buffer0 = dvz_buffer(gpu);
    DvzBuffer buffer1 = dvz_buffer(gpu);
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    VkDeviceSize bsize = 256 * sizeof(float);
    _make_buffer(&buffer0, bsize, usage);
    _make_buffer(&buffer1, bsize, usage);

    const uint32_t N = 20;
    const VkDeviceSize size = N * sizeof(float);

    // Send some data to the buffer.
    float* data0 = calloc(size, 1);
    for (uint32_t i = 0; i < N; i++)
        data0[i] = (float)i;
    VkDeviceSize offset = 0;
    dvz_buffer_upload(&buffer0, offset, size, data0);
    dvz_buffer_upload(&buffer1, offset, size, data0);
    dvz_queue_wait(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_double.comp.spv", SPIRV_DIR);
    DvzCompute compute = dvz_compute(gpu, path);

    // Create the slots.
    dvz_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&compute.slots, 1);
    DvzBufferRegions br = {.buffer = &buffer0, .size = size, .count = 1};
    br.offsets[0] = offset;
    dvz_bindings_buffer(&bindings, 0, br);
    dvz_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    dvz_compute_bindings(&compute, &bindings);
    dvz_compute_create(&compute);

    // Barrier.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_buffer(&barrier, br);
    dvz_barrier_buffer_queue(&barrier, 0, 0);

    // Command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_compute(&cmds, 0, &compute, (uvec3){N, 1, 1});
    dvz_barrier_stages(
        &barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_buffer_access(&barrier, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);
    dvz_cmd_copy_buffer(&cmds, 0, &buffer0, offset, &buffer1, offset, size);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data1 = calloc(size, 1);
    dvz_buffer_download(&buffer1, offset, size, data1);
    dvz_queue_wait(gpu, 0);
    for (uint32_t i = 0; i < N; i++)
        AT(data1[i] == 2 * data0[i]);

    FREE(data0);
    FREE(data1);

    dvz_bindings_destroy(&bindings);
    dvz_compute_destroy(&compute);

    dvz_buffer_destroy(&buffer0);
    dvz_buffer_destroy(&buffer1);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_barrier_image(TstSuite* suite)
{
    ANN(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Image.
    const uint32_t img_size = 16;
    DvzImages images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UINT);
    dvz_images_size(&images, (uvec3){img_size, img_size, 1});
    dvz_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(&images, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_memory(&images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_queue_access(&images, 0);
    dvz_images_create(&images);

    // Staging buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    const VkDeviceSize size = (size_t)(img_size * img_size * 4);
    _make_buffer(&buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    // Send some data to the staging buffer.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i % 256;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);
    FREE(data);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, &images);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);

    // Transfer the data from the staging buffer to the image.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // NOTE: raise warning
    dvz_cmd_copy_buffer_to_image(
        &cmds, 0, &buffer, 0, &images, (uvec3){0}, (uvec3){img_size, img_size, 1});

    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    dvz_buffer_destroy(&buffer);
    dvz_images_destroy(&images);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_submit(TstSuite* suite)
{
    ANN(suite);
    // DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    // Use the host setup fixture.
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_COMPUTE);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_double.comp.spv", SPIRV_DIR);
    DvzCompute compute1 = dvz_compute(gpu, path);

    snprintf(path, sizeof(path), "%s/test_sum.comp.spv", SPIRV_DIR);
    DvzCompute compute2 = dvz_compute(gpu, path);

    // Create the buffer
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    // dvz_buffer_memory(
    //     &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_vma_usage(&buffer, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_queue_access(&buffer, 1);
    dvz_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);

    // Create the slots.
    dvz_compute_slot(&compute1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    dvz_compute_slot(&compute2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    DvzBindings bindings1 = dvz_bindings(&compute1.slots, 1);
    DvzBufferRegions br1 = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings1, 0, br1);
    dvz_bindings_update(&bindings1);

    DvzBindings bindings2 = dvz_bindings(&compute2.slots, 1);
    DvzBufferRegions br2 = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings2, 0, br2);
    dvz_bindings_update(&bindings2);

    // Link the bindings1 to the compute1 pipeline and create it.
    dvz_compute_bindings(&compute1, &bindings1);
    dvz_compute_create(&compute1);

    // Link the bindings1 to the compute2 pipeline and create it.
    dvz_compute_bindings(&compute2, &bindings2);
    dvz_compute_create(&compute2);

    // Command buffers.
    DvzCommands cmds1 = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds1, 0);
    dvz_cmd_compute(&cmds1, 0, &compute1, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds1, 0);

    DvzCommands cmds2 = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds2, 0);
    dvz_cmd_compute(&cmds2, 0, &compute2, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds2, 0);

    // Semaphores
    DvzSemaphores semaphores = dvz_semaphores(gpu, 1);

    // Submit.
    DvzSubmit submit1 = dvz_submit(gpu);
    dvz_submit_commands(&submit1, &cmds1);
    dvz_submit_signal_semaphores(&submit1, &semaphores, 0);
    dvz_submit_send(&submit1, 0, NULL, 0);

    DvzSubmit submit2 = dvz_submit(gpu);
    dvz_submit_commands(&submit2, &cmds2);
    dvz_submit_wait_semaphores(&submit2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, &semaphores, 0);
    dvz_submit_send(&submit2, 0, NULL, 0);

    dvz_gpu_wait(gpu);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    dvz_buffer_download(&buffer, 0, size, data2);
    dvz_queue_wait(gpu, 0);
    for (uint32_t i = 0; i < n; i++)
        AT(data2[i] == 2 * i + 1);


    dvz_semaphores_destroy(&semaphores);
    dvz_bindings_destroy(&bindings1);
    dvz_bindings_destroy(&bindings2);
    dvz_buffer_destroy(&buffer);
    dvz_compute_destroy(&compute1);
    dvz_compute_destroy(&compute2);

    FREE(data);
    FREE(data2);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_offscreen(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    TestCanvas canvas = offscreen_canvas(gpu);
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    blank_commands(&canvas.renderpass, &canvas.framebuffers, &cmds, 0, NULL);
    dvz_cmd_submit_sync(&cmds, 0);

    uint8_t* rgb = screenshot(framebuffers->attachments[0], 1);
    for (uint32_t i = 0; i < WIDTH * HEIGHT * 3; i++)
        AT(rgb[i] >= 100);

    FREE(rgb);

    canvas_destroy(&canvas);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}



int test_vklite_shader(TstSuite* suite)
{
    ANN(suite);
#if HAS_GLSLANG
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, VK_NULL_HANDLE);

    VkShaderModule module = dvz_shader_compile(
        gpu,
        "#version 450\n"
        "layout (location = 0) in vec3 pos;\n"
        "layout (location = 1) in vec4 color;\n"
        "layout (location = 0) out vec4 out_color;\n"
        "void main() {\n"
        "    gl_Position = vec4(pos, 1.0);\n"
        "    out_color = color;\n"
        "}",
        VK_SHADER_STAGE_VERTEX_BIT);
    vkDestroyShaderModule(gpu->device, module, NULL);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
#else
    log_warn("skip shader compilation test as the library was not compiled with glslc support");
    return 0;
#endif
}



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_vklite_graphics(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create an offscreen canvas.
    TestCanvas canvas = offscreen_canvas(gpu);
    DvzRenderpass* renderpass = &canvas.renderpass;
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    // Make the graphics.
    DvzGraphics graphics = triangle_graphics(gpu, renderpass);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_update(&bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&graphics);

    // Create the buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    _make_vertex_buffer(&buffer, size);

    // Upload the triangle data.
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0); // DVZ_DEFAULT_QUEUE_TRANSFER

    // Create and submit the command buffer.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    DvzBufferRegions br = dvz_buffer_regions(&buffer, 1, 0, buffer.size, 0);
    triangle_commands(&cmds, 0, renderpass, framebuffers, &graphics, &bindings, br);
    dvz_cmd_submit_sync(&cmds, 0);

    // Save a screenshot.
    _save_screenshot(framebuffers, "vklite_screenshot");

    // Cleanup.
    dvz_graphics_destroy(&graphics);
    dvz_bindings_destroy(&bindings);
    dvz_buffer_destroy(&buffer);
    canvas_destroy(&canvas);
    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_indirect(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create an offscreen canvas.
    TestCanvas canvas = offscreen_canvas(gpu);
    DvzRenderpass* renderpass = &canvas.renderpass;
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    // Make the graphics.
    DvzGraphics graphics = triangle_graphics(gpu, renderpass);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_update(&bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&graphics);


    // Create the vertex buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    _make_vertex_buffer(&buffer, size);

    // Upload the quad data.
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0);


    // Create the indirect buffer.
    DvzBuffer indirect = dvz_buffer(gpu);
    VkDeviceSize size_indirect = 1 * sizeof(VkDrawIndirectCommand);
    _make_buffer(
        &indirect, size,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkDrawIndirectCommand data_indirect = {
        .firstInstance = 0, .firstVertex = 0, .instanceCount = 1, .vertexCount = 3};
    dvz_buffer_upload(&indirect, 0, size_indirect, &data_indirect);
    dvz_queue_wait(gpu, 0);


    // Create and submit the command buffer.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    DvzBufferRegions br = dvz_buffer_regions(&buffer, 1, 0, size, 0);
    DvzBufferRegions br_indirect = dvz_buffer_regions(&indirect, 1, 0, size_indirect, 0);

    // Commands.
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_begin_renderpass(&cmds, 0, renderpass, framebuffers);
    dvz_cmd_viewport(&cmds, 0, (VkViewport){0, 0, WIDTH, HEIGHT, 0, 1});
    dvz_cmd_bind_vertex_buffer(&cmds, 0, 1, (DvzBufferRegions[]){br}, (DvzSize[]){0});
    dvz_cmd_bind_graphics(&cmds, 0, &graphics, &bindings, 0);
    dvz_cmd_draw_indirect(&cmds, 0, br_indirect, 1);
    dvz_cmd_end_renderpass(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Save a screenshot.
    _save_screenshot(framebuffers, "vklite_indirect");

    // Cleanup.
    dvz_graphics_destroy(&graphics);
    dvz_bindings_destroy(&bindings);
    dvz_buffer_destroy(&buffer);
    dvz_buffer_destroy(&indirect);
    canvas_destroy(&canvas);
    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_indexed(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create an offscreen canvas.
    TestCanvas canvas = offscreen_canvas(gpu);
    DvzRenderpass* renderpass = &canvas.renderpass;
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    // Make the graphics.
    DvzGraphics graphics = triangle_graphics(gpu, renderpass);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_update(&bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&graphics);


    // Create the vertex buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    VkDeviceSize size = 4 * sizeof(TestVertex);
    _make_vertex_buffer(&buffer, size);

    // Upload the quad data.
    TestVertex data[] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{-1, -1, 0}, {0, 0, 1, 1}},
        {{+1, -1, 0}, {1, 1, 0, 1}},
    };
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0); // DVZ_DEFAULT_QUEUE_TRANSFER


    // Create the index buffer.
    DvzBuffer buffer_index = dvz_buffer(gpu);
    VkDeviceSize size_index = 6 * sizeof(uint32_t);
    _make_index_buffer(&buffer_index, size_index);

    // Upload the index data.
    uint32_t indices[] = {0, 1, 2, 3, 2, 1};
    dvz_buffer_upload(&buffer_index, 0, size_index, indices);
    dvz_queue_wait(gpu, 0); // DVZ_DEFAULT_QUEUE_TRANSFER


    // Create and submit the command buffer.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    DvzBufferRegions br = dvz_buffer_regions(&buffer, 1, 0, size, 0);
    DvzBufferRegions bri = dvz_buffer_regions(&buffer_index, 1, 0, size_index, 0);

    uint32_t n_vertices = 6;

    // Commands.
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_begin_renderpass(&cmds, 0, renderpass, framebuffers);
    dvz_cmd_viewport(&cmds, 0, (VkViewport){0, 0, WIDTH, HEIGHT, 0, 1});
    dvz_cmd_bind_vertex_buffer(&cmds, 0, 1, (DvzBufferRegions[]){br}, (DvzSize[]){0});
    dvz_cmd_bind_index_buffer(&cmds, 0, bri, 0);
    dvz_cmd_bind_graphics(&cmds, 0, &graphics, &bindings, 0);
    dvz_cmd_draw_indexed(&cmds, 0, 0, 0, n_vertices, 0, 1);
    dvz_cmd_end_renderpass(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Save a screenshot.
    _save_screenshot(framebuffers, "vklite_indexed");

    // Cleanup.
    dvz_graphics_destroy(&graphics);
    dvz_bindings_destroy(&bindings);
    dvz_buffer_destroy(&buffer);
    dvz_buffer_destroy(&buffer_index);
    canvas_destroy(&canvas);
    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_instanced(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create an offscreen canvas.
    TestCanvas canvas = offscreen_canvas(gpu);
    DvzRenderpass* renderpass = &canvas.renderpass;
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    // Make the graphics.
    DvzGraphics graphics = dvz_graphics(gpu);

    dvz_graphics_renderpass(&graphics, renderpass, 0);
    dvz_graphics_topology(&graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(&graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(&graphics, DVZ_DEPTH_TEST_ENABLE);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_triangle_instanced.vert.spv", SPIRV_DIR);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle.frag.spv", SPIRV_DIR);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(&graphics, 0, sizeof(TestVertex), DVZ_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        &graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        &graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));


    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_update(&bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&graphics);

    // Create the buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n_vertices = 3;
    const uint32_t n_instances = 7;
    VkDeviceSize size = n_vertices * sizeof(TestVertex);
    _make_vertex_buffer(&buffer, size);

    // Upload the triangle data.
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_buffer_upload(&buffer, 0, size, data);
    dvz_queue_wait(gpu, 0); // DVZ_DEFAULT_QUEUE_TRANSFER

    // Create and submit the command buffer.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    DvzBufferRegions br = dvz_buffer_regions(&buffer, 1, 0, buffer.size, 0);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_begin_renderpass(&cmds, 0, renderpass, framebuffers);
    dvz_cmd_viewport(&cmds, 0, (VkViewport){0, 0, (float)WIDTH, (float)HEIGHT, 0, 1});
    dvz_cmd_bind_vertex_buffer(&cmds, 0, 1, (DvzBufferRegions[]){br}, (DvzSize[]){0});
    dvz_cmd_bind_graphics(&cmds, 0, &graphics, &bindings, 0);
    dvz_cmd_draw(&cmds, 0, 0, n_vertices, 0, n_instances);
    dvz_cmd_end_renderpass(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Save a screenshot.
    _save_screenshot(framebuffers, "vklite_instanced");

    // Cleanup.
    dvz_graphics_destroy(&graphics);
    dvz_bindings_destroy(&bindings);
    dvz_buffer_destroy(&buffer);
    canvas_destroy(&canvas);
    dvz_gpu_destroy(gpu);
    return 0;
}



/*************************************************************************************************/
/*  Tests with window                                                                            */
/*************************************************************************************************/

int test_vklite_surface(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    // OFFSCREEN_SKIP
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_ALL);

    // Create a GLFW window and surface.
    VkSurfaceKHR surface = 0;
    void* window = NULL;
#if HAS_GLFW
    GLFWwindow* bwin = (GLFWwindow*)backend_window(host->backend, 100, 100, 0);
    glfwCreateWindowSurface(host->instance, bwin, NULL, &surface);
    ANN(bwin);
    ASSERT(surface != VK_NULL_HANDLE);
    window = (void*)bwin;
#endif
    dvz_gpu_create(gpu, surface);

    if (window)
        backend_window_destroy(host->backend, (void*)window);

    dvz_gpu_destroy(gpu);
    return 0;
}



int test_vklite_swapchain(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    // OFFSCREEN_SKIP
    DvzWindow window = dvz_window(host->backend, 100, 100, 0);
    DvzSurface surface = dvz_window_surface(host, &window);
    AT(surface.surface != VK_NULL_HANDLE);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_PRESENT);
    dvz_gpu_create(gpu, surface.surface);

    DvzSwapchain swapchain = dvz_swapchain(gpu, surface.surface, 3);
    dvz_swapchain_format(&swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    dvz_swapchain_present_mode(&swapchain, VK_PRESENT_MODE_FIFO_KHR);
    dvz_swapchain_create(&swapchain);
    dvz_swapchain_destroy(&swapchain);
    dvz_surface_destroy(host, surface);
    dvz_window_destroy(&window);

    dvz_gpu_destroy(gpu);
    // dvz_host_destroy(host);
    return 0;
}
