var<private> outColor: vec4<f32>;
var<private> fragColor_1: vec4<f32>;

fn main_1() {
    let _e2 = fragColor_1;
    outColor = _e2;
    return;
}

@fragment
fn main(@location(0) fragColor: vec4<f32>) -> @location(0) vec4<f32> {
    fragColor_1 = fragColor;
    main_1();
    let _e3 = outColor;
    return _e3;
}
