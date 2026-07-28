/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* cuda_external_buffer - This example shares a Datoviz-owned Vulkan vertex buffer with CUDA.
 *
 * Scenario: advanced_cuda_external_buffer
 * Style: advanced, Linux/NVIDIA-only, external memory and timeline semaphore interop
 *
 * Datoviz creates the renderable buffer and exports opaque file descriptors for its memory and
 * timeline semaphore. CUDA imports those descriptors, writes triangle positions into the mapped
 * allocation, and signals the semaphore. Vulkan waits before DRP2 consumes the same allocation as
 * vertex input. The final red pixel proves that no CPU upload into a second Vulkan buffer
 * occurred.
 *
 * Build:  just example-c advanced/cuda_external_buffer
 * Run:    ./build/examples/c/advanced/cuda_external_buffer [vulkan_gpu_index]
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cuda_runtime_api.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/functions.h"
#include "datoviz/drp2.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define ID_EXTERNAL_VERTEX_BUFFER 1
#define ID_VERTEX_SHADER          2
#define ID_FRAGMENT_SHADER        3
#define ID_PIPELINE               4
#define ID_COLOR_TARGET           5
#define ID_READBACK_BUFFER        6
#define ID_ENCODER                10
#define ID_RENDER_PASS            11
#define ID_COMMAND_BUFFER         12
#define ID_SUBMIT                 13



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct
{
    float position[2];
} Vertex;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Report one CUDA failure.
 *
 * @param label operation label
 * @param result CUDA result
 * @return whether the result is a failure
 */
static bool _cuda_failed(const char* label, cudaError_t result)
{
    if (result == cudaSuccess)
        return false;
    dvz_fprintf(stderr, "%s failed: %s\n", label, cudaGetErrorString(result));
    return true;
}



/**
 * Return the CUDA device matching an exported Vulkan device UUID.
 *
 * @param export_desc Datoviz interop descriptor
 * @return CUDA device index, or -1 if no match exists
 */
static int _matching_cuda_device(const DvzInteropBufferExport* export_desc)
{
    if (export_desc == NULL || export_desc->device_uuid_valid == 0)
        return -1;

    int count = 0;
    if (_cuda_failed("cudaGetDeviceCount", cudaGetDeviceCount(&count)))
        return -1;
    for (int i = 0; i < count; i++)
    {
        struct cudaDeviceProp properties = {0};
        if (_cuda_failed("cudaGetDeviceProperties", cudaGetDeviceProperties(&properties, i)))
            continue;
        if (memcmp(properties.uuid.bytes, export_desc->device_uuid, VK_UUID_SIZE) == 0)
            return i;
    }
    return -1;
}



/**
 * Append and execute the DRP2 render/readback proof.
 *
 * @param device Datoviz device
 * @param allocator Datoviz allocator
 * @param external_buffer shared external vertex buffer
 * @param vertex_size logical vertex-buffer size
 * @return 0 on success
 */
static int _render_external_buffer(
    DvzDevice* device, DvzVma* allocator, DvzBuffer* external_buffer, uint64_t vertex_size)
{
    int out = 1;
    DvzDrp2Runtime* runtime = NULL;
    DvzDrp2CommandStream* stream = NULL;

    DvzDrp2RuntimeConfig runtime_config = dvz_drp2_runtime_vklite_config(device, allocator);
    runtime = dvz_drp2_runtime_vklite(&runtime_config);
    if (runtime == NULL)
    {
        dvz_fprintf(stderr, "DRP2 runtime creation failed\n");
        goto cleanup;
    }

    DvzDrp2ExternalBufferDesc external_desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
        .buffer = external_buffer,
        .size = vertex_size,
        .usage = DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    bool ok = dvz_drp2_runtime_register_external_buffer(
        runtime, ID_EXTERNAL_VERTEX_BUFFER, &external_desc);
    if (!ok)
    {
        dvz_fprintf(stderr, "external DRP2 buffer registration failed\n");
        goto cleanup;
    }

    uint32_t binding_stride = sizeof(Vertex);
    uint32_t binding_step = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
    uint32_t attr_binding = 0;
    uint32_t attr_location = 0;
    DvzFormat attr_format = DVZ_FORMAT_R32G32_SFLOAT;
    uint32_t attr_offset = 0;

    stream = dvz_drp2_stream();
    if (stream == NULL)
    {
        dvz_fprintf(stderr, "DRP2 stream creation failed\n");
        goto cleanup;
    }

    ok = dvz_drp2_stream_hello_renderer(stream, "cuda-external-buffer");
    ok = ok && dvz_drp2_stream_renderer_hello_reply(stream, "datoviz");
    ok = ok && dvz_drp2_stream_create_shader_module_format(
                   stream, ID_VERTEX_SHADER, "vertex", "glsl",
                   "#version 450\n"
                   "layout(location=0) in vec2 position;\n"
                   "void main(){gl_Position=vec4(position,0,1);}\n");
    ok = ok && dvz_drp2_stream_create_shader_module_format(
                   stream, ID_FRAGMENT_SHADER, "fragment", "glsl",
                   "#version 450\n"
                   "layout(location=0) out vec4 color;\n"
                   "void main(){color=vec4(1,0,0,1);}\n");

    DvzDrp2RenderPipelineDesc pipeline = dvz_drp2_render_pipeline_desc();
    pipeline.id = ID_PIPELINE;
    pipeline.vertex_shader_module_id = ID_VERTEX_SHADER;
    pipeline.fragment_shader_module_id = ID_FRAGMENT_SHADER;
    pipeline.vertex_buffer_slots = 1;
    pipeline.binding_count = 1;
    pipeline.binding_strides = &binding_stride;
    pipeline.binding_step_modes = &binding_step;
    pipeline.attr_count = 1;
    pipeline.attr_bindings = &attr_binding;
    pipeline.attr_locations = &attr_location;
    pipeline.attr_formats = &attr_format;
    pipeline.attr_offsets = &attr_offset;
    ok = ok && dvz_drp2_stream_create_render_pipeline(stream, &pipeline);
    ok = ok && dvz_drp2_stream_create_texture_2d_usage(
                   stream, ID_COLOR_TARGET, 2, 2,
                   DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC);
    ok = ok && dvz_drp2_stream_create_buffer(
                   stream, ID_READBACK_BUFFER, 4,
                   DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ);
    ok = ok && dvz_drp2_stream_begin_command_encoder(stream, ID_ENCODER);
    ok = ok && dvz_drp2_stream_begin_render_pass_clear(
                   stream, ID_RENDER_PASS, ID_ENCODER, ID_COLOR_TARGET, 0, 0, 0, 1);
    ok = ok && dvz_drp2_stream_set_pipeline(stream, ID_RENDER_PASS, ID_PIPELINE);
    ok = ok && dvz_drp2_stream_set_vertex_buffer(
                   stream, ID_RENDER_PASS, 0, ID_EXTERNAL_VERTEX_BUFFER, 0);
    ok = ok && dvz_drp2_stream_draw(stream, ID_RENDER_PASS, 3, 1, 0, 0);
    ok = ok && dvz_drp2_stream_end_render_pass(stream, ID_RENDER_PASS);
    ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                   stream, ID_ENCODER, ID_COLOR_TARGET, ID_READBACK_BUFFER, 0, 1, 1, 4, 1);
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, ID_ENCODER, ID_COMMAND_BUFFER);
    ok = ok && dvz_drp2_stream_queue_submit(stream, ID_COMMAND_BUFFER, ID_SUBMIT);
    if (!ok)
    {
        dvz_fprintf(stderr, "DRP2 command construction failed\n");
        goto cleanup;
    }

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    if (!result.ok)
    {
        dvz_fprintf(
            stderr, "DRP2 execution failed at command %u with code %d\n", result.command_index,
            result.code);
        goto cleanup;
    }

    uint8_t pixel[4] = {0};
    ok = dvz_drp2_runtime_download_buffer(runtime, ID_READBACK_BUFFER, 0, 4, pixel);
    if (!ok || pixel[0] != 255 || pixel[1] != 0 || pixel[2] != 0 || pixel[3] != 255)
    {
        dvz_fprintf(
            stderr, "unexpected render result: rgba(%u, %u, %u, %u)\n", pixel[0], pixel[1],
            pixel[2], pixel[3]);
        goto cleanup;
    }

    dvz_fprintf(
        stdout, "CUDA external buffer: OK (rgba=%u,%u,%u,%u)\n", pixel[0], pixel[1], pixel[2],
        pixel[3]);
    out = 0;

cleanup:
    if (stream != NULL)
        dvz_drp2_stream_destroy(stream);
    if (runtime != NULL)
        dvz_drp2_runtime_destroy(runtime);
    return out;
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t gpu_index = 0;
    if (argc > 1)
        gpu_index = (uint32_t)strtoul(argv[1], NULL, 10);

    int out = 1;
    DvzGpuCtx* gpu = NULL;
    DvzBuffer* buffer = NULL;
    DvzSemaphore* semaphore = NULL;
    DvzInteropBufferExport export_desc = {0};
    export_desc.memory_handle = -1;
    export_desc.semaphore_handle = -1;
    cudaExternalMemory_t cuda_memory = NULL;
    cudaExternalSemaphore_t cuda_semaphore = NULL;
    cudaStream_t cuda_stream = NULL;
    void* cuda_pointer = NULL;

    const Vertex vertices[3] = {
        {{-1.0f, -1.0f}},
        {{3.0f, -1.0f}},
        {{-1.0f, 3.0f}},
    };
    const uint64_t vertex_size = sizeof(vertices);

    gpu = dvz_interop_gpu_ctx(gpu_index, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (gpu == NULL)
    {
        dvz_fprintf(stderr, "could not create exportable Vulkan GPU context %u\n", gpu_index);
        goto cleanup;
    }
    DvzDevice* device = dvz_gpu_ctx_device(gpu);
    DvzVma* allocator = dvz_gpu_ctx_alloc(gpu);

    buffer = dvz_buffer_create_wrapper();
    if (buffer == NULL)
        goto cleanup;
    dvz_buffer(device, allocator, buffer);
    dvz_buffer_size(buffer, vertex_size);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(buffer, DVZ_ALLOC_DEDICATED_MEMORY);
    int status = dvz_buffer_create(buffer);
    if (status != 0)
    {
        dvz_fprintf(stderr, "exportable vertex-buffer creation failed\n");
        goto cleanup;
    }

    semaphore = dvz_semaphore_create_wrapper();
    if (semaphore == NULL)
        goto cleanup;
    dvz_semaphore_timeline(device, 0, semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);

    DvzInteropBufferExportConfig export_config = dvz_interop_buffer_export_config();
    export_config.size = vertex_size;
    export_config.drp2_usage = DVZ_DRP2_BUFFER_USAGE_VERTEX;
    export_config.semaphore = semaphore;
    export_config.semaphore_handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    export_config.semaphore_value = 0;
    status = dvz_interop_buffer_export_from_buffer(buffer, &export_config, &export_desc);
    if (status != 0 || export_desc.memory_handle < 0 || export_desc.semaphore_handle < 0)
    {
        dvz_fprintf(stderr, "Datoviz external-buffer export failed\n");
        goto cleanup;
    }

    int cuda_device = _matching_cuda_device(&export_desc);
    if (cuda_device < 0)
    {
        dvz_fprintf(stderr, "no CUDA device matches Vulkan GPU %u\n", gpu_index);
        goto cleanup;
    }
    if (_cuda_failed("cudaSetDevice", cudaSetDevice(cuda_device)))
        goto cleanup;

    struct cudaExternalMemoryHandleDesc memory_desc = {0};
    memory_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    memory_desc.handle.fd = export_desc.memory_handle;
    memory_desc.size = export_desc.allocation_size;
    if (_cuda_failed(
            "cudaImportExternalMemory", cudaImportExternalMemory(&cuda_memory, &memory_desc)))
        goto cleanup;
    export_desc.memory_handle = -1;

    struct cudaExternalMemoryBufferDesc mapped_desc = {0};
    mapped_desc.offset = export_desc.offset;
    mapped_desc.size = export_desc.size;
    if (_cuda_failed(
            "cudaExternalMemoryGetMappedBuffer",
            cudaExternalMemoryGetMappedBuffer(&cuda_pointer, cuda_memory, &mapped_desc)))
        goto cleanup;

    struct cudaExternalSemaphoreHandleDesc semaphore_desc = {0};
    semaphore_desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    semaphore_desc.handle.fd = export_desc.semaphore_handle;
    if (_cuda_failed(
            "cudaImportExternalSemaphore",
            cudaImportExternalSemaphore(&cuda_semaphore, &semaphore_desc)))
        goto cleanup;
    export_desc.semaphore_handle = -1;

    if (_cuda_failed(
            "cudaStreamCreateWithFlags",
            cudaStreamCreateWithFlags(&cuda_stream, cudaStreamNonBlocking)))
        goto cleanup;
    if (_cuda_failed(
            "cudaMemcpyAsync",
            cudaMemcpyAsync(
                cuda_pointer, vertices, vertex_size, cudaMemcpyHostToDevice, cuda_stream)))
        goto cleanup;

    struct cudaExternalSemaphoreSignalParams signal_params = {0};
    signal_params.params.fence.value = 1;
    if (_cuda_failed(
            "cudaSignalExternalSemaphoresAsync",
            cudaSignalExternalSemaphoresAsync(&cuda_semaphore, &signal_params, 1, cuda_stream)))
        goto cleanup;

    bool waited = dvz_interop_buffer_wait_timeline(device, buffer, vertex_size, semaphore, 1);
    if (!waited)
    {
        dvz_fprintf(stderr, "Vulkan wait for CUDA writes failed\n");
        goto cleanup;
    }

    out = _render_external_buffer(device, allocator, buffer, vertex_size);

cleanup:
    if (cuda_stream != NULL)
        cudaStreamDestroy(cuda_stream);
    if (cuda_pointer != NULL)
        cudaFree(cuda_pointer);
    if (cuda_memory != NULL)
        cudaDestroyExternalMemory(cuda_memory);
    if (cuda_semaphore != NULL)
        cudaDestroyExternalSemaphore(cuda_semaphore);
    if (export_desc.memory_handle >= 0)
        close(export_desc.memory_handle);
    if (export_desc.semaphore_handle >= 0)
        close(export_desc.semaphore_handle);
    if (semaphore != NULL)
    {
        dvz_semaphore_destroy(semaphore);
        dvz_semaphore_free(semaphore);
    }
    if (buffer != NULL)
    {
        dvz_buffer_destroy(buffer);
        dvz_buffer_free(buffer);
    }
    if (gpu != NULL)
        dvz_gpu_ctx_destroy(gpu);
    return out;
}
