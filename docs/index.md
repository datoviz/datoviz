# Visky documentation

**Visky** is an **interactive scientific data visualization library** leveraging the graphics processing unit (**GPU**) for high performance and scalability. It targets **3D rendering** as well as high-quality antialiased **2D graphics**. Written mainly in C, it is currently based on the **Vulkan graphics API** created by the Khronos consortium, successor of OpenGL. Visky provides basic bindings in **Python** and could support other languages in the future (Julia, R, MATLAB, Rust, Java...).

!!! warning
    At this time, Visky is at an early stage of development. It is quite usable but the API is expected to break regularly. The documentation is still being written.

The documentation is divided in:

* [**Quick start**](quickstart/index.md): start here
* [**User manual**](user/index.md): for regular users who are mainly interested in scientific 2D/3D plotting
* [**Expert manual**](expert/index.md): for advanced users who need to create their own visuals by writing **shaders in GLSL**
* [**Developer manual**](developer/index.md): for advanced users who want to contribute back to Visky
* [**API reference**](api/index.md)

See also the [**gallery**](gallery/index.md).



*[Vulkan]: Low-level graphics API created by Khronos, successor of OpenGL but much more complex
*[shaders]: code written in GLSL and executed on the GPU to customize the graphics pipeline
*[GLSL]: OpenGL shading language, the C-like language used to write shaders
