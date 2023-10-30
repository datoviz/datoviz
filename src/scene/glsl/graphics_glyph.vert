#version 450
#include "common.glsl"
#include "params_glyph.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 anchor;
layout(location = 3) in vec2 shift;
layout(location = 4) in vec2 uv;
layout(location = 5) in float angle;
layout(location = 6) in vec4 color;

// layout(location = 0) out vec2 out_uv;

int dxs[4] = {0, 1, 1, 0};
int dys[4] = {0, 0, 1, 1};

void main()
{
    // Which vertex within the triangle strip forming the rectangle.
    int idx = gl_VertexIndex % 4;

    // Rectangle vertex displacement (one glyph = one rectangle = 6 vertices)
    float dx = params.size.x * dxs[idx];
    float dy = params.size.y * dys[idx];

    gl_Position = transform(pos);
    gl_Position.x += dx;
    gl_Position.y += dy;

    gl_PointSize = 5;
    // out_uv = uv;
}
