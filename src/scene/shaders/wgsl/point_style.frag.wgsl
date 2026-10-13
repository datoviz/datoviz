#include "scene_material.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
    @location(1) corner: vec2f,
    @location(2) size: f32,
}

#include "point_disc.wgsl"
#include "point_style_surface.wgsl"

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let dist = point_disc_distance(input.corner, input.size);
    let color = point_style_surface(input.color, dist);
    return color;
}
