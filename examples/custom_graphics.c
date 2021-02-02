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
    DvzVisual* visual = dvz_scene_visual_blank(scene, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);


    // Graphics pipeline.
    DvzGraphics graphics = dvz_graphics(gpu);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, "custom_point.vert.spv");
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, "custom_point.frag.spv");
    dvz_graphics_renderpass(&graphics, &canvas->renderpass, 0);
    dvz_graphics_topology(&graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

    // Vertex attributes.
    dvz_graphics_vertex_binding(&graphics, 0, sizeof(PointVertex));
    dvz_graphics_vertex_attr(
        &graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PointVertex, pos));
    dvz_graphics_vertex_attr(&graphics, 0, 1, VK_FORMAT_R8_USCALED, offsetof(PointVertex, size));

    // Common slots.
    dvz_graphics_slot(&graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
    dvz_graphics_slot(&graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, canvas->swapchain.img_count);

    // Binding resources.
    DvzBufferRegions br_mvp = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(DvzMVP));
    DvzBufferRegions br_viewport =
        dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzViewport));

    // Upload MVP.
    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_upload_buffers(canvas, br_mvp, 0, sizeof(DvzMVP), &mvp);

    // Bindings
    dvz_bindings_buffer(&bindings, 0, br_mvp);
    dvz_bindings_buffer(&bindings, 1, br_viewport);

    // Finally create the graphics pipeline.
    dvz_graphics_create(&graphics);


    // Custom visual.
    dvz_visual_graphics(visual, &graphics);
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(PointVertex), 0);

    const uint32_t N = 64;
    PointVertex* vertices = calloc(N, sizeof(PointVertex));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)(N - 1);
        vertices[i].pos[0] = -.75 + 1.5 * t * t;
        vertices[i].pos[1] = +.75 - 1.5 * t;

        // vertices[i].pos[0] = -.9 + 1.8 * t;
        vertices[i].size = 4 * i;
    }

    // Set the vertices.
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);

    // Add the custom visual to the main panel.
    dvz_scene_visual_custom(panel, visual);

    // dvz_app_run(app, 5);
    // dvz_screenshot_file(canvas, "custom_graphics.png");
    dvz_app_run(app, 0);

    dvz_graphics_destroy(&graphics);
    FREE(vertices);
    dvz_app_destroy(app);
    return 0;
}
