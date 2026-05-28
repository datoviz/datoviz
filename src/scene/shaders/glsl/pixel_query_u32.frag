#version 450

layout(location = 0) flat in uint fragId;
layout(location = 0) out uint outId;

void main()
{
    outId = fragId;
}
