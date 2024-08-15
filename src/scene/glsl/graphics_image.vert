#version 450
#include "common.glsl"

float total_zoom()
{
    // Combine the matrices: T = proj * view * model
    mat4 T = mvp.proj * mvp.view * mvp.model;

    // Extract the scaling factors along the x, y, and z axes
    float S_x = length(vec3(T[0][0], T[0][1], T[0][2]));
    float S_y = length(vec3(T[1][0], T[1][1], T[1][2]));
    float S_z = length(vec3(T[2][0], T[2][1], T[2][2]));

    // Calculate the total zoom as the average scaling factor
    float zoom = (S_x + S_y + S_z) / 3.0;

    return zoom;
}

// Specialization constants.
layout(constant_id = 0) const int SIZE_NDC = 0;
layout(constant_id = 1) const int RESCALE = 0;

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
    if (RESCALE == 1)
    {
        d *= total_zoom();
    }
    tr.xy += (SIZE_NDC == 0 ? d * 2. / viewport.size : d);

    gl_Position = tr;

    out_uv = uv;
}
