#include "common.wgsl"
#include "camera.wgsl"
#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) center_view: vec4f,
    @location(2) radius: f32,
    @location(3) sprite_scale: f32,
    @location(4) coord: vec2f,
}

struct FragmentOut {
    @builtin(frag_depth) depth: f32,
    @location(0) color: vec4f,
}

fn coverage_threshold(position: vec4f) -> f32 {
    return fract(52.9829189 * fract(0.06711056 * position.x + 0.00583715 * position.y));
}

fn project_depth(view_position: vec4f) -> vec4f {
    var clip = mvp.proj * view_position;
    clip.y = -clip.y;
    clip.z = 0.5 * (clip.z + clip.w);
    return clip;
}

fn raycast_sphere(coord: vec2f, center_view: vec4f, radius: f32) -> vec4f {
    let ortho = abs(mvp.proj[3][3]) > 0.5;
    var ro = vec3f(0.0);
    var rd = vec3f(0.0, 0.0, -1.0);
    let plane_point = center_view.xyz + vec3f(coord.x * radius, coord.y * radius, 0.0);
    if (ortho) {
        ro = plane_point + vec3f(0.0, 0.0, radius);
    } else {
        rd = normalize(plane_point);
    }

    let oc = ro - center_view.xyz;
    let b = dot(oc, rd);
    let c = dot(oc, oc) - radius * radius;
    let h = b * b - c;
    if (h < 0.0) {
        return vec4f(0.0);
    }
    let t = -b - sqrt(h);
    if (t <= 0.0) {
        return vec4f(0.0);
    }
    return vec4f(ro + t * rd, 1.0);
}

fn view_to_world_position(view_position: vec3f) -> vec3f {
    let linear = mat3x3f(mvp.view[0].xyz, mvp.view[1].xyz, mvp.view[2].xyz);
    let inv_linear = dvz_inverse_mat3x3f(linear);
    return inv_linear * (view_position - mvp.view[3].xyz);
}

fn view_to_world_direction(view_direction: vec3f) -> vec3f {
    let linear = mat3x3f(mvp.view[0].xyz, mvp.view[1].xyz, mvp.view[2].xyz);
    return dvz_inverse_mat3x3f(linear) * view_direction;
}

@fragment
fn main(input: FragmentIn, @builtin(position) frag_position: vec4f) -> FragmentOut {
    let coord = vec2f(input.coord.x, -input.coord.y);
    let dist = length(coord);
    let coverage = clamp((1.0 - dist) / max(fwidth(dist), 1e-6) + 0.5, 0.0, 1.0);
    if (coverage <= coverage_threshold(frag_position)) {
        discard;
    }

    let surface_coord = select(coord, coord / max(dist, 1e-6), dist > 1.0);
    let edge = 1.0 - dot(surface_coord, surface_coord);
    var normal_view = normalize(vec3f(surface_coord, sqrt(max(edge, 0.0))));
    var surface_view = input.center_view + vec4f(normal_view * input.radius, 0.0);
    let mode = i32(material.depth_cue_extra.w + 0.5);
    if (mode == 1) {
        surface_view = raycast_sphere(surface_coord, input.center_view, input.radius);
        if (surface_view.w == 0.0) {
            discard;
        }
        normal_view = normalize(surface_view.xyz - input.center_view.xyz);
    }

    let depth_clip = project_depth(surface_view);
    let depth = clamp(depth_clip.z / max(abs(depth_clip.w), 1e-6), 0.0, 1.0);
    let world_position = view_to_world_position(surface_view.xyz);
    let camera_position = camera_position_from_view();
    let normal_world = normalize(view_to_world_direction(normal_view));
    let shaded = evaluate_scene_material(input.color, normal_world, world_position, camera_position);
    let cue = vec3f(depth, length(camera_position - world_position), length(world_position));
    let color = vec4f(apply_depth_cue(shaded.rgb, cue), shaded.a);
    if (color.a <= 0.0) {
        discard;
    }
    return FragmentOut(depth, color);
}
