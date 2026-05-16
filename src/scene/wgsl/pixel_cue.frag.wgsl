struct PrimitiveShading {
    light_direction: vec4f,
    params: vec4f,
    depth_cue: vec4f,
    depth_cue_color: vec4f,
}

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) depth: f32,
}

@group(1) @binding(0) var<uniform> shading: PrimitiveShading;

fn apply_depth_cue(rgb: vec3f, depth: f32) -> vec3f {
    let strength = clamp(shading.depth_cue.z, 0.0, 1.0);
    let mode = i32(shading.depth_cue.w + 0.5);
    if (mode == 0 || strength <= 0.0) {
        return rgb;
    }

    let denom = max(shading.depth_cue.y - shading.depth_cue.x, 1e-6);
    let t = clamp((depth - shading.depth_cue.x) / denom, 0.0, 1.0) * strength;
    if (mode == 1) {
        return mix(rgb, shading.depth_cue_color.rgb, t);
    }
    if (mode == 2) {
        let luma = dot(rgb, vec3f(0.2126, 0.7152, 0.0722));
        return mix(rgb, vec3f(luma), t);
    }
    if (mode == 3) {
        return mix(rgb, vec3f(0.0), t);
    }
    return rgb;
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    return vec4f(apply_depth_cue(input.color.rgb, input.depth), input.color.a);
}
