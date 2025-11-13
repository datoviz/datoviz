#version 450

layout(location = 0) out vec4 outColor;

void main()
{
    // Use NDC coordinates from gl_FragCoord, hard-coded resolution 800x600.
    vec2 uv = (gl_FragCoord.xy / vec2(800.0, 600.0)) * 2.0 - 1.0;

    // Paraboloid depth: center is far, edges are nearer.
    float z = 0.5 + 0.5 * length(uv);

    gl_FragDepth = z;

    // Dummy color, not used (we render AO in second pass).
    outColor = vec4(z, z, z, 1.0);
}
