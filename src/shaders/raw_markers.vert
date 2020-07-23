#version 450
#include "common.glsl"

#define SCALING_OFF 0
#define SCALING_ON 1

layout (std140, binding = 2) uniform RawMarkersParams {
    vec2 marker_size;
    int scaling_mode;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_size;

void main() {
    gl_Position = transform_pos(pos);
    out_color = get_color(color);

    vec2 ms = params.marker_size;
    if (params.scaling_mode == SCALING_ON)
        ms = ms * vec2(mvp.proj[0].x, mvp.proj[1].y);
    out_size = ms;
    gl_PointSize = max(ms.x, ms.y);
}
