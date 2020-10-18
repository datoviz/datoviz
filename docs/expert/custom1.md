# Custom visuals: beginner level

Visky provides a relatively rich set of visuals that can be used to make a large variety of scientific visualizations. However, more complex use-cases and applications for which performance and more fine-grained memory footprint are desirable, require the user to write custom visuals.

Writing a custom visual is relatively complex. It typically requires writing custom shaders in GLSL, and following a set of rules implemented by Visky in order to make the visual compatible with the rest of the library.

Writing a custom visual also requires understanding the basics of the GPU graphics pipeline and rudiments of Vulkan (via the vklite interface).

This expert manual provides a comprehensive guide to writing custom visuals for users without prior knowledge and experience of GPU graphics.

## An example: the triangle visual

## Geometric primitives

## Vertices

## Indices

## Vertex shader

## Fragment shader

## Bindings and resources

## Uniform parameters

## Panel transformation matrices

## Visual props

## Checklist when writing a custom visual

1. Start with the template file
2. Determine the geometric primitive
3. Determine the vertex attributes and their types
4. Determine the uniform parameter structure and check alignment
5. Determine the required resources
6. Write the shaders in GLSL
7. Put it all together
8. Try the visual by setting directly the vertex attributes and, optionally, the indices
9. Determine the visual props
10. Make the visual data array structure
11. Write the baking function (optional)
