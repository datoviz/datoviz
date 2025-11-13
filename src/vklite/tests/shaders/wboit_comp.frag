#version 450

layout(binding = 0) uniform sampler2D u_accumColor;
layout(binding = 1) uniform sampler2D u_accumWeight;

layout(location = 0) out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_accumColor, 0));

    vec4 accumColor = texture(u_accumColor, uv);
    float accumWeight = texture(u_accumWeight, uv).r;

    float w = max(accumWeight, 1e-5);

    // Divide premultiplied accumulated color by weight.
    vec3 finalColor = accumColor.rgb / w;

    out_color = vec4(finalColor, 1.0);
}
