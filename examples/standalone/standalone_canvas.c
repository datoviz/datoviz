/*************************************************************************************************/
/*  Example of a standalone application using the vklite API in a canvas.                        */
/*  This script opens a live canvas with a triangle rendered on the GPU.                         */
/*************************************************************************************************/

/// Import the library public header.
#include <datoviz/datoviz.h>

// Objects we'll need in the refill callback.
// NOTE: Using static global variables in production code is bad practice.
static DvzBufferRegions vertex_buffer;
static DvzGraphics graphics;
static DvzBindings bindings;

// Refill callback. This function is called by the canvas whenever it needs to recreate its command
// buffers, for example when initializing the canvas, and when resizing it.
static void _triangle_refill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);

    // The callback is passed an event object with a list of command buffers to recreate. In
    // this particular example, we just use a single command buffer.
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    // There is by default just one command buffer, linked to the render queue.
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);

    // This is the current swapchain image index for which we need to refill the command buffer.
    // There is one command buffer per swapchain image, so whenever a command buffer refill is
    // needed, this callback function is called three times for example, if using triple buffering.
    uint32_t idx = ev.u.rf.img_idx;

    // We begin recording the command buffer here.
    dvz_cmd_begin(cmds, idx);

    // We begin the default render pass.
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);

    // We set the viewport to the entire framebuffer size.
    dvz_cmd_viewport(cmds, idx, canvas->viewport.viewport);

    // We bind the vertex buffer for the upcoming drawing command.
    dvz_cmd_bind_vertex_buffer(cmds, idx, vertex_buffer, 0);

    // We bind the graphics pipeline.
    dvz_cmd_bind_graphics(cmds, idx, &graphics, &bindings, 0);

    // We render 3 vertices (1 triangle).
    dvz_cmd_draw(cmds, idx, 0, 3);

    // End of the render pass and command buffer.
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

// Entry point.
int main(int argc, char** argv)
{
    // We create a singleton application with a GLFW backend.
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);

    // We use the first detected GPU. The last argument is the GPU index.
    DvzGpu* gpu = dvz_gpu_best(app);

    // We create a new canvas with the size specified. The last argument is for optional flags.
    DvzCanvas* canvas = dvz_canvas(gpu, 1024, 768, 0);

    // Graphics pipeline.
    {
        // We create a new graphics pipeline.
        graphics = dvz_graphics(gpu);

        // We set the renderpass.
        dvz_graphics_renderpass(&graphics, &canvas->renderpass, 0);

        // We specify the primitive.
        dvz_graphics_topology(&graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // We set the shaders.
        dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, "triangle.vert.spv");
        dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, "triangle.frag.spv");

        // We specify the vertex structure size.
        dvz_graphics_vertex_binding(&graphics, 0, sizeof(DvzVertex));

        // We specify the two vertex attributes.
        dvz_graphics_vertex_attr(
            &graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DvzVertex, pos));
        dvz_graphics_vertex_attr(
            &graphics, 0, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(DvzVertex, color));

        // Once we've set up the graphics pipeline, we create it.
        dvz_graphics_create(&graphics);
    }

    // We create the (empty) bindings: the shaders do not necessitate any uniform or
    // texture in this example.
    bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_update(&bindings);

    // Vertex data and GPU buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    {
        // We create the GPU buffer holding the vertex data.
        // NOTE: in real applications, once should use few, even a single large vertex buffer for
        // all graphics pipelines in the application. Defining many small GPU buffers is bad
        // practice.

        // There will be three vertices for 1 triangle.
        VkDeviceSize size = 3 * sizeof(DvzVertex);
        dvz_buffer_size(&buffer, size);

        // We declare that the buffer will be used as a vertex buffer.
        dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        // The buffer should be accessible directly from the CPU in this example (bad practice in
        // real applications).
        dvz_buffer_memory(
            &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Once set up, we create the GPU buffer.
        dvz_buffer_create(&buffer);

        // We define a view (buffer region) on the entire buffer.
        vertex_buffer = dvz_buffer_regions(&buffer, 1, 0, size, 0);

        // Define the vertex data.
        // NOTE: in this example, we don't include the common.glsl shader and we use raw Vulkan
        // shaders. So the Vulkan coordinate system is used, with the y axis going down.
        DvzVertex data[3] = {
            {{-1, +1, 0}, {255, 0, 0, 255}}, // bottom left, red
            {{+1, +1, 0}, {0, 255, 0, 255}}, // bottom right, green
            {{+0, -1, 0}, {0, 0, 255, 255}}, // top, blue
        };

        // We upload the data to the GPU vertex buffer.
        dvz_buffer_regions_upload(&vertex_buffer, 0, data);
    }

    // We set the command buffer refill callback, the function that will be called whenever the
    // canvas needs to refill its command buffers. The callback generates the GPU commands to draw
    // the triangle.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _triangle_refill, NULL);

    // We run the application. The last argument is the number of frames to run, or 0 for infinite
    // loop (stop when escape is pressed or when the window is closed).
    dvz_app_run(app, 0);

    // We need to clean up all objects handled by Datoviz at the end.
    dvz_graphics_destroy(&graphics);
    dvz_buffer_destroy(&buffer);
    dvz_app_destroy(app);

    return 0;
}
