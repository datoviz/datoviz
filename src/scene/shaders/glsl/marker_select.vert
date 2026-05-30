#version 450

#include "common.glsl"
#include "selection_style.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;
layout(location = 3) in float inAngle;
layout(location = 4) in uint inShape;
layout(location = 5) in uint inSelection;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragSize;
layout(location = 2) out float fragAngle;
layout(location = 3) flat out uint fragShape;
layout(location = 4) out float fragSpriteScale;

void main()
{
    float spriteScale = max(abs(cos(inAngle)) + abs(sin(inAngle)), 1.0);
    gl_Position = transform(inPos);
    gl_PointSize = max(inSize * spriteScale, 0.0);
    fragColor = applySelectionVisualStyle(inColor, inSelection);
    fragSize = max(inSize, 1.0);
    fragAngle = inAngle;
    fragShape = inShape;
    fragSpriteScale = spriteScale;
}
