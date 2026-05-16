struct SceneMaterial {
    light_direction: vec4f,
    params: vec4f,
    depth_cue: vec4f,
    depth_cue_color: vec4f,
}

@group(1) @binding(0) var<uniform> material: SceneMaterial;

fn depth_cue_factor(depth: f32) -> f32 {
    let strength = clamp(material.depth_cue.z, 0.0, 1.0);
    let mode = i32(material.depth_cue.w + 0.5);
    if (mode == 0 || strength <= 0.0) {
        return 0.0;
    }

    let denom = max(material.depth_cue.y - material.depth_cue.x, 1e-6);
    return clamp((depth - material.depth_cue.x) / denom, 0.0, 1.0) * strength;
}

fn apply_depth_cue(rgb: vec3f, depth: f32) -> vec3f {
    let mode = i32(material.depth_cue.w + 0.5);
    let t = depth_cue_factor(depth);
    if (t <= 0.0) {
        return rgb;
    }
    if (mode == 1) {
        return mix(rgb, material.depth_cue_color.rgb, t);
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
