/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_WINDOW
#define DVZ_HEADER_WINDOW



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWindow DvzWindow;
typedef struct DvzGpu DvzGpu;

// Forward declaration.
typedef struct DvzHost DvzHost;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindow
{
    DvzObject obj;
    DvzHost* host;

    void* backend_window;
    uint32_t width, height; // in screen coordinates

    bool close_on_esc;
    uint64_t surface; // VkSurfaceKHR
};



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

/**
 * Create a blank window.
 *
 * This function is rarely used on its own. A bare window offers
 * no functionality that allows one to render to it with Vulkan. One needs a swapchain, an event
 * loop, and so on, which are provided instead at the level of the Canvas.
 *
 * @param host the host
 * @param width the window width, in pixels
 * @param height the window height, in pixels
 * @returns the window
 */
DVZ_EXPORT DvzWindow* dvz_window(DvzHost* host, uint32_t width, uint32_t height);

/**
 * Get the window size, in pixels.
 *
 * @param window the window
 * @param[out] framebuffer_width the width, in pixels
 * @param[out] framebuffer_height the height, in pixels
 */
DVZ_EXPORT void
dvz_window_get_size(DvzWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height);

/**
 * Set the window size, in pixels.
 *
 * @param window the window
 * @param width the width, in pixels
 * @param height the height, in pixels
 */
DVZ_EXPORT void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height);

/**
 * Process the pending windowing events by the backend (glfw by default).
 *
 * @param window the window
 */
DVZ_EXPORT void dvz_window_poll_events(DvzWindow* window);

/**
 * Destroy a window.
 *
 * !!! warning
 *     This function must be imperatively called *after* `dvz_swapchain_destroy()`.
 *
 * @param window the window
 */
DVZ_EXPORT void dvz_window_destroy(DvzWindow* window);

/**
 * Destroy a canvas.
 *
 * @param canvas the canvas
 */
// DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);

/**
 * Destroy all canvases.
 *
 * @param canvases the container with the canvases.
 */
// DVZ_EXPORT void dvz_canvases_destroy(DvzContainer* canvases);



#endif
