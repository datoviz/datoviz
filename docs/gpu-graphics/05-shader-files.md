# 5. Shaders in their own files

**Your program at the end of this chapter: 350 C lines. The raw Vulkan equivalent: around 1250 lines, a rough estimate. You can now edit and reload the triangle's shaders.**

![The RGB triangle rendered from shaders loaded as separate files.](../assets/gpu-graphics/05-shader-files.webp)

Inline strings were useful while the triangle was new, but they are awkward to edit. This chapter moves them into ordinary `.vert` and `.frag` files, reports compiler diagnostics with their filenames, and rebuilds the pipeline when you press ++r++.

The image is deliberately identical to chapter 4. The pixels do not change in this chapter; the improvement is the shader-editing workflow and the ability to recover from a bad reload without closing the program.

## Load and compile source

Delete the `VERTEX_GLSL` and `FRAGMENT_GLSL` strings from `main.c`. Create `shader.vert` and `shader.frag` beside it, using the shader bodies from chapter 4, and add `<datoviz/fileio.h>`. The two filenames in C are relative to the directory where you launch `vkcourse`:

```c
#define VERTEX_PATH   "shader.vert"
#define FRAGMENT_PATH "shader.frag"
```

Write the complete shader files as follows:

```glsl
--8<-- "examples/c/vulkan/step05/shader.vert"
```

```glsl
--8<-- "examples/c/vulkan/step05/shader.frag"
```

`dvz_read_text()` returns an owned, null-terminated copy of the file. `compile_file()` frees that source copy after the compiler returns because compilation consumes the text before returning; neither the result nor a later shader module refers to it. The typed compiler request supplies the stage, the `main` entry point, and the reader-local filename used in diagnostics.

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

GLSL expresses shader types, control flow, stage inputs, and stage outputs as source text. Compilation checks those language rules and produces SPIR-V, the device-independent intermediate instructions passed to `vkCreateShaderModule`. The resulting shader module still is not a complete executable rendering pipeline: the driver combines it with the other shader stage and fixed-function state when the graphics pipeline is created. Runtime compilation is convenient for learning and live editing. Production applications often compile shaders during the build so startup is faster and a missing compiler cannot break deployment.

## Reload safely

Move the pipeline, shader-module, and slots teardown from `main()` into `destroy_pipeline()`, as shown in the full listing. The helper clears each pointer after freeing it, so it can clean either a complete active pipeline or a candidate that failed partway through creation.

Add `bool reload_requested;` to `Renderer`. The keyboard callback only sets that flag; it does not destroy GPU resources while input is being dispatched.

```c
static void keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    (void)router;
    Renderer* renderer = (Renderer*)user_data;
    if (event->type == DVZ_KEYBOARD_EVENT_PRESS && event->key == DVZ_KEY_R)
        renderer->reload_requested = true;
}
```

Subscribe after installing the draw callback:

```c
    DvzCallbackId keyboard_id =
        dvz_input_subscribe_keyboard(dvz_window_router(window), keyboard, &renderer);
    COURSE_CHECK(keyboard_id != DVZ_CALLBACK_ID_NONE, "keyboard subscription failed");
```

The main loop builds a complete candidate pipeline outside event dispatch. If compilation or pipeline creation fails, `destroy_pipeline()` releases the incomplete candidate and the working pipeline stays active. On success, wait for prior GPU work to finish before destroying the active pipeline and moving the candidate pointers into `renderer`. A shader edit requires a new graphics pipeline because shader stages are part of the pipeline's compiled state; Vulkan does not replace one stage inside an existing pipeline. Pass the resolved `color_format` from chapter 4 into each candidate as well because the replacement pipeline must remain compatible with the canvas attachment.

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

After `dvz_graphics_create()` succeeds, the shader modules may be destroyed without invalidating the pipeline. This course keeps them until `destroy_pipeline()` so startup failure, failed candidates, successful reload, and final cleanup all use the same short ownership path.

## Build, run, and capture

Build from your `vkcourse` directory. Run the executable from that same directory, where `shader.vert` and `shader.frag` live; changing into `build` would make the relative shader paths fail.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    ```

Edit either shader, save it, and press ++r++. A syntax error prints the filename, status, and compiler message while the existing triangle remains usable. Correct the file and press ++r++ again to install the new pipeline.

Capture the triangle from the same directory:

=== "Linux and macOS"

    ```sh
    ./build/vkcourse --png chapter05.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    .\build\Release\vkcourse.exe --png chapter05.png
    ```

The terminal should end with `validation errors: 0`. With the supplied shaders, `chapter05.png` should match chapter 4 exactly.

## Try it

- Change one fragment color component to `0.0`, save, and press ++r++. The window should update without recompiling the C program.
- Remove a semicolon and reload. The terminal should name the shader file and source line.
- Change the three clip-space positions. The pipeline rebuild is necessary because those constants belong to the shader module.

!!! info "Under the hood"
    Vulkan shader modules are immutable. A graphics pipeline incorporates their code when it is created, and Vulkan provides no operation that patches one shader stage in place. Live reload is therefore destroy-and-create work around the same pipeline description.

!!! failure "When it goes wrong"
    “Could not read” means the process cannot find `shader.vert` or `shader.frag`; start it from the directory containing those files. A compiler error is different: the file was found, and its diagnostic tells you which GLSL rule failed.

??? example "Full current listing"

    ```c
    --8<-- "examples/c/vulkan/step05.c"
    ```

## Checkpoint

1. Why does the compiler request include `source_name`?
2. Why must a shader edit rebuild the graphics pipeline?
3. Why does reload wait for the device before destroying the old pipeline?

The next chapter replaces shader-owned positions and colors with application data in a vertex buffer.
