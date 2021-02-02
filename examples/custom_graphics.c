/*************************************************************************************************/
/*  Example of a custom visual.                                                                  */
/*************************************************************************************************/

#include <datoviz/datoviz.h>

typedef struct
{
    vec3 pos;
    uint8_t size;
} PointVertex;

int main(int argc, char** argv)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    // Create a blank visual.
    DvzVisual* visual = dvz_blank_visual(scene, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);

    // Create a blank graphics to be added to the custom visual.
    DvzGraphics* graphics = dvz_blank_graphics(scene, 0);

    // Custom graphics.
    {
        dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, "custom_point.vert.spv");
        dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, "custom_point.frag.spv");
        dvz_graphics_renderpass(graphics, &canvas->renderpass, 0);
        dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

        // Vertex attributes.
        dvz_graphics_vertex_binding(graphics, 0, sizeof(PointVertex));
        dvz_graphics_vertex_attr(
            graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PointVertex, pos));
        dvz_graphics_vertex_attr(
            graphics, 0, 1, VK_FORMAT_R8_USCALED, offsetof(PointVertex, size));

        // Create the graphics.
        dvz_graphics_create(graphics);
    }

    // Add the created graphics to the custom visual.
    dvz_custom_graphics(visual, graphics);

    // Add the custom visual to the panel.
    dvz_custom_visual(panel, visual);

    // Set the vertex data.
    const uint32_t N = 64;
    PointVertex* vertices = calloc(N, sizeof(PointVertex));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)(N - 1);
        vertices[i].pos[0] = -.75 + 1.25 * t * t;
        vertices[i].pos[1] = +.75 - 1.25 * t;
        vertices[i].size = 4 * i;
    }

    // Set the vertices.
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
    FREE(vertices);

    // dvz_app_run(app, 5);
    // dvz_screenshot_file(canvas, "custom_graphics.png");
    dvz_app_run(app, 0);

    // dvz_graphics_destroy(graphics);
    dvz_app_destroy(app);
    return 0;
}
