/*
 * Advanced CUDA external-memory bridge for experimental Datoviz scene buffers.
 *
 * This helper is intentionally tiny and optional. It owns CUDA Runtime external-memory and
 * external-semaphore imports created from Datoviz/Vulkan-exported opaque FDs, and exposes only the
 * mapped device pointer needed by cupy.cuda.UnownedMemory.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include <cuda_runtime_api.h>


typedef struct DvzCudaInteropBridge DvzCudaInteropBridge;

struct DvzCudaInteropBridge
{
    cudaExternalMemory_t memory;
    cudaExternalSemaphore_t semaphore;
    void* ptr;
    uint64_t size;
};

static char g_last_error[256] = "ok";


static void _set_error(const char* label, cudaError_t err)
{
    const char* msg = cudaGetErrorString(err);
    if (label == NULL)
        label = "cuda";
    if (msg == NULL)
        msg = "unknown CUDA error";
#if defined(_MSC_VER)
    _snprintf_s(g_last_error, sizeof(g_last_error), _TRUNCATE, "%s: %s (%d)", label, msg, err);
#else
    snprintf(g_last_error, sizeof(g_last_error), "%s: %s (%d)", label, msg, err);
#endif
}


static void _close_fd_if_needed(int fd)
{
#if !defined(_WIN32)
    if (fd >= 0)
        close(fd);
#else
    (void)fd;
#endif
}


const char* dvz_cuda_bridge_last_error(void)
{
    return g_last_error;
}


int dvz_cuda_bridge_import(
    int memory_fd, uint64_t allocation_size, uint64_t offset, uint64_t size, int semaphore_fd,
    DvzCudaInteropBridge** out)
{
    if (out == NULL)
        return -1;
    *out = NULL;
    if (memory_fd < 0 || allocation_size == 0 || size == 0 || offset > allocation_size ||
        size > allocation_size - offset)
    {
        _close_fd_if_needed(memory_fd);
        _close_fd_if_needed(semaphore_fd);
        return -1;
    }

    DvzCudaInteropBridge* bridge = (DvzCudaInteropBridge*)calloc(1, sizeof(DvzCudaInteropBridge));
    if (bridge == NULL)
    {
        _close_fd_if_needed(memory_fd);
        _close_fd_if_needed(semaphore_fd);
        return -1;
    }

    struct cudaExternalMemoryHandleDesc mem_desc = {0};
    mem_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    mem_desc.handle.fd = memory_fd;
    mem_desc.size = (unsigned long long)allocation_size;
    cudaError_t err = cudaImportExternalMemory(&bridge->memory, &mem_desc);
    if (err != cudaSuccess)
    {
        _set_error("cudaImportExternalMemory", err);
        _close_fd_if_needed(memory_fd);
        _close_fd_if_needed(semaphore_fd);
        free(bridge);
        return -1;
    }

    struct cudaExternalMemoryBufferDesc buf_desc = {0};
    buf_desc.offset = (unsigned long long)offset;
    buf_desc.size = (unsigned long long)size;
    err = cudaExternalMemoryGetMappedBuffer(&bridge->ptr, bridge->memory, &buf_desc);
    if (err != cudaSuccess)
    {
        _set_error("cudaExternalMemoryGetMappedBuffer", err);
        _close_fd_if_needed(semaphore_fd);
        cudaDestroyExternalMemory(bridge->memory);
        free(bridge);
        return -1;
    }
    bridge->size = size;

    if (semaphore_fd >= 0)
    {
        struct cudaExternalSemaphoreHandleDesc sem_desc = {0};
        sem_desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
        sem_desc.handle.fd = semaphore_fd;
        err = cudaImportExternalSemaphore(&bridge->semaphore, &sem_desc);
        if (err != cudaSuccess)
        {
            _set_error("cudaImportExternalSemaphore", err);
            _close_fd_if_needed(semaphore_fd);
            cudaFree(bridge->ptr);
            cudaDestroyExternalMemory(bridge->memory);
            free(bridge);
            return -1;
        }
    }

    *out = bridge;
    return 0;
}


uint64_t dvz_cuda_bridge_ptr(DvzCudaInteropBridge* bridge)
{
    if (bridge == NULL)
        return 0;
    return (uint64_t)(uintptr_t)bridge->ptr;
}


uint64_t dvz_cuda_bridge_size(DvzCudaInteropBridge* bridge)
{
    if (bridge == NULL)
        return 0;
    return bridge->size;
}


int dvz_cuda_bridge_wait(DvzCudaInteropBridge* bridge, uint64_t value, uint64_t stream_ptr)
{
    if (bridge == NULL || bridge->semaphore == NULL)
        return -1;
    struct cudaExternalSemaphoreWaitParams params = {0};
    params.params.fence.value = (unsigned long long)value;
    cudaExternalSemaphore_t sem = bridge->semaphore;
    cudaError_t err = cudaWaitExternalSemaphoresAsync(
        &sem, &params, 1, (cudaStream_t)(uintptr_t)stream_ptr);
    if (err != cudaSuccess)
    {
        _set_error("cudaWaitExternalSemaphoresAsync", err);
        return -1;
    }
    return 0;
}


int dvz_cuda_bridge_signal(DvzCudaInteropBridge* bridge, uint64_t value, uint64_t stream_ptr)
{
    if (bridge == NULL || bridge->semaphore == NULL)
        return -1;
    struct cudaExternalSemaphoreSignalParams params = {0};
    params.params.fence.value = (unsigned long long)value;
    cudaExternalSemaphore_t sem = bridge->semaphore;
    cudaError_t err = cudaSignalExternalSemaphoresAsync(
        &sem, &params, 1, (cudaStream_t)(uintptr_t)stream_ptr);
    if (err != cudaSuccess)
    {
        _set_error("cudaSignalExternalSemaphoresAsync", err);
        return -1;
    }
    return 0;
}


void dvz_cuda_bridge_destroy(DvzCudaInteropBridge* bridge)
{
    if (bridge == NULL)
        return;
    if (bridge->ptr != NULL)
        cudaFree(bridge->ptr);
    if (bridge->semaphore != NULL)
        cudaDestroyExternalSemaphore(bridge->semaphore);
    if (bridge->memory != NULL)
        cudaDestroyExternalMemory(bridge->memory);
    free(bridge);
}
