uvec4 unpack_32_8(uint n) {
    /* convert one uint32 into four uint8 */
    return uvec4(
        n & 0xFF,
        (n >> 8) & 0xFF,
        (n >> 16) & 0xFF,
        (n >> 24) & 0xFF
    );
}


uvec2 unpack_32_16(uint n) {
    /* convert one uint32 into two uint16 */
    return uvec2(
        256 * (n         & 0xFF)  + ((n >> 8 ) & 0xFF),
        256 * ((n >> 16) & 0xFF)  + ((n >> 24) & 0xFF)
    );
}


uvec2 unpack_16_8(uint n) {
    /* convert one uint16 into two uint8 */
    return uvec2(
        n & 0xFF,
        (n >> 8) & 0xFF
    );
}


vec4 get_color(int cmap, float x, float a) {
    // Fetch the color from the color texture.
    // NOTE: one can also `#include "colormaps.glsl"` and use `vec4 colormap(int cmap, float x)`
    // in order to compute colormaps directly in the shader rather than fetching it from a texture.
    // This works only for a small subset of colormaps. This prevents visual artifacts when
    // the value x is itself sampled from another data texture.
    vec4 color = texture(color_texture, vec2(x, cmap / 255.0));
    color.a = a;
    return color;
}


vec4 get_color(vec4 color) {
    // no-op when the color is already in RGBA format (4 bytes of uint8 transformed to float by Vulkan via FORMAT_UNORM)
    return color;
}


vec4 rgb2hsv(vec4 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec4(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x, c.a);
}


vec4 hsv2rgb(vec4 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y), c.a);
}
