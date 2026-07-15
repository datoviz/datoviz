#version 450

layout(set = 0, binding = 0) uniform texture2D colorTex;
layout(set = 0, binding = 1) uniform texture2D depthTex;
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform EdlParams {
    mat4 invProj;
    vec4 viewport;
    vec4 params;
} edl;
layout(location = 0) out vec4 outColor;

float viewDepth(ivec2 texel, float depth)
{
    vec2 uv = (vec2(texel) + vec2(0.5) - edl.viewport.xy) /
              max(edl.viewport.zw, vec2(1.0));
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc.x, -ndc.y, depth * 2.0 - 1.0, 1.0);
    vec4 view = edl.invProj * clip;
    return max(-view.z / max(abs(view.w), 1e-6), 1e-6);
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    ivec2 extent = textureSize(sampler2D(colorTex, samp), 0);
    if (p.x < 0 || p.y < 0 || p.x >= extent.x || p.y >= extent.y)
        discard;

    vec4 color = texelFetch(sampler2D(colorTex, samp), p, 0);
    float center = texelFetch(sampler2D(depthTex, samp), p, 0).r;
    if (edl.params.w <= 0.0 || center >= 0.999999)
    {
        outColor = color;
        return;
    }

    int radius = int(clamp(round(edl.params.x), 1.0, 8.0));
    float depthScale = max(edl.params.z, 0.001);
    float centerViewDepth = viewDepth(p, center);
    ivec2 panelMin = max(ivec2(floor(edl.viewport.xy)), ivec2(0));
    ivec2 panelMax = min(
        ivec2(ceil(edl.viewport.xy + edl.viewport.zw)) - ivec2(1), extent - ivec2(1));
    ivec2 offsets[8] = ivec2[](
        ivec2(+1, 0), ivec2(-1, 0), ivec2(0, +1), ivec2(0, -1),
        ivec2(+1, +1), ivec2(-1, +1), ivec2(+1, -1), ivec2(-1, -1));

    float response = 0.0;
    for (int i = 0; i < 8; i++)
    {
        ivec2 q = clamp(p + radius * offsets[i], panelMin, panelMax);
        float neighbor = texelFetch(sampler2D(depthTex, samp), q, 0).r;
        float relativeDepth = 1.0;
        if (neighbor < 0.999999)
        {
            float neighborViewDepth = viewDepth(q, neighbor);
            relativeDepth = max(0.0, neighborViewDepth - centerViewDepth) / centerViewDepth;
        }
        response += min(relativeDepth, 1.0) * depthScale;
    }
    response *= 0.1 / 8.0;

    float shade = exp(-max(edl.params.y, 0.0) * response);
    outColor = vec4(color.rgb * shade, color.a);
}
