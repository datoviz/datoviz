#include "color.wgsl"

struct SceneMaterial {
    params: vec4f,
    model: vec4f,
    base_color_factor: vec4f,
    standard_params: vec4f,
    emissive_rim: vec4f,
    limb_params: vec4f,
    depth_cue: vec4f,
    depth_cue_color: vec4f,
    depth_cue_extra: vec4f,
}

struct SceneLight {
    color_intensity: vec4f,
    direction_type: vec4f,
    position_attenuation: vec4f,
}

struct ScenePanelLights {
    active_count: vec4u,
    lights: array<SceneLight, 8>,
}

@group(1) @binding(0) var<uniform> material: SceneMaterial;
@group(1) @binding(4) var<uniform> panel_lights: ScenePanelLights;

fn scene_ambient_radiance() -> vec3f {
    var radiance = vec3f(0.0);
    for (var i = 0u; i < 8u; i++) {
        if (i >= panel_lights.active_count.x) {
            break;
        }
        let light = panel_lights.lights[i];
        if (i32(light.direction_type.w + 0.5) == 0) {
            radiance += light.color_intensity.rgb * light.color_intensity.w;
        }
    }
    return radiance;
}

fn scene_phong_direct(base: vec3f, n: vec3f, v: vec3f) -> vec3f {
    var direct = vec3f(0.0);
    for (var i = 0u; i < 8u; i++) {
        if (i >= panel_lights.active_count.x) {
            break;
        }
        let light = panel_lights.lights[i];
        if (i32(light.direction_type.w + 0.5) != 1) {
            continue;
        }
        let radiance = light.color_intensity.rgb * light.color_intensity.w;
        let l = normalize(light.direction_type.xyz);
        let h = normalize(l + v);
        let lambert = max(dot(n, l), 0.0);
        let specular = pow(max(dot(n, h), 0.0), max(material.params.w, 1.0));
        direct += radiance *
            (base * material.params.y * lambert + vec3f(material.params.z * specular));
    }
    return direct;
}

fn scene_standard_direct(base: vec3f, n: vec3f, v: vec3f) -> vec3f {
    let roughness = clamp(material.standard_params.x, 0.0, 1.0);
    let specular_strength = max(material.standard_params.y, 0.0);
    let metallic = clamp(material.standard_params.z, 0.0, 1.0);
    let shininess = max(1.0, 128.0 * (1.0 - roughness) + 1.0);
    var direct = vec3f(0.0);
    for (var i = 0u; i < 8u; i++) {
        if (i >= panel_lights.active_count.x) {
            break;
        }
        let light = panel_lights.lights[i];
        if (i32(light.direction_type.w + 0.5) != 1) {
            continue;
        }
        let radiance = light.color_intensity.rgb * light.color_intensity.w;
        let l = normalize(light.direction_type.xyz);
        let h = normalize(l + v);
        let lambert = max(dot(n, l), 0.0);
        let specular = pow(max(dot(n, h), 0.0), shininess) * specular_strength;
        direct += radiance * (base * (1.0 - metallic) * lambert + vec3f(specular));
    }
    return direct;
}

fn scene_primary_directional() -> vec3f {
    for (var i = 0u; i < 8u; i++) {
        if (i >= panel_lights.active_count.x) {
            break;
        }
        if (i32(panel_lights.lights[i].direction_type.w + 0.5) == 1) {
            return normalize(panel_lights.lights[i].direction_type.xyz);
        }
    }
    return vec3f(0.0, 0.0, 1.0);
}

fn evaluate_scene_material_linear_item_with_ambient_visibility(
    linear_item_color: vec4f,
    normal: vec3f,
    world_position: vec3f,
    camera_position: vec3f,
    ambient_visibility: f32,
) -> vec4f {
    let model = i32(material.model.x + 0.5);
    let opacity = clamp(material.model.y, 0.0, 1.0);
    let linear_base_color = semantic_color_to_linear(material.base_color_factor);
    let emissive = srgb_to_linear(material.emissive_rim.rgb);
    let base = linear_item_color.rgb * linear_base_color.rgb;
    let alpha = linear_item_color.a * linear_base_color.a * opacity;
    if (model == 0) {
        return vec4f(base + emissive, alpha);
    }

    let n = normalize(normal);
    let v = normalize(camera_position - world_position);
    if (model == 3) {
        let l = scene_primary_directional();
        let falloff = max(material.limb_params.x, 0.01);
        let sun_bias = material.limb_params.y;
        let terminator_width = max(material.limb_params.z, 1e-4);
        let night_factor = clamp(material.limb_params.w, 0.0, 1.0);
        let facing = clamp(dot(n, v), 0.0, 1.0);
        let peak_facing = 2.0 / (falloff + 2.0);
        let peak = peak_facing * peak_facing * pow(1.0 - peak_facing, falloff);
        let limb = facing * facing * pow(1.0 - facing, falloff) / max(peak, 1e-6);
        let sunlight = smoothstep(-terminator_width, terminator_width, dot(n, l) + sun_bias);
        let illumination = mix(night_factor, 1.0, sunlight);
        return vec4f(base, alpha * limb * illumination);
    }

    let visibility = clamp(ambient_visibility, 0.0, 1.0);
    let ambient_radiance = scene_ambient_radiance();
    if (model == 2) {
        let metallic = clamp(material.standard_params.z, 0.0, 1.0);
        let rim_strength = max(material.standard_params.w, 0.0);
        let rim = pow(1.0 - max(dot(n, v), 0.0), 2.0) * rim_strength;
        let indirect = base * (1.0 - metallic) * ambient_radiance;
        let rgb = emissive + scene_standard_direct(base, n, v) +
            visibility * indirect + vec3f(rim);
        return vec4f(rgb, alpha);
    }

    let indirect = base * material.params.x * ambient_radiance;
    let rgb = emissive + scene_phong_direct(base, n, v) + visibility * indirect;
    return vec4f(rgb, alpha);
}

fn evaluate_scene_material_linear_item(
    linear_item_color: vec4f,
    normal: vec3f,
    world_position: vec3f,
    camera_position: vec3f,
) -> vec4f {
    return evaluate_scene_material_linear_item_with_ambient_visibility(
        linear_item_color, normal, world_position, camera_position, 1.0);
}

fn evaluate_scene_material_with_ambient_visibility(
    item_color: vec4f,
    normal: vec3f,
    world_position: vec3f,
    camera_position: vec3f,
    ambient_visibility: f32,
) -> vec4f {
    return evaluate_scene_material_linear_item_with_ambient_visibility(
        semantic_color_to_linear(item_color), normal, world_position, camera_position,
        ambient_visibility);
}

fn evaluate_scene_material(
    item_color: vec4f,
    normal: vec3f,
    world_position: vec3f,
    camera_position: vec3f,
) -> vec4f {
    return evaluate_scene_material_with_ambient_visibility(
        item_color, normal, world_position, camera_position, 1.0);
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
