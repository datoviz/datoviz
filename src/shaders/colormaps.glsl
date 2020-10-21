#define CLAMP x = clamp(x, 0, 1);

#define VKY_CMAP_BINARY 0
#define VKY_CMAP_HSV 1
#define VKY_CMAP_CIVIDIS 2
#define VKY_CMAP_INFERNO 3
#define VKY_CMAP_MAGMA 4
#define VKY_CMAP_PLASMA 5
#define VKY_CMAP_VIRIDIS 6
#define VKY_CMAP_AUTUMN 25
#define VKY_CMAP_BONE 26
#define VKY_CMAP_COOL 27
#define VKY_CMAP_COPPER 28
#define VKY_CMAP_HOT 31
#define VKY_CMAP_SPRING 33
#define VKY_CMAP_SUMMER 34
#define VKY_CMAP_WINTER 35
#define VKY_CMAP_JET 61



// The following colormaps come from (quintic approximation):
// https://www.shadertoy.com/view/XtGGzG
vec4 viridis(float x)
{
    CLAMP
	vec4 x1 = vec4(1.0, x, x * x, x * x * x); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return vec4(
		dot(x1.xyzw, vec4(+0.280268003, -0.143510503, +2.225793877, -14.815088879)) + dot(x2.xy, vec2(+25.212752309, -11.772589584)),
		dot(x1.xyzw, vec4(-0.002117546, +1.617109353, -1.909305070, +2.701152864)) + dot(x2.xy, vec2(-1.685288385, +0.178738871)),
		dot(x1.xyzw, vec4(+0.300805501, +2.614650302, -12.019139090, +28.933559110)) + dot(x2.xy, vec2(-33.491294770, +13.762053843)), 1);
}


vec4 inferno(float x)
{
    CLAMP
	vec4 x1 = vec4( 1.0, x, x * x, x * x * x ); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return vec4(
		dot(x1.xyzw, vec4(-0.027780558, +1.228188385, +0.278906882, +3.892783760 )) + dot(x2.xy, vec2(-8.490712758, +4.069046086)),
		dot(x1.xyzw, vec4(+0.014065206, +0.015360518, +1.605395918, -4.821108251 )) + dot(x2.xy, vec2(+8.389314011, -4.193858954)),
		dot(x1.xyzw, vec4(-0.019628385, +3.122510347, -5.893222355, +2.798380308 )) + dot(x2.xy, vec2(-3.608884658, +4.324996022)), 1);
}


vec4 magma(float x)
{
    CLAMP
	vec4 x1 = vec4(1.0, x, x * x, x * x * x); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return vec4(
		dot(x1.xyzw, vec4(-0.023226960, +1.087154378, -0.109964741, +6.333665763)) + dot(x2.xy, vec2(-11.640596589, +5.33762535)),
		dot(x1.xyzw, vec4(+0.010680993, +0.176613780, +1.638227448, -6.743522237)) + dot(x2.xy, vec2(+11.426396979, -5.52323637)),
		dot(x1.xyzw, vec4(-0.008260782, +2.244286052, +3.005587601, -24.279769818)) + dot(x2.xy, vec2(+32.484310068, -12.68825973)), 1);
}


vec4 plasma(float x)
{
    CLAMP
	vec4 x1 = vec4(1.0, x, x * x, x * x * x); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return vec4(
		dot(x1.xyzw, vec4(+0.063861086, +1.992659096, -1.023901152, -0.490832805)) + dot(x2.xy, vec2(+1.308442123, -0.914547012)),
		dot(x1.xyzw, vec4(+0.049718590, -0.791144343, +2.892305078, +0.811726816)) + dot(x2.xy, vec2(-4.686502417, +2.717794514)),
		dot(x1.xyzw, vec4(+0.513275779, +1.580255060, -5.164414457, +4.559573646)) + dot(x2.xy, vec2(-1.916810682, +0.570638854)), 1);
}


// The following colormaps come from:
// https://github.com/kbinani/colormap-shaders
vec4 autumn(float x) {
    CLAMP
    return vec4(1.0, x, 0.0, 1.0);
}


vec4 winter(float x) {
    CLAMP
    return vec4(0.0, x, clamp(-0.5 * x + 1.0, 0.0, 1.0), 1.0);
}


vec4 summer(float x) {
    CLAMP
    return vec4(x, clamp(0.5 * x + 0.5, 0.0, 1.0), 0.4, 1.0);
}


vec4 spring(float x) {
    CLAMP
    return vec4(1.0, x, clamp(1.0 - x, 0.0, 1.0), 1.0);
}


vec4 copper(float x) {
    float r = clamp(80.0 / 63.0 * x + 5.0 / 252.0, 0.0, 1.0);
    float g = clamp(0.7936 * x - 0.0124, 0.0, 1.0);
    float b = clamp(796.0 / 1575.0 * x + 199.0 / 25200.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}


vec4 hot(float x) {
    float r = clamp(8.0 / 3.0 * x, 0.0, 1.0);
    float g = clamp(8.0 / 3.0 * x - 1.0, 0.0, 1.0);
    float b = clamp(4.0 * x - 3.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}


vec4 cool(float x) {
    float r = clamp((1.0 + 1.0 / 63.0) * x - 1.0 / 63.0, 0.0, 1.0);
    float g = clamp(-(1.0 + 1.0 / 63.0) * x + (1.0 + 1.0 / 63.0), 0.0, 1.0);
    float b = 1.0;
    return vec4(r, g, b, 1.0);
}



float _hsv_red(float x) {
    if (x < 0.5) return -6.0 * x + 67.0 / 32.0;
    else return 6.0 * x - 79.0 / 16.0;
}
float _hsv_green(float x) {
    if (x < 0.4) return 6.0 * x - 3.0 / 32.0;
    else return -6.0 * x + 79.0 / 16.0;
}
float _hsv_blue(float x) {
    if (x < 0.7) return 6.0 * x - 67.0 / 32.0;
    else return -6.0 * x + 195.0 / 32.0;
}
vec4 hsv(float x) {
    CLAMP
    float r = clamp(_hsv_red(x), 0.0, 1.0);
    float g = clamp(_hsv_green(x), 0.0, 1.0);
    float b = clamp(_hsv_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}



float _bone_red(float x) {
    if (x < 0.75) return 8.0 / 9.0 * x - (13.0 + 8.0 / 9.0) / 1000.0;
    else return (13.0 + 8.0 / 9.0) / 10.0 * x - (3.0 + 8.0 / 9.0) / 10.0;
}
float _bone_green(float x) {
    if (x <= 0.375) return 8.0 / 9.0 * x - (13.0 + 8.0 / 9.0) / 1000.0;
    else if (x <= 0.75) return (1.0 + 2.0 / 9.0) * x - (13.0 + 8.0 / 9.0) / 100.0;
    else return 8.0 / 9.0 * x + 1.0 / 9.0;
}
float _bone_blue(float x) {
    if (x <= 0.375) return (1.0 + 2.0 / 9.0) * x - (13.0 + 8.0 / 9.0) / 1000.0;
    else return 8.0 / 9.0 * x + 1.0 / 9.0;
}
vec4 bone(float x) {
    CLAMP
    float r = clamp(_bone_red(x), 0.0, 1.0);
    float g = clamp(_bone_green(x), 0.0, 1.0);
    float b = clamp(_bone_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}



float _jet_red(float x) {
    if (x < 0.7) return 4.0 * x - 1.5;
    else return -4.0 * x + 4.5;
}
float _jet_green(float x) {
    if (x < 0.5) return 4.0 * x - 0.5;
    else return -4.0 * x + 3.5;
}
float _jet_blue(float x) {
    if (x < 0.3) return 4.0 * x + 0.5;
    else return -4.0 * x + 2.5;
}
vec4 jet(float x) {
    CLAMP
    float r = clamp(_jet_red(x), 0.0, 1.0);
    float g = clamp(_jet_green(x), 0.0, 1.0);
    float b = clamp(_jet_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}



vec4 colormap(int cmap, float x) {
    CLAMP
    if (cmap == VKY_CMAP_HSV) return hsv(x);
    else if (cmap == VKY_CMAP_INFERNO) return inferno(x);
    else if (cmap == VKY_CMAP_MAGMA) return magma(x);
    else if (cmap == VKY_CMAP_PLASMA) return plasma(x);
    else if (cmap == VKY_CMAP_VIRIDIS) return viridis(x);
    else if (cmap == VKY_CMAP_AUTUMN) return autumn(x);
    else if (cmap == VKY_CMAP_BONE) return bone(x);
    else if (cmap == VKY_CMAP_COOL) return cool(x);
    else if (cmap == VKY_CMAP_COPPER) return copper(x);
    else if (cmap == VKY_CMAP_HOT) return hot(x);
    else if (cmap == VKY_CMAP_SPRING) return spring(x);
    else if (cmap == VKY_CMAP_SUMMER) return summer(x);
    else if (cmap == VKY_CMAP_WINTER) return winter(x);
    else if (cmap == VKY_CMAP_JET) return jet(x);
    // Fallback to texture sampling from the colormap texture.
    // NOTE: this may cause visual artifacts when the value x is itself sampled from a texture.
    // Computing the colormap directly without fetching the color from a color texture
    // does not result in such visual artifacts.
    else return get_color(cmap, x, 1);
}
