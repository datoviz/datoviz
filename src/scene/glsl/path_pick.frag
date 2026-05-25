#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragCoord;
layout(location = 2) in float fragLength;
layout(location = 3) in float fragLineWidth;
layout(location = 4) in float fragHasPrev;
layout(location = 5) in float fragHasNext;
layout(location = 6) in float fragBevelDistance;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform SceneMaterial {
    vec4 lightDir;
    vec4 params;
    vec4 model;
    vec4 baseColorFactor;
    vec4 standardParams;
    vec4 emissiveRim;
    vec4 depthCue;
    vec4 depthCueColor;
    vec4 depthCueExtra;
} material;

float strokeAlpha(float distance, float lineWidth)
{
    float aa = 1.0;
    float halfWidth = max(lineWidth, 0.0) * 0.5;
    return 1.0 - smoothstep(halfWidth - aa, halfWidth + aa, abs(distance));
}

float capDistance(int capType, float dx, float dy, float lineWidth)
{
    float aa = 1.0;
    float halfWidth = max(lineWidth, 0.0) * 0.5;
    float t = max(halfWidth - aa, 0.0);
    float x = abs(dx);
    float y = abs(dy);

    if (capType == 0)
        return 1e6;
    if (capType == 1)
        return length(vec2(x, y));
    if (capType == 2)
        return max(y, t + x - y);
    if (capType == 3)
        return x + y;
    if (capType == 4)
        return max(x, y);
    if (capType == 5)
        return max(x + t, y);
    return max(x + t, y);
}

void main()
{
    float distance = fragCoord.y;
    int joinType = int(round(material.params.z));
    if (fragCoord.x < 0.0)
    {
        if (!(fragHasPrev >= 0.5 && joinType == 2))
        {
            int capType = fragHasPrev < 0.5 ? int(round(material.params.x)) : joinType;
            if (fragHasPrev >= 0.5 && joinType == 0)
                capType = 5;
            distance = capDistance(capType, fragCoord.x, fragCoord.y, fragLineWidth);
        }
    }
    else if (fragCoord.x > fragLength)
    {
        if (!(fragHasNext >= 0.5 && joinType == 2))
        {
            int capType = fragHasNext < 0.5 ? int(round(material.params.y)) : joinType;
            if (fragHasNext >= 0.5 && joinType == 0)
                capType = 5;
            distance = capDistance(
                capType, fragCoord.x - fragLength, fragCoord.y, fragLineWidth);
        }
    }

    float alpha = strokeAlpha(distance, fragLineWidth);
    bool bevelOverhang = joinType == 2 &&
                         ((fragCoord.x < 0.0 && fragHasPrev >= 0.5) ||
                          (fragCoord.x > fragLength && fragHasNext >= 0.5));
    if (bevelOverhang)
        alpha *= 1.0 - smoothstep(-1.0, 1.0, fragBevelDistance);
    if (alpha <= 0.0)
        discard;
    outColor = fragColor;
}
