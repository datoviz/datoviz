# Install

These instructions target the published Datoviz v0.4.0rc1 release candidate. Choose a path by what
you want to run.

<div class="dvz-context-strip">
  <span>v0.4 release candidate</span>
  <span>Python 3.10+</span>
  <span>C and C++</span>
  <span>Vulkan runtime required</span>
</div>

See the [v0.4.0rc1 release notes](../releases/v0.4.0rc1.md) for the exact package, artifacts,
validation evidence, and known limitations. Check
[project status](../reference/project-status.md) for the broader release posture.


## Choose an installation path

| You want to... | Use this path now |
| --- | --- |
| install the published Python RC | use the exact PyPI command below or a wheel from the release assets |
| develop a C or C++ application now | [Build from source](build-from-source.md), then follow [C/C++ integration](../how-to/c-integration.md) |
| contribute to Datoviz or use the latest `v0.4-dev` code | [Build from source](build-from-source.md) |
| inspect browser support without installing native Datoviz | open a `webgpu-live` route from the [Examples](../examples/index.md) section |

Native windows and offscreen rendering require a working Vulkan-capable GPU, driver, and runtime.
Browser routes use the smaller, experimental [WebGPU subset](../reference/webgpu-subset.md).


## Python package path

Create an isolated environment, then install the exact RC1 version from PyPI.

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install --pre datoviz==0.4.0rc1
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    py -m pip install --upgrade pip
    py -m pip install --pre datoviz==0.4.0rc1
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

This CMake fragment is an integration example, not a complete project. The RC1 wheels expose the
installed C package and passed the release CMake-consumer checks. Source developers and system
packagers may instead follow [Build from source](build-from-source.md), then use the
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
| Python RC package | Published on PyPI as `datoviz==0.4.0rc1`; six native wheels are validated. |
| Source build | Available now for development, C/C++ integration, and package validation. |
| Native Windows wheels | AMD64 and ARM64 wheels are published and passed hosted package-index verification. |
| Source bundle | Published with the GitHub prerelease and covered by the release checksums. |
| vcpkg and conda-forge | Engineering and preflight paths; not published as RC1 package channels. |
