#version 450

void main()
{
    // Fullscreen triangle using gl_VertexIndex trick.
    const vec2 pos[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
}
