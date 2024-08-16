#version 450
#include "common.glsl"
#include "params_image.glsl"

// Specialization constants.
layout(constant_id = 0) const int SIZE_NDC = 0;
layout(constant_id = 1) const int RESCALE = 0;

// Attributes.
layout(location = 0) in vec3 pos;    // in NDC
layout(location = 1) in vec2 size;   // in pixels
layout(location = 2) in vec2 anchor; // in relative coordinates
layout(location = 3) in vec2 uv;     // in texel coordinates

// Varyings.
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_size; // w, h, zoom

vec2 ds[6] = {{0, 0}, {0, +1}, {+1, +1}, {+1, +1}, {+1, 0}, {0, 0}};

void main()
{
    vec4 tr = transform(pos);

    int idx = gl_VertexIndex % 6;
    vec2 d = size * (ds[idx] - anchor);
    float zoom = 1;
    if (RESCALE == 1)
    {
        zoom = total_zoom();
        d *= zoom;
    }
    tr.xy += (SIZE_NDC == 0 ? d * 2. / viewport.size : d);

    gl_Position = tr;

    out_uv = uv;
    out_size.xy = size * zoom;
    out_size.z = zoom;
}
