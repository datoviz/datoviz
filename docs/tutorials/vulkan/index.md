# Modern GPU Graphics in C

This course teaches Vulkan concepts through Datoviz Canvas and `vklite`. You will see a live triangle first, then progressively take ownership of shaders, pipeline state, and GPU data.

## What you need

- A C11 compiler and CMake 3.21 or newer.
- An installed Datoviz v0.4 release-candidate package or a Datoviz source checkout.
- A working Vulkan runtime and driver.
- The runtime shaderc provider distributed with official Datoviz packages, or a source installation where shaderc is discoverable.

The canonical pilot project is [`examples/c/tutorial/`](https://github.com/datoviz/datoviz/tree/v0.4-dev/examples/c/tutorial). Its CMake project deliberately depends only on the installed package:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(first_triangle PRIVATE datoviz::datoviz)
```

Configure it with the prefix containing `DatovizConfig.cmake`:

```console
cmake -S examples/c/tutorial -B build/gpu-tutorial -GNinja \
  -DCMAKE_PREFIX_PATH=/path/to/datoviz/prefix
cmake --build build/gpu-tutorial
```

Run chapter programs in a live resizable window:

```console
./build/gpu-tutorial/first_triangle --live
./build/gpu-tutorial/shaders_and_pipeline --live
./build/gpu-tutorial/vertex_buffers --live
```

Every program also has a deterministic offscreen path:

```console
./build/gpu-tutorial/first_triangle --offscreen --frames 3 --validate --png triangle.png
```

`--validate` requests Vulkan validation and turns reported validation errors into a failing exit status. Use `--shader-dir PATH` to compile a copied shader directory without rebuilding C.

## The boundary

Datoviz owns device selection, the window surface, swapchain or offscreen images, acquisition, submission, presentation, and resize recovery. Your renderer owns its shader modules during pipeline creation, pipeline layout, graphics pipeline, optional vertex buffer, and small wrapper objects. During each draw callback, Canvas lends the renderer a recording command buffer and color image view; the renderer records into them but never resets, submits, destroys, transitions, or retains them.

The recurring questions are:

1. Is this data in CPU memory or GPU memory?
2. Who owns the object or handle?
3. How long is it valid?

Start with [First live triangle](first-triangle.md).
