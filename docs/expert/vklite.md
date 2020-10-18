# vklite: Vulkan for mere mortals

**Vulkan** is an extremely powerful but highly complex low-level graphics API. It gives a very high level of control on the GPU, which is crucial when developing demanding 3D video games or making game engines. Scientific visualization has less stringent requirements and it can still benefit from a small, restricted subset of Vulkan.

**vklite** is an attempt to provide a relatively easy, but much less powerful API on top of Vulkan. It has been primarily developed for scientific visualization needs, but it could used for other types of interactive graphics (animations, demos, simple video games) for which working directly on top of the Vulkan API would be overkill.

This page provides a brief documentation of the vklite API. Familiarity with GPUs and graphics APIs like OpenGL, or more modern ones like Vulkan, Metal, DirectX 12, WebGPU is not strictly required but somewhat helpful.

!!! note
    Visky currently uses Vulkan almost entirely via the vklite interface. This interface provides abstractions such as memory buffers, graphics pipelines, shaders, textures, resource bindings and so on, which are relatively similar to those in other low-level graphics APIs. Therefore, it might be feasible to make Visky with these other APIs in the future. In particular, leveraging emerging technologies such as **WebGPU and WebAssembly** would be an interesting future direction to make Visky work in the browser

## Canvas

## Overview of the graphics pipeline

## Vertex shader

## Fragment shader

## Other shaders

## Vertex layout

## Buffer

## Resource layout

## Texture

## Uniform buffer

## Compute pipeline

## Event loop
