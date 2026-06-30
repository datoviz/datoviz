fn camera_position_from_view() -> vec3f {
    return (inverse(mvp.view) * vec4f(0.0, 0.0, 0.0, 1.0)).xyz;
}
