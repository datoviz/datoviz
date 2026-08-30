# Install

These instructions target Datoviz v0.4.0rc2, the latest published release candidate on PyPI and GitHub. Native windows and offscreen rendering require a Vulkan-capable GPU, driver, and runtime.

See the [v0.4.0rc2 release notes](../releases/v0.4.0rc2.md) for the package, validation scope, and known limitations. See [platform support](../reference/platform-support.md) for supported systems and current limitations.


## Install the Python package

Create an isolated environment and install the exact version from PyPI.

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install --pre datoviz==0.4.0rc2
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    py -m pip install --upgrade pip
    py -m pip install --pre datoviz==0.4.0rc2
    ```


## Verify the installation

Run this command in the active environment:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

Continue with the [Python Quickstart](quickstart.md) to open an interactive window and render your first scene.


## Use Datoviz from C or C++

For C or C++ development, [build Datoviz from source](build-from-source.md), then follow the [C/C++ integration guide](../how-to/c-integration.md). An installed Datoviz package exposes the native library, public headers, runtime assets, and this CMake target:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE datoviz::datoviz)
```

This fragment assumes an existing CMake project. The integration guide contains complete setup and verification instructions.


## Other installation paths

Use [Build from source](build-from-source.md) when you need the current `main` code, want to contribute to Datoviz, or do not have a suitable published package. To inspect browser support without installing native Datoviz, open a `webgpu-live` route from [Examples](../examples/index.md); browser support is an experimental [WebGPU subset](../reference/webgpu-subset.md).
