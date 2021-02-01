#ifndef DVZ_SPIRV_HEADER
#define DVZ_SPIRV_HEADER

#include "../include/datoviz/common.h"
#include <vulkan/vulkan.h>

typedef struct DvzGpu DvzGpu;



DVZ_EXPORT VkShaderModule
dvz_shader_compile(DvzGpu* gpu, const char* code, VkShaderStageFlagBits stage);



#endif
