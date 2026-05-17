#version 450

layout(location = 0) out float outDepth;

void main()
{
    outDepth = gl_FragCoord.z;
}
