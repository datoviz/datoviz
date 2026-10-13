vec4 pointStyleSurface()
{
    float dist = pointDiscDistance();
    float aa = max(fwidth(dist), 1e-6);
    float outer = pointDiscCoverage(dist);
    if (outer <= 0.0)
        discard;

    float lineWidth = max(material.params.x, 0.0);
    int aspect = int(material.params.y + 0.5);
    bool filled = aspect == 0 || aspect == 2;
    bool stroke = (aspect == 1 || aspect == 2) && lineWidth > 0.0;
    float strokeWidth = stroke ? lineWidth : 0.0;
    float edgeMix = stroke ? smoothstep(-aa, aa, dist + strokeWidth) : 0.0;
    float fillMask = filled ? 1.0 - edgeMix : 0.0;
    float strokeMask = stroke ? edgeMix : 0.0;
    float coverage = outer * max(fillMask, strokeMask);
    if (coverage <= 0.0)
        discard;

    vec4 edgeColor = semanticColorToLinear(material.baseColorFactor);
    vec4 linearColor = semanticColorToLinear(fragColor);
    vec4 color = filled ? mix(linearColor, edgeColor, strokeMask) : edgeColor;
    vec4 surface = vec4(color.rgb, color.a * coverage);
    if (surface.a <= 0.0)
        discard;
    return surface;
}
