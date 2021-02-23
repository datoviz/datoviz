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


## What is the difference between the FPS and eFPS?

* The **FPS** is the **number of Frames Per Second**. It is used to evaluate the raw rendering performance.
* The **eFPS** is the **effective Frames Per Second**. It indicates the rendering smoothness as *perceived* by the user.

The eFPS is computed from the average maximum delay between two consecutive frames. If every second, five frames take 100 ms to render, but all other frames render in 1 ms, the rendering will be perceived as slow as an application at ~10 FPS, even though the total frame count would be close to 1000 FPS.
