#version 450
#include "common.glsl"

#define VKY_RAW_PATH_MAX_PATHS 4 * 500

layout (binding = 2) uniform MultiRawPathParams {
    vec4 info;  // path_count, vertex_count_per_path, scaling, unused
    vec4 y_offsets[VKY_RAW_PATH_MAX_PATHS / 4];  // needed for 16 bytes alignment
    vec4 colors[VKY_RAW_PATH_MAX_PATHS];
} params;

layout (location = 0) in float value;

layout (location = 0) out vec4 out_color;
layout (location = 1) out float out_path_idx;

void main() {
    // Get the visual info.
    float path_count = params.info.x;
    float vertex_count_per_path = params.info.y;
    float scaling = params.info.z;

    // Path index.
    uint path_idx = gl_VertexIndex % int(path_count);
    // Vertex index within the current path.
    int vertex_idx = int(gl_VertexIndex / path_count);

    // Retrieve the path offset and color.
    float offset = params.y_offsets[path_idx / 4][path_idx % 4];
    vec4 color = params.colors[path_idx];

    // Compute the vertex position.
    float x = -1 + 2 * float(vertex_idx) / (vertex_count_per_path - 1);
    float y = offset + value * scaling;
    vec3 pos = vec3(x, y, 0);
    gl_Position = transform_pos(pos);

    // Varyings.
    out_color = color;
    out_path_idx = float(path_idx);
}
