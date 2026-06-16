#include "color.wgsl"

struct FragmentIn {
    @location(0) color: vec4f,
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    return semantic_color_to_linear(input.color);
}
