#version 450

// Constants for viewport size (hard-coded)
const float WIDTH  = 800.0;
const float HEIGHT = 600.0;

// Output is ignored (color writes disabled in the pipeline)
layout(location = 0) out vec4 outColor;

void main()
{
    // Normalize pixel coordinates to [0,1]
    vec2 uv = vec2(gl_FragCoord.x / WIDTH, gl_FragCoord.y / HEIGHT);

    // Compute distance from center of screen
    float dist = distance(uv, vec2(0.5, 0.5));

    // Circle radius (in normalized units)
    float radius = 0.3;

    // Discard fragments outside the circle (no stencil write there)
    if (dist > radius)
        discard;

    // Inside the circle: stencil write will happen automatically.
    // No need to output color — we just fill the stencil buffer.
    outColor = vec4(0.0);
}
