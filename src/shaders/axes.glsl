layout (binding = 2) uniform AxesParams {
    vec4 margins;
    vec4 linewidths;
    mat4 colors;
    vec4 user_linewidths;
    mat4 user_colors;
    vec2 tick_lengths;
    int cap;
} params;
