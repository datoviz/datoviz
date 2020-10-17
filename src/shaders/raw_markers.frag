#version 450

layout (location = 0) in vec4 in_color;
layout (location = 1) in vec2 in_size;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = in_color;
    // if (out_color.a < .001)
    //     discard;
    vec2 ms = in_size;
    vec2 p = gl_PointCoord * max(ms.x, ms.y);
    bool do_discard = (
        ((ms.x < ms.y) && (abs(p.x - .5 * ms.y) > .5 * ms.x)) ||
        ((ms.x > ms.y) && (abs(p.y - .5 * ms.x) > .5 * ms.y))
    );
    if (do_discard)
        discard;
}
