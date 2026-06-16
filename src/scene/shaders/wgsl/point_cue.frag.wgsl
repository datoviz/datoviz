#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) cue: vec3f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = length(input.corner);
    let aa = max(fwidth(dist), 1e-6);
    let alpha = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, dist);
    if (alpha <= 0.0) {
        discard;
    }
    let color = semantic_color_to_linear(input.color);
    return vec4f(apply_depth_cue(color.rgb, input.cue), color.a * alpha);
}
