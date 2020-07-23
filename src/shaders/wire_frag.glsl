
float aastep (float threshold, float dist) {
    float afwidth = fwidth(dist) * 0.5;
    return smoothstep(threshold - afwidth, threshold + afwidth, dist);
}

vec4 add_wire(vec4 color, vec3 triangle_coords, float linewidth) {
    float d = min(min(triangle_coords.x, triangle_coords.y), triangle_coords.z);
    float edge = 1.0 - aastep(linewidth, d);
    vec3 stroke = vec3(0.0);
    color.rgb = mix(color.rgb, stroke, edge);
    color.a = 1;
    return color;
}
