#include <visky/visky.h>

BEGIN_INCL_NO_WARN
#include <imgui.h>
END_INCL_NO_WARN


#define SHOW_CANVAS 0



// Resources used in this example.
static VkyBufferRegion vertex_buffer;
static VkyGraphicsPipeline pipeline;
static VkyImGuiTexture imtexture;
static VkyCanvas* imcanvas;


static void fill_command_buffer(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    // Begin the render pass.
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);

    // Bind the vertex buffer.
    vky_bind_vertex_buffer(cmd_buf, vertex_buffer, 0);

    // Bind the graphics pipeline.
    vky_bind_graphics_pipeline(cmd_buf, &pipeline);

    // Set the full viewport.
    vky_set_viewport(
        cmd_buf, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);

    // Draw 3 vertices = 1 triangle.
    vky_draw(cmd_buf, 0, 3);

    // End the render pass.
    vky_end_render_pass(cmd_buf, canvas);
}


static void fill_live_command_buffer(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_imgui_newframe();

    ImGui::Begin("FPS");
    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
    if (SHOW_CANVAS)
    {
        vky_imgui_canvas_next_frame(imcanvas);
        vky_imgui_image(
            &imtexture, imtexture.texture.params.width, imtexture.texture.params.height);
    }
    else
    {
        vky_imgui_image(&imtexture, 400, 400);
    }
    ImGui::End();

    vky_imgui_render(canvas, cmd_buf);
}



int main(int argc, char** argv)
{
    log_set_level_env();

    // Create the app and canvas.
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);

    vky_imgui_init(canvas);

    // This callback function is called when the command buffers need to be recreated,
    // at initialization and resize.
    canvas->cb_fill_command_buffer = fill_command_buffer;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "default.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "default.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));

    // GLSL: layout (location = 0) in vec3 pos;
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));

    // GLSL: layout (location = 1) in vec4 color;
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));

    // Resource layout. None here, but one could add uniform buffers, textures...
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);

    // Create the graphics pipeline.
    pipeline = vky_create_graphics_pipeline(
        canvas,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // triangles, 3 vertices per primitive
        shaders, vertex_layout, resource_layout, {true});

    // Create the vertex buffer.
    VkDeviceSize size = 3 * sizeof(VkyDefaultVertex); // 3 vertices
    // Typically one uses one giant vertex buffer for the whole application and allocates parts of
    // the buffer to different visuals.
    VkyBuffer buffer = vky_create_buffer(
        canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, (VkMemoryPropertyFlagBits)0);
    vertex_buffer = vky_allocate_buffer(&buffer, size);

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0},
         {1, 0, 0, 1}}, // vec3 pos, two uint8 bytes for the color (using the colormap system)
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vky_upload_buffer(vertex_buffer, 0, size, data);


    if (SHOW_CANVAS)
    {
        // ImGui Canvas.
        imcanvas = vky_imgui_canvas_create(canvas->gpu, 300, 300);
        imcanvas->cb_fill_command_buffer = fill_command_buffer;
        imtexture = vky_imgui_image_from_canvas(imcanvas);
        vky_imgui_canvas_init(imcanvas);
    }
    else
    {
        // Texture.
        VkyTextureParams tex_params = VKY_TEXTURE_PARAMS_NEAREST(2, 2, 1);
        VkyTexture texture = vky_create_texture(canvas->gpu, &tex_params);
        uint8_t pixels[4 * 4] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 0, 255};
        vky_upload_texture(&texture, pixels);
        imtexture = vky_imgui_image_from_texture(texture);
    }
    canvas->cb_fill_live_command_buffer = fill_live_command_buffer;

    // Main loop.
    vky_run_app(app);

    // Destroy the resources.
    vky_destroy_buffer(&buffer);
    vky_destroy_vertex_layout(&pipeline.vertex_layout);
    vky_destroy_resource_layout(&pipeline.resource_layout);
    vky_destroy_shaders(&pipeline.shaders);
    vky_destroy_graphics_pipeline(&pipeline);

    if (SHOW_CANVAS)
    {
        vky_destroy_canvas(imcanvas);
        vkDestroySampler(canvas->gpu->device, imtexture.texture.sampler, NULL);
        free(imcanvas);
    }
    else
    {
        vky_destroy_texture(&imtexture.texture);
    }

    vky_imgui_destroy();

    // Destroy the app and canvas.
    vky_destroy_app(app);
    return 0;
}
