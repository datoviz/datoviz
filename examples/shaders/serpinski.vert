#version 450
#include "../../src/shaders/common.glsl"


layout (binding = 1) uniform FractalParams {
    float point_size;
    mat4 control_points;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;

float rand(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
    vec2 p = pos.xy;

    int niter = 100;
    int N = 3;

    for (int iter = 0; iter < niter; iter++) {
        int i = int(rand(vec2(gl_VertexIndex, iter)) * N);

        vec2 cp = vec2(0, 0);
        cp.x = params.control_points[i / 2][2 * (i % 2) + 0];
        cp.y = params.control_points[i / 2][2 * (i % 2) + 1];

        vec2 newp = .5 * (p + cp);
        p = newp;
    }

    gl_Position = transform_pos(vec3(p, pos.z));
    out_color = color;

    gl_PointSize = params.point_size;
}
