#include <stdint.h>
#include <cuda_runtime.h>

extern "C" __global__ void add1_kernel(uint32_t* data, size_t n)
{
    uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n)
        data[idx] += 1;
}

extern "C" void launch_add1_kernel(uint32_t* dev_ptr, size_t count)
{
    const int BLOCK_SIZE = 256;
    int num_blocks = (count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    add1_kernel<<<num_blocks, BLOCK_SIZE>>>(dev_ptr, count);
    cudaDeviceSynchronize();
}
