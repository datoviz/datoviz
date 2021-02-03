// GLSL version
#version 450

// To be compatible with the scene API, all graphics shaders must include a common GLSL include
// file. It defines common uniform bindings and common functions (for example, transform()).
// These .glsl files are found in `include/datoviz/glsl`, so one needs to pass
// `-Ipath/to/datoviz/include/datoviz/glsl` to the `glslc` command (see build.sh script).
#include "common.glsl"

// Here, we also use GLSL colormaps to compute the square color directly in the vertex shader,
// without using the colormap texture.
#include "colormaps.glsl"

// Here, we describe the vertex shader attributes.
layout (location = 0) in vec3 pos;   // x, y, z positions
layout (location = 1) in float size; // point size, between 0.0 and 255.0

// We made the unusual choice here to use a float in the shader, which corresponds to a uint8 byte
// in C. We also decided not to normalize it, i.e. that the byte 255 corresponds to the float 255.0
// in the shader (VK_FORMAT_R8_USCALED format). Other choices can be made when specifying the
// attribute format in the custom graphics definition.

// Here, we describe the "varying" variables. These are special values output by the vertex shader,
// and passed to the fragment shader.
layout (location = 0) out vec4 out_color;

// Main shader code.
void main() {
    // Here, "pos" is the position of the vertex being processed. gl_Position is a special output
    // variable that returns the vertex final position in normalized device coordinates. It
    // is a vec4 vector, the fourth component is the homogeneous coordinate.
    // The "transform()" function is defined in common.glsl. It applies the model, view, proj
    // matrices stored in the first (common) uniform buffer (MVP buffer).
    gl_Position = transform(pos);

    // This special variable is an output variable that contains the point size, in pixels,
    // of the vertex being processed. It is reserved to graphics pipelines with a point list
    // primitive.
    gl_PointSize = size;

    // Here, we set the varying variable that will be passed to the fragment shader.
    // The function colormap() is implemented in colormaps.glsl. It supports a few colormaps
    // that are implemented directly in GLSL, without using a texture. The second argument
    // is a value between 0 and 1, hence the normalization with the size which is in pixels.
    out_color = colormap(DVZ_CMAP_HSV, size / 255.0);
    out_color.a = .25; // alpha component for some transparency
}
