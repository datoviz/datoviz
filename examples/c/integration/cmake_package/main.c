#include <stdio.h>

#include <datoviz.h>
#include <datoviz/canvas.h>
#include <datoviz/vklite.h>

int main(void)
{
    const char* version = dvz_version();
    if (version == NULL)
        return 1;

    DvzResult (*configure_gpu_ctx)(
        DvzWindowHost*, DvzBackend, DvzCanvasRenderMode,
        DvzGpuCtxConfig*) = dvz_canvas_configure_gpu_ctx;
    VkFormat (*frame_format)(const DvzCanvas*) = dvz_canvas_frame_format;
    DvzResult (*commands_unwrap)(DvzCommands*) = dvz_commands_unwrap;
    void (*set_viewport)(DvzCommands*, const VkViewport*) = dvz_cmd_set_viewport;
    void (*set_scissor)(DvzCommands*, const VkRect2D*) = dvz_cmd_set_scissor;
    void (*set_viewport_scissor)(DvzCommands*, VkExtent2D) = dvz_cmd_set_viewport_scissor;
    if (configure_gpu_ctx == NULL || frame_format == NULL || commands_unwrap == NULL ||
        set_viewport == NULL || set_scissor == NULL || set_viewport_scissor == NULL)
    {
        return 1;
    }

    printf("Datoviz %s\n", version);
    return 0;
}
