#include "../include/visky/vklite2.h"
#include "glfw.h"
#include "vklite2_utils.h"
// #include "vkutils.h"
#include <stdlib.h>

#ifndef ENABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS 1
#endif

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
    log_trace("create %d %s's", (n), #s);                                                         \
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



/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/

static void _create_gpu(VkPhysicalDevice physical_device, VklGpu* gpu)
{
    vkGetPhysicalDeviceProperties(physical_device, &gpu->device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &gpu->device_features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &gpu->memory_properties);
    gpu->name = gpu->device_properties.deviceName;
}


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
    if (app->gpu_count == 0)
    {
        log_error("no compatible device found! aborting");
        exit(1);
    }

    // Allocate the GPU structures.
    // app->gpus = calloc(app->gpu_count, sizeof(VklGpu));
    INSTANCE_ARR(VklGpu, app->gpus, app->gpu_count, VKL_OBJECT_TYPE_GPU)

    // Initialize the GPUs.
    VkPhysicalDevice* physical_devices = calloc(app->gpu_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, physical_devices);
    for (int i = 0; i < (int)app->gpu_count; i++)
    {
        _create_gpu(physical_devices[i], &app->gpus[i]);
        app->gpus[i].obj.status = VKL_OBJECT_STATUS_CREATED;
        log_debug("found device #%d: %s", i, app->gpus[i].name);
    }

    FREE(physical_devices);

    return app;
}


VklGpu* vkl_app_get_gpu(VklApp* app, uint32_t idx)
{
    ASSERT(app->gpu_count > 0);
    ASSERT(app->gpus != NULL);
    if (idx < app->gpu_count)
        return &app->gpus[idx];
    log_error("Invalid GPU index #%d (total %d GPUs)", idx, app->gpu_count);
    return NULL;
}


void vkl_app_destroy(VklApp* app)
{
    FREE(app->gpus);
    FREE(app);
}
