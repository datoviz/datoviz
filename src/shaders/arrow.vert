#version 450
#include "constants.glsl"
#include "common.glsl"

layout (location = 0) in vec3 P0;
layout (location = 1) in vec3 P1;
layout (location = 2) in vec4 color;
layout (location = 3) in float head;
layout (location = 4) in float linewidth;
layout (location = 5) in float arrow_type;

layout (location = 0) out vec4  out_color;
layout (location = 1) out vec2  out_texcoord;
layout (location = 2) out float out_length;
layout (location = 3) out float out_body;
layout (location = 4) out float out_head;
layout (location = 5) out float out_linewidth;
layout (location = 6) out float out_arrow_type;

void main (void)
{
    out_color = get_color(color);
    out_head = head;
    out_linewidth = linewidth;
    out_arrow_type = arrow_type;

    int index = gl_VertexIndex % 4;

    vec4 P0_ = transform_pos(P0);
    vec4 P1_ = transform_pos(P1);

    // Viewport coordinates.
    mat4 ortho = get_ortho_matrix(mvp.viewport.zw);
    vec2 p0 = (inverse(ortho) * P0_).xy;
    vec2 p1 = (inverse(ortho) * P1_).xy;

    vec2 position;
    vec2 T = p1 - p0;
    out_length = length(T);
    float w = head + 1.5 * antialias;
    out_body = 2*out_length;
    T = w * normalize(T);
    float z;
    if (index < 0.5) {
       position = vec2(p0.x - T.y - T.x, p0.y + T.x - T.y);
       out_texcoord = vec2(-w, +w);
       z = P0.z;
    }
    else if (index < 1.5) {
       position = vec2(p0.x + T.y - T.x, p0.y - T.x - T.y);
       out_texcoord = vec2(-w, -w);
       z = P0.z;
    }
    else if (index < 2.5) {
       position = vec2(p1.x + T.y + T.x, p1.y - T.x + T.y);
       out_texcoord = vec2(out_length + w, -w);
       z = P1.z;
    }
    else {
       position = vec2(p1.x - T.y + T.x, p1.y + T.x + T.y);
       out_texcoord = vec2(out_length + w, +w);
       z = P1.z;
    }

    gl_Position = ortho * vec4(position, z, 1.0);

}
