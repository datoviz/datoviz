# Install

These instructions target Datoviz v0.4. Choose a path by what you want to run and by whether a
release package is currently published.

<div class="dvz-context-strip">
  <span>v0.4 release candidate</span>
  <span>Python 3.10+</span>
  <span>C and C++</span>
  <span>Vulkan runtime required</span>
</div>

The [release notes index](../releases/index.md) is currently a draft, pre-publication record. Until
it names a published package or artifact, use the [source-build fallback](build-from-source.md).
Check [project status](../reference/project-status.md) for the current release posture.


## Choose an installation path

| You want to... | Use this path now |
| --- | --- |
| try Python before a package is published | [Build from source](build-from-source.md), then install the checkout in a virtual environment |
| install a published Python RC or final version | use the exact command or artifact URL in the published release notes |
| develop a C or C++ application now | [Build from source](build-from-source.md), then follow [C/C++ integration](../how-to/c-integration.md) |
| contribute to Datoviz or use the latest `v0.4-dev` code | [Build from source](build-from-source.md) |
| inspect browser support without installing native Datoviz | open a `webgpu-live` route from the [Examples](../examples/index.md) section |

Native windows and offscreen rendering require a working Vulkan-capable GPU, driver, and runtime.
Browser routes use the smaller, experimental [WebGPU subset](../reference/webgpu-subset.md).


## Python package path

After an RC is published, its release notes will provide an exact version or artifact URL. Create an
isolated environment first, then use that published command. Do not guess a pre-release version.

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    # After publication, run the exact command from the linked release notes.
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    py -m pip install --upgrade pip
    # After publication, run the exact command from the linked release notes.
    ```

After the final v0.4 package is published, the normal installation command will be:

```sh
python -m pip install datoviz
```

Verify the active environment after installation:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

Then continue with the [Quickstart](quickstart.md). Before choosing optional providers or deployment
targets, review [platform support and known limitations](../reference/platform-support.md).


## C and C++ package path

Installed Datoviz packages expose the native library, public headers, runtime assets, and an
exported CMake package:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE datoviz::datoviz)
```

This CMake fragment is an integration example, not a complete project. No public installed package
is assumed before the RC notes publish one. Until then, follow
[Build from source](build-from-source.md), then use the
[C/C++ integration guide](../how-to/c-integration.md).


## Source-build path

Build from source when no published package exists, when you need the current development branch,
or when you are contributing to Datoviz. The dedicated guide covers prerequisites, shader tools,
vendored dependencies, macOS, Linux, native Windows, WSL2, verification, and example commands:

[Build Datoviz from source](build-from-source.md), then return to the
[Quickstart](quickstart.md) or [First C Program](first-c-program.md).


## Package availability

| Path | Current v0.4 posture |
| --- | --- |
| Python RC/final package | Not assumed published; use only a version or artifact linked by published release notes. |
| Source build | Available now for development, C/C++ integration, and package validation. |
| Native Windows wheels | AMD64/ARM64 lanes are validated, but validation is not publication. |
| vcpkg, conda-forge, source bundle | Pre-publication engineering paths; see the draft release notes and project status. |
