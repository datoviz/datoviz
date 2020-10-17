#version 450
#include "common.glsl"

#define SCALING_OFF 0
#define SCALING_ON 1

#define ALPHA_SCALING_OFF 0
#define ALPHA_SCALING_ON 1

layout (std140, binding = 2) uniform RawMarkersParams {
    vec2 marker_size;
    int scaling_mode;
    int alpha_scaling_mode;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_size;

void main() {
    gl_Position = transform_pos(pos);
    out_color = get_color(color);
    // if (gl_Position.x / gl_Position.w > 1 || gl_Position.y / gl_Position.w > 1) {
    //     out_color.a = 0;
    //     return;
    // }
    float alpha = out_color.a;
    if (params.alpha_scaling_mode == ALPHA_SCALING_ON) {
        float c = .2 * (mvp.proj[0][0] + mvp.proj[1][1]);
        out_color.a = clamp(alpha * c, alpha, 1);
    }

    vec2 ms = params.marker_size;
    if (params.scaling_mode == SCALING_ON) {
        ms = ms * vec2(mvp.proj[0].x, mvp.proj[1].y);
    }
    out_size = ms;
    gl_PointSize = max(ms.x, ms.y);
}
