# Writing custom graphics

In this section, we'll show how to **create a custom graphics** by writing **custom GLSL shaders**. This is an advanced topic as it requires understanding the basic of GPU graphics programming.

!!! note
    Only the C API supports custom graphics at the moment. Python bindings for custom graphics will come in an upcoming version. Datoviz already includes the code necessary to compile GLSL shaders to SPIR-V on the fly (based on [Khronos glslang](https://github.com/KhronosGroup/glslang/)).


As a toy example, we'll create a graphics with basic points of various size and color. Specifically, we'll require each point to have a different size (which is not supported in the existing `point` graphics), and a color depending directly on the size and computed directly by the GPU. We'll use a single byte per vertex to store the vertex size (between 1 and 255 pixels), and we'll use no memory for the color since it will be determined directly by this value.

Writing a custom graphics involves the following steps:

1. Choosing the primitive type.
2. Defining the vertex data structure and corresponding vertex shader attributes.
3. Writing the vertex shader.
4. Writing the fragment shader.
5. Writing a simple test.

## Choosing the graphics primitive type

Vulkan supports six major types of primitives:

* **point list**: square points with an arbitrary size,
* **line list**: disjoint aliased line segments,
* **line strip**: joined aliased line segments,
* **triangle strip**: joined triangles consecutively sharing an edge,
* **triangle fan**: joined triangles all sharing a single corner (the first vertex),
* **triangle list**: disjoint triangles,

Other less common primitive types are described in the [Vulkan specification](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPrimitiveTopology.html).

!!! warning
    Triangle fans are not supported on macOS.

![Vulkan primitive types](../images/primitive_types.png)
*Schematic from the [Vulkan Cookbook by Pawel Lapinski, O'Reilly](https://www.oreilly.com/library/view/vulkan-cookbook/9781786468154/f2ec181c-fca0-4121-aa67-c2cfd337a126.xhtml)*

!!! note
    The circular arrows in the triangles above indicate the orientation of the triangles, which is taken into account by the GPU. It is good practice to always ensure that all triangles constituting a given object are consistent. For example, when triangulating a square with two triangles, the order of the vertices should be chosen such that both triangles are directly oriented. The graphics pipeline can be configured to handle triangle orientation in a specific way. For example, one can make a graphics pipeline where indirectly oriented triangles are automatically discarded.

Line primitives typically have a width of 1 pixel, although some hardware supports thicker lines. In Datoviz, thick, high-quality lines are implemented with triangles instead (line triangulation), and the antialiased thick line is drawn directly in the fragment shader. "Basic" line primitives are only used for testing and for special high-performance applications where scalability and performance are more important than visual quality.

The most commonly-used primitive types in scientific visualization are essentially **point lists** and **triangle lists** (and to a lesser extent, triangle strips).
