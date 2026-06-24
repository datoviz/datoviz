#ifndef DVZ_COLOR_GLSL
#define DVZ_COLOR_GLSL

vec3 srgbToLinear(vec3 srgb)
{
    vec3 lo = srgb / 12.92;
    vec3 hi = pow((srgb + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(hi, lo, lessThanEqual(srgb, vec3(0.04045)));
}

vec4 semanticColorToLinear(vec4 color)
{
#ifdef DVZ_LEGACY_SRGB_BLEND
    return vec4(clamp(color.rgb, 0.0, 1.0), clamp(color.a, 0.0, 1.0));
#else
    return vec4(srgbToLinear(clamp(color.rgb, 0.0, 1.0)), clamp(color.a, 0.0, 1.0));
#endif
}

vec4 sampledTextureColorToLinear(vec4 color, vec4 params)
{
    if (params.x > 0.5) {
        return semanticColorToLinear(color);
    }
    return vec4(color.rgb, clamp(color.a, 0.0, 1.0));
}

#endif
