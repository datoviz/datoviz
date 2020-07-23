#include "constants.glsl"

vec3 compute_distance(float distance, float linewidth) {
    vec4 frag_color;
    float t = linewidth / 2.0 - antialias;
    float signed_distance = distance;
    float border_distance = abs(signed_distance) - t;
    float alpha = border_distance / antialias;
    alpha = exp(-alpha * alpha);
    return vec3(signed_distance, border_distance, alpha);
}


vec4 filled(float distance, float linewidth, vec4 bg_color) {
    vec3 dis = compute_distance(distance, linewidth);
    vec4 frag_color;

    if (dis.y < 0.0)
        frag_color = bg_color;
    else if (dis.x < 0.0)
        frag_color = bg_color;
    else
        frag_color = vec4(bg_color.rgb, dis.z * bg_color.a);

    return frag_color;
}


vec4 stroke(float distance, float linewidth, vec4 fg_color) {
    vec3 dis = compute_distance(distance, linewidth);
    vec4 frag_color;

    if (dis.y < 0.0)
        frag_color = fg_color;
    else
        frag_color = vec4(fg_color.rgb, fg_color.a * dis.z);

    return frag_color;
}


vec4 outline(float distance, float linewidth, vec4 fg_color, vec4 bg_color) {
    vec3 dis = compute_distance(distance, linewidth);
    vec4 frag_color;

    if (dis.y < 0.0)
        frag_color = fg_color;
    else if (dis.x < 0.0)
        frag_color = mix(bg_color, fg_color, sqrt(dis.z));
    else {
        if (abs(dis.x) < (linewidth/2.0 + antialias)) {
            frag_color = vec4(fg_color.rgb, fg_color.a * dis.z);
        } else {
            discard;
        }
    }
    return frag_color;
}


vec4 cap(int type, float dx, float dy, float linewidth, vec4 color) {
    float d = 0.0;
    dx = abs(dx);
    dy = abs(dy);
    float t = linewidth / 2.0 - antialias;

    // None
    if      (type == 0)  discard;
    // Round
    else if (type == 1)  d = sqrt(dx*dx+dy*dy);
    // Triangle out
    else if (type == 2)  d = max(abs(dy),(t+dx-abs(dy)));
    // Triangle in
    else if (type == 3)  d = (dx+abs(dy));
    // Square
    else if (type == 4)  d = max(dx,dy);
    // Butt
    else if (type == 5)  d = max(dx+t,dy);

    return stroke(d, linewidth, color);
}
