#include "../include/visky/vklite2.h"
#include "glfw.h"
#include "vklite2_utils.h"
// #include "vkutils.h"
#include <stdlib.h>

BEGIN_INCL_NO_WARN
#include <stb_image.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define INSTANCE_OBJ(s, o, t)                                                                     \
    log_trace("create %s", #s);                                                                   \
    s* o = calloc(1, sizeof(s));                                                                  \
    obj_init(&o->obj, t);

#define INSTANCE_ARR(s, arr, n, t)                                                                \
    log_trace("create %d %s(s)", (n), #s);                                                        \
    arr = calloc((n), sizeof(s));                                                                 \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        obj_init(&arr[i].obj, t);



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

static void obj_init(VklObject* obj, VklObjectType type)
{
    obj->type = type;
    obj->status = VKL_OBJECT_STATUS_INIT;
}

static void obj_created(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_CREATED; }

static void obj_destroyed(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_DESTROYED; }



/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/

VklApp* vkl_app(VklBackend backend)
{
    INSTANCE_OBJ(VklApp, app, VKL_OBJECT_TYPE_APP)

    // Which extensions are required? Depends on the backend.
    uint32_t required_extension_count = 0;
    const char** required_extensions = NULL;

    // Backend initialization and required extensions.
    {
        switch (backend)
        {
        case VKL_BACKEND_GLFW:
            glfwInit();
            ASSERT(glfwVulkanSupported() != 0);
            required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
            break;
        default:
            break;
        }
    }

    // Create the instance.
    create_instance(
        required_extension_count, required_extensions, &app->instance, &app->debug_messenger);
    // debug_messenger != 0 means validation enabled
    obj_created(&app->obj);

    // Count the number of devices.
    vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, NULL);
    log_trace("found %d GPU(s)", app->gpu_count);
    if (app->gpu_count == 0)
    {
        log_error("no compatible device found! aborting");
        exit(1);
    }

    // Allocate the GPU structures.
    INSTANCE_ARR(VklGpu, app->gpus, app->gpu_count, VKL_OBJECT_TYPE_GPU)

    // Initialize the GPU(s).
    VkPhysicalDevice* physical_devices = calloc(app->gpu_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, physical_devices);
    for (int i = 0; i < (int)app->gpu_count; i++)
    {
        app->gpus[i].app = app;
        discover_gpu(physical_devices[i], &app->gpus[i]);
        log_debug("found device #%d: %s", i, app->gpus[i].name);
    }

    FREE(physical_devices);

    return app;
}



void vkl_app_destroy(VklApp* app)
{
    ASSERT(app->gpus != NULL);
    // Destroy the GPUs.
    for (uint32_t i = 0; i < app->gpu_count; i++)
    {
        vkl_gpu_destroy(&app->gpus[i]);
        obj_destroyed(&app->gpus[i].obj);
    }

    // Destroy the debug messenger.
    if (app->debug_messenger)
        destroy_debug_utils_messenger_EXT(app->instance, app->debug_messenger, NULL);

    // Destroy the instance.
    log_trace("destroy instance");
    if (app->instance != 0)
    {
        vkDestroyInstance(app->instance, NULL);
        app->instance = 0;
    }

    // Free the memory.
    FREE(app->gpus);
    FREE(app);
}



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

VklGpu* vkl_gpu(VklApp* app, uint32_t idx)
{
    if (idx >= app->gpu_count)
    {
        log_error("GPU index %d higher than number of GPUs %d", idx, app->gpu_count);
        idx = 0;
    }
    VklGpu* gpu = &app->gpus[idx];
    return gpu;
}

void vkl_gpu_request_features(VklGpu* gpu, VkPhysicalDeviceFeatures requested_features)
{
    gpu->requested_features = requested_features;
}

void vkl_gpu_create(VklGpu* gpu, VkSurfaceKHR surface)
{
    log_trace("starting creation of GPU WITH%s surface", surface != 0 ? "" : "OUT");
    create_device(gpu, surface);

    // Create command pools
    create_command_pool(
        gpu->device, gpu->queues.indices[VKL_QUEUE_GRAPHICS], &gpu->cmd_pools[VKL_QUEUE_GRAPHICS]);

    create_command_pool(
        gpu->device, gpu->queues.indices[VKL_QUEUE_COMPUTE], &gpu->cmd_pools[VKL_QUEUE_COMPUTE]);

    // Create descriptor pool
    // TODO

    obj_created(&gpu->obj);
    log_trace("GPU created");
}

void vkl_gpu_destroy(VklGpu* gpu)
{
    log_trace("started destruction of GPU");
    ASSERT(gpu != NULL);
    if (gpu->obj.status < VKL_OBJECT_STATUS_CREATED)
    {

        log_trace("skip destruction of GPU as it was not properly created");
        ASSERT(gpu->device == 0);
        return;
    }
    VkDevice device = gpu->device;
    ASSERT(device != 0);

    // Destroy the command pool.
    log_trace("destroy command pools");
    for (uint32_t i = 0; i < gpu->cmd_pool_count; i++)
    {
        if (gpu->cmd_pools[i] != 0)
        {
            vkDestroyCommandPool(device, gpu->cmd_pools[i], NULL);
            gpu->cmd_pools[i] = 0;
        }
    }

    // log_trace("destroy descriptor pool");
    // if (gpu->descriptor_pool)
    //     vkDestroyDescriptorPool(gpu->device, gpu->descriptor_pool, NULL);

    // Destroy the device.
    log_trace("destroy device");
    vkDestroyDevice(gpu->device, NULL);
    gpu->device = 0;

    log_trace("GPU destroyed");
}



/*************************************************************************************************/
/*  Commands */
/*************************************************************************************************/

VklCommands* vkl_commands(VklGpu* gpu, VklQueueType queue, uint32_t count)
{
    INSTANCE_OBJ(VklCommands, commands, VKL_OBJECT_TYPE_COMMANDS)

    return commands;
}

void vkl_cmd_begin(VklCommands* cmds) {}

void vkl_cmd_end(VklCommands* cmds) {}

void vkl_cmd_reset(VklCommands* cmds) {}

void vkl_cmd_free(VklCommands* cmds) { FREE(cmds); }
