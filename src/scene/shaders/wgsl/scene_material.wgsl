#include "color.wgsl"

struct SceneMaterial {
    light_direction: vec4f,
    params: vec4f,
    model: vec4f,
    base_color_factor: vec4f,
    standard_params: vec4f,
    emissive_rim: vec4f,
    depth_cue: vec4f,
    depth_cue_color: vec4f,
    depth_cue_extra: vec4f,
}

@group(1) @binding(0) var<uniform> material: SceneMaterial;

fn evaluate_scene_material(
    item_color: vec4f,
    normal: vec3f,
    world_position: vec3f,
    camera_position: vec3f,
) -> vec4f {
    let model = i32(material.model.x + 0.5);
    let opacity = clamp(material.model.y, 0.0, 1.0);
    let linear_item_color = semantic_color_to_linear(item_color);
    let linear_base_color = semantic_color_to_linear(material.base_color_factor);
    let emissive = srgb_to_linear(material.emissive_rim.rgb);
    let base = linear_item_color.rgb * linear_base_color.rgb;
    let alpha = linear_item_color.a * linear_base_color.a * opacity;
    if (model == 0) {
        return vec4f(clamp(base + emissive, vec3f(0.0), vec3f(1.0)), alpha);
    }

    let n = normalize(normal);
    let l = normalize(material.light_direction.xyz);
    let v = normalize(camera_position - world_position);
    let h = normalize(l + v);
    let lambert = max(dot(n, l), 0.0);
    if (model == 2) {
        let roughness = clamp(material.standard_params.x, 0.0, 1.0);
        let specular_strength = max(material.standard_params.y, 0.0);
        let metallic = clamp(material.standard_params.z, 0.0, 1.0);
        let rim_strength = max(material.standard_params.w, 0.0);
        let shininess = max(1.0, 128.0 * (1.0 - roughness) + 1.0);
        let specular = pow(max(dot(n, h), 0.0), shininess) * specular_strength;
        let rim = pow(1.0 - max(dot(n, v), 0.0), 2.0) * rim_strength;
        let diffuse = base * (0.04 + (1.0 - metallic) * lambert);
        let rgb = diffuse + vec3f(specular + rim) + emissive;
        return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), alpha);
    }

    let specular = pow(max(dot(n, h), 0.0), max(material.params.w, 1.0));
    let rgb = base * (material.params.x + material.params.y * lambert) +
        vec3f(material.params.z * specular);
    return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), alpha);
}

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
        return mix(rgb, srgb_to_linear(material.depth_cue_color.rgb), t);
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
