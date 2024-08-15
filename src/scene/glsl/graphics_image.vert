#version 450
#include "common.glsl"

// Specialization constants.
layout(constant_id = 0) const int SIZE_NDC = 0;

// Attributes.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 size;
layout(location = 2) in vec2 anchor;
layout(location = 3) in vec2 uv;

// Varyings.
layout(location = 0) out vec2 out_uv;

vec2 ds[6] = {{0, 0}, {0, +1}, {+1, +1}, {+1, +1}, {+1, 0}, {0, 0}};

void main()
{
    vec4 tr = transform(pos);

    int idx = gl_VertexIndex % 6;
    vec2 d = size * (ds[idx] - anchor);
    tr.xy += (SIZE_NDC == 0 ? d * 2. / viewport.size : d);

    gl_Position = tr;

    out_uv = uv;
}
