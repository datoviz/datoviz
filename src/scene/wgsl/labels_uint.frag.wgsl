struct FragmentIn {
    @location(0) uv: vec2f,
}

@group(1) @binding(0) var tex: texture_2d<u32>;
@group(1) @binding(1) var samp: sampler;

fn hashLabel(value: u32) -> u32 {
    var x = value;
    x = x ^ (x >> 16u);
    x = x * 0x7feb352du;
    x = x ^ (x >> 15u);
    x = x * 0x846ca68bu;
    x = x ^ (x >> 16u);
    return x;
}

fn labelColor(id: u32) -> vec4f {
    let h = hashLabel(id);
    let rgb = vec3f(
        f32((h >> 0u) & 0xffu),
        f32((h >> 8u) & 0xffu),
        f32((h >> 16u) & 0xffu)) / 255.0;
    return vec4f(mix(vec3f(0.18), rgb, vec3f(0.82)), 0.82);
}

@fragment
fn main(input: FragmentIn) -> @location(0) vec4f {
    let extent = vec2f(textureDimensions(tex));
    let coord = vec2i(clamp(floor(input.uv * extent), vec2f(0.0), extent - vec2f(1.0)));
    let id = textureLoad(tex, coord, 0).r;
    if id == 0u {
        discard;
    }
    return labelColor(id);
}
