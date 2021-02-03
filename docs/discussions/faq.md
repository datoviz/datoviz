# Frequently asked questions

## What is the distinction between the scene, the canvas, and the window?

Datoviz provides three similar, but different abstractions:

* the **scene**,
* the **canvas**,
* the **window**.

The **scene** provides a relatively high-level plotting interface that allows to arrange panels (subplots) in a grid, define controllers, and add visuals to the panels.

The **canvas** is lower-level object that allows to use Vulkan directly via vklite. While the scene deals with *visual* elements, the canvas deals with *Vulkan* objects.

The **window** is an abstraction provided by the backend windowing library, glfw at the moment. It is a bare window that doesn't allow for any kind rendering, unless manually creating a swapchain and so on by using Vulkan or vklite directly.

!!! note
    With the Python bindings, a scene is automatically created when creating a canvas.

**Most users will only work at the scene level.** Advanced users will use the canvas to create custom applications, interactive animations, or even small video games. Finally, the window is only used internally and will probably never be used directly.
