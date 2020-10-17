#version 450

layout (location = 0) in vec4 in_color;
layout (location = 1) in float in_size;

layout (location = 0) out vec4 out_color;

vec3 compute_distance(float distance) {
    vec4 frag_color;
    float t = -1;
    float border_distance = abs(distance) - t;
    float alpha = border_distance;
    alpha = exp(-alpha * alpha);
    return vec3(distance, border_distance, alpha);
}

vec4 filled(float distance, vec4 bg_color) {
    vec3 dis = compute_distance(distance);
    vec4 frag_color;
    if (dis.y < 0.0)
        frag_color = bg_color;
    else if (dis.x < 0.0)
        frag_color = bg_color;
    else
        frag_color = vec4(bg_color.rgb, dis.z * bg_color.a);
    return frag_color;
}

void main()
{
    vec2 p = 2 * (gl_PointCoord - vec2(.5, .5)) * (in_size + 2);
    out_color = filled(length(p) - .5 * in_size, in_color);
    bool do_discard = out_color.a < .01;// || length(p) > 1;
    if (do_discard)
        discard;
}
