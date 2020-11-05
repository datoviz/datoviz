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
- rename Vkl/VKL/vkl to Vkl
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
typedef struct VklQueues VklQueues;
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


// NOTE: the order is important, status >= CREATED means the object has been created
typedef enum
{
    VKL_OBJECT_STATUS_UNDEFINED,     // invalid state
    VKL_OBJECT_STATUS_DESTROYED,     // after destruction
    VKL_OBJECT_STATUS_INIT,          // after memory allocation
    VKL_OBJECT_STATUS_CREATED,       // after proper creation on the GPU
    VKL_OBJECT_STATUS_NEED_RECREATE, // need to be recreated
    VKL_OBJECT_STATUS_NEED_UPDATE,   // need to be updated
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
    VKL_QUEUE_PRESENT,
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



struct VklQueues
{
    VklObject obj;

    uint32_t queue_count;
    int32_t indices[3]; // graphics, compute, present
    VkQueue queues[3];
};



struct VklGpu
{
    VklObject obj;
    VklApp* app;

    const char* name;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VklQueues queues;

    VkPhysicalDeviceFeatures requested_features;
    VkDevice device;

    uint32_t cmd_pool_count;
    VkCommandPool cmd_pools[2]; // graphics, compute
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

/**
 * Create an application instance.
 *
 * There is typically only one App object in a given application. This object holds a pointer to
 * the Vulkan instance and is responsible for discovering the available GPUs.
 *
 * @param backend the backend, typically either VKL_BACKEND_GLFW or VKL_BACKEND_OFFSCREEN
 * @returns a pointer to the created VklApp struct
 */
VKY_EXPORT VklApp* vkl_app(VklBackend backend);



/**
 * Destroy the application.
 *
 * This function automatically destroys all objects created within the application.
 *
 * @param app the application to destroy
 */
VKY_EXPORT void vkl_app_destroy(VklApp* app);



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

VKY_EXPORT VklGpu* vkl_gpu(VklApp* app, uint32_t idx);

VKY_EXPORT void vkl_gpu_request_features(VklGpu* gpu, VkPhysicalDeviceFeatures requested_features);

VKY_EXPORT void vkl_gpu_create(VklGpu* gpu, VkSurfaceKHR surface);

VKY_EXPORT void vkl_gpu_destroy(VklGpu* gpu);



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VklCommands* vkl_commands(VklGpu* gpu, VklQueueType queue, uint32_t count);

VKY_EXPORT void vkl_cmd_begin(VklCommands* cmds);

VKY_EXPORT void vkl_cmd_end(VklCommands* cmds);

VKY_EXPORT void vkl_cmd_reset(VklCommands* cmds);

VKY_EXPORT void vkl_cmd_free(VklCommands* cmds);



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
