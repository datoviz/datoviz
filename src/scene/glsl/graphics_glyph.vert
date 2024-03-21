#version 450
#include "common.glsl"
#include "params_glyph.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 axis;
layout(location = 2) in vec2 size;
layout(location = 3) in vec2 anchor;
layout(location = 4) in vec2 shift;
layout(location = 5) in vec2 uv;
layout(location = 6) in float angle;
layout(location = 7) in vec4 color;
layout(location = 8) in float group_size; // width, in pixels of the group this vertex belongs to

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

int dxs[4] = {0, 1, 1, 0};
int dys[4] = {0, 0, 1, 1};

void main()
{

    // Which vertex within the triangle strip forming the rectangle.
    int idx = gl_VertexIndex % 4;

    // Rectangle vertex displacement (one glyph = one rectangle = 6 vertices)
    float dx = size.x * dxs[idx];
    float dy = size.y * dys[idx];

    // Shift in pixels.
    vec2 trans = vec2(dx, dy);
    trans += shift;

    // NOTE: the x anchor is relative to the group size
    trans -= anchor * vec2(group_size != 0 ? group_size : size.x, size.y);

    // mat4 mvp = mvp.proj * mvp.view * mvp.model;
    mat4 rot = get_rotation_matrix(axis, angle);
    mat4 rot_inv = inverse(rot);
    mat4 tra = get_translation_matrix(trans);

    // TODO: store in a uniform for optimization
    mat4 ortho = get_ortho_matrix();
    mat4 ortho_inv = inverse(ortho);

    // NOTE: manual transform.
    vec4 tr = transform_mvp(pos);
    tr = transform_fixed(tr, pos);
    // NOTE: custom rotation, this need to be improved
    tr = ortho * rot * tra * rot_inv * ortho_inv * tr;
    tr = transform_margins(tr);
    tr = to_vulkan(tr);
    // HACK: without this the z is negative and clipped
    tr.z = 0;

    gl_Position = tr;
    // gl_PointSize = 20; // DEBUG

    // Varying.
    out_uv = uv;
    out_color = color;
}
