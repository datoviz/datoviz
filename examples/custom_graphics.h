/*************************************************************************************************/
/*  Example of a custom graphics                                                                 */
/*************************************************************************************************/

// NOTE: ignore this.
#ifndef SCREENSHOT
#define SCREENSHOT(name)
#endif
#ifndef SPIRV_DIR
#define SPIRV_DIR ""
#endif



// We include the library header file.
#include <datoviz/datoviz.h>



// We define the vertex structure for our graphics pipeline.
typedef struct
{
    vec3 pos;     // 3D point position
    uint8_t size; // point size, in pixels, between 0 and 255.
} PointVertex;



// Entry point of the standalone example.
static int demo_custom_graphics()
{
    // We create an app, canvas, scene, panel.
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    // We create a blank visual, to which we'll add our custom visual.
    DvzVisual* visual = dvz_blank_visual(scene, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);

    // We create a blank graphics that will become our custom graphics.
    DvzGraphics* graphics = dvz_blank_graphics(scene, 0);

    // Custom graphics creation.
    {
        // The first step is to define the vertex and fragment shaders. When using
        // dvz_graphics_shader(), one must specify a path to the compiled SPIR-V shaders.
        // When writing the shaders in GLSL, it is thus necessary to compile them separately with
        // glslc.
        char path[1024];
        snprintf(path, sizeof(path), "%s/custom_point.vert.spv", SPIRV_DIR);
        dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
        snprintf(path, sizeof(path), "%s/custom_point.frag.spv", SPIRV_DIR);
        dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);

        // Define the graphics pipeline topology: point list here.
        dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

        // Next, we declare the size of our vertex structure.
        dvz_graphics_vertex_binding(graphics, 0, sizeof(PointVertex));

        // We declare the vertex shader attributes, that should correspond to the different
        // structure fields in the vertex structure.

        // The first attribute is a vec3 in GLSL, and a vec3 in C too.
        dvz_graphics_vertex_attr(
            graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PointVertex, pos));

        // The second attribute is a float in GLSL, and a byte (uint8) in C. We use the special
        // format VK_FORMAT_R8_USCALED to declare this.
        dvz_graphics_vertex_attr(
            graphics, 0, 1, VK_FORMAT_R8_USCALED, offsetof(PointVertex, size));

        // Now that we've set up the graphics, we create it.
        dvz_graphics_create(graphics);
    }

    // We add our custom graphics to a custom visual.
    dvz_custom_graphics(visual, graphics);

    // We add the custom visual to the panel.
    dvz_custom_visual(panel, visual);

    // Custom data.
    {
        // Now, we prepare the vertex data. We could have defined and used props, but we'll show
        // another method instead. We create the vertex buffer directly, using the PointVertex
        // structure we've created.
        const uint32_t N = 64;                                  // number of points
        PointVertex* vertices = calloc(N, sizeof(PointVertex)); // vertex buffer
        float t = 0;
        for (uint32_t i = 0; i < N; i++)
        {
            t = i / (float)(N - 1);
            // vertex position
            vertices[i].pos[0] = -.75 + 1.25 * t * t;
            vertices[i].pos[1] = +.75 - 1.25 * t;
            // vertex size, in byte, between 0 and 255.
            vertices[i].size = 4 * i + 1;
        }

        // Here is the crucial bit: we bind the GPU vertex buffer with our struct array.
        dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
        FREE(vertices);
    }

    SCREENSHOT("custom_graphics")
    dvz_app_run(app, 0);

    // dvz_graphics_destroy(graphics);
    dvz_app_destroy(app);
    return 0;
}
