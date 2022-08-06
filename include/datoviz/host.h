/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_HOST
#define DVZ_HEADER_HOST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <vulkan/vulkan.h>

#include "_enums.h"
#include "_obj.h"
#include "_time.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzHost DvzHost;
typedef struct DvzGpu DvzGpu;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzHost
{
    DvzObject obj;
    uint32_t n_errors;

    // Backend
    DvzBackend backend;

    // Global clock
    DvzClock clock;
    // bool is_running;

    // Vulkan objects.
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Containers.
    DvzContainer gpus;
};



/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

/**
 * Create a host.
 *
 * This object represents a computer with one or multiple GPUs.
 * It holds the Vulkan instance and it is responsible for discovering the available GPUs.
 *
 * @param backend the backend
 * @returns a pointer to the created host
 */
DVZ_EXPORT DvzHost* dvz_host(DvzBackend backend);



/**
 * Full synchronization on all GPUs.
 *
 * This function waits on all queues of all GPUs. The strongest, least efficient of the
 * synchronization methods.
 *
 * @param host the host
 */
DVZ_EXPORT void dvz_host_wait(DvzHost* host);



/**
 * Destroy the host.
 *
 * This function automatically destroys all objects created within the host.
 *
 * @param host the host to destroy
 */
DVZ_EXPORT int dvz_host_destroy(DvzHost* host);



#endif
