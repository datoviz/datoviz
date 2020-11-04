#ifndef VKL_VKLITE2_HEADER
#define VKL_VKLITE2_HEADER

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "common.h"

BEGIN_INCL_NO_WARN
#include <cglm/struct.h>
END_INCL_NO_WARN


/*
TODO later
- rename Vkl/VKL/vkl to Vky
- move constants to constants.c
- put glfw-specific code in the same place
*/



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_GPU 32



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VklObject VklObject;
typedef struct VklApp VklApp;
typedef struct VklGpu VklGpu;
typedef struct VklCommands VklCommands;
typedef struct VklBuffer VklBuffer;
typedef struct VklImage VklImage;
typedef struct VklSampler VklSampler;
typedef struct VklBinding VklBinding;
typedef struct VklCompute VklCompute;
typedef struct VklPipeline VklPipeline;
typedef struct VklBarrier VklBarrier;
typedef struct VklSync VklSync;
typedef struct VklRenderpass VklRenderpass;
typedef struct VklSubmit VklSubmit;
typedef struct VklCanvas VklCanvas;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_OBJECT_TYPE_UNDEFINED,
    VKL_OBJECT_TYPE_APP,
    VKL_OBJECT_TYPE_GPU,
    VKL_OBJECT_TYPE_COMMANDS,
    VKL_OBJECT_TYPE_BUFFER,
    VKL_OBJECT_TYPE_IMAGE,
    VKL_OBJECT_TYPE_SAMPLER,
    VKL_OBJECT_TYPE_BINDING,
    VKL_OBJECT_TYPE_COMPUTE,
    VKL_OBJECT_TYPE_PIPELINE,
    VKL_OBJECT_TYPE_BARRIER,
    VKL_OBJECT_TYPE_SYNC,
    VKL_OBJECT_TYPE_RENDERPASS,
    VKL_OBJECT_TYPE_SUBMIT,
    VKL_OBJECT_TYPE_CANVAS,
    VKL_OBJECT_TYPE_CUSTOM,
} VklObjectType;


typedef enum
{
    VKL_OBJECT_STATUS_UNDEFINED,
    VKL_OBJECT_STATUS_INIT,
    VKL_OBJECT_STATUS_CREATED,
    VKL_OBJECT_STATUS_NEED_RECREATE,
    VKL_OBJECT_STATUS_NEED_UPDATE,
    VKL_OBJECT_STATUS_DESTROYED,
} VklObjectStatus;


typedef enum
{
    VKL_BACKEND_NONE,
    VKL_BACKEND_GLFW,
    VKL_BACKEND_OFFSCREEN,
} VklBackend;


typedef enum
{
    VKL_QUEUE_GRAPHICS,
    VKL_QUEUE_COMPUTE,
} VklQueueType;


typedef enum
{
    VKL_COMMAND_TRANSFERS,
    VKL_COMMAND_GRAPHICS,
    VKL_COMMAND_COMPUTE,
    VKL_COMMAND_GUI,
} VklCommandBufferType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklObject
{
    VklObjectType type;
    VklObjectStatus status;
};



struct VklApp
{
    VklObject obj;

    // GPUs.
    uint32_t gpu_count;
    VklGpu* gpus;

    // Vulkan objects.
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
};



struct VklGpu
{
    VklObject obj;

    const char* name;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;
};



struct VklCommands
{
    VklObject obj;
};



struct VklBuffer
{
    VklObject obj;
};



struct VklImage
{
    VklObject obj;
};



struct VklSampler
{
    VklObject obj;
};



struct VklBinding
{
    VklObject obj;
};



struct VklCompute
{
    VklObject obj;
};



struct VklPipeline
{
    VklObject obj;
};



struct VklBarrier
{
    VklObject obj;
};



struct VklSync
{
    VklObject obj;
};



struct VklRenderpass
{
    VklObject obj;
};



struct VklSubmit
{
    VklObject obj;
};



struct VklCanvas
{
    VklObject obj;
};



/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/

VKY_EXPORT VklApp* vkl_app(VklBackend backend);

VKY_EXPORT VklGpu* vkl_app_get_gpu(VklApp* app, uint32_t idx);

VKY_EXPORT void vkl_app_destroy(VklApp* app);



/*************************************************************************************************/
/*  Gpu                                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Buffer                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Binding                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Sync                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Submit                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/



#endif
