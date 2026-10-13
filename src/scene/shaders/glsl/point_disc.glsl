float pointDiscDistance()
{
    float size = max(fragSize, 0.0);
    float spriteSize = max(size + 4.0, 1.0);
    vec2 p = gl_PointCoord.xy - vec2(0.5);
    return length(p * spriteSize) - 0.5 * size;
}

float pointDiscCoverage(float dist)
{
    float aa = max(fwidth(dist), 1e-6);
    return 1.0 - smoothstep(-aa, aa, dist);
}
