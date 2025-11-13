#version 450

layout(location = 0) in vec4 v_color;

// Output attachments:
layout(location = 0) out vec4 out_accumColor;   // RGBA16F
layout(location = 1) out float out_accumWeight; // R16F

void main()
{
    // Use a fixed alpha = 0.5 for all triangles.
    const float alpha = 0.5;

    // Standard weighted blended OIT:
    // accumulate premultiplied contribution
    out_accumColor = vec4(v_color.rgb * alpha, alpha);

    // accumulate weight
    out_accumWeight = alpha;
}
