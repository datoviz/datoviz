#version 450
#include "common.glsl"

// The "in" variables here MUST correspond to the "out" variables from the vertex shader.
layout (location = 0) in vec4 in_color;

// The first output variable of the fragment shader must be a vec4 variable with the RGBA
// components of the pixel being processed.
layout (location = 0) out vec4 out_color;

void main()
{
    // This macro is used to implement clipping in the inner viewport, when using panel margins.
    CLIP

    // Here, we simply pass the varying color (already computed by the vertex shader) to the
    // output variable.

    // NOTE: we could also have passed the point size from the vertex shader to the fragment
    // shader, and compute the colormap here. HOWEVER the computation of the colormap would have
    // been done for each *pixel*, rather than each *vertex*. Since we're using uniform square
    // colors in this example, that would have resulted in wasted duplicate computations and
    // unjustified loss of performance.
    // When the pixel color is non uniform across the primitive, and not just linearly interpolated
    // between the vertices, one can compute the pixel color directly in the fragment shader.
    out_color = in_color;
}
