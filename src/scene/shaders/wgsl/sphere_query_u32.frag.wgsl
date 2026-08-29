struct MVP {
    model: mat4x4f,
    view: mat4x4f,
    proj: mat4x4f,
    time: f32,
    flags: u32,
}

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) center_view: vec4f,
    @location(2) radius: f32,
    @location(3) sprite_scale: f32,
    @location(4) @interpolate(flat) id: u32,
    @location(5) coord: vec2f,
}

@group(0) @binding(0) var<uniform> mvp: MVP;

fn project_depth(view_position: vec4f) -> vec4f {
    return scene_clip_to_device_clip(mvp.proj * view_position);
}

fn raycast_sphere(coord: vec2f, center_view: vec4f, radius: f32) -> vec4f {
    let ortho = abs(mvp.proj[3][3]) > 0.5;
    var ro = vec3f(0.0);
    var rd = vec3f(0.0, 0.0, -1.0);
    let plane_point = center_view.xyz + vec3f(coord.x * radius, coord.y * radius, 0.0);
    if (ortho) {
        ro = plane_point + vec3f(0.0, 0.0, 2.0 * radius);
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

struct FragmentOut {
    @builtin(frag_depth) depth: f32,
    @location(0) id: u32,
}

@fragment
fn main(input: FragmentIn) -> FragmentOut {
    let coord = input.coord;
    if (dot(coord, coord) > 1.0) {
        discard;
    }

    let surface_view = raycast_sphere(coord, input.center_view, input.radius);
    if (surface_view.w == 0.0) {
        discard;
    }

    let depth_clip = project_depth(surface_view);
    return FragmentOut(clamp(depth_clip.z / max(abs(depth_clip.w), 1e-6), 0.0, 1.0), input.id);
}
