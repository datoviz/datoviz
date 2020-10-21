#version 450
#include "common.glsl"

layout (std140, binding = 2) uniform MarkerTransientParams {
    float local_time;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in float size;
layout (location = 3) in float half_life;
layout (location = 4) in float last_active;

layout (location = 0) out vec4 out_color;
layout (location = 1) out float out_size;

void main() {
    gl_Position = transform_pos(pos);

    float t = params.local_time;
    float k = 0.6931471805599453 / half_life; // -log(2) / half_life
    float u = exp(-k * (t - last_active));

    out_color = color;
    vec3 hsv = rgb2hsv(color).rgb;
    float value = hsv.z;

    if (t < last_active) {
        out_color.a = 0;
    }
    else {
        out_color.rgb = hsv2rgb(vec4(hsv.xy, .2 + .8 * u, color.a)).rgb;
    }

    out_size = size;
    gl_PointSize = size;
}
