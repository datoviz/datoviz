"""# Datoviz Renderer Protocol (DRP) example

Show a simple triangle using raw DRP requests.

Illustrates:

- Using the DRP API

"""

import numpy as np
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)

# Constants.
width = 1024
height = 768

# Define the Vertex dtype
vertex_dtype = np.dtype([
    ('pos', np.float32, (3,)),  # 3D position (vec3)
    ('color', np.uint8, (4,))   # RGBA color (cvec4)
])
vertex_size = vertex_dtype.itemsize
pos_offset = vertex_dtype.fields['pos'][1]
color_offset = vertex_dtype.fields['color'][1]


# Create a canvas.
req = dvz.create_canvas(batch, width, height, dvz.DEFAULT_CLEAR_COLOR, 0)
canvas_id = req.id


# Create a custom graphics.
req = dvz.create_graphics(batch, dvz.GRAPHICS_CUSTOM, 0)
graphics_id = req.id


# Vertex shader.
vertex_glsl = """
#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 out_color;

void main()
{
    gl_Position = vec4(pos, 1.0);
    out_color = color;
}
"""

req = dvz.create_glsl(
    batch, dvz.SHADER_VERTEX, dvz.S_(vertex_glsl))

# Assign the shader to the graphics pipe.
vertex_id = req.id
dvz.set_shader(batch, graphics_id, vertex_id)


# Fragment shader.
fragment_glsl = """
#version 450

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = in_color;
}
"""

req = dvz.create_glsl(
    batch, dvz.SHADER_FRAGMENT, dvz.S_(fragment_glsl))

# Assign the shader to the graphics pipe.
fragment_id = req.id
dvz.set_shader(batch, graphics_id, fragment_id)


# Primitive topology.
dvz.set_primitive(batch, graphics_id, dvz.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)

# Polygon mode.
dvz.set_polygon(batch, graphics_id, dvz.POLYGON_MODE_FILL)


# Vertex binding.
dvz.set_vertex(
    batch, graphics_id, 0, vertex_size, dvz.VERTEX_INPUT_RATE_VERTEX)

# Vertex attrs.
dvz.set_attr(batch, graphics_id, 0, 0, dvz.FORMAT_R32G32B32_SFLOAT, pos_offset)
dvz.set_attr(batch, graphics_id, 0, 1, dvz.FORMAT_R8G8B8A8_UNORM, color_offset)


# Create the vertex buffer dat.
req = dvz.create_dat(batch, dvz.BUFFER_TYPE_VERTEX, 3 * vertex_size, 0)
dat_id = req.id

# Bind the vertex buffer dat to the graphics pipe.
req = dvz.bind_vertex(batch, graphics_id, 0, dat_id, 0)

# Upload the triangle data.
data = np.array([
    ((-1, +1, 0), (255, 0, 0, 255)),
    ((+1, +1, 0), (0, 255, 0, 255)),
    ((+0, -1, 0), (0, 0, 255, 255)),
], dtype=vertex_dtype)
req = dvz.upload_dat(batch, dat_id, 0, 3 * vertex_size, dvz.A_(data), 0)


# Commands.
dvz.record_begin(batch, canvas_id)
dvz.record_viewport(
    batch, canvas_id, dvz.DEFAULT_VIEWPORT, dvz.DEFAULT_VIEWPORT)
dvz.record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1)
dvz.record_end(batch, canvas_id)


# Run the application.
dvz.app_run(app, 0)

# Cleanup.
dvz.app_destroy(app)
