#ifndef VKL_SPIRV_HEADER
#define VKL_SPIRV_HEADER

#include "../include/visky/common.h"
#include <vulkan/vulkan.h>

typedef struct VklGpu VklGpu;



VKY_EXPORT VkShaderModule
vkl_shader_compile(VklGpu* gpu, const char* code, VkShaderStageFlagBits stage);



#endif
