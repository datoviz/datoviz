struct SceneMaterial {
    light_direction: vec4f,
    params: vec4f,
    depth_cue: vec4f,
    depth_cue_color: vec4f,
    depth_cue_extra: vec4f,
}

@group(1) @binding(0) var<uniform> material: SceneMaterial;

fn depth_cue_coordinate(cue: vec3f) -> f32 {
    let metric = i32(material.depth_cue_extra.x + 0.5);
    if (metric == 1) {
        return cue.y;
    }
    if (metric == 2) {
        return cue.z;
    }
    return cue.x;
}

fn depth_cue_factor(cue: vec3f) -> f32 {
    let strength = clamp(material.depth_cue.z, 0.0, 1.0);
    let mode = i32(material.depth_cue.w + 0.5);
    if (mode == 0 || strength <= 0.0) {
        return 0.0;
    }

    let denom = max(material.depth_cue.y - material.depth_cue.x, 1e-6);
    let coord = depth_cue_coordinate(cue);
    var t = clamp((coord - material.depth_cue.x) / denom, 0.0, 1.0);
    let falloff = i32(material.depth_cue_extra.y + 0.5);
    if (falloff == 1) {
        let density = max(material.depth_cue_extra.z, 1e-6);
        t = (1.0 - exp(-density * t)) / max(1.0 - exp(-density), 1e-6);
    }
    return t * strength;
}

fn apply_depth_cue(rgb: vec3f, cue: vec3f) -> vec3f {
    let mode = i32(material.depth_cue.w + 0.5);
    let t = depth_cue_factor(cue);
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
