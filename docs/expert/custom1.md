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
