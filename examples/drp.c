/*************************************************************************************************/
/*  Datoviz Rendering Protocol example                                                           */
/*************************************************************************************************/

// Imports.
#include <datoviz_protocol.h>
#include <stddef.h>

// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);
    DvzRequest req = {0};

    // Constants.
    uint32_t width = 1024;
    uint32_t height = 768;

    // Structure holding the vertex data.
    struct Vertex
    {
        vec3 pos;
        DvzColor color;
    };


    // Create a canvas.
    req = dvz_create_canvas(batch, width, height, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId canvas_id = req.id;


    // Create a custom graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_CUSTOM, 0);
    DvzId graphics_id = req.id;


    // Vertex shader.
    const char* vertex_glsl = //
        "#version 450\n"
        "\n"
        "layout(location = 0) in vec3 pos;\n"
        "layout(location = 1) in vec4 color;\n"
        "layout(location = 0) out vec4 out_color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(pos, 1.0);\n"
        "    out_color = color;\n"
        "}\n";
    req = dvz_create_glsl(batch, DVZ_SHADER_VERTEX, vertex_glsl);

    // Assign the shader to the graphics pipe.
    DvzId vertex_id = req.id;
    dvz_set_shader(batch, graphics_id, vertex_id);


    // Fragment shader.
    const char* fragment_glsl = //
        "#version 450\n"
        "\n"
        "layout(location = 0) in vec4 in_color;\n"
        "layout(location = 0) out vec4 out_color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    out_color = in_color;\n"
        "}\n";
    req = dvz_create_glsl(batch, DVZ_SHADER_FRAGMENT, fragment_glsl);

    // Assign the shader to the graphics pipe.
    DvzId fragment_id = req.id;
    dvz_set_shader(batch, graphics_id, fragment_id);


    // Primitive topology.
    dvz_set_primitive(batch, graphics_id, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Polygon mode.
    dvz_set_polygon(batch, graphics_id, DVZ_POLYGON_MODE_FILL);


    // Vertex binding.
    dvz_set_vertex(batch, graphics_id, 0, sizeof(struct Vertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Vertex attrs.
    dvz_set_attr(
        batch, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, pos));
    dvz_set_attr(
        batch, graphics_id, 0, 1, DVZ_FORMAT_R8G8B8A8_UNORM, offsetof(struct Vertex, color));


    // Create the vertex buffer dat.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(struct Vertex), 0);
    DvzId dat_id = req.id;

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_bind_vertex(batch, graphics_id, 0, dat_id, 0);

    // Upload the triangle data.
    struct Vertex data[] = {
        {{-1, +1, 0}, {255, 0, 0, 255}},
        {{+1, +1, 0}, {0, 255, 0, 255}},
        {{+0, -1, 0}, {0, 0, 255, 255}},
    };
    req = dvz_upload_dat(batch, dat_id, 0, sizeof(data), data, 0);


    // Commands.
    dvz_record_begin(batch, canvas_id);
    dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, canvas_id);


    // Run the application.
    dvz_app_run(app, 0);

    // Cleanup.
    dvz_app_destroy(app);

    return 0;
}
