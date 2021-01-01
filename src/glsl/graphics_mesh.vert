#version 450
#include "common.glsl"

vec3 triangle_coords() {
    // Triangle barycentric coordinates available in the fragment shader.
    int id = gl_VertexIndex % 3;
    vec3 triangle_coords = vec3(0);
    if (id == 0) triangle_coords.x = 1;
    else if (id == 1) triangle_coords.y = 1;
    else if (id == 2) triangle_coords.z = 1;
    return triangle_coords;
}

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;
// layout (location = 3) out vec3 out_triangle;

void main() {
    gl_Position = transform(pos);

    out_pos = ((mvp.model * vec4(pos, 1.0))).xyz;
    out_normal = ((transpose(inverse(mvp.model)) * vec4(normal, 1.0))).xyz;

    out_uv = uv;
    // out_triangle = triangle_coords();
}
