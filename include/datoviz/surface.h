/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SURFACE
#define DVZ_HEADER_SURFACE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "host.h"
#include "window.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a surface out of a window.
 *
 * @param host the host
 * @param window the window
 * @returns a Vulkan surface object
 */
DVZ_EXPORT VkSurfaceKHR dvz_window_surface(DvzHost* host, DvzWindow* window);



/**
 * Create a GPU with a temporary invisible mock window in order to get Vulkan capabilities.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_create_with_surface(DvzGpu* gpu);



/**
 * Destroy a surface.
 *
 * @param surface the surface to destroy
 */
DVZ_EXPORT void dvz_surface_destroy(DvzHost* host, VkSurfaceKHR surface);



#endif
