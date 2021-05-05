/*************************************************************************************************/
/*  Mandelbrot fractal example.                                                                  */
/*************************************************************************************************/

// // NOTE: ignore this.
// #ifndef SCREENSHOT
// #define SCREENSHOT
// #endif
// #ifndef NFRAMES
// #define NFRAMES 0
// #endif



// We include the library header file.
#include <datoviz/datoviz.h>



static int demo_mandelbrot()
{
    // Initialization.
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 1024, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    // Custom visual and graphics.
    DvzVisual* visual = dvz_blank_visual(scene, 0);
    DvzGraphics* graphics = dvz_blank_graphics(scene, 0);

    // Shaders.
    char path[1024];
    snprintf(path, sizeof(path), "%s/mandelbrot.vert.spv", SPIRV_DIR);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/mandelbrot.frag.spv", SPIRV_DIR);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);

    // Graphics and custom visual.
    dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    dvz_graphics_vertex_binding(graphics, 0, sizeof(vec3));
    dvz_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_graphics_create(graphics);

    // Add the custom graphics to the custom visual, and add the custom visual to the panel.
    dvz_custom_graphics(visual, graphics);
    dvz_custom_visual(panel, visual);

    // Visual data.
    vec3 vertices[] = {
        {-1, -1, 0},
        {+1, -1, 0},
        {-1, +1, 0},
        {+1, +1, 0},
    };
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, 4, 4, vertices);

    // Run.
    // SCREENSHOT
    dvz_app_run(app, 0);

    // Destroy and quit.
    dvz_app_destroy(app);
    return 0;
}
