struct MVP {
    model: mat4x4<f32>,
    view: mat4x4<f32>,
    proj: mat4x4<f32>,
    time: f32,
    flags: u32,
}

struct gl_PerVertex {
    @builtin(position) gl_Position: vec4<f32>,
    gl_PointSize: f32,
    gl_ClipDistance: array<f32, 1>,
    gl_CullDistance: array<f32, 1>,
}

struct VertexOutput {
    @builtin(position) gl_Position: vec4<f32>,
    @location(0) member: vec2<f32>,
}

@group(0) @binding(0)
var<uniform> mvp: MVP;
var<private> unnamed: gl_PerVertex = gl_PerVertex(vec4<f32>(0f, 0f, 0f, 1f), 1f, array<f32, 1>(), array<f32, 1>());
var<private> inPos_1: vec3<f32>;
var<private> fragUV: vec2<f32>;
var<private> inUV_1: vec2<f32>;

fn transform_u0028_vf3_u003b(pos: ptr<function, vec3<f32>>) -> vec4<f32> {
    var tr: vec4<f32>;

    let _e16 = mvp.proj;
    let _e18 = mvp.view;
    let _e21 = mvp.model;
    let _e23 = (*pos);
    tr = (((_e16 * _e18) * _e21) * vec4<f32>(_e23.x, _e23.y, _e23.z, 1f));
    let _e30 = tr[1u];
    tr[1u] = -(_e30);
    let _e34 = tr[2u];
    let _e36 = tr[3u];
    tr[2u] = (0.5f * (_e34 + _e36));
    let _e40 = tr;
    return _e40;
}

fn main_1() {
    var param: vec3<f32>;

    let _e14 = inPos_1;
    param = _e14;
    let _e15 = transform_u0028_vf3_u003b((&param));
    unnamed.gl_Position = _e15;
    let _e17 = inUV_1;
    fragUV = _e17;
    return;
}

@vertex
fn main(@location(0) inPos: vec3<f32>, @location(1) inUV: vec2<f32>) -> VertexOutput {
    inPos_1 = inPos;
    inUV_1 = inUV;
    main_1();
    let _e8 = unnamed.gl_Position.y;
    unnamed.gl_Position.y = -(_e8);
    let _e10 = unnamed.gl_Position;
    let _e11 = fragUV;
    return VertexOutput(_e10, _e11);
}
