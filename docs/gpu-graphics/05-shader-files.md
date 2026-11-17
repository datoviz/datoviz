# 5. Shaders in their own files

*About 350 lines of C; raw Vulkan would need about 1250.*

![The RGB triangle rendered from shaders loaded as separate files.](../assets/gpu-graphics/05-shader-files.webp)

Chapter 4 drew a triangle with two shaders written as C strings. That works, but it is awkward: every shader edit means editing quoted lines, rebuilding the C program, and restarting it, and a GLSL error points into a string rather than a file. The shaders grow in almost every chapter from here on, so it is worth fixing how you work with them now.

This chapter moves the shaders into ordinary `.vert` and `.frag` files, reports compiler errors with file names and line numbers, and rebuilds the pipeline when you press **R**, without restarting the program. The image is identical to chapter 4; what changes is how fast you can experiment.

In this chapter you will:

1. move the two shaders into their own files;
2. compile them with a call that reports errors by file and line;
3. gather the pipeline teardown into one helper;
4. rebuild the pipeline when you press **R**, and keep the old one if the new one fails.

## Why reloading means a new pipeline

Reloading a shader sounds like replacing one file. Vulkan does not allow that: shader modules and pipelines cannot change once created, because the driver has already compiled them into GPU code (chapter 4). A shader edit therefore means building a whole new pipeline and throwing the old one away.

Two rules make that swap safe:

- Build the new pipeline completely before touching the old one. If your edit does not compile, you keep drawing with the pipeline you have.
- Do not destroy the old pipeline while the GPU may still use it. Chapter 3 showed that submitted frames run later, so a frame recorded with the old pipeline may still be executing. Wait for the GPU to finish, then swap.

```mermaid
flowchart TD
    K[R pressed] --> B[Build a candidate pipeline]
    B -->|success| W[Wait for the GPU]
    W --> S[Destroy the old one, keep the candidate]
    B -->|failure| F[Destroy the candidate, keep the old one]
```

## Move the shaders into files

Delete the `VERTEX_GLSL` and `FRAGMENT_GLSL` strings from `main.c`. Create `shader.vert` and `shader.frag` beside it, and add `<datoviz/fileio.h>`. The paths are relative to the directory where you launch `vkcourse`:

```c
#define VERTEX_PATH   "shader.vert"
#define FRAGMENT_PATH "shader.frag"
```

The two files hold the chapter 4 shader bodies:

```glsl
--8<-- "examples/c/vulkan/step05/shader.vert"
```

```glsl
--8<-- "examples/c/vulkan/step05/shader.frag"
```

## Compile from a file

Next, a helper that reads one file and compiles it. `compile_file()` replaces chapter 4's `dvz_compile_glsl()` with `dvz_shader_compile()`, which returns a typed status and diagnostics that name the file and line. `dvz_read_text()` returns an owned, null-terminated copy of the file; the helper frees it once the compiler returns. `source_name` is the file name used in diagnostics.

```c
static bool compile_file(
    const char* path, DvzShaderStage stage, DvzShaderCompileResult* result)
{
    DvzSize source_size = 0;
    char* source = dvz_read_text(path, &source_size);
    if (source == NULL)
    {
        fprintf(stderr, "could not read %s\n", path);
        return false;
    }
    DvzShaderCompileRequest request = {
        .stage = stage,
        .profile = DVZ_SHADER_PROFILE_GRAPHICS,
        .source = source,
        .source_size = source_size,
        .source_name = path,
        .entry_point = "main",
    };
    DvzShaderCompileStatus status = dvz_shader_compile(&request, result);
    dvz_memory_free(source);
    if (status != DVZ_SHADER_COMPILE_SUCCESS)
    {
        fprintf(
            stderr, "%s: %s\n", dvz_shader_compile_status_name(status),
            result->diagnostics != NULL ? result->diagnostics : "shader compilation failed");
        return false;
    }
    return true;
}
```

Read the request as: compile this source as a graphics-stage shader, report errors under the file's name, and start at `main`. The graphics profile targets Vulkan 1.0 and SPIR-V 1.0, enough for every shader in the course. On failure, the helper prints the status name and the compiler's message, then returns `false`.

Compiling at startup is convenient for learning and live editing. Production applications often compile shaders at build time, so startup is faster and deployment does not need a compiler.

In `create_pipeline()`, replace the `dvz_compile_glsl()` calls with `compile_file()`. A `DvzShaderCompileResult` owns its SPIR-V words and diagnostics, so destroy both results on every path:

```c
    DvzShaderCompileResult vertex = {0};
    DvzShaderCompileResult fragment = {0};
    if (!compile_file(VERTEX_PATH, DVZ_SHADER_STAGE_VERTEX, &vertex) ||
        !compile_file(FRAGMENT_PATH, DVZ_SHADER_STAGE_FRAGMENT, &fragment))
    {
        dvz_shader_compile_result_destroy(&vertex);
        dvz_shader_compile_result_destroy(&fragment);
        return -1;
    }
```

The wrapper allocation that follows is unchanged, except that its failure branch destroys the two results instead of freeing raw SPIR-V. The shader modules read `spirv` and `spirv_size` from each result:

```c
    int vertex_result =
        dvz_shader(renderer->device, vertex.spirv_size, vertex.spirv, renderer->vertex_shader);
    int fragment_result = dvz_shader(
        renderer->device, fragment.spirv_size, fragment.spirv, renderer->fragment_shader);
    dvz_shader_compile_result_destroy(&vertex);
    dvz_shader_compile_result_destroy(&fragment);
    if (vertex_result != 0 || fragment_result != 0)
        return -1;
```

## Gather the teardown in one helper

Reload needs to tear down a pipeline from two places, so move the pipeline, shader-module, and slots teardown from `main()` into a helper. Call it at `cleanup:` right after the device wait:

```c
static void destroy_pipeline(Renderer* renderer)
{
    if (renderer->pipeline != NULL)
        dvz_graphics_destroy(renderer->pipeline);
    dvz_graphics_free(renderer->pipeline);
    renderer->pipeline = NULL;
    if (renderer->fragment_shader != NULL)
        dvz_shader_destroy(renderer->fragment_shader);
    dvz_shader_free(renderer->fragment_shader);
    renderer->fragment_shader = NULL;
    if (renderer->vertex_shader != NULL)
        dvz_shader_destroy(renderer->vertex_shader);
    dvz_shader_free(renderer->vertex_shader);
    renderer->vertex_shader = NULL;
    if (renderer->slots != NULL)
        dvz_slots_destroy(renderer->slots);
    dvz_slots_free(renderer->slots);
    renderer->slots = NULL;
}
```

Clearing each pointer lets the helper clean either a complete pipeline or a candidate that failed halfway.

## Request a reload from the keyboard

Add `bool reload_requested;` to `Renderer`. The keyboard callback only sets that flag; destroying GPU resources in the middle of event dispatch would be unsafe.

```c
static void keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    (void)router;
    Renderer* renderer = (Renderer*)user_data;
    if (event->type == DVZ_KEYBOARD_EVENT_PRESS && event->key == DVZ_KEY_R)
        renderer->reload_requested = true;
}
```

The window delivers input through a `DvzInputRouter`, its event dispatcher: you subscribe callbacks to it, and it calls them as events arrive. `dvz_window_router()` returns the window's router, borrowed and valid until the window is destroyed. Subscribe after installing the draw callback. The router keeps the `&renderer` pointer, so `renderer` must outlive the subscription:

```c
    DvzCallbackId keyboard_id =
        dvz_input_subscribe_keyboard(dvz_window_router(window), keyboard, &renderer);
    COURSE_CHECK(keyboard_id != DVZ_CALLBACK_ID_NONE, "keyboard subscription failed");
```

## Swap pipelines in the loop

The loop applies the two rules from the start of the chapter. It builds a complete candidate first. If it fails, `destroy_pipeline()` releases the partial candidate and the working pipeline stays. If it succeeds, wait for earlier frames to finish, destroy the old pipeline, and move the candidate's pointers into `renderer`. The candidate needs the resolved `color_format` from chapter 4 to stay compatible with the canvas.

```c
        if (renderer.reload_requested)
        {
            Renderer candidate = {
                .device = renderer.device,
                .color_format = renderer.color_format,
            };
            pipeline_result = create_pipeline(&candidate);
            if (pipeline_result == 0)
            {
                dvz_device_wait(renderer.device);
                destroy_pipeline(&renderer);
                renderer.slots = candidate.slots;
                renderer.vertex_shader = candidate.vertex_shader;
                renderer.fragment_shader = candidate.fragment_shader;
                renderer.pipeline = candidate.pipeline;
                printf("reloaded %s and %s\n", VERTEX_PATH, FRAGMENT_PATH);
            }
            else
            {
                destroy_pipeline(&candidate);
                fprintf(stderr, "reload failed; keeping the current pipeline\n");
            }
            renderer.reload_requested = false;
        }
```

Put this block in the render loop right after `dvz_window_host_poll(host);`. It reuses the `pipeline_result` variable from chapter 4.

## Build and run

From now on, run the executable from your `vkcourse` directory, where `shader.vert` and `shader.frag` live. Running it from `build` would break the relative shader paths.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter05.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter05.png
    ```

Edit either shader, save, and press **R** in the live window. `chapter05.png` matches chapter 4 exactly, and the terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. In `shader.frag`, change the output to `vec4(color.r, 0.0, color.b, 1.0)`, save, and press **R**. The green drains out of the triangle without recompiling C.
    2. Remove a semicolon and press **R**. The terminal names the file and line, prints `reload failed; keeping the current pipeline`, and the triangle keeps drawing. Fix it and press **R** again.
    3. Scale the three positions in `shader.vert` by 0.5 and press **R**. The triangle shrinks: the positions are constants inside the shader module, so they change only with a new pipeline.

??? info "Under the hood: immutable shader modules"

    Raw Vulkan reads the file, calls a compiler library such as shaderc, and passes the SPIR-V to `vkCreateShaderModule`. Shader modules and pipelines are immutable: no Vulkan call patches one stage in place. Live reload is therefore destroy-and-create work around the same pipeline description, guarded by `vkDeviceWaitIdle` or fences.

!!! warning "When it goes wrong"

    `could not read shader.vert` means the program cannot find the shader files; start it from the directory that contains them. A compiler error is different: the file was found, and the message names the GLSL rule that failed.

## What carries over

- Treat shaders as source files of their own, with real names, so compiler errors point at a file and line as in any other language.
- To replace a GPU object safely, build the replacement first, wait until no submitted work uses the old one, then swap and destroy.
- Input callbacks should record what the user asked for, and the main loop should act on it at a point where the program controls what the GPU is doing.

## Checkpoint

1. Why does the compiler request include `source_name`?
2. Why must a shader edit rebuild the graphics pipeline?
3. Why does reload wait for the device before destroying the old pipeline?

??? example "Your `main.c` after chapter 5"

    ```c
    --8<-- "examples/c/vulkan/step05.c"
    ```

Next: [Vertex buffers](06-vertex-buffers.md).
