#ifndef VKY_OFFSCREEN_HEADER
#define VKY_OFFSCREEN_HEADER

#include "vklite.h"


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct Video Video;
typedef struct VkyVideo VkyVideo;
typedef struct VkyScreenshot VkyScreenshot;



/*************************************************************************************************/
/*  Structs                                                                                     */
/*************************************************************************************************/

struct VkyScreenshot
{
    VkyCanvas* canvas;
    uint32_t width, height, row_pitch;
    uint8_t* image;

    VkDeviceMemory dstImageMemory;
    VkImage dstImage;
    VkCommandBuffer cmd_buf;
};

struct VkyVideo
{
    VkyCanvas* canvas;
    VkyScreenshot* screenshot;
    Video* video;

    VkDeviceSize image_size;
    uint8_t* buffer;
    uint32_t buffer_frame_count;
    uint32_t current_frame_index;
};



/*************************************************************************************************/
/*  Offscreen rendereing                                                                         */
/*************************************************************************************************/

void create_offscreen_render_pass(
    VkDevice device, VkFormat format, VkImageLayout layout, VkRenderPass* render_pass);

VKY_EXPORT VkyCanvas* vky_create_offscreen_canvas(VkyGpu* gpu, uint32_t widht, uint32_t height);

VKY_EXPORT void vky_offscreen_frame(VkyCanvas* canvas, double time);



/*************************************************************************************************/
/*  Screenshots                                                                                  */
/*************************************************************************************************/

VKY_EXPORT VkyScreenshot* vky_create_screenshot(VkyCanvas* canvas);

VKY_EXPORT void vky_begin_screenshot(VkyScreenshot* screenshot);

VKY_EXPORT void vky_end_screenshot(VkyScreenshot* screenshot);

VKY_EXPORT void vky_destroy_screenshot(VkyScreenshot* screenshot);

VKY_EXPORT uint8_t* vky_screenshot_to_rgb(VkyScreenshot* screenshot, bool);

VKY_EXPORT void vky_screenshot(VkyCanvas* canvas, char* filename);



/*************************************************************************************************/
/*  Video                                                                                        */
/*************************************************************************************************/

VKY_EXPORT void
vky_create_video(VkyCanvas* canvas, const char* filename, double duration, int fps, int bitrate);



/*************************************************************************************************/
/*  Offscreen backend                                                                            */
/*************************************************************************************************/

VKY_EXPORT void vky_run_offscreen_app(VkyApp* app);



#endif
